#include "OutputsManager.h"
#include "Config.h"
#include "MachineData.h"

static const int pines_out[] = {
  OUT_SOL_APERTURA_PRENSA, OUT_SOL_CIERRE_PRENSA, OUT_SOL_UNIDAD_ATRAS,
  OUT_SOL_UNIDAD_ADELANTE, OUT_SOL_AVANCE_HUSILLO, OUT_SOL_MOTOR_CARGA,
  OUT_SOL_PRESION_GENERAL, OUT_BOBINA_MOTOR_PRINCIPAL, OUT_RELAY_ALARMA,
  OUT_SOL_EXP_AVANCE, OUT_SOL_EXP_RETROCESO
};

void OutputsManager_Init() {
  for (int i = 0; i < 11; i++) {
    pinMode(pines_out[i], OUTPUT);
    digitalWrite(pines_out[i], LOW);
  }
}

void OutputsManager_Update() {
  if (bitRead(PLC.regs[MB_SDIGITALES], 1) == 1) {
    if (bitRead(PLC.regs[MB_SENSORES], 5) == 0 || bitRead(PLC.regs[MB_SENSORES], 6) == 0) {
      bitWrite(PLC.regs[MB_SDIGITALES], 1, 0);
      bitWrite(PLC.regs[MB_CONTROL], 9, 1);
    }
  }

  bool actuador_hidraulico_activo = (PLC.regs[MB_SDIGITALES] & 0x063F) != 0;
  bitWrite(PLC.regs[MB_SDIGITALES], 6, actuador_hidraulico_activo);

  for (int i = 0; i < 11; i++) {
    digitalWrite(pines_out[i], bitRead(PLC.regs[MB_SDIGITALES], i));
  }
}