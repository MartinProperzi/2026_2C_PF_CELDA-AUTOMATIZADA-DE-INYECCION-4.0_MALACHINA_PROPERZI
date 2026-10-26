#include "ManualManager.h"
#include "MachineData.h"
#include <Arduino.h>

void ManualManager_Init() {}

void ManualManager_Update() {
  PLC.ciclo_activo = false;
  PLC.paso_actual = 0;

  // Apertura y Cierre de Prensa
  bitWrite(PLC.regs[MB_SDIGITALES], 0, bitRead(PLC.regs[MB_HMI_BOTONES], 9));
  bitWrite(PLC.regs[MB_SDIGITALES], 1, bitRead(PLC.regs[MB_HMI_BOTONES], 8));

  // Unidad de Inyección Adelante y Atrás
  bitWrite(PLC.regs[MB_SDIGITALES], 2, bitRead(PLC.regs[MB_HMI_BOTONES], 10));
  bitWrite(PLC.regs[MB_SDIGITALES], 3, bitRead(PLC.regs[MB_HMI_BOTONES], 11));

  // Inyección y Carga con interbloqueo mutuo
  if (bitRead(PLC.regs[MB_SDIGITALES], 5) == 0) {
    bitWrite(PLC.regs[MB_SDIGITALES], 4, bitRead(PLC.regs[MB_HMI_BOTONES], 7));
  }
  if (bitRead(PLC.regs[MB_SDIGITALES], 4) == 0) {
    bitWrite(PLC.regs[MB_SDIGITALES], 5, bitRead(PLC.regs[MB_HMI_BOTONES], 6));
  }

  // Expulsor Adelante y Atrás
  bitWrite(PLC.regs[MB_SDIGITALES], 9, bitRead(PLC.regs[MB_HMI_BOTONES], 12));
  bitWrite(PLC.regs[MB_SDIGITALES], 10, bitRead(PLC.regs[MB_HMI_BOTONES], 13));
}
