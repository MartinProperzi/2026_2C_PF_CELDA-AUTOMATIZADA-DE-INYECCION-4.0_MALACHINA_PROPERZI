#include "ModbusMasterManager.h"
#include "HMIData.h"
#include <ModbusRTU.h>

// =========================================================================
// Mapeo verificado en conector UART de la placa Guition JC8012P4A1C (MAX3485)
// =========================================================================
#define RX_PIN 38 // GPIO38 RX
#define TX_PIN 37 // GPIO37 TX

#define SLAVE_PLC_ID   1
#define SLAVE_ROBOT_ID 2

ModbusRTU mb_master;
static unsigned long ultimo_transaccion_fin = 0;
static unsigned long ultimo_poll_tick = 0;
static unsigned long t_ultimo_plc_log = 0;
static unsigned long t_ultimo_robot_log = 0;
static TaskHandle_t hModbusTask = NULL;

// Buffers locales aislados para recepciones Modbus (no interfieren con la UI mientras se llenan)
static uint16_t rx_plc_regs[PLC_CANTIDAD_REGISTROS];
static uint16_t rx_robot_regs[ROBOT_CANTIDAD_REGISTROS];

// Alternancia de sondeo de esclavos (1 = PLC, 2 = Robot)
static uint8_t esclavo_proximo_poll = SLAVE_PLC_ID;

// Registros de configuración del PLC (temperaturas y tiempos fijos)
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

// Variables de transacción de escritura en vuelo
static uint8_t  inflight_slave = 0;
static uint16_t inflight_write_reg = 0xFFFF;
static uint16_t inflight_write_val = 0;

// Conteo de fallos consecutivos por esclavo
static uint8_t fallos_lectura_plc = 0;
static uint8_t fallos_lectura_robot = 0;

// =========================================================================
// VARIABLES DE ESTADO DEL HANDSHAKE AUTOMÁTICO (PLC <-> ROBOT)
// =========================================================================
static bool handshake_extraccion_disparada = false;
static bool handshake_ejector_fwd_disparado = false;
static bool handshake_ejector_rev_disparado = false;
static bool handshake_permiso_cierre_disparado = false;

// Banderas de memorizacion (latch) para evitar condiciones de carrera por tiempos Modbus
static bool latch_robot_pide_fwd = false;
static bool latch_robot_pide_rev = false;

static uint16_t pending_plc_control_bits = 0;
static uint16_t pending_robot_cmd_bits   = 0;

// =========================================================================
// CALLBACKS DE LECTURA MODBUS
// =========================================================================

// Callback lectura PLC (Esclavo ID 1)
bool cb_plc_read(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  ultimo_transaccion_fin = millis();

  if (event == Modbus::EX_SUCCESS) {
    fallos_lectura_plc = 0;

    HMI_Lock();
    uint16_t local_botones = HMI.plc_regs[MB_HMI_BOTONES];
    memcpy(HMI.plc_regs, rx_plc_regs, sizeof(HMI.plc_regs));
    HMI.plc_regs[MB_HMI_BOTONES] = local_botones; // Los botones los gestiona la pantalla
    HMI.plc_conectado = true;

    for (size_t i = 0; i < NUM_REGS_CONFIG; i++) {
      confirmed_config_sent[i] = HMI.plc_regs[REGS_CONFIG[i]];
    }
    HMI_Unlock();

    if (millis() - t_ultimo_plc_log > 3000) {
      t_ultimo_plc_log = millis();
      Serial.printf("[PLC OK] Paso: %d | Piezas: %d | Z1: %d C | Z2: %d C | Z3: %d C\n",
                    rx_plc_regs[MB_PASO_ACTUAL], rx_plc_regs[MB_TOTAL_PIEZAS],
                    rx_plc_regs[MB_TEMPZ1], rx_plc_regs[MB_TEMPZ2], rx_plc_regs[MB_TEMPZ3]);
    }
  } else {
    fallos_lectura_plc++;
    if (fallos_lectura_plc >= 3) {
      HMI_Lock();
      HMI.plc_conectado = false;
      HMI_Unlock();
      Serial.printf("[PLC ALERTA] Fallo comunicacion con PLC (ID 1). Codigo: 0x%02X\n", event);
    }
  }
  return true;
}

