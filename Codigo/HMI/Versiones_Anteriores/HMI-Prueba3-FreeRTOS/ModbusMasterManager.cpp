#include "ModbusMasterManager.h"
#include "HMIData.h"
#include <ModbusRTU.h>

// =========================================================================
// Mapeo verificado en conector UART de la placa Guition JC8012P4A1C (MAX3485)
// =========================================================================
#define RX_PIN 38 // GPIO38 RX
#define TX_PIN 37 // GPIO37 TX

ModbusRTU mb_master;
static unsigned long ultimo_transaccion_fin = 0;
static unsigned long ultimo_read_poll = 0;
static unsigned long t_ultimo_req_log = 0;
static unsigned long t_ultimo_ok_log = 0;
static TaskHandle_t hModbusTask = NULL;

// Buffer local aislado para recepciones Modbus (no interfiere con la UI mientras se llena)
static uint16_t rx_plc_regs[PLC_CANTIDAD_REGISTROS];

// Registros de configuración (temperaturas y tiempos fijos)
static const uint8_t REGS_CONFIG[] = {
  MB_SETPOINTZ1, MB_SETPOINTZ2, MB_SETPOINTZ3,
  MB_TINYECCION, MB_TSOSTENIMIENTO, MB_TENFRIAMIENTO,
  MB_TEXP_AVANCE, MB_TEXP_RETROCESO
};
#define NUM_REGS_CONFIG (sizeof(REGS_CONFIG) / sizeof(REGS_CONFIG[0]))

// Valores confirmados que el PLC tiene actualmente
static uint16_t confirmed_config_sent[NUM_REGS_CONFIG] = {
  180, 190, 200,
  5, 3, 12, 2, 2
};
static uint16_t confirmed_botones_sent = 0;

// Control de transacción de escritura en vuelo
static uint16_t inflight_write_reg = 0xFFFF;
static uint16_t inflight_write_val = 0;

static uint8_t fallos_lectura_consecutivos = 0;

// Callback de lectura de registros del PLC
bool cb_plc_read(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  ultimo_transaccion_fin = millis();

  if (event == Modbus::EX_SUCCESS) {
    fallos_lectura_consecutivos = 0;

    // Sincronización atómica con la memoria global de la HMI
    HMI_Lock();
    uint16_t local_botones = HMI.plc_regs[MB_HMI_BOTONES];
    memcpy(HMI.plc_regs, rx_plc_regs, sizeof(HMI.plc_regs));
    HMI.plc_regs[MB_HMI_BOTONES] = local_botones; // Los botones son generados por la HMI, no por el PLC
    HMI.plc_conectado = true;

    // Sincronizamos la referencia confirmada con lo leído para no reenviar lo que ya tiene el PLC
    for (size_t i = 0; i < NUM_REGS_CONFIG; i++) {
      uint8_t reg = REGS_CONFIG[i];
      confirmed_config_sent[i] = HMI.plc_regs[reg];
    }
    HMI_Unlock();

    // Log periódico de éxito cada 3 segundos
    if (millis() - t_ultimo_ok_log > 3000) {
      t_ultimo_ok_log = millis();
      Serial.printf("[MODBUS OK] Leido del PLC -> Piezas: %d | Temp Z1: %d C | Z2: %d C | Z3: %d C\n",
                    rx_plc_regs[MB_TOTAL_PIEZAS], rx_plc_regs[MB_TEMPZ1],
                    rx_plc_regs[MB_TEMPZ2], rx_plc_regs[MB_TEMPZ3]);
    }
  } else {
    fallos_lectura_consecutivos++;
    if (fallos_lectura_consecutivos >= 3) {
      HMI_Lock();
      HMI.plc_conectado = false;
      HMI_Unlock();
      Serial.printf("[MODBUS ALERTA] Fallo lectura PLC. Codigo: 0x%02X (Fallos: %d)\n",
                    event, fallos_lectura_consecutivos);
    }
  }
  return true;
}

// Callback de escritura hacia el PLC
bool cb_plc_write(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  ultimo_transaccion_fin = millis();

  if (event == Modbus::EX_SUCCESS) {
    HMI_Lock();
    HMI.plc_conectado = true;
    HMI_Unlock();

    // Confirmación exitosa: actualizamos la referencia confirmada
    if (inflight_write_reg == MB_HMI_BOTONES) {
      confirmed_botones_sent = inflight_write_val;
      Serial.printf("[MODBUS OK] Orden Botones CONFIRMADA por PLC (0x%04X)\n", confirmed_botones_sent);
    } else {
      for (size_t i = 0; i < NUM_REGS_CONFIG; i++) {
        if (REGS_CONFIG[i] == inflight_write_reg) {
          confirmed_config_sent[i] = inflight_write_val;
          Serial.printf("[MODBUS OK] Parametro Reg %d CONFIRMADO por PLC (Valor: %d)\n",
                        inflight_write_reg, inflight_write_val);
          break;
        }
      }
    }
  } else {
    // Si falló (ruido o timeout), NO actualizamos confirmed_*. El bucle reintentará automáticamente.
    Serial.printf("[MODBUS ERROR] Fallo confirmacion Reg %d (Codigo: 0x%02X). Reintentando...\n",
                  inflight_write_reg, event);
  }

  inflight_write_reg = 0xFFFF;
  return true;
}

