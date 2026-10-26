#include <ESP32Servo.h>

// --- MAPEO DE PINES DE POTENCIA (Motores NEMA 17) ---
const int PIN_STEP_1 = 12; // Torre izquierda
const int PIN_DIR_1  = 13;
const int PIN_STEP_2 = 14; // Torre derecha
const int PIN_DIR_2  = 25;

// --- FINALES DE CARRERA (HOMING) ---
// Conexión NO: pata COM a GND, pata NO al GPIO.
// En reposo leen HIGH (pull-up), al presionarse van a LOW.
const int PIN_HOME_X = 23; // Switch en extremo derecho
const int PIN_HOME_Y = 22; // Switch en extremo superior

// --- SERVO GRIPPER ---
const int PIN_SERVO  = 27;
Servo servoGripper;

const int ANGULO_ABIERTO = 0;    // Gripper abierto
const int ANGULO_CERRADO = 110;  // Gripper cerrado (más presión)

// --- PARÁMETROS CINEMÁTICOS ---
// 5 pasos por mm (GT2 20T, 1 paso = 0.2 mm)
const long PASOS_POR_MM = 5;

const long DISTANCIA_60CM = 600 * PASOS_POR_MM; // 3000 pasos
const long DISTANCIA_50CM = 500 * PASOS_POR_MM; // 2500 pasos
const long DISTANCIA_40CM = 400 * PASOS_POR_MM; // 2000 pasos

// Velocidad de trabajo segura
const int RETARDO_US = 4000; 

// Prototipos de funciones
void moverCinematica(long dx, long dy, int retardoUs);
void conmutarMotores(long pasosM1, long pasosM2, int retardoUs);
void pasoUnicoCinematica(long dx_dir, long dy_dir, int retardoUs);
void ejecutarHoming();

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configuración de pines de motores
  pinMode(PIN_STEP_1, OUTPUT);
  pinMode(PIN_DIR_1, OUTPUT);
  pinMode(PIN_STEP_2, OUTPUT);
  pinMode(PIN_DIR_2, OUTPUT);

  digitalWrite(PIN_STEP_1, LOW);
  digitalWrite(PIN_STEP_2, LOW);

  // Configuración de Finales de Carrera con Pull-Up interno
  pinMode(PIN_HOME_X, INPUT_PULLUP);
  pinMode(PIN_HOME_Y, INPUT_PULLUP);

  // Inicialización de Servo
  servoGripper.setPeriodHertz(50);
  servoGripper.attach(PIN_SERVO, 500, 2400);
  servoGripper.write(ANGULO_ABIERTO); // Arranca abierto para no chocar nada

  Serial.println("==================================================");
  Serial.println(">>> INICIANDO ROBOT EXTRACTOR T-BOT EN INYECTORA <<<");
  Serial.println("==================================================");

  delay(2000);

  // 1. Ejecutar secuencia de Homing antes de iniciar cualquier ciclo
  ejecutarHoming();

  Serial.println("Homing completado. Cero de maquina establecido.");
  delay(1500);
}