// Callback lectura Robot Extractor (Esclavo ID 2)
bool cb_robot_read(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  ultimo_transaccion_fin = millis();

  if (event == Modbus::EX_SUCCESS) {
    fallos_lectura_robot = 0;

    HMI_Lock();
    memcpy(HMI.robot_regs, rx_robot_regs, sizeof(HMI.robot_regs));
    HMI.robot_conectado = true;
    HMI_Unlock();

    if (millis() - t_ultimo_robot_log > 3000) {
      t_ultimo_robot_log = millis();
      Serial.printf("[ROBOT OK] Estado: %d | Handshake: 0x%04X | Ciclos: %d | Servo: %d deg\n",
                    rx_robot_regs[MB_ROBOT_ESTADO], rx_robot_regs[MB_ROBOT_HANDSHAKE],
                    rx_robot_regs[MB_ROBOT_TOTAL_CICLOS], rx_robot_regs[MB_ROBOT_ANGULO_SERVO]);
    }
  } else {
    fallos_lectura_robot++;
    if (fallos_lectura_robot >= 3) {
      HMI_Lock();
      HMI.robot_conectado = false;
      HMI_Unlock();
      Serial.printf("[ROBOT ALERTA] Fallo comunicacion con Robot (ID 2). Codigo: 0x%02X\n", event);
    }
  }
  return true;
}

// =========================================================================
// CALLBACK DE ESCRITURA MODBUS (PLC O ROBOT)
// =========================================================================
bool cb_master_write(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  ultimo_transaccion_fin = millis();

  if (event == Modbus::EX_SUCCESS) {
    if (inflight_slave == SLAVE_PLC_ID) {
      HMI_Lock();
      HMI.plc_conectado = true;
      HMI_Unlock();

      if (inflight_write_reg == MB_HMI_BOTONES) {
        confirmed_botones_sent = inflight_write_val;
        Serial.printf("[PLC TX OK] Botones confirmados por PLC (0x%04X)\n", confirmed_botones_sent);
      } else if (inflight_write_reg == MB_CONTROL) {
        pending_plc_control_bits = 0;
        Serial.printf("[PLC TX OK] Handshake Bit(s) confirmados por PLC (0x%04X)\n", inflight_write_val);
      } else {
        for (size_t i = 0; i < NUM_REGS_CONFIG; i++) {
          if (REGS_CONFIG[i] == inflight_write_reg) {
            confirmed_config_sent[i] = inflight_write_val;
            Serial.printf("[PLC TX OK] Parametro Reg %d confirmado por PLC (%d)\n",
                          inflight_write_reg, inflight_write_val);
            break;
          }
        }
      }
    } else if (inflight_slave == SLAVE_ROBOT_ID) {
      HMI_Lock();
      HMI.robot_conectado = true;
      HMI_Unlock();

      if (inflight_write_reg == MB_ROBOT_COMANDO) {
        pending_robot_cmd_bits = 0;
        Serial.printf("[ROBOT TX OK] Comando confirmado por Robot (0x%04X)\n", inflight_write_val);
      }
    }
  } else {
    Serial.printf("[MODBUS ERROR] Fallo confirmacion escritura en Esclavo %d, Reg %d (Codigo: 0x%02X)\n",
                  inflight_slave, inflight_write_reg, event);
  }

  inflight_slave = 0;
  inflight_write_reg = 0xFFFF;
  return true;
}

// =========================================================================
// INICIALIZACIÓN DEL MAESTRO RS-485
// =========================================================================
void ModbusMasterManager_Init() {
  // Destruir UART0 por defecto que secuestra pines 37 y 38
  Serial0.end();

  pinMode(RX_PIN, INPUT_PULLUP);

  // Inicializar UART1 a 9600 baud
  Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  mb_master.begin(&Serial1);
  mb_master.master();
  mb_master.setBaudrate(9600);
  mb_master.setInterFrameTime(5000); // 5ms de tolerancia entre tramas

  confirmed_botones_sent = 0;
  inflight_slave = 0;
  inflight_write_reg = 0xFFFF;
  ultimo_transaccion_fin = millis();
  ultimo_poll_tick = 0;
  esclavo_proximo_poll = SLAVE_PLC_ID;
  latch_robot_pide_fwd = false;
  latch_robot_pide_rev = false;

  Serial.println("[MODBUS] Maestro RS-485 iniciado en UART1 (RX:38, TX:37 @ 9600 baud).");
  Serial.println("[MODBUS] Controlando Red Multidrop: PLC (ID 1) y Robot Cartesiano (ID 2).");
}

