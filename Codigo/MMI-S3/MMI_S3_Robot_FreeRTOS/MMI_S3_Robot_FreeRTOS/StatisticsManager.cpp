#include "StatisticsManager.h"
#include "MachineData.h"
#include <Arduino.h>

static int paso_anterior = 0;
static unsigned long tiempo_inicio_ciclo = 0;
static bool midiendo_ciclo = false;

void StatisticsManager_Init() {
  PLC.regs[MB_TOTAL_PIEZAS] = 0;
  PLC.regs[MB_TIEMPO_CICLO] = 0;
  paso_anterior = 0;
  tiempo_inicio_ciclo = 0;
  midiendo_ciclo = false;
}

void StatisticsManager_Update() {
  // Al arrancar el paso 1 (cierre de molde), inicia la medición del tiempo de ciclo
  if (PLC.paso_actual == 1 && paso_anterior == 0) {
    tiempo_inicio_ciclo = millis();
    midiendo_ciclo = true;
  }

  // Al finalizar el paso 12 (retroceso de expulsor y retorno a 0), se completa la pieza
  if (paso_anterior == 12 && PLC.paso_actual == 0) {
    PLC.regs[MB_TOTAL_PIEZAS]++;
    if (midiendo_ciclo) {
      unsigned long duracion_ms = millis() - tiempo_inicio_ciclo;
      PLC.regs[MB_TIEMPO_CICLO] = (uint16_t)(duracion_ms / 1000); // Duración en segundos
      midiendo_ciclo = false;
      Serial.printf("[PLC-S3] >>> PIEZA FABRICADA #%d | Tiempo de Ciclo: %d s <<<\n",
                    PLC.regs[MB_TOTAL_PIEZAS], PLC.regs[MB_TIEMPO_CICLO]);
    }
  }

  // Petición desde HMI para resetear el contador de piezas (Bit 14 de MB_HMI_BOTONES)
  if (bitRead(PLC.regs[MB_HMI_BOTONES], 14) == 1) {
    PLC.regs[MB_TOTAL_PIEZAS] = 0;
    PLC.regs[MB_TIEMPO_CICLO] = 0;
    bitWrite(PLC.regs[MB_HMI_BOTONES], 14, 0);
    Serial.println("[PLC-S3] Contador de piezas reseteado a 0.");
  }

  paso_anterior = PLC.paso_actual;
}
