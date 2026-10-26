/*******************************************************************************
 * ROBOT CARTESIANO T-BOT EXTRACTOR DE PIEZAS (ESP32 - FreeRTOS Doble Nucleo)
 * Inyectora Fluidmec H40-60 - Esclavo Modbus RTU ID 2
 * 
 * Arquitectura de Software Modular (Idem PLC ESP32-S3):
 *   - Config.h: Mapeo de pines, constantes cinematicas, registros Modbus y estados
 *   - RobotData.h/.cpp: Estructura global del Robot y mutex inter-nucleos
 *   - SafetyManager.h/.cpp: Fines de carrera (Home X, Home Y, Zona Segura) e interlocks
 *   - GripperManager.h/.cpp: Control de pinza con servomotor (abrir, cerrar, posicion)
 *   - MotionManager.h/.cpp: Cinematica T-Bot / CoreXY e interpolacion Bresenham
 *   - ExtractionManager.h/.cpp: Rutina de Homing y secuencia de extraccion en 8 etapas
 *   - ModbusManager.h/.cpp: Driver Esclavo Modbus RTU RS-485 y sincronizacion
 *   - TaskManager.h/.cpp: Creacion de tareas FreeRTOS ancladas a nucleos 0 y 1
 ******************************************************************************/

#include <Arduino.h>
#include "Config.h"
#include "RobotData.h"
#include "TaskManager.h"

void setup() {
  Serial.begin(115200);
  delay(1500); // Pausa para estabilizacion de la consola serie

  Serial.println("\n############################################################");
  Serial.println(">>> INICIANDO ROBOT CARTESIANO T-BOT (ESP32 - FREERTOS) <<<");
  Serial.println("  Core 0: Bus Industrial Modbus RTU RS-485 (Esclavo ID 2)");
  Serial.println("  Core 1: Control Cinematico T-Bot, Homing y Gripper");
  Serial.printf("  Velocidades: Eje X = %d us | Eje Y = %d us\n", 
                RETARDO_X_US_DEFAULT, RETARDO_Y_US_DEFAULT);
  Serial.println("############################################################\n");

  TaskManager_Start();
}

void loop() {
  // Consola interactiva por Monitor Serie para mantenimiento y pruebas de taller
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'h' || c == 'H') {
      Robot_Lock();
      bitWrite(Robot.comandos, 0, 1);
      Robot_Unlock();
      Serial.println("[CLI] Comando recibido: Homing");
    }
    else if (c == 'e' || c == 'E') {
      Robot_Lock();
      bitWrite(Robot.comandos, 1, 1);
      Robot_Unlock();
      Serial.println("[CLI] Comando recibido: Iniciar Ciclo de Extraccion");
    }
    else if (c == 'a' || c == 'A') {
      Robot_Lock();
      bitWrite(Robot.comandos, 4, 1);
      Robot_Unlock();
      Serial.println("[CLI] Comando recibido: Abrir Gripper");
    }
    else if (c == 'c' || c == 'C') {
      Robot_Lock();
      bitWrite(Robot.comandos, 5, 1);
      Robot_Unlock();
      Serial.println("[CLI] Comando recibido: Cerrar Gripper");
    }
    else if (c == '?' || c == 'm' || c == 'M') {
      Robot_Lock();
      Serial.println("\n--- MENU DE PRUEBA MANUAL ROBOT CARTESIANO ---");
      Serial.println(" [h] Ejecutar Homing (Cero de maquina)");
      Serial.println(" [e] Ejecutar Ciclo Completo de Extraccion");
      Serial.println(" [a] Abrir Pinza (Gripper 110 deg)");
      Serial.println(" [c] Cerrar Pinza (Gripper 0 deg)");
      Serial.printf(" Estado actual: %d | Piezas: %d\n", Robot.estado, Robot.total_piezas_extraidas);
      Serial.printf(" Velocidades: X = %d us (%.1f cm/s) | Y = %d us (%.1f cm/s)\n",
                    Robot.velocidad_x_us, 50000.0 / Robot.velocidad_x_us,
                    Robot.velocidad_y_us, 50000.0 / Robot.velocidad_y_us);
      Serial.printf(" Switches: Home X=%d | Home Y=%d | Zona Segura=%d\n",
                    Robot.switch_home_x, Robot.switch_home_y, Robot.en_zona_segura_fisica);
      Serial.println("----------------------------------------------\n");
      Robot_Unlock();
    }
  }

  // Ceder CPU para mantener el watchdog de FreeRTOS alimentado
  vTaskDelay(pdMS_TO_TICKS(50));
}