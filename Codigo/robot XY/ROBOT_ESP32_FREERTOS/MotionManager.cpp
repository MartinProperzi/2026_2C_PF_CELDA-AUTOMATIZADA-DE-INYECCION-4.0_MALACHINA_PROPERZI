#include "MotionManager.h"
#include "Config.h"
#include "RobotData.h"

void MotionManager_Init() {
  pinMode(PIN_STEP_1, OUTPUT);
  pinMode(PIN_DIR_1,  OUTPUT);
  pinMode(PIN_STEP_2, OUTPUT);
  pinMode(PIN_DIR_2,  OUTPUT);

  digitalWrite(PIN_STEP_1, LOW);
  digitalWrite(PIN_STEP_2, LOW);
}

// Paso unico para rutinas de busqueda de origen (Homing)
void MotionManager_PasoUnico(long dx_dir, long dy_dir, int retardoUs) {
  // Ecuaciones de Cinematica Inversa T-Bot:
  // dm1 = dy + dx
  // dm2 = dy - dx
  long dm1 = dy_dir + dx_dir;
  long dm2 = dy_dir - dx_dir;

  if (dm1 > 0) digitalWrite(PIN_DIR_1, HIGH);
  else if (dm1 < 0) digitalWrite(PIN_DIR_1, LOW);

  if (dm2 > 0) digitalWrite(PIN_DIR_2, HIGH);
  else if (dm2 < 0) digitalWrite(PIN_DIR_2, LOW);

  if (dm1 != 0) digitalWrite(PIN_STEP_1, HIGH);
  if (dm2 != 0) digitalWrite(PIN_STEP_2, HIGH);
  delayMicroseconds(5);

  digitalWrite(PIN_STEP_1, LOW);
  digitalWrite(PIN_STEP_2, LOW);
  delayMicroseconds(retardoUs);
}

// Traslacion coordinada en espacio cartesiano (dx, dy en pasos)
void MotionManager_Mover(long dx, long dy, int retardoUs) {
  Robot.en_movimiento = true;
  long dm1 = dy + dx;
  long dm2 = dy - dx;
  MotionManager_ConmutarMotores(dm1, dm2, retardoUs);
  Robot.en_movimiento = false;
}

// Interpolacion lineal tipo Bresenham para sincronizar ambos motores paso a paso
void MotionManager_ConmutarMotores(long pasosM1, long pasosM2, int retardoUs) {
  if (pasosM1 >= 0) {
    digitalWrite(PIN_DIR_1, HIGH);
  } else {
    digitalWrite(PIN_DIR_1, LOW);
    pasosM1 = -pasosM1;
  }

  if (pasosM2 >= 0) {
    digitalWrite(PIN_DIR_2, HIGH);
  } else {
    digitalWrite(PIN_DIR_2, LOW);
    pasosM2 = -pasosM2;
  }

  long maxPasos = max(pasosM1, pasosM2);
  long acum1 = 0;
  long acum2 = 0;

  for (long i = 0; i < maxPasos; i++) {
    acum1 += pasosM1;
    acum2 += pasosM2;

    bool pulsoM1 = false;
    bool pulsoM2 = false;

    if (acum1 >= maxPasos) {
      acum1 -= maxPasos;
      digitalWrite(PIN_STEP_1, HIGH);
      pulsoM1 = true;
    }

    if (acum2 >= maxPasos) {
      acum2 -= maxPasos;
      digitalWrite(PIN_STEP_2, HIGH);
      pulsoM2 = true;
    }

    delayMicroseconds(5);

    if (pulsoM1) digitalWrite(PIN_STEP_1, LOW);
    if (pulsoM2) digitalWrite(PIN_STEP_2, LOW);

    delayMicroseconds(retardoUs);
  }
}
