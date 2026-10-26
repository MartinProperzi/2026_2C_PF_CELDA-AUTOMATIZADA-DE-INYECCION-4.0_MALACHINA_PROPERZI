/*******************************************************************************
 * MONITOR ESCLAVO MODBUS RTU (ESP32-S3 + MAX3485 - Arquitectura FreeRTOS)
 * Para pruebas de comunicación y diagnóstico con la HMI Fluidmec (ESP32-P4)
 * 
 * Distribución de Núcleos (ESP32-S3 Dual-Core):
 *   - Núcleo 0: Tarea FreeRTOS dedicada al bus Modbus RTU RS-485 (tiempo real)
 *   - Núcleo 1: loop() Arduino (simulación de proceso, eventos y dashboard USB CDC)
 * 
 * Hardware:
 *   - ESP32-S3
 *   - Módulo transceptor MAX3485 con circuito de auto-dirección (Auto-Flow)
 *   - RX: GPIO16 | TX: GPIO17 (Serial2)
 *   - Puerto Serial USB CDC a 115200 baud para visualización en PC
 ******************************************************************************/
#include <Arduino.h>
#include <ModbusRTU.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Pines UART RS-485 en ESP32-S3
#define RX_PIN 16
#define TX_PIN 17

// Cantidad de Holding Registers compartidos (SharedModbusMap)
#define MB_REGS 26

// Mapeo de Registros Modbus
enum ModbusRegisters {
  MB_SENSORES = 0,
  MB_HMI_BOTONES = 1,
  MB_SDIGITALES = 2,
  MB_CONTROL = 3,
  MB_TESPERA = 4,
  MB_TESPERAI = 5,
  MB_TINYECCION = 6,
  MB_TINYECCIONI = 7,
  MB_TSOSTENIMIENTO = 8,
  MB_TSOSTENIMIENTOI = 9,
  MB_TENFRIAMIENTO = 10,
  MB_TENFRIAMIENTOI = 11,
  MB_TEXP_AVANCE = 12,
  MB_TEXP_AVANCEI = 13,
  MB_TEXP_RETROCESO = 14,
  MB_TEXP_RETROCESOI = 15,
  MB_TEMPZ1 = 16,
  MB_SETPOINTZ1 = 17,
  MB_TEMPZ2 = 18,
  MB_SETPOINTZ2 = 19,
  MB_TEMPZ3 = 20,
  MB_SETPOINTZ3 = 21,
  MB_HISTERESIS = 22,
  MB_TOTAL_PIEZAS = 23,
  MB_TIEMPO_CICLO = 24,
  MB_PASO_ACTUAL = 25
};

// Mapeo de Bits de MB_HMI_BOTONES (Registro 1)
#define BIT_MODO_MANUAL       0
#define BIT_MODO_SEMI         1
#define BIT_MODO_AUTO         2
#define BIT_MOTOR_ON          3
#define BIT_MOTOR_OFF         4
#define BIT_BTN_CARGA         6
#define BIT_BTN_INYECCION     7
#define BIT_BTN_CIERRE        8
#define BIT_BTN_APERTURA      9
#define BIT_BTN_UNIDAD_ATRAS  10
#define BIT_BTN_UNIDAD_ADEL   11
#define BIT_BTN_EXP_ADEL      12
#define BIT_BTN_EXP_ATRAS     13

ModbusRTU mb_esclavo;
static SemaphoreHandle_t s3_mutex = NULL;
static TaskHandle_t hSlaveTask = NULL;

// Copia local para detectar cambios
uint16_t prev_regs[MB_REGS] = {0};

// Variables para simulación de feedback hacia la pantalla
unsigned long t_ultimo_segundo = 0;
unsigned long t_ultimo_dashboard = 0;
uint16_t piezas_contador = 0;
uint16_t temp_sim_z1 = 180;
uint16_t temp_sim_z2 = 190;
uint16_t temp_sim_z3 = 200;

