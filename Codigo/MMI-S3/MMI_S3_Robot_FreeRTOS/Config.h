#pragma once
#include <Arduino.h>

// ---------------------------------------------------------
// RED RS-485 (Modbus RTU Esclavo)
// ---------------------------------------------------------
#define MODBUS_SLAVE_ID 1
#define RS485_BAUD      9600
#define RS485_RX_PIN    16
#define RS485_TX_PIN    17

// ---------------------------------------------------------
// ENTRADAS FÍSICAS (10 Pines Discretos)
// ---------------------------------------------------------
#define IN_LIM_APERTURA_PRENSA   4
#define IN_LIM_CIERRE_PRENSA     5
#define IN_LIM1_UNIDAD_INYECCION 6
#define IN_LIM2_UNIDAD_INYECCION 7
#define IN_LIM_HUSILLO           8
#define IN_LIM1_GUARDA           9
#define IN_LIM2_GUARDA           10
#define IN_BTN_ARRANQUE_MOTOR    11
#define IN_BTN_PARO_MOTOR        14
#define IN_BTN_PARADA_EMERGENCIA 46  // Cambiado de 37 (conflicto PSRAM Octal) a 46 (Pin entrada segura)

// ---------------------------------------------------------
// SALIDAS FÍSICAS MÁQUINA (11 Pines)
// ---------------------------------------------------------
#define OUT_SOL_APERTURA_PRENSA    1
#define OUT_SOL_CIERRE_PRENSA      2
#define OUT_SOL_UNIDAD_ATRAS       38
#define OUT_SOL_UNIDAD_ADELANTE    39
#define OUT_SOL_AVANCE_HUSILLO     40
#define OUT_SOL_MOTOR_CARGA        41
#define OUT_SOL_PRESION_GENERAL    42
#define OUT_BOBINA_MOTOR_PRINCIPAL 47
#define OUT_RELAY_ALARMA           48
#define OUT_SOL_EXP_AVANCE         19  // Cambiado de 35 (conflicto PSRAM Octal) a 19
#define OUT_SOL_EXP_RETROCESO      20  // Cambiado de 36 (conflicto PSRAM Octal) a 20

// ---------------------------------------------------------
// PINES CONTROL TÉRMICO (SPI MAX6675 y SSR)
// ---------------------------------------------------------
#define MAX6675_SCK  12
#define MAX6675_MISO 13
#define MAX6675_CS1  15
#define MAX6675_CS2  18
#define MAX6675_CS3  3
#define OUT_SSR_Z1   45
#define OUT_SSR_Z2   21
#define OUT_SSR_Z3   0   // Cambiado de 26 (inexistente en ESP32-S3) a 0
