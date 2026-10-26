#include "GripperManager.h"
#include <ESP32Servo.h>
#include "Config.h"
#include "RobotData.h"

static Servo servoGripper;

void GripperManager_Init() {
  servoGripper.setPeriodHertz(50);
  servoGripper.attach(PIN_SERVO, 500, 2400);
  GripperManager_Abrir();
}

void GripperManager_Abrir() {
  servoGripper.write(ANGULO_ABIERTO);
  Robot.angulo_servo = ANGULO_ABIERTO;
  bitWrite(Robot.handshake, 4, 0); // Bit 4 = 0: Gripper abierto
}

void GripperManager_Cerrar() {
  servoGripper.write(ANGULO_CERRADO);
  Robot.angulo_servo = ANGULO_CERRADO;
  bitWrite(Robot.handshake, 4, 1); // Bit 4 = 1: Gripper cerrado
}

void GripperManager_SetAngle(int angle) {
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;
  servoGripper.write(angle);
  Robot.angulo_servo = (uint8_t)angle;
  if (abs(angle - ANGULO_CERRADO) <= 15) {
    bitWrite(Robot.handshake, 4, 1); // 1 = Pinza cerrada
  } else {
    bitWrite(Robot.handshake, 4, 0); // 0 = Pinza abierta
  }
}

uint8_t GripperManager_GetAngle() {
  return (uint8_t)servoGripper.read();
}
