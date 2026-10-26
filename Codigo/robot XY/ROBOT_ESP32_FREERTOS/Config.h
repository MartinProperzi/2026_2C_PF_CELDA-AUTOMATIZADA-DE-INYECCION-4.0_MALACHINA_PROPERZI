#pragma once
#include <Arduino.h>

// =============================================================================
// MAPEO DE PINES DE POTENCIA (Motores NEMA 17 - Cinematica T-Bot)
// =============================================================================
const int PIN_STEP_1 = 12; // Torre izquierda
const int PIN_DIR_1  = 13;
const int PIN_STEP_2 = 14; // Torre derecha
const int PIN_DIR_2  = 25;

// =============================================================================
// FINALES DE CARRERA (HOMING Y ZONA SEGURA)
// Conexion NO: pata COM a GND, pata NO al GPIO.
// En reposo leen HIGH (pull-up interno), al presionarse van a LOW.
// =============================================================================
const int PIN_HOME_X         = 23; // Switch en extremo derecho (origen X)
const int PIN_HOME_Y         = 22; // Switch en extremo superior (origen Y)
const int PIN_SW_ZONA_SEGURA = 21; // Fin de carrera fisico de Zona Segura (despeje de molde) - Cableado en GPIO 21

// =============================================================================
// SERVO GRIPPER (PINZA MECANICA)
// =============================================================================
const int PIN_SERVO      = 27;
const int ANGULO_ABIERTO = 110;  // Gripper abierto (angulo invertido segun montaje fisico)
const int ANGULO_CERRADO = 0;    // Gripper cerrado (presion de sujecion)

// =============================================================================
// PINES RS-485 (MODULO MAX3485)
// =============================================================================
#define RS485_RX_PIN 16
#define RS485_TX_PIN 17
#define MODBUS_ROBOT_ID 2
#define RS485_BAUD 9600

// =============================================================================
// PARAMETROS CINEMATICOS T-BOT
// =============================================================================
// 5 pasos por mm (GT2 20T, 1 paso = 0.2 mm)
const long PASOS_POR_MM = 5;

const long DISTANCIA_60CM = 600 * PASOS_POR_MM; // 3000 pasos
const long DISTANCIA_50CM = 500 * PASOS_POR_MM; // 2500 pasos
const long DISTANCIA_40CM = 400 * PASOS_POR_MM; // 2000 pasos

// Velocidades independientes por eje:
// Eje X: 3500 us = 5.7 cm/s (traslacion horizontal segura)
// Eje Y: 1800 us = 11.1 cm/s (descenso y ascenso rapido con doble torque T-Bot)
const int RETARDO_X_US_DEFAULT = 3500;
const int RETARDO_Y_US_DEFAULT = 3500;

// =============================================================================
// REGISTROS MODBUS DEL ROBOT (ESCLAVO ID 2)
// =============================================================================
#define MB_ROBOT_REGS 10

enum RobotModbusRegisters {
  MB_ROBOT_ESTADO       = 0, // Estado de la maquina de estados
  MB_ROBOT_COMANDO      = 1, // Comandos enviados por la HMI
  MB_ROBOT_HANDSHAKE    = 2, // Banderas de sincronizacion para PLC
  MB_ROBOT_TOTAL_CICLOS = 3, // Total de piezas extraidas
  MB_ROBOT_ANGULO_SERVO = 4, // Posicion actual del servomotor
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
  ROBOT_SUJETANDO_PIEZA    = 5, // Gripper cerrandose
  ROBOT_SALIENDO_PRENSA    = 6, // Subiendo y saliendo
  ROBOT_ZONA_SEGURA        = 7, // Ya despejo el molde (puede cerrar la prensa)
  ROBOT_DESCARGANDO        = 8, // Soltando la pieza en deposito
  ROBOT_ALARMA_ERROR       = 9  // Error de carrera o aborto
};
