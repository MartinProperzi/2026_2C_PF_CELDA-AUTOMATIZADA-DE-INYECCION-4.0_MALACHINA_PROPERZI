#pragma once
#include <Arduino.h>
#include "Config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

struct RobotState {
  uint16_t estado;
  uint16_t comandos;
  uint16_t handshake;
  uint16_t total_piezas_extraidas;
  int velocidad_x_us;
  int velocidad_y_us;
  uint16_t distancia_x_mm;
  uint16_t distancia_y_mm;
  uint8_t angulo_servo;
  bool homing_completado;
  bool en_zona_segura_fisica;
  bool switch_home_x;
  bool switch_home_y;
  bool en_movimiento;
};

extern RobotState Robot;
extern SemaphoreHandle_t robot_mutex;

inline void Robot_Lock() {
  if (robot_mutex != NULL) xSemaphoreTake(robot_mutex, portMAX_DELAY);
}

inline void Robot_Unlock() {
  if (robot_mutex != NULL) xSemaphoreGive(robot_mutex);
}
