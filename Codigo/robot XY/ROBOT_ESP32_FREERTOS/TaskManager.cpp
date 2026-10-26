#include "TaskManager.h"
#include "Config.h"
#include "RobotData.h"
#include "SafetyManager.h"
#include "GripperManager.h"
#include "MotionManager.h"
#include "ExtractionManager.h"
#include "ModbusManager.h"

TaskHandle_t TaskModbusHandle = NULL;
TaskHandle_t TaskControlHandle = NULL;

// Nucleo 0: Tarea dedicada al bus industrial Modbus RTU RS-485
static void vTaskModbus(void *pvParameters) {
  Serial.printf("[FREERTOS] Tarea Modbus Robot ejecutandose en Nucleo %d\n", xPortGetCoreID());

  for (;;) {
    ModbusManager_Update();
    vTaskDelay(pdMS_TO_TICKS(1)); // 1ms para ceder CPU y mantener latencia minima
  }
}

// Nucleo 1: Control cinematico T-Bot, maquina de estados y ejecucion de secuencias
static void vTaskControl(void *pvParameters) {
  Serial.printf("[FREERTOS] Tarea Control Robot ejecutandose en Nucleo %d\n", xPortGetCoreID());

  // 1. Ejecutar secuencia de Homing inicial al arrancar
  ExtractionManager_Homing();

  for (;;) {
    bool cmd_homing    = false;
    bool cmd_extraer   = false;
    bool cmd_abrir_gp  = false;
    bool cmd_cerrar_gp = false;

    // 2. Comprobar comandos recibidos (Modbus HMI o Monitor Serie)
    Robot_Lock();
    SafetyManager_Update();

    if (bitRead(Robot.comandos, 0)) { cmd_homing = true;    bitWrite(Robot.comandos, 0, 0); }
    if (bitRead(Robot.comandos, 1)) { cmd_extraer = true;   bitWrite(Robot.comandos, 1, 0); }
    if (bitRead(Robot.comandos, 4)) { cmd_abrir_gp = true;  bitWrite(Robot.comandos, 4, 0); }
    if (bitRead(Robot.comandos, 5)) { cmd_cerrar_gp = true; bitWrite(Robot.comandos, 5, 0); }
    Robot_Unlock();

    // 3. Ejecutar las ordenes de forma secuencial y segura
    if (cmd_homing) {
      ExtractionManager_Homing();
    }
    else if (cmd_abrir_gp) {
      Serial.println("[ROBOT] Abriendo Gripper...");
      GripperManager_Abrir();
    }
    else if (cmd_cerrar_gp) {
      Serial.println("[ROBOT] Cerrando Gripper...");
      GripperManager_Cerrar();
    }
    else if (cmd_extraer) {
      ExtractionManager_CicloCompleto();
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void TaskManager_Start() {
  // Crear el semaforo Mutex para sincronizacion inter-nucleos
  robot_mutex = xSemaphoreCreateMutex();

  // Inicializar todos los subsistemas del Robot
  SafetyManager_Init();
  GripperManager_Init();
  MotionManager_Init();
  ExtractionManager_Init();
  ModbusManager_Init();

  // Lanzar las tareas FreeRTOS ancladas a sus nucleos
  // Nucleo 0: Exclusivo para Modbus RTU RS-485
  xTaskCreatePinnedToCore(
    vTaskModbus,
    "TaskModbusRobot",
    4096,
    NULL,
    2,
    &TaskModbusHandle,
    0
  );

  // Nucleo 1: Control de Motores NEMA 17, Cinematica T-Bot, Homing y Ciclos
  xTaskCreatePinnedToCore(
    vTaskControl,
    "TaskControlRobot",
    8192,
    NULL,
    3,
    &TaskControlHandle,
    1
  );
}
