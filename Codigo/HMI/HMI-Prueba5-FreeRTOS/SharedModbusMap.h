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

// =============================================================================
// REGISTROS MODBUS DEL ROBOT (ESCLAVO ID 2)
// =============================================================================
enum RobotModbusRegisters {
  MB_ROBOT_ESTADO       = 0, // Estado de la máquina de estados
  MB_ROBOT_COMANDO      = 1, // Comandos enviados por la HMI
  MB_ROBOT_HANDSHAKE    = 2, // Banderas de sincronización para PLC
  MB_ROBOT_TOTAL_CICLOS = 3, // Total de piezas extraídas
  MB_ROBOT_ANGULO_SERVO = 4, // Posición actual del servomotor
  MB_ROBOT_VEL_X_US     = 5, // Velocidad eje X (retardo en microsegundos)
  MB_ROBOT_VEL_Y_US     = 6, // Velocidad eje Y (retardo en microsegundos)
  MB_ROBOT_DIST_X       = 7, // Cota X de entrada a prensa (en mm)
  MB_ROBOT_DIST_Y       = 8  // Cota Y de descenso al molde (en mm)
};

// Estados del Robot (Registro MB_ROBOT_ESTADO)
enum EstadoRobot {
  ROBOT_DESCONOCIDO        = 0,
  ROBOT_HOMING_EN_CURSO    = 1,
  ROBOT_LISTO_EN_HOME      = 2, // Espera seguro fuera de la prensa
  ROBOT_ENTRANDO_PRENSA    = 3, // En movimiento X/Y hacia la matriz
  ROBOT_EN_POSICION_AGARRE = 4, // Posicionado frente a cavidad (espera expulsor)
  ROBOT_SUJETANDO_PIEZA    = 5, // Gripper cerrándose
  ROBOT_SALIENDO_PRENSA    = 6, // Subiendo y saliendo
  ROBOT_ZONA_SEGURA        = 7, // Ya despejó el molde (puede cerrar la prensa)
  ROBOT_DESCARGANDO        = 8, // Soltando la pieza en depósito
  ROBOT_ALARMA_ERROR       = 9  // Error de carrera o aborto
};