// Nombre de los botones individuales
const char* nombreBoton(uint8_t bit) {
  switch (bit) {
    case BIT_BTN_CIERRE:       return "CIERRE PRENSA";
    case BIT_BTN_APERTURA:     return "APERTURA PRENSA";
    case BIT_BTN_INYECCION:    return "INYECCION MANUAL";
    case BIT_BTN_CARGA:        return "CARGA / DOSIFICACION";
    case BIT_BTN_UNIDAD_ADEL:  return "UNIDAD ADELANTE";
    case BIT_BTN_UNIDAD_ATRAS: return "UNIDAD ATRAS";
    case BIT_BTN_EXP_ADEL:     return "EXPULSOR ADELANTE";
    case BIT_BTN_EXP_ATRAS:    return "EXPULSOR ATRAS";
    case BIT_MOTOR_ON:         return "MOTOR PRINCIPAL ON";
    case BIT_MOTOR_OFF:        return "MOTOR PRINCIPAL OFF";
    default:                   return "BOTON DESCONOCIDO";
  }
}

// Analizar y mostrar cambios en los bits de MB_HMI_BOTONES
void procesarCambiosBotones(uint16_t actual, uint16_t anterior) {
  // 1. Detección de Modo de Operación (Bits 0, 1, 2)
  uint8_t modo_actual = actual & 0x07;
  uint8_t modo_anterior = anterior & 0x07;
  if (modo_actual != modo_anterior) {
    Serial.println("\n------------------------------------------------------------");
    if (bitRead(actual, BIT_MODO_AUTO)) {
      Serial.println("  >>> [MODO DE OPERACION] -> MODO AUTOMATICO ACTIVO <<<");
    } else if (bitRead(actual, BIT_MODO_SEMI)) {
      Serial.println("  >>> [MODO DE OPERACION] -> MODO SEMIAUTOMATICO ACTIVO <<<");
    } else if (bitRead(actual, BIT_MODO_MANUAL)) {
      Serial.println("  >>> [MODO DE OPERACION] -> MODO MANUAL ACTIVO <<<");
    } else {
      Serial.println("  >>> [MODO DE OPERACION] -> NINGUNO (MODO DESELECCIONADO) <<<");
    }
    Serial.println("------------------------------------------------------------");
  }

  // 2. Detección de Motores y Pulsadores de Movimiento
  uint8_t bits_a_chequear[] = {
    BIT_MOTOR_ON, BIT_MOTOR_OFF,
    BIT_BTN_CARGA, BIT_BTN_INYECCION,
    BIT_BTN_CIERRE, BIT_BTN_APERTURA,
    BIT_BTN_UNIDAD_ATRAS, BIT_BTN_UNIDAD_ADEL,
    BIT_BTN_EXP_ADEL, BIT_BTN_EXP_ATRAS
  };

  for (uint8_t bit : bits_a_chequear) {
    bool estado_act = bitRead(actual, bit);
    bool estado_ant = bitRead(anterior, bit);
    if (estado_act != estado_ant) {
      if (estado_act) {
        Serial.printf("[BOTON ACTIVADO]  ===> %s (Bit %d = 1)\n", nombreBoton(bit), bit);
      } else {
        Serial.printf("[BOTON LIBERADO]  ---  %s (Bit %d = 0)\n", nombreBoton(bit), bit);
      }
    }
  }
}

