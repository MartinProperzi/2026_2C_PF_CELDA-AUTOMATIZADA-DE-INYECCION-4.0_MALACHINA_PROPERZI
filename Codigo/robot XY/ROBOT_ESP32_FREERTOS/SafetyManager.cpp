#include "SafetyManager.h"
#include "Config.h"
#include "RobotData.h"

void SafetyManager_Init() {
  pinMode(PIN_HOME_X, INPUT_PULLUP);
  pinMode(PIN_HOME_Y, INPUT_PULLUP);
  pinMode(PIN_SW_ZONA_SEGURA, INPUT_PULLUP);
}

void SafetyManager_Update() {
  Robot.switch_home_x = (digitalRead(PIN_HOME_X) == LOW);
  Robot.switch_home_y = (digitalRead(PIN_HOME_Y) == LOW);
  Robot.en_zona_segura_fisica = (digitalRead(PIN_SW_ZONA_SEGURA) == LOW);

  // Interlock de Seguridad Fisico: El permiso de cierre al PLC solo se concede si
  // el fin de carrera de Zona Segura esta fisicamente presionado (LOW) y el robot
  // se encuentra en un estado que garantiza el despeje del molde.
  if (Robot.en_zona_segura_fisica && 
     (Robot.estado == ROBOT_LISTO_EN_HOME || Robot.estado == ROBOT_ZONA_SEGURA || Robot.estado == ROBOT_DESCARGANDO)) {
    bitWrite(Robot.handshake, 3, 1);
  } else {
    bitWrite(Robot.handshake, 3, 0);
  }
}

bool Safety_IsHomeX() {
  return (digitalRead(PIN_HOME_X) == LOW);
}

bool Safety_IsHomeY() {
  return (digitalRead(PIN_HOME_Y) == LOW);
}

bool Safety_IsZonaSegura() {
  return (digitalRead(PIN_SW_ZONA_SEGURA) == LOW);
}
