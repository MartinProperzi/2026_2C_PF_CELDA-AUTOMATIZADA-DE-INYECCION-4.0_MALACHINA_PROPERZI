#include "TemperatureManager.h"
#include "MachineData.h"
#include "Config.h"
#include <max6675.h>

static MAX6675 *thermoZ1 = nullptr;
static MAX6675 *thermoZ2 = nullptr;
static MAX6675 *thermoZ3 = nullptr;

void TemperatureManager_Init() {
  pinMode(OUT_SSR_Z1, OUTPUT);
  pinMode(OUT_SSR_Z2, OUTPUT);
  pinMode(OUT_SSR_Z3, OUTPUT);
  digitalWrite(OUT_SSR_Z1, LOW);
  digitalWrite(OUT_SSR_Z2, LOW);
  digitalWrite(OUT_SSR_Z3, LOW);

  thermoZ1 = new MAX6675(MAX6675_SCK, MAX6675_CS1, MAX6675_MISO);
  thermoZ2 = new MAX6675(MAX6675_SCK, MAX6675_CS2, MAX6675_MISO);
  thermoZ3 = new MAX6675(MAX6675_SCK, MAX6675_CS3, MAX6675_MISO);
}

void TemperatureManager_Update() {
  float temp1 = (thermoZ1 != nullptr) ? thermoZ1->readCelsius() : NAN;
  float temp2 = (thermoZ2 != nullptr) ? thermoZ2->readCelsius() : NAN;
  float temp3 = (thermoZ3 != nullptr) ? thermoZ3->readCelsius() : NAN;

  PLC_Lock();
  // Si la termocupla está desconectada (retorna NaN o 0 en banco de pruebas), reflejar el setpoint
  uint16_t t1 = (isnan(temp1) || temp1 <= 0.0) ? PLC.regs[MB_SETPOINTZ1] : (uint16_t)round(temp1);
  uint16_t t2 = (isnan(temp2) || temp2 <= 0.0) ? PLC.regs[MB_SETPOINTZ2] : (uint16_t)round(temp2);
  uint16_t t3 = (isnan(temp3) || temp3 <= 0.0) ? PLC.regs[MB_SETPOINTZ3] : (uint16_t)round(temp3);

  PLC.regs[MB_TEMPZ1] = t1;
  PLC.regs[MB_TEMPZ2] = t2;
  PLC.regs[MB_TEMPZ3] = t3;

  if (PLC.estado_emergencia) {
    digitalWrite(OUT_SSR_Z1, LOW);
    digitalWrite(OUT_SSR_Z2, LOW);
    digitalWrite(OUT_SSR_Z3, LOW);
    PLC_Unlock();
    return;
  }

  uint16_t hist = PLC.regs[MB_HISTERESIS];
  if (hist == 0) hist = 2; // Histéresis mínima por defecto (2 °C)

  // Control Térmico Zona 1
  uint16_t sp1 = PLC.regs[MB_SETPOINTZ1];
  if (sp1 > 0) {
    if (t1 < (sp1 - hist)) digitalWrite(OUT_SSR_Z1, HIGH);
    else if (t1 >= sp1)    digitalWrite(OUT_SSR_Z1, LOW);
  } else {
    digitalWrite(OUT_SSR_Z1, LOW);
  }

  // Control Térmico Zona 2
  uint16_t sp2 = PLC.regs[MB_SETPOINTZ2];
  if (sp2 > 0) {
    if (t2 < (sp2 - hist)) digitalWrite(OUT_SSR_Z2, HIGH);
    else if (t2 >= sp2)    digitalWrite(OUT_SSR_Z2, LOW);
  } else {
    digitalWrite(OUT_SSR_Z2, LOW);
  }

  // Control Térmico Zona 3
  uint16_t sp3 = PLC.regs[MB_SETPOINTZ3];
  if (sp3 > 0) {
    if (t3 < (sp3 - hist)) digitalWrite(OUT_SSR_Z3, HIGH);
    else if (t3 >= sp3)    digitalWrite(OUT_SSR_Z3, LOW);
  } else {
    digitalWrite(OUT_SSR_Z3, LOW);
  }
  PLC_Unlock();
}