void ModbusMasterManager_Init() {
  // 1. Destruimos UART0 por defecto que secuestra pines 37 y 38 (Workaround de Guition)
  Serial0.end();

  // Aseguramos nivel alto (Mark/Idle) en la línea de recepción RS485
  pinMode(RX_PIN, INPUT_PULLUP);

  // 2. Reclamamos pines limpios para Modbus (UART1 a 9600 baudios)
  Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  // 3. Inicializamos ModbusRTU
  mb_master.begin(&Serial1);
  mb_master.master();
  mb_master.setBaudrate(9600);
  mb_master.setInterFrameTime(5000); // 5ms de tolerancia para recepción de tramas continuas a 9600 baud

  confirmed_botones_sent = 0;
  inflight_write_reg = 0xFFFF;
  ultimo_transaccion_fin = millis();
  ultimo_read_poll = 0;

  Serial.println("[MODBUS] Maestro RS485 inicializado en UART1 (RX:38, TX:37 @ 9600 baud).");
}

void ModbusMasterManager_Update() {
  mb_master.task();

  // 1. Si hay una petición en vuelo esperando respuesta del esclavo, esperar a que termine
  if (mb_master.slave() != 0) {
    return;
  }

  // 2. Guard de silencio (15 ms) tras cada fin de trama: permite la conmutación limpia de dirección en RS-485
  if (millis() - ultimo_transaccion_fin < 15) {
    return;
  }

  // 3. Obtenemos una copia atómica de lo que la HMI tiene solicitado
  uint16_t cur_botones = 0;
  uint16_t cur_config[NUM_REGS_CONFIG];
  HMI_Lock();
  cur_botones = HMI.plc_regs[MB_HMI_BOTONES];
  for (size_t i = 0; i < NUM_REGS_CONFIG; i++) {
    cur_config[i] = HMI.plc_regs[REGS_CONFIG[i]];
  }
  HMI_Unlock();

  // PRIORIDAD 1: Si hay cambio en BOTONES o MODOS -> ENVIAR INMEDIATAMENTE
  if (cur_botones != confirmed_botones_sent) {
    inflight_write_reg = MB_HMI_BOTONES;
    inflight_write_val = cur_botones;
    Serial.printf("[MODBUS TX] Despacho Botones (Reg %d = 0x%04X)...\n",
                  MB_HMI_BOTONES, inflight_write_val);
    mb_master.writeHreg(1, MB_HMI_BOTONES, inflight_write_val, cb_plc_write);
    return;
  }

  // PRIORIDAD 2: Si el operador editó algún SETPOINT -> ENVIAR INMEDIATAMENTE
  for (size_t i = 0; i < NUM_REGS_CONFIG; i++) {
    uint8_t reg = REGS_CONFIG[i];
    if (cur_config[i] != confirmed_config_sent[i]) {
      inflight_write_reg = reg;
      inflight_write_val = cur_config[i];
      Serial.printf("[MODBUS TX] Despacho INMEDIATO Parametro Reg %d (Nuevo valor: %d)...\n",
                    reg, inflight_write_val);
      mb_master.writeHreg(1, reg, inflight_write_val, cb_plc_write);
      return;
    }
  }

  // PRIORIDAD 3: Si no hay escrituras pendientes, LECTURA PERIÓDICA cada 250 ms
  if (millis() - ultimo_read_poll >= 250) {
    ultimo_read_poll = millis();
    if (millis() - t_ultimo_req_log > 3000) {
      t_ultimo_req_log = millis();
      Serial.printf("[MODBUS TX] Solicitando lectura de %d registros (Reg 0 a %d)...\n",
                    PLC_CANTIDAD_REGISTROS, PLC_CANTIDAD_REGISTROS - 1);
    }
    mb_master.readHreg(1, 0, rx_plc_regs, PLC_CANTIDAD_REGISTROS, cb_plc_read);
  }
}

// Bucle de la Tarea FreeRTOS dedicada al Núcleo 0
static void TareaModbusLoop(void *pvParameters) {
  Serial.printf("[FREERTOS] Tarea Modbus Master ejecutandose en Nucleo %d\n", xPortGetCoreID());

  while (true) {
    ModbusMasterManager_Update();
    vTaskDelay(pdMS_TO_TICKS(2)); // Cadencia ultra-rápida de 2ms para respuesta inmediata
  }
}

void ModbusMasterManager_StartTask() {
  xTaskCreatePinnedToCore(
    TareaModbusLoop,
    "TaskModbusMaster",
    4096,
    NULL,
    2,                // Prioridad adecuada por encima de idle
    &hModbusTask,
    0                 // Núcleo 0 del procesador ESP32-P4
  );
  Serial.println("[FREERTOS] Tarea Modbus anclada exitosamente al Nucleo 0.");
}