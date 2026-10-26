#include "InputsManager.h"
#include "Config.h"
#include "MachineData.h"

static const int pines_in[] = {
  IN_LIM_APERTURA_PRENSA, IN_LIM_CIERRE_PRENSA, IN_LIM1_UNIDAD_INYECCION,
  IN_LIM2_UNIDAD_INYECCION, IN_LIM_HUSILLO, IN_LIM1_GUARDA, IN_LIM2_GUARDA,
  IN_BTN_ARRANQUE_MOTOR, IN_BTN_PARO_MOTOR, IN_BTN_PARADA_EMERGENCIA
};

void InputsManager_Init() {
  for (int i = 0; i < 10; i++) pinMode(pines_in[i], INPUT_PULLUP);
}

void InputsManager_Update() {
  for (int i = 0; i < 10; i++) {
    bitWrite(PLC.regs[MB_SENSORES], i, digitalRead(pines_in[i]));
  }
}