// Imprimir panel general periódico en consola
void imprimirDashboard() {
  if (s3_mutex != NULL) xSemaphoreTake(s3_mutex, portMAX_DELAY);
  uint16_t b = mb_esclavo.Hreg(MB_HMI_BOTONES);
  uint16_t sp_z1 = mb_esclavo.Hreg(MB_SETPOINTZ1);
  uint16_t sp_z2 = mb_esclavo.Hreg(MB_SETPOINTZ2);
  uint16_t sp_z3 = mb_esclavo.Hreg(MB_SETPOINTZ3);
  uint16_t t_iny = mb_esclavo.Hreg(MB_TINYECCION);
  uint16_t t_sos = mb_esclavo.Hreg(MB_TSOSTENIMIENTO);
  uint16_t t_enf = mb_esclavo.Hreg(MB_TENFRIAMIENTO);
  uint16_t t_ea  = mb_esclavo.Hreg(MB_TEXP_AVANCE);
  uint16_t t_er  = mb_esclavo.Hreg(MB_TEXP_RETROCESO);
  uint16_t pz    = mb_esclavo.Hreg(MB_TOTAL_PIEZAS);
  uint16_t tz1   = mb_esclavo.Hreg(MB_TEMPZ1);
  uint16_t tz2   = mb_esclavo.Hreg(MB_TEMPZ2);
  uint16_t tz3   = mb_esclavo.Hreg(MB_TEMPZ3);
  if (s3_mutex != NULL) xSemaphoreGive(s3_mutex);

  Serial.println("\n============================================================");
  Serial.println("  RESUMEN DE ESTADO RECIBIDO DESDE LA HMI (FREERTOS CORE 0) ");
  Serial.println("============================================================");

  // Modo actual
  Serial.print(" * MODO DE OPERACION: ");
  if (bitRead(b, BIT_MODO_AUTO))        Serial.println("AUTOMATICO [OK]");
  else if (bitRead(b, BIT_MODO_SEMI))   Serial.println("SEMIAUTOMATICO [OK]");
  else if (bitRead(b, BIT_MODO_MANUAL)) Serial.println("MANUAL [OK]");
  else                                  Serial.println("NINGUNO (Reposo)");

  // Estado del motor
  Serial.print(" * MOTOR PRINCIPAL:   ");
  if (bitRead(b, BIT_MOTOR_ON))       Serial.println("ENCENDIDO (ON)");
  else if (bitRead(b, BIT_MOTOR_OFF)) Serial.println("APAGADO (OFF)");
  else                                Serial.println("SIN CAMBIOS");

  // Setpoints de Temperatura
  Serial.println(" * SETPOINTS TEMPERATURA:");
  Serial.printf("     - Zona 1: %d °C  (Reg %d)\n", sp_z1, MB_SETPOINTZ1);
  Serial.printf("     - Zona 2: %d °C  (Reg %d)\n", sp_z2, MB_SETPOINTZ2);
  Serial.printf("     - Zona 3: %d °C  (Reg %d)\n", sp_z3, MB_SETPOINTZ3);

  // Setpoints de Tiempos
  Serial.println(" * TIEMPOS DE CICLO (Setpoints fijos):");
  Serial.printf("     - Inyeccion:          %d s  (Reg %d)\n", t_iny, MB_TINYECCION);
  Serial.printf("     - Sostenimiento:      %d s  (Reg %d)\n", t_sos, MB_TSOSTENIMIENTO);
  Serial.printf("     - Moldeo/Enfriamiento:%d s  (Reg %d)\n", t_enf, MB_TENFRIAMIENTO);
  Serial.printf("     - Avance Expulsor:    %d s  (Reg %d)\n", t_ea, MB_TEXP_AVANCE);
  Serial.printf("     - Retroceso Expulsor: %d s  (Reg %d)\n", t_er, MB_TEXP_RETROCESO);

  // Datos enviados como feedback a la pantalla
  Serial.println(" * DATOS ENVIADOS A HMI (Feedback):");
  Serial.printf("     - Total Piezas:  %d  (Visible en cabecera HMI)\n", pz);
  Serial.printf("     - Temp Leidas:   Z1=%d °C, Z2=%d °C, Z3=%d °C\n", tz1, tz2, tz3);
  Serial.println("============================================================\n");
}

