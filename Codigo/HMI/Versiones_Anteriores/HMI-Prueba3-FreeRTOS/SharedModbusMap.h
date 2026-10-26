#pragma once
#include <Arduino.h>

#define MB_REGS 26

enum ModbusRegisters {
  MB_SENSORES = 0,
  MB_HMI_BOTONES,
  MB_SDIGITALES,
  MB_CONTROL,
  MB_TESPERA,
  MB_TESPERAI,
  MB_TINYECCION,
  MB_TINYECCIONI,
  MB_TSOSTENIMIENTO,
  MB_TSOSTENIMIENTOI,
  MB_TENFRIAMIENTO,
  MB_TENFRIAMIENTOI,
  MB_TEXP_AVANCE,
  MB_TEXP_AVANCEI,
  MB_TEXP_RETROCESO,
  MB_TEXP_RETROCESOI,
  MB_TEMPZ1,
  MB_SETPOINTZ1,
  MB_TEMPZ2,
  MB_SETPOINTZ2,
  MB_TEMPZ3,
  MB_SETPOINTZ3,
  MB_HISTERESIS,
  MB_TOTAL_PIEZAS,
  MB_TIEMPO_CICLO,
  MB_PASO_ACTUAL
};

struct MachineState {
  uint16_t regs[MB_REGS];
  bool ciclo_activo;
  int paso_actual;
  bool estado_emergencia;
  bool aux_gh;
  bool aux_gl;
  bool robot_permite_cierre;
  bool robot_pide_expulsor_adelante;
  bool robot_pide_expulsor_atras;
};

