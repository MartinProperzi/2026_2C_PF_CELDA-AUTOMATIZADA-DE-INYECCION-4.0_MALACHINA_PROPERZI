#include "RobotData.h"

RobotState Robot = {
  .estado = ROBOT_DESCONOCIDO,
  .comandos = 0,
  .handshake = 0,
  .total_piezas_extraidas = 0,
  .velocidad_x_us = RETARDO_X_US_DEFAULT,
  .velocidad_y_us = RETARDO_Y_US_DEFAULT,
  .distancia_x_mm = 600,
  .distancia_y_mm = 400,
  .angulo_servo = ANGULO_ABIERTO,
  .homing_completado = false,
  .en_zona_segura_fisica = false,
  .switch_home_x = false,
  .switch_home_y = false,
  .en_movimiento = false
};

SemaphoreHandle_t robot_mutex = NULL;
