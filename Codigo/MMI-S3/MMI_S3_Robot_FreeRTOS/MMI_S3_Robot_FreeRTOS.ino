#include <Arduino.h>
#include "Config.h"
#include "MachineData.h"
#include "TaskManager.h"

void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial0.begin(115200);
#endif
  delay(1500); // Pausa para estabilización del puerto serie / USB CDC

  Serial.println("\n############################################################");
  Serial.println("  INICIANDO PLC INYECTORA FLUIDMEC - ESP32-S3 (FREERTOS)");
  Serial.println("  Core 0: Modbus RTU RS-485 | Core 1: Control PLC y Seguridad");
  Serial.println("############################################################\n");

#if ARDUINO_USB_CDC_ON_BOOT
  Serial0.println("\n############################################################");
  Serial0.println("  INICIANDO PLC INYECTORA FLUIDMEC - ESP32-S3 (FREERTOS)");
  Serial0.println("  Core 0: Modbus RTU RS-485 | Core 1: Control PLC y Seguridad");
  Serial0.println("############################################################\n");
#endif

  TaskManager_Start();
}

void loop() {
  // Mantenemos loopTask activo con baja prioridad para alimentar el Task Watchdog
  vTaskDelay(pdMS_TO_TICKS(500));
}