void loop() {
  Serial.println("\n--- [INICIO DE CICLO DE EXTRACCION] ---");

  // 1. Entrar a la prensa: 60 cm a la izquierda (-X)
  Serial.println("1. Entrando a prensa: 60 cm Izquierda (-X)");
  moverCinematica(-DISTANCIA_60CM, 0, RETARDO_US);
  delay(300);

  // 2. Bajar a buscar la pieza: 40 cm hacia abajo (+Y)
  Serial.println("2. Bajando al molde: 40 cm Abajo (+Y)");
  moverCinematica(0, DISTANCIA_40CM, RETARDO_US);
  delay(300);

  // 3. Sujetar la pieza con el gripper
  Serial.println("3. Cerrando gripper sobre la pieza...");
  servoGripper.write(ANGULO_CERRADO);
  delay(1000); // Tiempo para asegurar el agarre

  // 4. Subir con la pieza: 40 cm hacia arriba (-Y)
  Serial.println("4. Subiendo con la pieza: 40 cm Arriba (-Y)");
  moverCinematica(0, -DISTANCIA_40CM, RETARDO_US);
  delay(300);

  // 5. Salir hacia la zona de descarga: 50 cm a la derecha (+X)
  Serial.println("5. Trasladando a zona de descarga: 50 cm Derecha (+X)");
  moverCinematica(DISTANCIA_50CM, 0, RETARDO_US);
  delay(300);

  // 6. Bajar a descargar: 40 cm hacia abajo (+Y)
  Serial.println("6. Bajando para descargar: 40 cm Abajo (+Y)");
  moverCinematica(0, DISTANCIA_40CM, RETARDO_US);
  delay(300);

  // 7. Soltar la pieza
  Serial.println("7. Abriendo gripper (Descarga)");
  servoGripper.write(ANGULO_ABIERTO);
  delay(800);

  // 8. Regreso al punto de espera seguro: Subir 40 cm (-Y) y volver 10 cm derecha (+X) a Home
  Serial.println("8. Subiendo a cota superior y retornando a posicion Home");
  moverCinematica(0, -DISTANCIA_40CM, RETARDO_US);
  delay(200);
  moverCinematica(DISTANCIA_60CM - DISTANCIA_50CM, 0, RETARDO_US); // Retorna los 10 cm restantes a X=0

  Serial.println("--- [CICLO TERMINADO: ESPERANDO 5 SEGUNDOS] ---");
  delay(5000);
}

// ========================================================
// RUTINA DE HOMING
// ========================================================
void ejecutarHoming() {
  Serial.println("Iniciando Homing Eje X (Hacia la derecha)...");
  // Avanza hacia la derecha (+X) hasta que el switch cierre a GND (LOW)
  long pasosSeguridadX = 0;
  while (digitalRead(PIN_HOME_X) == HIGH) {
    pasoUnicoCinematica(1, 0, RETARDO_US);
    pasosSeguridadX++;
    if (pasosSeguridadX > 6000) { // Limite de proteccion (~1.2 metros)
      Serial.println("ERROR: No se detecto FdC de X dentro del rango!");
      break;
    }
  }
  delay(300);

  // Despegue leve de 5 mm para no dejar el switch presionado
  Serial.println("Despegando switch X...");
  moverCinematica(-25, 0, RETARDO_US);
  delay(500);

  Serial.println("Iniciando Homing Eje Y (Hacia arriba)...");
  // Avanza hacia arriba (-Y) hasta que el switch cierre a GND (LOW)
  long pasosSeguridadY = 0;
  while (digitalRead(PIN_HOME_Y) == HIGH) {
    pasoUnicoCinematica(0, -1, RETARDO_US);
    pasosSeguridadY++;
    if (pasosSeguridadY > 6000) { // Limite de proteccion (~1.2 metros)
      Serial.println("ERROR: No se detecto FdC de Y dentro del rango!");
      break;
    }
  }
  delay(300);

  // Despegue leve de 5 mm para no dejar el switch presionado
  Serial.println("Despegando switch Y...");
  moverCinematica(0, 25, RETARDO_US);
  delay(500);
}

// Da un único paso coordinado en el sentido indicado (-1, 0 o 1)
void pasoUnicoCinematica(long dx_dir, long dy_dir, int retardoUs) {
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

// Transformación cinemática T-Bot completa
void moverCinematica(long dx, long dy, int retardoUs) {
  long dm1 = dy + dx;
  long dm2 = dy - dx;
  conmutarMotores(dm1, dm2, retardoUs);
}

// Interpolación tipo Bresenham
void conmutarMotores(long pasosM1, long pasosM2, int retardoUs) {
  if (pasosM1 >= 0) digitalWrite(PIN_DIR_1, HIGH);
  else {
    digitalWrite(PIN_DIR_1, LOW);
    pasosM1 = -pasosM1;
  }

  if (pasosM2 >= 0) digitalWrite(PIN_DIR_2, HIGH);
  else {
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