#include "AlarmManager.h"
#include "MachineData.h"

void AlarmManager_Init() {}

void AlarmManager_Update() {
  bool hay_alarma = (bitRead(PLC.regs[MB_CONTROL], 8) == 1) || (bitRead(PLC.regs[MB_CONTROL], 9) == 1);
  if (hay_alarma) bitWrite(PLC.regs[MB_SDIGITALES], 8, 1);

  bool sel_manual = bitRead(PLC.regs[MB_HMI_BOTONES], 0);
  if (sel_manual) {
    bitWrite(PLC.regs[MB_SDIGITALES], 8, 0);
    bitWrite(PLC.regs[MB_CONTROL], 8, 0);
    bitWrite(PLC.regs[MB_CONTROL], 9, 0);
  }
}