#include "RobotManager.h"
#include "MachineData.h"
#include <Arduino.h>

void RobotManager_Init() {
  PLC.robot_permite_cierre = false;
  PLC.robot_pide_expulsor_adelante = false;
  PLC.robot_pide_expulsor_atras = false;
}

void RobotManager_Update() {
  if (bitRead(PLC.regs[MB_CONTROL], 4) == 1) {
    PLC.robot_permite_cierre = true;
    bitWrite(PLC.regs[MB_CONTROL], 4, 0);
  }
  if (bitRead(PLC.regs[MB_CONTROL], 10) == 1) {
    PLC.robot_pide_expulsor_adelante = true;
    bitWrite(PLC.regs[MB_CONTROL], 10, 0);
  }
  if (bitRead(PLC.regs[MB_CONTROL], 11) == 1) {
    PLC.robot_pide_expulsor_atras = true;
    bitWrite(PLC.regs[MB_CONTROL], 11, 0);
  }
}