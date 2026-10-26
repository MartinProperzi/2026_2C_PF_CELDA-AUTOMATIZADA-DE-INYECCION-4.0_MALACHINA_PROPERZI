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

#ifdef SIMULACION_VIRTUAL_INTERNA
  // En reposo virtual: Molde Abierto = 1, Guardas = 1 (NC), E-Stop = 1 (NC)
  bitWrite(PLC.regs[MB_SENSORES], 0, 1);
  bitWrite(PLC.regs[MB_SENSORES], 2, 0);
  bitWrite(PLC.regs[MB_SENSORES], 1, 1);
  bitWrite(PLC.regs[MB_SENSORES], 4, 1);
  bitWrite(PLC.regs[MB_SENSORES], 5, 1);
  bitWrite(PLC.regs[MB_SENSORES], 6, 1);
  bitWrite(PLC.regs[MB_SENSORES], 9, 1);
#endif
}

void InputsManager_Update() {
#ifdef SIMULACION_VIRTUAL_INTERNA
  static unsigned long t_sim_cierre = 0;
  static unsigned long t_sim_apertura = 0;
  static unsigned long t_sim_carga = 0;

  // Estado seguro por defecto
  bitWrite(PLC.regs[MB_SENSORES], 9, 1); // E-stop NC
  bitWrite(PLC.regs[MB_SENSORES], 6, 1); // Guarda 2 NC
  bitWrite(PLC.regs[MB_SENSORES], 5, 1); // Guarda 1 NC
  bitWrite(PLC.regs[MB_SENSORES], 1, 1); // Unidad adelante en contacto

  // Simulación de cierre de prensa
  if (bitRead(PLC.regs[MB_SDIGITALES], 1) == 1) { // Solenoide Cierre activo
    if (t_sim_cierre == 0) t_sim_cierre = millis();
    if (millis() - t_sim_cierre > 1500) {
      bitWrite(PLC.regs[MB_SENSORES], 2, 1); // Molde cerrado
      bitWrite(PLC.regs[MB_SENSORES], 0, 0); // Molde abierto = 0
    }
  } else {
    t_sim_cierre = 0;
  }

  // Simulación de apertura de prensa
  if (bitRead(PLC.regs[MB_SDIGITALES], 0) == 1) { // Solenoide Apertura activo
    if (t_sim_apertura == 0) t_sim_apertura = millis();
    if (millis() - t_sim_apertura > 1500) {
      bitWrite(PLC.regs[MB_SENSORES], 0, 1); // Molde abierto
      bitWrite(PLC.regs[MB_SENSORES], 2, 0); // Molde cerrado = 0
    }
  } else {
    t_sim_apertura = 0;
  }

  // Simulación de plastificación / carga de material
  if (bitRead(PLC.regs[MB_SDIGITALES], 5) == 1) { // Motor de carga activo
    if (t_sim_carga == 0) t_sim_carga = millis();
    if (millis() - t_sim_carga > 2000) {
      bitWrite(PLC.regs[MB_SENSORES], 4, 1); // Husillo cargado
    }
  } else {
    t_sim_carga = 0;
    if (bitRead(PLC.regs[MB_SDIGITALES], 4) == 1) { // Durante inyección se vacía
      bitWrite(PLC.regs[MB_SENSORES], 4, 0);
    }
  }
#else
  for (int i = 0; i < 10; i++) {
    bitWrite(PLC.regs[MB_SENSORES], i, digitalRead(pines_in[i]));
  }
#endif
}
