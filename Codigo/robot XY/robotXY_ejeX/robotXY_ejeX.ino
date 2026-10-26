// --- ASIGNACIÓN DE PINES (PCB) ---
// Motor 1 (Torre izquierda)
const int PIN_STEP_1 = 12;
const int PIN_DIR_1  = 13;

// Motor 2 (Torre derecha)
const int PIN_STEP_2 = 14;
const int PIN_DIR_2  = 25;

void moverCoordinadoLento(long pasosM1, long pasosM2, int retardoUs);

void setup() {
  Serial.begin(115200);
  pinMode(PIN_STEP_1, OUTPUT);
  pinMode(PIN_DIR_1, OUTPUT);
  pinMode(PIN_STEP_2, OUTPUT);
  pinMode(PIN_DIR_2, OUTPUT);

  digitalWrite(PIN_STEP_1, LOW);
  digitalWrite(PIN_STEP_2, LOW);

  Serial.println(">>> Prueba de Eje X (Movimiento horizontal) <<<");
  delay(2000); // Margen de seguridad antes de arrancar
}

void loop() {
  // 1. Desplazamiento horizontal (+X): motores en sentido opuesto
  Serial.println("1. Avance horizontal (+X)");
  moverCoordinadoLento(200, -200, 3500); 
  delay(1000);

  // 2. Retorno horizontal (-X): invierte ambos sentidos
  Serial.println("2. Retorno horizontal (-X)");
  moverCoordinadoLento(-200, 200, 3500);
  delay(2000);
}

void moverCoordinadoLento(long pasosM1, long pasosM2, int retardoUs) {
  // Sentido de giro Motor 1
  if (pasosM1 >= 0) {
    digitalWrite(PIN_DIR_1, HIGH);
  } else {
    digitalWrite(PIN_DIR_1, LOW);
    pasosM1 = -pasosM1;
  }

  // Sentido de giro Motor 2
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

    bool s1 = false;
    bool s2 = false;

    if (acum1 >= maxPasos) {
      acum1 -= maxPasos;
      digitalWrite(PIN_STEP_1, HIGH);
      s1 = true;
    }
    if (acum2 >= maxPasos) {
      acum2 -= maxPasos;
      digitalWrite(PIN_STEP_2, HIGH);
      s2 = true;
    }

    delayMicroseconds(5); // Pulso mínimo para el A4988

    if (s1) digitalWrite(PIN_STEP_1, LOW);
    if (s2) digitalWrite(PIN_STEP_2, LOW);

    delayMicroseconds(retardoUs);
  }
}