#include "HMIData.h"
#include "SharedModbusMap.h"

SemaphoreHandle_t hmi_mutex = NULL;

void HMIData_Init() {
  if (hmi_mutex == NULL) {
    hmi_mutex = xSemaphoreCreateMutex();
  }
}

void HMI_Lock() {
  if (hmi_mutex != NULL) {
    xSemaphoreTake(hmi_mutex, portMAX_DELAY);
  }
}

void HMI_Unlock() {
  if (hmi_mutex != NULL) {
    xSemaphoreGive(hmi_mutex);
  }
}

void HMI_EnviarComandoRobot(uint16_t cmd) {
  HMI_Lock();
  HMI.robot_cmd_pendiente |= cmd;
  HMI_Unlock();
  Serial.printf("[HMI ROBOT] Comando encolado para Robot ID 2: 0x%04X\n", cmd);
}

HMIData_t HMI = {
  .plc_regs = {
    0,   // 0: MB_SENSORES
    0,   // 1: MB_HMI_BOTONES
    0,   // 2: MB_SDIGITALES
    0,   // 3: MB_CONTROL
    0,   // 4: MB_TESPERA
    0,   // 5: MB_TESPERAI
    5,   // 6: MB_TINYECCION (5 s)
    0,   // 7: MB_TINYECCIONI
    3,   // 8: MB_TSOSTENIMIENTO (3 s)
    0,   // 9: MB_TSOSTENIMIENTOI
    12,  // 10: MB_TENFRIAMIENTO (12 s)
    0,   // 11: MB_TENFRIAMIENTOI
    2,   // 12: MB_TEXP_AVANCE (2 s)
    0,   // 13: MB_TEXP_AVANCEI
    2,   // 14: MB_TEXP_RETROCESO (2 s)
    0,   // 15: MB_TEXP_RETROCESOI
    180, // 16: MB_TEMPZ1 (180 °C)
    180, // 17: MB_SETPOINTZ1 (180 °C)
    190, // 18: MB_TEMPZ2 (190 °C)
    190, // 19: MB_SETPOINTZ2 (190 °C)
    200, // 20: MB_TEMPZ3 (200 °C)
    200, // 21: MB_SETPOINTZ3 (200 °C)
    5,   // 22: MB_HISTERESIS
    0,   // 23: MB_TOTAL_PIEZAS
    0,   // 24: MB_TIEMPO_CICLO
    0    // 25: MB_PASO_ACTUAL
  },
  .robot_regs = {0},
  .plc_conectado = false,
  .robot_conectado = false,
  .robot_cmd_pendiente = 0
};
