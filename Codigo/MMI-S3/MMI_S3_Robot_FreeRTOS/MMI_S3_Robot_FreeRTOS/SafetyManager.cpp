#include "SafetyManager.h"
#include "MachineData.h"
#include "Config.h"

void SafetyManager_Init() {
  PLC.estado_emergencia = false;
}

void SafetyManager_AbortCycle() {
  bool estado_motor = bitRead(PLC.regs[MB_SDIGITALES], 7);
  bool estado_baliza = bitRead(PLC.regs[MB_SDIGITALES], 8);
  PLC.regs[MB_SDIGITALES] = 0;
  bitWrite(PLC.regs[MB_SDIGITALES], 7, estado_motor);
  bitWrite(PLC.regs[MB_SDIGITALES], 8, estado_baliza);
  PLC.ciclo_activo = false;
  PLC.paso_actual = 0;
}

void SafetyManager_Update() {
  bool emergencia_fisica = (bitRead(PLC.regs[MB_SENSORES], 9) == 0);
  bool emergencia_virtual = (bitRead(PLC.regs[MB_CONTROL], 1) == 1);

  if (emergencia_fisica || emergencia_virtual) {
    PLC.estado_emergencia = true;
    digitalWrite(OUT_SSR_Z1, LOW);
    digitalWrite(OUT_SSR_Z2, LOW);
    digitalWrite(OUT_SSR_Z3, LOW);
    if (emergencia_fisica) bitWrite(PLC.regs[MB_SDIGITALES], 0, 1);
    SafetyManager_AbortCycle();
  } else {
    PLC.estado_emergencia = false;
  }
}