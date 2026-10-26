#include "ModbusManager.h"
#include <ModbusRTU.h>
#include "Config.h"
#include "RobotData.h"

static ModbusRTU mb_robot;

void ModbusManager_Init() {
  pinMode(RS485_RX_PIN, INPUT_PULLUP);
  Serial2.setRxBufferSize(512);
  Serial2.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);

  mb_robot.begin(&Serial2);
  mb_robot.slave(MODBUS_ROBOT_ID);
  mb_robot.setBaudrate(RS485_BAUD);

  for (int i = 0; i < MB_ROBOT_REGS; i++) {
    mb_robot.addHreg(i, 0);
  }

  mb_robot.Hreg(MB_ROBOT_VEL_X_US, Robot.velocidad_x_us);
  mb_robot.Hreg(MB_ROBOT_VEL_Y_US, Robot.velocidad_y_us);
  mb_robot.Hreg(MB_ROBOT_DIST_X,    Robot.distancia_x_mm);
  mb_robot.Hreg(MB_ROBOT_DIST_Y,    Robot.distancia_y_mm);

  // Diagnostico de escrituras Modbus por consola USB
  mb_robot.onRequest([](Modbus::FunctionCode fc, const Modbus::RequestData data) -> Modbus::ResultCode {
    if (fc == Modbus::FC_WRITE_REG) {
      Serial.printf("[ROBOT RX] Modbus Write HMI: Reg %d = %d (0x%04X)\n", 
                    data.reg.address, data.regCount, data.regCount);
    }
    return Modbus::EX_SUCCESS;
  });

  Serial.println("[MODBUS] Esclavo ID 2 iniciado en Serial2 (RX:16, TX:17 @ 9600 baud).");
}

void ModbusManager_Update() {
  // 1. Sincronizar datos locales hacia los Holding Registers de Modbus
  Robot_Lock();
  mb_robot.Hreg(MB_ROBOT_ESTADO,       Robot.estado);
  mb_robot.Hreg(MB_ROBOT_HANDSHAKE,    Robot.handshake);
  mb_robot.Hreg(MB_ROBOT_TOTAL_CICLOS, Robot.total_piezas_extraidas);
  mb_robot.Hreg(MB_ROBOT_ANGULO_SERVO, Robot.angulo_servo);
  mb_robot.Hreg(MB_ROBOT_VEL_X_US,     Robot.velocidad_x_us);
  mb_robot.Hreg(MB_ROBOT_VEL_Y_US,     Robot.velocidad_y_us);
  mb_robot.Hreg(MB_ROBOT_DIST_X,       Robot.distancia_x_mm);
  mb_robot.Hreg(MB_ROBOT_DIST_Y,       Robot.distancia_y_mm);
  Robot_Unlock();

  // 2. Procesar tramas del bus Modbus RTU RS-485
  mb_robot.task();

  // 3. Tomar comandos enviados por la HMI
  Robot_Lock();
  uint16_t hreg_cmd = mb_robot.Hreg(MB_ROBOT_COMANDO);
  if (hreg_cmd != 0) {
    Robot.comandos |= hreg_cmd;
    mb_robot.Hreg(MB_ROBOT_COMANDO, 0); // Consumir inmediatamente en el registro
  }

  // Permitir actualizar velocidades dinamicamente si la HMI escribe valores validos (800..10000 us)
  uint16_t v_x = mb_robot.Hreg(MB_ROBOT_VEL_X_US);
  uint16_t v_y = mb_robot.Hreg(MB_ROBOT_VEL_Y_US);
  if (v_x >= 800 && v_x <= 10000) Robot.velocidad_x_us = v_x;
  if (v_y >= 800 && v_y <= 10000) Robot.velocidad_y_us = v_y;
  Robot_Unlock();
}
