#pragma once
#include <Arduino.h>

void MotionManager_Init();
void MotionManager_PasoUnico(long dx_dir, long dy_dir, int retardoUs);
void MotionManager_Mover(long dx, long dy, int retardoUs);
void MotionManager_ConmutarMotores(long pasosM1, long pasosM2, int retardoUs);
