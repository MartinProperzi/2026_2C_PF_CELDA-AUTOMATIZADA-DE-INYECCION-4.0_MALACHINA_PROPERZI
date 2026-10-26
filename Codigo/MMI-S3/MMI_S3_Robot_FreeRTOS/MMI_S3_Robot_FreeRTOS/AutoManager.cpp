#include "AutoManager.h"
#include "MachineData.h"
#include "SafetyManager.h"
#include <Arduino.h>

static unsigned long t_paso_inicio = 0;
static int paso_previo = -1;

void AutoManager_Init() {
  PLC.ciclo_activo = false;
  PLC.paso_actual = 0;
  t_paso_inicio = millis();
  paso_previo = -1;
}

void AutoManager_Update(bool es_modo_continuo) {
  bool btn_inicio = bitRead(PLC.regs[MB_HMI_BOTONES], 5);

  if (bitRead(PLC.regs[MB_SENSORES], 5) == 0 && PLC.aux_gh == false) PLC.aux_gl = true;
  if (bitRead(PLC.regs[MB_SENSORES], 5) == 0 && PLC.aux_gh == true && PLC.aux_gl == true) {
    PLC.ciclo_activo = true;
    PLC.aux_gl = false;
    PLC.aux_gh = false;
  }

  if (btn_inicio) PLC.ciclo_activo = true;
  if (!PLC.ciclo_activo) {
    PLC.paso_actual = 0;
    PLC.regs[MB_PASO_ACTUAL] = 0;
    return;
  }

  // Si hubo cambio de paso, reiniciar la referencia temporal
  if (PLC.paso_actual != paso_previo) {
    t_paso_inicio = millis();
    paso_previo = PLC.paso_actual;
  }

  unsigned long transcurrido_ms = millis() - t_paso_inicio;
  uint16_t transcurrido_seg = (uint16_t)(transcurrido_ms / 1000);

  switch (PLC.paso_actual) {
    case 0:
      // Espera señal de handshake del robot para permitir cierre
      if (PLC.robot_permite_cierre) {
        PLC.robot_permite_cierre = false;
        PLC.regs[MB_TESPERAI] = 0;
        PLC.regs[MB_TINYECCIONI] = 0;
        PLC.regs[MB_TSOSTENIMIENTOI] = 0;
        PLC.regs[MB_TENFRIAMIENTOI] = 0;
        PLC.regs[MB_TEXP_AVANCEI] = 0;
        PLC.regs[MB_TEXP_RETROCESOI] = 0;
        PLC.paso_actual = 1;
      }
      break;

    case 1:
      // Activar solenoide de cierre de prensa
      bitWrite(PLC.regs[MB_SDIGITALES], 1, 1);
      PLC.paso_actual = 2;
      break;

    case 2:
      // Fin de carrera de cierre alcanzado
      if (bitRead(PLC.regs[MB_SENSORES], 2) == 1) {
        bitWrite(PLC.regs[MB_SDIGITALES], 1, 0); // Desactivar solenoide de cierre
        PLC.paso_actual = 3;
      }
      break;

    case 3:
      // Tiempo de espera / estabilización de presión de cierre
      PLC.regs[MB_TESPERAI] = transcurrido_seg;
      if (transcurrido_seg >= PLC.regs[MB_TESPERA]) {
        bitWrite(PLC.regs[MB_SDIGITALES], 2, 1); // Solenoide unidad de inyección adelante
        PLC.paso_actual = 4;
      }
      break;

    case 4:
      // Fin de carrera de unidad adelante (contacto de boquilla con molde)
      if (bitRead(PLC.regs[MB_SENSORES], 1) == 1) {
        bitWrite(PLC.regs[MB_SDIGITALES], 2, 0); // Detener carro inyección
        bitWrite(PLC.regs[MB_SDIGITALES], 4, 1); // Solenoide avance husillo (inyección)
        PLC.paso_actual = 5;
      }
      break;

    case 5:
      // Fase de inyección (por tiempo o por final de carrera)
      PLC.regs[MB_TINYECCIONI] = transcurrido_seg;
      if (bitRead(PLC.regs[MB_SENSORES], 4) == 1 || transcurrido_seg >= PLC.regs[MB_TINYECCION]) {
        PLC.paso_actual = 6;
      }
      break;

    case 6:
      // Fase de sostenimiento de presión
      PLC.regs[MB_TSOSTENIMIENTOI] = transcurrido_seg;
      if (transcurrido_seg >= PLC.regs[MB_TSOSTENIMIENTO]) {
        bitWrite(PLC.regs[MB_SDIGITALES], 4, 0); // Fin inyección
        bitWrite(PLC.regs[MB_SDIGITALES], 5, 1); // Encender motor de dosificación / carga
        PLC.paso_actual = 7;
      }
      break;

    case 7:
      // Moldeo / Enfriamiento y carga de material
      PLC.regs[MB_TENFRIAMIENTOI] = transcurrido_seg;
      // Cuando el husillo llega a su posición de carga, se apaga el motor hidráulico de plastificación
      if (bitRead(PLC.regs[MB_SENSORES], 4) == 1) {
        bitWrite(PLC.regs[MB_SDIGITALES], 5, 0);
      }
      // Cuando vence el tiempo de enfriamiento:
      if (transcurrido_seg >= PLC.regs[MB_TENFRIAMIENTO]) {
        if (bitRead(PLC.regs[MB_SDIGITALES], 5) == 1) {
          // El motor aún estaba intentando cargar cuando terminó el enfriamiento -> Alarma Falla Carga
          bitWrite(PLC.regs[MB_CONTROL], 8, 1);
          SafetyManager_AbortCycle();
        } else {
          // Carga y enfriamiento exitosos: iniciar apertura
          bitWrite(PLC.regs[MB_SDIGITALES], 0, 1); // Solenoide apertura prensa
          PLC.paso_actual = 8;
        }
      }
      break;

    case 8:
      // Fin de carrera de apertura de prensa alcanzado
      if (bitRead(PLC.regs[MB_SENSORES], 0) == 1) {
        bitWrite(PLC.regs[MB_SDIGITALES], 0, 0); // Detener apertura
        PLC.paso_actual = 9;
      }
      break;

    case 9:
      // Esperar orden del Robot para avanzar expulsor
      if (PLC.robot_pide_expulsor_adelante) {
        PLC.robot_pide_expulsor_adelante = false;
        bitWrite(PLC.regs[MB_SDIGITALES], 9, 1); // Solenoide expulsor adelante
        PLC.paso_actual = 10;
      }
      break;

    case 10:
      // Tiempo de avance del expulsor
      PLC.regs[MB_TEXP_AVANCEI] = transcurrido_seg;
      if (transcurrido_seg >= PLC.regs[MB_TEXP_AVANCE]) {
        bitWrite(PLC.regs[MB_SDIGITALES], 9, 0); // Fin avance expulsor
        PLC.paso_actual = 11;
      }
      break;

    case 11:
      // Esperar orden del Robot para retraer expulsor (pieza asegurada)
      if (PLC.robot_pide_expulsor_atras) {
        PLC.robot_pide_expulsor_atras = false;
        bitWrite(PLC.regs[MB_SDIGITALES], 10, 1); // Solenoide expulsor atrás
        PLC.paso_actual = 12;
      }
      break;

    case 12:
      // Tiempo de retroceso del expulsor
      PLC.regs[MB_TEXP_RETROCESOI] = transcurrido_seg;
      if (transcurrido_seg >= PLC.regs[MB_TEXP_RETROCESO]) {
        bitWrite(PLC.regs[MB_SDIGITALES], 10, 0); // Fin retroceso expulsor
        if (es_modo_continuo) {
          PLC.paso_actual = 0; // Continuar ciclo en modo automático
        } else {
          PLC.ciclo_activo = false; // Detener tras 1 ciclo en semiautomático
          PLC.paso_actual = 0;
        }
      }
      break;
  }

  PLC.regs[MB_PASO_ACTUAL] = PLC.paso_actual;
}