// Bucle de la Tarea FreeRTOS dedicada al Core 0 para el Esclavo RS485
static void TareaModbusEsclavoLoop(void *pvParameters) {
  Serial.printf("[FREERTOS] Tarea Esclavo Modbus ejecutandose en Nucleo %d\n", xPortGetCoreID());

  while (true) {
    if (s3_mutex != NULL) xSemaphoreTake(s3_mutex, portMAX_DELAY);
    mb_esclavo.task();
    if (s3_mutex != NULL) xSemaphoreGive(s3_mutex);

    vTaskDelay(pdMS_TO_TICKS(1)); // 1ms de descanso para ceder CPU y mantener latencia minima
  }
}

void setup() {
  // 1. Monitor USB CDC nativo para la consola de la PC
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n############################################################");
  Serial.println("  INICIANDO ESCLAVO MODBUS RTU - FREERTOS DUAL-CORE (ESP32-S3)");
  Serial.println("  Core 0: Bus Modbus RTU RS-485 | Core 1: Dashboard y Proceso ");
  Serial.println("############################################################");

  // 2. RS-485 industrial en Serial2 a 9600 baud (RX:16, TX:17)
  pinMode(RX_PIN, INPUT_PULLUP);
  Serial2.setRxBufferSize(512);
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  // 3. Modbus RTU Esclavo ID 1 (Módulo MAX3485 auto-flow, sin pin de control)
  mb_esclavo.begin(&Serial2);
  mb_esclavo.slave(1);
  mb_esclavo.setBaudrate(9600);

  // Callbacks de diagnóstico para ver en consola de S3 cada petición y respuesta Modbus
  mb_esclavo.onRequest([](Modbus::FunctionCode fc, const Modbus::RequestData data) -> Modbus::ResultCode {
    if (fc == Modbus::FC_READ_REGS) {
      static unsigned long t_s3_read_log = 0;
      if (millis() - t_s3_read_log > 2000) {
        t_s3_read_log = millis();
        Serial.printf("[S3 RX] Peticion LECTURA de la Pantalla: Reg %d (Cantidad: %d)\n", data.reg.address, data.regCount);
      }
    } else if (fc == Modbus::FC_WRITE_REG) {
      Serial.printf("[S3 RX] Peticion ESCRITURA de la Pantalla: Reg %d = %d (0x%04X)\n", 
                    data.reg.address, data.regCount, data.regCount);
    }
    return Modbus::EX_SUCCESS;
  });

  mb_esclavo.onRequestSuccess([](Modbus::FunctionCode fc, const Modbus::RequestData data) -> Modbus::ResultCode {
    if (fc == Modbus::FC_READ_REGS) {
      static unsigned long t_s3_resp_log = 0;
      if (millis() - t_s3_resp_log > 2000) {
        t_s3_resp_log = millis();
        Serial.printf("[S3 TX] RESPUESTA ENVIADA A PANTALLA: %d registros devueltos.\n", data.regCount);
      }
    } else if (fc == Modbus::FC_WRITE_REG) {
      Serial.printf("[S3 TX] CONFIRMACION ENVIADA: Reg %d respondido a la pantalla.\n", data.reg.address);
    }
    return Modbus::EX_SUCCESS;
  });

  // 4. Inicializamos los 26 Holding Registers con valores iniciales coherentes
  for (int i = 0; i < MB_REGS; i++) {
    mb_esclavo.addHreg(i, 0);
  }

  // Valores predeterminados iniciales de proceso
  mb_esclavo.Hreg(MB_SETPOINTZ1, 180);
  mb_esclavo.Hreg(MB_SETPOINTZ2, 190);
  mb_esclavo.Hreg(MB_SETPOINTZ3, 200);
  mb_esclavo.Hreg(MB_TEMPZ1, 180);
  mb_esclavo.Hreg(MB_TEMPZ2, 190);
  mb_esclavo.Hreg(MB_TEMPZ3, 200);
  mb_esclavo.Hreg(MB_TINYECCION, 5);
  mb_esclavo.Hreg(MB_TINYECCIONI, 5);
  mb_esclavo.Hreg(MB_TSOSTENIMIENTO, 3);
  mb_esclavo.Hreg(MB_TSOSTENIMIENTOI, 3);
  mb_esclavo.Hreg(MB_TENFRIAMIENTO, 12);
  mb_esclavo.Hreg(MB_TENFRIAMIENTOI, 12);
  mb_esclavo.Hreg(MB_TEXP_AVANCE, 2);
  mb_esclavo.Hreg(MB_TEXP_AVANCEI, 2);
  mb_esclavo.Hreg(MB_TEXP_RETROCESO, 2);
  mb_esclavo.Hreg(MB_TEXP_RETROCESOI, 2);
  mb_esclavo.Hreg(MB_TOTAL_PIEZAS, 0);

  // Inicialización de Sensores en estado de operación normal (Seguridades cerradas / NC):
  // Bit 9 = 1 (Seta Emergencia rearmada)
  // Bit 6 = 1 (Puerta trasera cerrada)
  // Bit 5 = 1 (Puerta delantera cerrada)
  // Bit 4 = 1 (Filtro y presión de aceite normales)
  uint16_t sens_init = 0;
  bitWrite(sens_init, 9, 1);
  bitWrite(sens_init, 6, 1);
  bitWrite(sens_init, 5, 1);
  bitWrite(sens_init, 4, 1);
  mb_esclavo.Hreg(MB_SENSORES, sens_init);
  mb_esclavo.Hreg(MB_SDIGITALES, 0); // Motor principal inicialmente apagado
  for (int i = 0; i < MB_REGS; i++) {
    prev_regs[i] = mb_esclavo.Hreg(i);
  }

  // 5. Mutex para acceso concurrente thread-safe
  s3_mutex = xSemaphoreCreateMutex();

  // 6. Lanzamos la tarea FreeRTOS anclada al Core 0
  xTaskCreatePinnedToCore(
    TareaModbusEsclavoLoop,
    "TaskModbusSlave",
    4096,
    NULL,
    2,
    &hSlaveTask,
    0 // Núcleo 0
  );

  Serial.println("[MODBUS] Esclavo ID 1 listo en Serial2 (9600 baud, 8N1).");
  Serial.println("[FREERTOS] Tarea anclada al Core 0. Esperando comandos de HMI ESP32-P4...\n");
}