// =========================================================================
// LÓGICA DE ACTUALIZACIÓN Y PUENTE DE SINCRONIZACIÓN (HANDSHAKE)
// =========================================================================
void ModbusMasterManager_Update() {
  mb_master.task();

  // 1. Si hay una petición en el bus esperando respuesta, aguardar
  if (mb_master.slave() != 0) {
    return;
  }

  // 2. Tiempo de silencio de seguridad en el bus (15 ms)
  if (millis() - ultimo_transaccion_fin < 15) {
    return;
  }

  // 3. Tomar captura atómica del estado del sistema
  uint16_t cur_botones = 0;
  uint16_t cur_config[NUM_REGS_CONFIG];
  uint16_t cur_paso = 0;
  bool plc_ok = false;
  bool robot_ok = false;
  uint16_t rb_estado = 0;
  uint16_t rb_handshake = 0;
  uint16_t user_cmd_robot = 0;

  HMI_Lock();
  cur_botones = HMI.plc_regs[MB_HMI_BOTONES];
  cur_paso    = HMI.plc_regs[MB_PASO_ACTUAL];
  plc_ok      = HMI.plc_conectado;
  robot_ok    = HMI.robot_conectado;
  rb_estado   = HMI.robot_regs[MB_ROBOT_ESTADO];
  rb_handshake = HMI.robot_regs[MB_ROBOT_HANDSHAKE];
  user_cmd_robot = HMI.robot_cmd_pendiente;
  HMI.robot_cmd_pendiente = 0; // Consumido

  for (size_t i = 0; i < NUM_REGS_CONFIG; i++) {
    cur_config[i] = HMI.plc_regs[REGS_CONFIG[i]];
  }
  HMI_Unlock();

  // Si el operador pulsó algún comando manual para el Robot en la HMI
  if (user_cmd_robot != 0) {
    pending_robot_cmd_bits |= user_cmd_robot;
  }

  // =========================================================================
  // PUENTE DE HANDSHAKE INDUSTRIAL (PLC S3 <---> ROBOT T-BOT)
  // =========================================================================
  if (plc_ok && robot_ok) {
    // CONDICIÓN A: El molde completó su apertura (Paso 9) -> Ordenar Extracción al Robot
    if (cur_paso == 9) {
      if (!handshake_extraccion_disparada) {
        if (rb_estado == ROBOT_LISTO_EN_HOME || bitRead(rb_handshake, 0) == 1) {
          pending_robot_cmd_bits |= (1 << 1); // Bit 1 = cmd_extraer
          handshake_extraccion_disparada = true;
          Serial.println("[HANDSHAKE] Molde abierto (Paso 9) -> Disparando Ciclo de Extracción al Robot!");
        }
      }
    } else {
      handshake_extraccion_disparada = false;
    }

    // CONDICIÓN B: Robot posicionado en cavidad pidiendo avance de expulsor (Bit 1 de Handshake)
    if (bitRead(rb_handshake, 1) == 1) {
      latch_robot_pide_fwd = true;
    }
    if (latch_robot_pide_fwd && cur_paso == 9) {
      if (!handshake_ejector_fwd_disparado) {
        pending_plc_control_bits |= (1 << 10); // Bit 10 = robot_pide_expulsor_adelante
        handshake_ejector_fwd_disparado = true;
        Serial.println("[HANDSHAKE] Robot en cavidad -> Ordenando Expulsor Adelante al PLC (Bit 10).");
      }
    }
    if (cur_paso != 9 && cur_paso != 10) {
      handshake_ejector_fwd_disparado = false;
      latch_robot_pide_fwd = false;
    }

    // CONDICIÓN C: Robot cerró pinza sobre la pieza y pide retroceso de expulsor (Bit 2 de Handshake)
    if (bitRead(rb_handshake, 2) == 1) {
      latch_robot_pide_rev = true;
    }
    if (latch_robot_pide_rev && cur_paso == 11) {
      if (!handshake_ejector_rev_disparado) {
        pending_plc_control_bits |= (1 << 11); // Bit 11 = robot_pide_expulsor_atras
        handshake_ejector_rev_disparado = true;
        Serial.println("[HANDSHAKE] Robot sujeto pieza -> Ordenando Expulsor Atras al PLC (Bit 11).");
      }
    }
    if (cur_paso != 11 && cur_paso != 12) {
      handshake_ejector_rev_disparado = false;
      latch_robot_pide_rev = false;
    }

    // CONDICIÓN D: Robot en Zona Segura o Home -> Permiso de Cierre para el PLC (Bit 3 de Handshake)
    if (bitRead(rb_handshake, 3) == 1) {
      bool modo_ciclico = bitRead(cur_botones, 1) || bitRead(cur_botones, 2);
      if (!handshake_permiso_cierre_disparado && cur_paso == 0 && modo_ciclico) {
        pending_plc_control_bits |= (1 << 4); // Bit 4 = robot_permite_cierre
        handshake_permiso_cierre_disparado = true;
        Serial.println("[HANDSHAKE] Robot en Zona Segura -> Concediendo Permiso de Cierre al PLC (Bit 4).");
      }
    } else {
      handshake_permiso_cierre_disparado = false;
    }
  }

  // =========================================================================
  // DESPACHO DE ESCRITURAS PRIORITARIAS HACIA EL BUS RS-485
  // =========================================================================

  // PRIORIDAD 1: Botones del Operador para el PLC
  if (cur_botones != confirmed_botones_sent) {
    inflight_slave = SLAVE_PLC_ID;
    inflight_write_reg = MB_HMI_BOTONES;
    inflight_write_val = cur_botones;
    Serial.printf("[MODBUS TX] Despacho Botones PLC (Reg %d = 0x%04X)...\n",
                  MB_HMI_BOTONES, inflight_write_val);
    mb_master.writeHreg(SLAVE_PLC_ID, MB_HMI_BOTONES, inflight_write_val, cb_master_write);
    return;
  }

  // PRIORIDAD 2: Bits de Handshake hacia el PLC (MB_CONTROL)
  if (pending_plc_control_bits != 0) {
    inflight_slave = SLAVE_PLC_ID;
    inflight_write_reg = MB_CONTROL;
    inflight_write_val = pending_plc_control_bits;
    Serial.printf("[MODBUS TX] Despacho Handshake PLC (Reg %d = 0x%04X)...\n",
                  MB_CONTROL, inflight_write_val);
    mb_master.writeHreg(SLAVE_PLC_ID, MB_CONTROL, inflight_write_val, cb_master_write);
    return;
  }

  // PRIORIDAD 3: Comandos hacia el Robot Extractor (MB_ROBOT_COMANDO)
  if (pending_robot_cmd_bits != 0) {
    inflight_slave = SLAVE_ROBOT_ID;
    inflight_write_reg = MB_ROBOT_COMANDO;
    inflight_write_val = pending_robot_cmd_bits;
    Serial.printf("[MODBUS TX] Despacho Comando ROBOT (Reg %d = 0x%04X)...\n",
                  MB_ROBOT_COMANDO, inflight_write_val);
    mb_master.writeHreg(SLAVE_ROBOT_ID, MB_ROBOT_COMANDO, inflight_write_val, cb_master_write);
    return;
  }

  // PRIORIDAD 4: Setpoints de Proceso editados hacia el PLC
  for (size_t i = 0; i < NUM_REGS_CONFIG; i++) {
    uint8_t reg = REGS_CONFIG[i];
    if (cur_config[i] != confirmed_config_sent[i]) {
      inflight_slave = SLAVE_PLC_ID;
      inflight_write_reg = reg;
      inflight_write_val = cur_config[i];
      Serial.printf("[MODBUS TX] Despacho Parametro Reg %d (Nuevo valor: %d)...\n",
                    reg, inflight_write_val);
      mb_master.writeHreg(SLAVE_PLC_ID, reg, inflight_write_val, cb_master_write);
      return;
    }
  }

  // =========================================================================
  // PRIORIDAD 5: LECTURA PERIÓDICA ALTERNADA (ROUND-ROBIN CADA 120 MS)
  // Cada esclavo se actualiza cada 240 ms (~4 veces por segundo)
  // =========================================================================
  if (millis() - ultimo_poll_tick >= 120) {
    ultimo_poll_tick = millis();

    if (esclavo_proximo_poll == SLAVE_PLC_ID) {
      esclavo_proximo_poll = SLAVE_ROBOT_ID; // Próximo turno para el robot
      mb_master.readHreg(SLAVE_PLC_ID, 0, rx_plc_regs, PLC_CANTIDAD_REGISTROS, cb_plc_read);
    } else {
      esclavo_proximo_poll = SLAVE_PLC_ID; // Próximo turno para el PLC
      mb_master.readHreg(SLAVE_ROBOT_ID, 0, rx_robot_regs, ROBOT_CANTIDAD_REGISTROS, cb_robot_read);
    }
  }
}

// Bucle de la Tarea FreeRTOS dedicada al Núcleo 0
static void TareaModbusLoop(void *pvParameters) {
  Serial.printf("[FREERTOS] Tarea Modbus Master (PLC + Robot) ejecutandose en Nucleo %d\n", xPortGetCoreID());

  while (true) {
    ModbusMasterManager_Update();
    vTaskDelay(pdMS_TO_TICKS(2)); // Cadencia ultra-rápida de 2ms para respuesta de bus inmediata
  }
}

void ModbusMasterManager_StartTask() {
  xTaskCreatePinnedToCore(
    TareaModbusLoop,
    "TaskModbusMaster",
    4096,
    NULL,
    2,                // Prioridad adecuada
    &hModbusTask,
    0                 // Núcleo 0 del procesador ESP32-P4
  );
  Serial.println("[FREERTOS] Tarea Modbus Master anclada exitosamente al Nucleo 0.");
}
