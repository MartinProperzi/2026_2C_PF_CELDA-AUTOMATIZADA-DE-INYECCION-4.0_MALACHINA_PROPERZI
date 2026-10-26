#pragma once
#include <Arduino.h>

void SafetyManager_Init();
void SafetyManager_Update();
bool Safety_IsHomeX();
bool Safety_IsHomeY();
bool Safety_IsZonaSegura();