void loop() {
  // -------------------------------------------------------------------------
  // DETECCION DE REGISTROS ENVIADOS / ESCRITOS POR LA HMI
  // -------------------------------------------------------------------------
  uint16_t snap_actual_regs[MB_REGS];
  if (s3_mutex != NULL) xSemaphoreTake(s3_mutex, portMAX_DELAY);
  for (int i = 0; i < MB_REGS; i++) {
    snap_actual_regs[i] = mb_esclavo.Hreg(i);
  }
  if (s3_mutex != NULL) xSemaphoreGive(s3_mutex);

  for (int i = 0; i < MB_REGS; i++) {
    uint16_t actual = snap_actual_regs[i];
    uint16_t anterior = prev_regs[i];

    if (actual != anterior) {
      // 1. Registro de Botones y Modos de Operación
      if (i == MB_HMI_BOTONES) {
        procesarCambiosBotones(actual, anterior);
      }
      // 2. Setpoints de Temperatura
      else if (i == MB_SETPOINTZ1) {
        Serial.printf("[SETPOINT TEMP] Zona 1 modificado desde HMI: %d °C (antes %d °C)\n", actual, anterior);
      }
      else if (i == MB_SETPOINTZ2) {
        Serial.printf("[SETPOINT TEMP] Zona 2 modificado desde HMI: %d °C (antes %d °C)\n", actual, anterior);
      }
      else if (i == MB_SETPOINTZ3) {
        Serial.printf("[SETPOINT TEMP] Zona 3 modificado desde HMI: %d °C (antes %d °C)\n", actual, anterior);
      }
      // 3. Setpoints de Tiempos
      else if (i == MB_TINYECCION || i == MB_TINYECCIONI) {
        Serial.printf("[SETPOINT TIEMPO] Inyeccion modificado desde HMI: %d s (Reg %d)\n", actual, i);
      }
      else if (i == MB_TSOSTENIMIENTO || i == MB_TSOSTENIMIENTOI) {
        Serial.printf("[SETPOINT TIEMPO] Sostenimiento modificado desde HMI: %d s (Reg %d)\n", actual, i);
      }
      else if (i == MB_TENFRIAMIENTO || i == MB_TENFRIAMIENTOI) {
        Serial.printf("[SETPOINT TIEMPO] Enfriamiento/Moldeo modificado desde HMI: %d s (Reg %d)\n", actual, i);
      }
      else if (i == MB_TEXP_AVANCE || i == MB_TEXP_AVANCEI) {
        Serial.printf("[SETPOINT TIEMPO] Avance Expulsor modificado desde HMI: %d s (Reg %d)\n", actual, i);
      }
      else if (i == MB_TEXP_RETROCESO || i == MB_TEXP_RETROCESOI) {
        Serial.printf("[SETPOINT TIEMPO] Retroceso Expulsor modificado desde HMI: %d s (Reg %d)\n", actual, i);
      }
      // 4. Otros Registros
      else {
        Serial.printf("[DATO HMI] Registro %d actualizado: %d (antes %d)\n", i, actual, anterior);
      }

      // Actualizamos la copia local
      prev_regs[i] = actual;
    }
  }

  // -------------------------------------------------------------------------
  // SIMULACION DE FEEDBACK A LA HMI (Conteo de piezas y temperaturas en vivo)
  // -------------------------------------------------------------------------
  if (millis() - t_ultimo_segundo > 2000) {
    t_ultimo_segundo = millis();

    // Incrementa contador de piezas para que la HMI lo muestre en pantalla
    piezas_contador++;

    int osc = (piezas_contador % 2 == 0) ? 1 : -1;

    if (s3_mutex != NULL) xSemaphoreTake(s3_mutex, portMAX_DELAY);
    mb_esclavo.Hreg(MB_TOTAL_PIEZAS, piezas_contador);
    mb_esclavo.Hreg(MB_TEMPZ1, mb_esclavo.Hreg(MB_SETPOINTZ1) + osc);
    mb_esclavo.Hreg(MB_TEMPZ2, mb_esclavo.Hreg(MB_SETPOINTZ2) - osc);
    mb_esclavo.Hreg(MB_TEMPZ3, mb_esclavo.Hreg(MB_SETPOINTZ3) + osc);

    prev_regs[MB_TOTAL_PIEZAS] = piezas_contador;
    prev_regs[MB_TEMPZ1] = mb_esclavo.Hreg(MB_TEMPZ1);
    prev_regs[MB_TEMPZ2] = mb_esclavo.Hreg(MB_TEMPZ2);
    prev_regs[MB_TEMPZ3] = mb_esclavo.Hreg(MB_TEMPZ3);
    if (s3_mutex != NULL) xSemaphoreGive(s3_mutex);
  }

  // -------------------------------------------------------------------------
  // DASHBOARD PERIODICO CADA 5 SEGUNDOS EN EL MONITOR SERIE
  // -------------------------------------------------------------------------
  if (millis() - t_ultimo_dashboard > 5000) {
    t_ultimo_dashboard = millis();
    imprimirDashboard();
  }

  // -------------------------------------------------------------------------
  // CONTROL INTERACTIVO POR MONITOR SERIE (Para pruebas con la HMI)
  // -------------------------------------------------------------------------
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'm' || c == 'M') {
      if (s3_mutex != NULL) xSemaphoreTake(s3_mutex, portMAX_DELAY);
      uint16_t v = mb_esclavo.Hreg(MB_SDIGITALES);
      bool act = bitRead(v, 7);
      bitWrite(v, 7, !act);
      mb_esclavo.Hreg(MB_SDIGITALES, v);
      bool nuevo = bitRead(v, 7);
      if (s3_mutex != NULL) xSemaphoreGive(s3_mutex);
      Serial.printf("\n>>> [SIMULADOR S3] Motor Principal: %s <<<\n\n", nuevo ? "ENCENDIDO / EN MARCHA (ON)" : "APAGADO / DETENIDO (OFF)");
    } else if (c == 'e' || c == 'E') {
      if (s3_mutex != NULL) xSemaphoreTake(s3_mutex, portMAX_DELAY);
      uint16_t v = mb_esclavo.Hreg(MB_SENSORES);
      bool act = bitRead(v, 9);
      bitWrite(v, 9, !act);
      mb_esclavo.Hreg(MB_SENSORES, v);
      bool nuevo = bitRead(v, 9);
      if (s3_mutex != NULL) xSemaphoreGive(s3_mutex);
      Serial.printf("\n>>> [SIMULADOR S3] Seta de Emergencia: %s <<<\n\n", nuevo ? "NORMAL (Circuito cerrado)" : "ALARMA (Seta pulsada)");
    } else if (c == 'p' || c == 'P') {
      if (s3_mutex != NULL) xSemaphoreTake(s3_mutex, portMAX_DELAY);
      uint16_t v = mb_esclavo.Hreg(MB_SENSORES);
      bool act = bitRead(v, 5);
      bitWrite(v, 5, !act);
      bitWrite(v, 6, !act);
      mb_esclavo.Hreg(MB_SENSORES, v);
      bool nuevo = bitRead(v, 5);
      if (s3_mutex != NULL) xSemaphoreGive(s3_mutex);
      Serial.printf("\n>>> [SIMULADOR S3] Puertas Seguridad: %s <<<\n\n", nuevo ? "CERRADAS (Normal)" : "ABIERTAS (Alarma)");
    } else if (c == 'a' || c == 'A') {
      if (s3_mutex != NULL) xSemaphoreTake(s3_mutex, portMAX_DELAY);
      uint16_t v = mb_esclavo.Hreg(MB_SENSORES);
      bool act = bitRead(v, 4);
      bitWrite(v, 4, !act);
      mb_esclavo.Hreg(MB_SENSORES, v);
      bool nuevo = bitRead(v, 4);
      if (s3_mutex != NULL) xSemaphoreGive(s3_mutex);
      Serial.printf("\n>>> [SIMULADOR S3] Filtro Aceite / Presostato: %s <<<\n\n", nuevo ? "NORMAL (Presion OK)" : "ALARMA (Filtro Obstruido)");
    } else if (c == 's' || c == 'S') {
      if (s3_mutex != NULL) xSemaphoreTake(s3_mutex, portMAX_DELAY);
      uint16_t v = mb_esclavo.Hreg(MB_SDIGITALES);
      bool act = bitRead(v, 8);
      bitWrite(v, 8, !act);
      mb_esclavo.Hreg(MB_SDIGITALES, v);
      bool nuevo = bitRead(v, 8);
      if (s3_mutex != NULL) xSemaphoreGive(s3_mutex);
      Serial.printf("\n>>> [SIMULADOR S3] Baliza / Sirena de Alarma: %s <<<\n\n", nuevo ? "SONANDO (Alarma activa)" : "SILENCIADA");
    } else if (c == '?' || c == 'h' || c == 'H') {
      Serial.println("\n---------------- TECLAS DE PRUEBA SERIE ----------------");
      Serial.println(" [m] Conmutar Motor Principal ON / OFF (MB_SDIGITALES Bit 7)");
      Serial.println(" [e] Conmutar Parada Emergencia (MB_SENSORES Bit 9)");
      Serial.println(" [p] Conmutar Puertas de Seguridad (MB_SENSORES Bit 5 y 6)");
      Serial.println(" [a] Conmutar Filtro / Presión Aceite (MB_SENSORES Bit 4)");
      Serial.println(" [s] Conmutar Baliza / Sirena Alarma (MB_SDIGITALES Bit 8)");
      Serial.println("--------------------------------------------------------\n");
    }
  }

  delay(10);
}
