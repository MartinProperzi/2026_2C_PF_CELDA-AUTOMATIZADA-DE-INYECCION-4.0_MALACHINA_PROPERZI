#include <ESP32Servo.h>

// --- MAPEO DE PINES (PCB validado) ---
// Motor 1 (Torre izquierda)
const int PIN_STEP_1 = 12;
const int PIN_DIR_1  = 13;

// Motor 2 (Torre derecha)
const int PIN_STEP_2 = 14;
const int PIN_DIR_2  = 25;

// Servo Gripper
const int PIN_SERVO  = 27;
Servo servoGripper;

// --- PARÁMETROS CINEMÁTICOS ---
// 250 pasos = 50 mm (5 cm) para polea GT2 20T en paso completo
const long PASOS_5CM = 250;

// Retardo de 4000 us (velocidad ultra lenta con torque máximo)
const int RETARDO_LENTO_US = 4000; 

void moverCinematica(long dx, long dy, int retardoUs);
void conmutarMotores(long pasosM1, long pasosM2, int retardoUs);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configuración de pines de potencia
  pinMode(PIN_STEP_1, OUTPUT);
  pinMode(PIN_DIR_1, OUTPUT);
  pinMode(PIN_STEP_2, OUTPUT);
  pinMode(PIN_DIR_2, OUTPUT);

  digitalWrite(PIN_STEP_1, LOW);
  digitalWrite(PIN_STEP_2, LOW);

  // Inicialización del Gripper (arranca cerrado a 90°)
  servoGripper.setPeriodHertz(50);
  servoGripper.attach(PIN_SERVO, 500, 2400);
  servoGripper.write(90); 

  Serial.println(">>> Sistema T-Bot Integral: Velocidad Reducida <<<");
  delay(2000); // Margen de seguridad previo al ciclo
}

void loop() {
  Serial.println("--- Inicio de Ciclo de Verificación Integral ---");

  // --- 1. PRUEBA EJE X (Movimiento horizontal) ---
  Serial.println("1. Avance X (+5 cm)");
  moverCinematica(PASOS_5CM, 0, RETARDO_LENTO_US);
  delay(800);

  Serial.println("2. Retorno X al centro (-5 cm)");
  moverCinematica(-PASOS_5CM, 0, RETARDO_LENTO_US);
  delay(1000);

  // --- 2. PRUEBA EJE Y (Movimiento transversal/brazo) ---
  Serial.println("3. Avance Y (+5 cm)");
  moverCinematica(0, PASOS_5CM, RETARDO_LENTO_US);
  delay(800);

  Serial.println("4. Retorno Y al centro (-5 cm)");
  moverCinematica(0, -PASOS_5CM, RETARDO_LENTO_US);
  delay(1000);

  // --- 3. ACCIONAMIENTO DEL GRIPPER ---
  Serial.println("5. Gripper: Abriendo (0 deg)");
  servoGripper.write(0);
  delay(1200);

  Serial.println("6. Gripper: Cerrando (90 deg)");
  servoGripper.write(90);
  delay(2500); // Pausa previa a repetir la secuencia
}

// Transformación cinemática T-Bot / H-Bot para correa única
void moverCinematica(long dx, long dy, int retardoUs) {
  // Según las pruebas previas en banco:
  // dx (Eje X puro): motores en sentidos opuestos
  // dy (Eje Y puro): motores en el mismo sentido
  long dm1 = dy + dx;
  long dm2 = dy - dx;
  conmutarMotores(dm1, dm2, retardoUs);
}

// Algoritmo de interpolación coordinada tipo Bresenham
void conmutarMotores(long pasosM1, long pasosM2, int retardoUs) {
  // Sentido Motor 1
  if (pasosM1 >= 0) {
    digitalWrite(PIN_DIR_1, HIGH);
  } else {
    digitalWrite(PIN_DIR_1, LOW);
    pasosM1 = -pasosM1;
  }

  // Sentido Motor 2
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

    delayMicroseconds(5); // Duración mínima de pulso para el A4988

    if (pulsoM1) digitalWrite(PIN_STEP_1, LOW);
    if (pulsoM2) digitalWrite(PIN_STEP_2, LOW);

    delayMicroseconds(retardoUs);
  }
}