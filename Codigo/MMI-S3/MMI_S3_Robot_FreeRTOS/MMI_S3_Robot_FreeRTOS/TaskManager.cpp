#include "TaskManager.h"
#include <Arduino.h>

#include "MachineData.h"
#include "InputsManager.h"
#include "OutputsManager.h"
#include "ModbusManager.h"
#include "SafetyManager.h"
#include "AlarmManager.h"
#include "RobotManager.h"
#include "CycleManager.h"
#include "TemperatureManager.h"
#include "StatisticsManager.h"
#include "DiagnosticsManager.h"

TaskHandle_t TaskModbus, TaskPLC, TaskTemperatura;

// Núcleo 0: Tarea dedicada al bus industrial Modbus RTU RS-485
void vTaskModbus(void *pvParameters) {
  for (;;) {
    ModbusManager_Update();
    vTaskDelay(pdMS_TO_TICKS(1)); // 1ms de descanso para ceder CPU y mantener latencia mínima
  }
}

// Núcleo 0: Lectura periódica de termocuplas SPI y control SSR
void vTaskTemperatura(void *pvParameters) {
  for (;;) {
    TemperatureManager_Update();
    vTaskDelay(pdMS_TO_TICKS(800));
  }
}

// Núcleo 1: Ciclo principal de control PLC y máquinas de estado
void vTaskPLC(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(100);

  for (;;) {
    PLC_Lock();
    InputsManager_Update();
    SafetyManager_Update();
    AlarmManager_Update();
    RobotManager_Update();
    CycleManager_Update();
    OutputsManager_Update();
    StatisticsManager_Update();
    DiagnosticsManager_Update();
    PLC_Unlock();

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void TaskManager_Start() {
  // Crear el semáforo Mutex para sincronización inter-núcleos
  plc_mutex = xSemaphoreCreateMutex();

  InputsManager_Init();
  OutputsManager_Init();
  TemperatureManager_Init();
  SafetyManager_Init();
  AlarmManager_Init();
  RobotManager_Init();
  CycleManager_Init();
  ModbusManager_Init();
  StatisticsManager_Init();
  DiagnosticsManager_Init();

  // Asignación de tareas a los núcleos del ESP32-S3
  // Núcleo 0: EXCLUSIVO para Modbus RTU RS-485 (máxima prioridad y latencia mínima)
  xTaskCreatePinnedToCore(vTaskModbus,      "Modbus", 8192, NULL, 2, &TaskModbus,      0);

  // Núcleo 1: Control PLC, Lógica de Máquina y Lectura Térmica
  xTaskCreatePinnedToCore(vTaskTemperatura, "Temp",   4096, NULL, 1, &TaskTemperatura, 1);
  xTaskCreatePinnedToCore(vTaskPLC,         "PLC",    8192, NULL, 3, &TaskPLC,         1);
}
