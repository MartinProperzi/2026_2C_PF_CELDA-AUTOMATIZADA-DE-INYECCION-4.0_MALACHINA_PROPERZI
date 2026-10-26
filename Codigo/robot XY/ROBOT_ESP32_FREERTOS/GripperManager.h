#pragma once
#include <Arduino.h>

void GripperManager_Init();
void GripperManager_Abrir();
void GripperManager_Cerrar();
void GripperManager_SetAngle(int angle);
uint8_t GripperManager_GetAngle();
