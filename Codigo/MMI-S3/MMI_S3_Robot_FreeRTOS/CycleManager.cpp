#include "CycleManager.h"
#include "MachineData.h"
#include "ManualManager.h"
#include "AutoManager.h"
#include "SafetyManager.h"

void CycleManager_Init() {
  ManualManager_Init();
  AutoManager_Init();
}

void CycleManager_Update() {
  if (PLC.estado_emergencia) return;

  // Control del Motor Principal (funcional en cualquier modo: Manual, Semiauto o Auto)
  // Encendido por pulsador físico (pin 11) o desde HMI (MB_HMI_BOTONES bit 3)
  if (bitRead(PLC.regs[MB_SENSORES], 7) == 1 || bitRead(PLC.regs[MB_HMI_BOTONES], 3) == 1) {
    bitWrite(PLC.regs[MB_SDIGITALES], 7, 1);
  }
  // Apagado por pulsador físico (pin 14) o desde HMI (MB_HMI_BOTONES bit 4)
  if (bitRead(PLC.regs[MB_SENSORES], 8) == 1 || bitRead(PLC.regs[MB_HMI_BOTONES], 4) == 1) {
    bitWrite(PLC.regs[MB_SDIGITALES], 7, 0);
  }

  bool modo_manual   = bitRead(PLC.regs[MB_HMI_BOTONES], 0);
  bool modo_semiauto = bitRead(PLC.regs[MB_HMI_BOTONES], 1);
  bool modo_auto     = bitRead(PLC.regs[MB_HMI_BOTONES], 2);

  if (modo_manual) {
    if (PLC.ciclo_activo) SafetyManager_AbortCycle();
    ManualManager_Update();
  } else if (modo_semiauto || modo_auto) {
    AutoManager_Update(modo_auto);
  }
}
