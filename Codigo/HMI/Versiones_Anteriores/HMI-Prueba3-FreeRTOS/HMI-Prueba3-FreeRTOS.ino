/*******************************************************************************
 * HMI INYECTORA FLUIDMEC H40-60 (Arquitectura FreeRTOS Doble Núcleo)
 * Para ESP32-P4 JC8012P4A1C - LVGL 9.3
 * 
 * Distribución de Núcleos:
 *   - Núcleo 0: Tarea FreeRTOS dedicada para Modbus RTU RS485 (tiempo real)
 *   - Núcleo 1: HMI gráfica LVGL, PPA GPU, touch y animación de vistas
 ******************************************************************************/
#include <Arduino.h>
#include <chipguy_JC8012P4A1C_display.h>  // Librería de la pantalla
#include <lvgl.h>                         // Motor gráfico
#include "lv_setup.hpp"
#include "UIManager.h"                    // Gestor visual de la HMI

// Capa industrial
#include "ModbusMasterManager.h"
#include "HMIData.h"
#include "SharedModbusMap.h"

void setup() {
  // 1. Inicializamos puerto USB CDC (Nativo) con pausa para estabilización en Windows
  Serial.begin(115200);
  delay(1500);
  Serial.println("==================================================");
  Serial.println(">>> INICIANDO HMI FLUIDMEC H40-60 (ESP32-P4) <<<");
  Serial.println(">>> MODO FREERTOS DUAL-CORE (Core 0: Modbus | Core 1: GUI) <<<");
  Serial.println("==================================================");

  // 2. Inicializamos el Mutex de sincronización inter-núcleos
  HMIData_Init();

  // 3. Iniciamos RS485 (desbloquea UART0 y toma UART1 a 9600 baud)
  ModbusMasterManager_Init();

  // 4. Lanzamos la tarea de Modbus Master anclada al Núcleo 0
  ModbusMasterManager_StartTask();

  // 5. Arranque visual base del hardware de pantalla
  lv_setup.begin();

  // 6. Cargamos el gestor gráfico con las vistas de la inyectora
  UIManager_Init();
}

void loop() {
  // El Núcleo 1 queda 100% libre para la interfaz de usuario:
  // 1. Actualización visual de datos en pantalla
  UIManager_Update();

  // 2. Manejador de eventos y refresco LVGL
  lv_timer_handler();
  delay(2);
}