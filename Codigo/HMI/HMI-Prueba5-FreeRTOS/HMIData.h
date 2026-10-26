#pragma once
#include <Arduino.h>
#include "SharedModbusMap.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Definiciones de tamaño por seguridad en este archivo
#define PLC_CANTIDAD_REGISTROS 26
#define ROBOT_CANTIDAD_REGISTROS 10

struct HMIData_t {
    uint16_t plc_regs[PLC_CANTIDAD_REGISTROS];
    uint16_t robot_regs[ROBOT_CANTIDAD_REGISTROS];
    bool plc_conectado;
    bool robot_conectado;
    uint16_t robot_cmd_pendiente; // Cola de comandos para enviar al Robot (Esclavo ID 2)
};

extern HMIData_t HMI;
extern SemaphoreHandle_t hmi_mutex;

void HMIData_Init();
void HMI_Lock();
void HMI_Unlock();
void HMI_EnviarComandoRobot(uint16_t cmd);
