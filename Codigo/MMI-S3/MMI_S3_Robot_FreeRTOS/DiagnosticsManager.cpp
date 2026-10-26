#include "DiagnosticsManager.h"
#include "MachineData.h"
#include <Arduino.h>

#if ARDUINO_USB_CDC_ON_BOOT
  #define S3_PRINT(...) do { Serial.print(__VA_ARGS__); Serial0.print(__VA_ARGS__); } while(0)
  #define S3_PRINTLN(...) do { Serial.println(__VA_ARGS__); Serial0.println(__VA_ARGS__); } while(0)
#else
  #define S3_PRINT(...) Serial.print(__VA_ARGS__)
  #define S3_PRINTLN(...) Serial.println(__VA_ARGS__)
#endif

static int ultimo_paso = -1;
static bool ultimo_ciclo = false;
static bool ultima_emergencia = false;
static bool ultimo_manual = false;
static bool ultima_falla_carga = false;
static bool ultima_falla_guarda = false;

void DiagnosticsManager_Init() {
  S3_PRINTLN("\n=========================================");
  S3_PRINTLN("  FLUIDMEC PLC - SISTEMA INICIALIZADO  ");
  S3_PRINTLN("=========================================\n");
}

void DiagnosticsManager_Update() {
  if (PLC.estado_emergencia != ultima_emergencia) {
    ultima_emergencia = PLC.estado_emergencia;
    if (ultima_emergencia) S3_PRINTLN("[!] ALARMA CRÍTICA: PARADA DE EMERGENCIA ACTIVADA.");
    else S3_PRINTLN("[i] INFO: Sistema restablecido de Emergencia.");
  }

  bool falla_carga = bitRead(PLC.regs[MB_CONTROL], 8);
  if (falla_carga != ultima_falla_carga) {
    ultima_falla_carga = falla_carga;
    if (falla_carga) S3_PRINTLN("[!] ERROR: Tiempo excedido en Carga de Material.");
  }

  bool falla_guarda = bitRead(PLC.regs[MB_CONTROL], 9);
  if (falla_guarda != ultima_falla_guarda) {
    ultima_falla_guarda = falla_guarda;
    if (falla_guarda) S3_PRINTLN("[!] PELIGRO: Intento de cierre con guarda abierta.");
  }

  bool modo_manual = bitRead(PLC.regs[MB_HMI_BOTONES], 0);
  if (modo_manual != ultimo_manual) {
    ultimo_manual = modo_manual;
    if (modo_manual) S3_PRINTLN("\n[MODO] ---> MÁQUINA EN MODO MANUAL");
    else S3_PRINTLN("\n[MODO] ---> MÁQUINA EN MODO AUTOMÁTICO/SEMIAUTO");
  }

  if (PLC.ciclo_activo != ultimo_ciclo) {
    ultimo_ciclo = PLC.ciclo_activo;
    if (ultimo_ciclo) S3_PRINTLN("\n >>> CICLO DE INYECCIÓN INICIADO <<< ");
    else S3_PRINTLN(" <<< CICLO DE INYECCIÓN DETENIDO <<< \n");
  }

  if (PLC.ciclo_activo && (PLC.paso_actual != ultimo_paso)) {
    ultimo_paso = PLC.paso_actual;
    S3_PRINT("   [Paso ");
    S3_PRINT(ultimo_paso);
    S3_PRINT("] - ");
    switch (ultimo_paso) {
      case 0: S3_PRINTLN("Esperando permiso de cierre (Handshake Robot)..."); break;
      case 1: S3_PRINTLN("Iniciando cierre de matriz."); break;
      case 2: S3_PRINTLN("Matriz cerrada. Esperando presión final."); break;
      case 3: S3_PRINTLN("Bloqueo completado. Tiempo de espera en curso..."); break;
      case 4: S3_PRINTLN("Avanzando unidad de inyección (Carro adelante)."); break;
      case 5: S3_PRINTLN("Inyectando material en cavidad..."); break;
      case 6: S3_PRINTLN("Fase de Sostenimiento iniciada."); break;
      case 7: S3_PRINTLN("Carga de material nuevo y tiempo de enfriamiento..."); break;
      case 8: S3_PRINTLN("Iniciando apertura de matriz."); break;
      case 9: S3_PRINTLN("Robot ordena: Expulsor Adelante."); break;
      case 10: S3_PRINTLN("Expulsando pieza (Temporizado)..."); break;
      case 11: S3_PRINTLN("Robot ordena: Expulsor Atrás."); break;
      case 12: S3_PRINTLN("Retrayendo expulsor. Finalizando ciclo."); break;
      default: S3_PRINTLN("Estado desconocido."); break;
    }
  }

  if (!PLC.ciclo_activo && ultimo_paso != -1) ultimo_paso = -1;
}
