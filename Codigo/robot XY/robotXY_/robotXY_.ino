#include <ESP32Servo.h>

// --- ASIGNACIÓN DE PINES (Mapeo PCB validado) ---
// Motor 1 (Torre izquierda - Driver X)
const int PIN_STEP_1 = 12;
const int PIN_DIR_1  = 13;

// Motor 2 (Torre derecha - Driver Y)
const int PIN_STEP_2 = 14;
const int PIN_DIR_2  = 25;

// Servo Gripper (MG90s)
const int PIN_SERVO  = 27;

Servo gripperServo;

// 250 pasos = 50 mm (5 cm) para polea GT2 20T en paso completo
const long PASOS_5CM = 250;
const int PULSE_DELAY_US = 1500; // Velocidad moderada de test

// Declaración de funciones
void moverTrayectoria(long dx, long dy, int pulseDelay);
void ejecutarPulsos(long pasosM1, long pasosM2, int pulseDelay);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("--- Inicio de prueba: Robot T-Bot/H-Bot + Gripper ---");

  // Configuración de pines de los drivers A4988
  pinMode(PIN_STEP_1, OUTPUT);
  pinMode(PIN_DIR_1,  OUTPUT);
  pinMode(PIN_STEP_2, OUTPUT);
  pinMode(PIN_DIR_2,  OUTPUT);

  digitalWrite(PIN_STEP_1, LOW);
  digitalWrite(PIN_STEP_2, LOW);

  // Configuración del servo MG90s
  ESP32PWM::allocateTimer(0);
  gripperServo.setPeriodHertz(50);
  gripperServo.attach(PIN_SERVO, 500, 2400);

  // Posición inicial: Físicamente cerrado en 0 deg
  gripperServo.write(0);
  Serial.println("Gripper en posicion inicial cerrado (0 deg).");
  delay(2000);
}

void loop() {
  Serial.println("--- Inicio de ciclo cartesiano (±5 cm desde el centro) ---");

  // 1. Eje Longitudinal: +5 cm (Ambos motores giran en sincronía)
  Serial.println("1. Longitudinal: +5 cm");
  moverTrayectoria(PASOS_5CM, 0, PULSE_DELAY_US);
  delay(600);

  // 2. Retorno longitudinal al centro (-5 cm)
  Serial.println("2. Retorno longitudinal al centro (-5 cm)");
  moverTrayectoria(-PASOS_5CM, 0, PULSE_DELAY_US);
  delay(600);

  // 3. Eje Longitudinal: -5 cm hacia el otro lado
  Serial.println("3. Longitudinal: -5 cm");
  moverTrayectoria(-PASOS_5CM, 0, PULSE_DELAY_US);
  delay(600);

  // 4. Retorno longitudinal al centro (+5 cm)
  Serial.println("4. Retorno longitudinal al centro (+5 cm)");
  moverTrayectoria(PASOS_5CM, 0, PULSE_DELAY_US);
  delay(800);

  // 5. Eje Transversal (Cabezal Gripper): +5 cm (Motores en sentido opuesto)
  Serial.println("5. Transversal: +5 cm");
  moverTrayectoria(0, PASOS_5CM, PULSE_DELAY_US);
  delay(600);

  // 6. Retorno transversal al centro (-5 cm)
  Serial.println("6. Retorno transversal al centro (-5 cm)");
  moverTrayectoria(0, -PASOS_5CM, PULSE_DELAY_US);
  delay(600);

  // 7. Eje Transversal: -5 cm hacia el otro lado
  Serial.println("7. Transversal: -5 cm");
  moverTrayectoria(0, -PASOS_5CM, PULSE_DELAY_US);
  delay(600);

  // 8. Retorno transversal al centro (+5 cm)
  Serial.println("8. Retorno transversal al centro (+5 cm)");
  moverTrayectoria(0, PASOS_5CM, PULSE_DELAY_US);
  delay(800);

  // 9. Secuencia del Gripper (Arrancó en 0°, ahora abre a 90° y vuelve a cerrar en 0°)
  Serial.println("Abriendo gripper (90 deg)...");
  gripperServo.write(90);
  delay(1500);

  Serial.println("Cerrando gripper (0 deg)...");
  gripperServo.write(0);
  delay(2500);
}

// Cinemática acoplada para sistema de correa única
void moverTrayectoria(long dx, long dy, int pulseDelay) {
  long dm1 = dx + dy;
  long dm2 = dx - dy;
  ejecutarPulsos(dm1, dm2, pulseDelay);
}

// Generador simultáneo de pulsos tipo Bresenham
void ejecutarPulsos(long pasosM1, long pasosM2, int pulseDelay) {
  // Dirección Motor 1
  if (pasosM1 >= 0) {
    digitalWrite(PIN_DIR_1, HIGH);
  } else {
    digitalWrite(PIN_DIR_1, LOW);
    pasosM1 = -pasosM1;
  }

  // Dirección Motor 2
  if (pasosM2 >= 0) {
    digitalWrite(PIN_DIR_2, HIGH);
  } else {
    digitalWrite(PIN_DIR_2, LOW);
    pasosM2 = -pasosM2;
  }

  long maxSteps = max(pasosM1, pasosM2);
  long acum1 = 0;
  long acum2 = 0;

  for (long i = 0; i < maxSteps; i++) {
    acum1 += pasosM1;
    acum2 += pasosM2;

    bool step1 = false;
    bool step2 = false;

    if (acum1 >= maxSteps) {
      acum1 -= maxSteps;
      digitalWrite(PIN_STEP_1, HIGH);
      step1 = true;
    }

    if (acum2 >= maxSteps) {
      acum2 -= maxSteps;
      digitalWrite(PIN_STEP_2, HIGH);
      step2 = true;
    }

    delayMicroseconds(5); // Duración mínima del pulso en HIGH para A4988

    if (step1) digitalWrite(PIN_STEP_1, LOW);
    if (step2) digitalWrite(PIN_STEP_2, LOW);

    delayMicroseconds(pulseDelay);
  }
}