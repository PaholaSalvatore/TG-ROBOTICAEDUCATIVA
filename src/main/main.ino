/*============= LIBRERIAS =================*/
#include <LedControl.h>
#include <Stepper.h>
#include <RGBLED.h>
#include <DFRobotDFPlayerMini.h>

DFRobotDFPlayerMini player;
HardwareSerial &mp3 = Serial1;

bool melodyPlaying = false;
/*============= CONFIGURACIONES GLOBALES ================*/

#define STEPS_PER_REV 2048    // Pasos por vuelta
#define MOTOR_SPEED 10        // Velocidad RPM

// Corrección física de la orientacion de los motores
#define LEFT_MOTOR_INVERTED  1
#define RIGHT_MOTOR_INVERTED -1

#define STEP_DURATION 8000    // tiempo de cada paso en milisegundos
#define MAX_STEPS 8           //Cantidad de pasos maxima permitida

#define MOVEMENTS 5   // Opciones de movimientos que puede ejecutar el robot
#define LIGHTS 6      // Opciones de luces que puede ejecutar el robot
#define MELODIES 6    // Opciones de melodias que puede reproducir el robot
#define ANIMATIONS 6  // Opciones de animacion que puede hacer el robot

#define MAX_ITERATIONS 4              // Iteraciones maximas ejecutables
#define MIN_ITERATIONS 1              // Iteraciones minimas ejecutables
#define ENCODER_STEPS_PER_CHANGE 3    // Regulacion de pulsos del encoder para seleccionar iteraciones
#define ENCODER_STEPS_PER_REV 15

// Colores RGB 
#define RED    255, 0, 0
#define GREEN   0, 255, 0
#define BLUE    0, 0, 255
#define WHITE  255, 255, 255
#define OFF 0, 0, 0

/*============= DEFINICION DE COMPONENTES ===========*/

RGBLED led(32, 34, 36);                       // luces led rgb
LedControl face = LedControl(51, 52, 53, 2);  // matrices led

// Pines para los Motores
#define leftIn1 39
#define leftIn2 41
#define leftIn3 43
#define leftIn4 45

#define rightIn1 38
#define rightIn2 40
#define rightIn3 42
#define rightIn4 44

Stepper motorLeft(STEPS_PER_REV, leftIn1, leftIn3, leftIn2, leftIn4);
Stepper motorRight(STEPS_PER_REV, rightIn1, rightIn3, rightIn2, rightIn4);

// Botones y encoder
const int movementButton = 22;      //boton para seleccionar los movimientos
const int melodyButton = 23;        //Boton para seleccionar las melodia
const int lightButton = 24;         //Boton para seleccionar las luces
const int animationButton = 25;     //Boton para seleccionar las animacion de ojos y boca
const int previousStepButton = 26;  //Boton para regresar al paso anterior
const int nextStepButton = 27;      //Boton para avanzar al siguiente paso

const int startButton = 28;         //Boton para iniciar ejecucion de pasos
const int encoderS1 = 29;           //PIN S1
const int encoderS2 = 30;           //PIN S2

/*=========== VARIABLES DE CONTROL =============*/

//Variables de control del encoder
int iterations = MIN_ITERATIONS;
int lastS1State = HIGH;
int encoderStepCounter = 0;
int encoderPosition = 0;
const int ledPins[4] = {31, 33, 35, 37};

//Variables de estados relacionadas a los botones de accion
int movementCounter = 0;
int lightCounter = 0;
int melodyCounter = 0; 
int animationCounter = 0;

struct buttonState {
  bool stableState;
  bool lastRead;
  unsigned long lastChange;
};

buttonState btnMove   = {HIGH, HIGH, 0};
buttonState btnLight  = {HIGH, HIGH, 0};
buttonState btnMelody = {HIGH, HIGH, 0};
buttonState btnAnimation   = {HIGH, HIGH, 0};
buttonState btnStart  = {HIGH, HIGH, 0};
buttonState btnNextStep   = {HIGH, HIGH, 0};
buttonState btnPreviousStep = {HIGH, HIGH, 0};

/*========== MODELO DE PASOS ==========*/

// Modelo de datos para los pasos
struct step{
  uint8_t movement;
  uint8_t light;
  uint8_t melody;
  uint8_t animation;
};

step currentStep;             // paso actual
step sequence[MAX_STEPS];     // Arreglo para la Secuencia de pasos

int maxConfiguredSteps = 4;   // Configuracion del selector de pasos 4, 6 u 8 
int stepCount = 0;            // Cuántos pasos ya se han guardado (programado por el usuario)
int currentStepIndex = 0;     // Controlador del indice del arreglo de pasos

/*============== MOTOR SCHEDULER ===============*/

long motorLeftRemaining = 0;
long motorRightRemaining = 0;

unsigned long lastMotorStep = 0;
const unsigned long motorInterval = 2;

// Iniciar movimientos
void startMove(long L, long R){
  motorLeftRemaining = L;
  motorRightRemaining = R;
}

// Actualizar estados de los motores
void updateMotors(){

  if(millis() - lastMotorStep < motorInterval) return;
  lastMotorStep = millis();

  if(motorLeftRemaining != 0){
    int d = (motorLeftRemaining>0)?1:-1;
    motorLeft.step(d*LEFT_MOTOR_INVERTED);
    motorLeftRemaining -= d;
  }

  if(motorRightRemaining != 0){
    int d = (motorRightRemaining>0)?1:-1;
    motorRight.step(d*RIGHT_MOTOR_INVERTED);
    motorRightRemaining -= d;
  }
}

// Detener motores
void stopMotors() {
  digitalWrite(leftIn1, LOW);
  digitalWrite(leftIn2, LOW);
  digitalWrite(leftIn3, LOW);
  digitalWrite(leftIn4, LOW);

  digitalWrite(rightIn1, LOW);
  digitalWrite(rightIn2, LOW);
  digitalWrite(rightIn3, LOW);
  digitalWrite(rightIn4, LOW);
}

/*============= MOVIMIENTOS =============*/

void forward() {
  Serial.println("Avanzar");
  startMove(STEPS_PER_REV, STEPS_PER_REV);
}

void backward() {
  Serial.println("Retroceder");
  startMove(-STEPS_PER_REV, -STEPS_PER_REV);
}

void turnLeft() {
  Serial.println("Girar izquierda");
  startMove(-STEPS_PER_REV/2, STEPS_PER_REV/2);
}

void turnRight() {
  Serial.println("Girar derecha");
  startMove(STEPS_PER_REV/2, -STEPS_PER_REV/2);
}

void spin360() {
  Serial.println("Giro 360");
  startMove(STEPS_PER_REV, STEPS_PER_REV);
}

/*-------------- ANIMACIONES -------------*/
//Apagar matrices
void turnDownLed() {
  for (int d = 0; d < 2; d++) {
    for (int r = 0; r < 8; r++) {
      for (int c = 0; c < 8; c++) {
        face.setLed(d, r, c, false);
      }
    }
  }
}

//Cara de mareado
void dizzyEyes(int d) {
  for (int i = 0; i < 8; i++) {
    face.setLed(d, i, i, true);
    face.setLed(d, i, 7 - i, true);
  }
}

//Ojo abierto
void openedEye(int d) {
  int puntos[][2] = {
    {0,2},{0,3},{0,4},{0,5},
    {1,1},{1,2},{1,3},{1,4},{1,5},{1,6},
    {2,1},{2,2},{2,3},{2,6},
    {3,1},{3,2},{3,3},{3,6},
    {4,1},{4,2},{4,3},{4,4},{4,5},{4,6},
    {5,1},{5,2},{5,3},{5,4},{5,5},{5,6},
    {6,1},{6,2},{6,3},{6,4},{6,5},{6,6},
    {7,2},{7,3},{7,4},{7,5}
  };
  
  for (auto &p : puntos) face.setLed(d, p[0], p[1], true);
}

//Ojo cerrado (guino)
void blinkedEye(int d) {
  int puntos[][2] = {
    {3,1},{3,6},
    {4,1},{4,6},
    {5,2},{5,3},{5,4},{5,5}
  };
  
  for (auto &p : puntos) face.setLed(d, p[0], p[1], true);
}

//Ojos de enamorado (corazones)
void heartEyes(int d) {
  int puntos[][2] = {
    {0,1},{0,2},{0,5},{0,6},
    {1,0},{1,1},{1,2},{1,3},{1,4},{1,5},{1,6},{1,7},
    {2,0},{2,1},{2,2},{2,3},{2,4},{2,5},{2,6},{2,7},
    {3,0},{3,1},{3,2},{3,3},{3,4},{3,5},{3,6},{3,7},
    {4,1},{4,2},{4,3},{4,4},{4,5},{4,6},
    {5,2},{5,3},{5,4},{5,5},
    {6,3},{6,4}
  };
  
  for (auto &p : puntos) face.setLed(d, p[0], p[1], true);
}

//Ojitos tristes
void leftSadEye(int d) {
  int puntos[][2] = {
    {1,0},{1,7},
    {2,0},{2,7},
    {3,1},{3,6},
    {4,2},{4,3},{4,4},{4,5},
    {6,0},{6,1},
    {7,0},{7,1}
  };
  
  for (auto &p : puntos) face.setLed(d, p[0], p[1], true);
}

void rightSadEye(int d) {
  int puntos[][2] = {
    {1,0},{1,7},
    {2,0},{2,7},
    {3,1},{3,6},
    {4,2},{4,3},{4,4},{4,5},
    {6,6},{6,7},
    {7,6},{7,7}
  };
  
  for (auto &p : puntos) face.setLed(d, p[0], p[1], true);
}

//Ojitos enojados
void angryEyes(int d) {
  int puntos[][2] = {
    {0,1},{0,6},
    {1,2},{1,5},
    {2,3},{2,4},
    {4,2},{4,3},{4,4},{4,5},
    {5,2},{5,3},{5,5},
    {6,2},{6,3},{6,4},{6,5},
    {7,2},{7,3},{7,4},{7,5},
  };
  
  for (auto &p : puntos) face.setLed(d, p[0], p[1], true);
}


/* ------ MEMORIA -------*/
void saveCurrentStepAtIndex() {
  sequence[currentStepIndex] = currentStep;

  // Si estamos creando un paso nuevo, aumentar stepCount
  if (currentStepIndex == stepCount && stepCount < maxConfiguredSteps)
    stepCount++;
  
  Serial.print("PASO GUARDADO: ");
  Serial.println(currentStepIndex);
  Serial.print("M:");
  Serial.print(sequence[currentStepIndex].movement);
  Serial.print(" | L:");
  Serial.print(sequence[currentStepIndex].light);
  Serial.print(" | Me:");
  Serial.print(sequence[currentStepIndex].melody);
  Serial.print(" | A:");
  Serial.println(sequence[currentStepIndex].animation);
}

void loadStep(int index) {
  currentStep = sequence[index];

  // Sincronizar contadores con el paso cargado
  movementCounter  = currentStep.movement;
  lightCounter     = currentStep.light;
  melodyCounter    = currentStep.melody;
  animationCounter = currentStep.animation;

    Serial.print("PASO CARGADO: ");
    Serial.println(index);
    Serial.print("M:");
    Serial.print(currentStep.movement);
    Serial.print(" | L:");
    Serial.print(currentStep.light);
    Serial.print(" | Me:");
    Serial.print(currentStep.melody);
    Serial.print(" | A:");
    Serial.println(currentStep.animation);
}

/*void printSequence() {
  Serial.println("=== Contenido de la secuencia ===");

  for (int i = 0; i < stepCount; i++) {
    Serial.print("Paso ");
    Serial.print(i);
    Serial.print(" -> ");

    Serial.print("M:");
    Serial.print(sequence[i].movement);
    Serial.print(" | L:");
    Serial.print(sequence[i].light);
    Serial.print(" | Me:");
    Serial.print(sequence[i].melody);
    Serial.print(" | A:");
    Serial.println(sequence[i].animation);
  }

  Serial.println("===============================");
}*/

// Ejecuciones

void executeMovement(int option){
  switch (option) {
    case 0:
      Serial.println("Sin movimiento seleccionado");
      stopMotors();
      break;

    case 1:
      forward();
      break;

    case 2:
      backward();
      break;

    case 3:
      turnRight();
      break;
    
    case 4:
      turnLeft();
      break;
    
    case 5:
      spin360();
      break;
  }
}

void executeLight(int option){
  switch (option) {
    case 0:
      led.setRGB(OFF);
      Serial.println("Sin luces seleccionadas");
      break;

    case 1:
      led.setRGB(RED);
      Serial.println("ROJO");
      break;

    case 2:
      led.setRGB(BLUE);
      Serial.println("AZUL");
      break;

    case 3:
      led.setRGB(GREEN);
      Serial.println("VERDE");
      break;

    case 4:
      led.setRGB(WHITE);
      Serial.println("BLANCO");
      break;
  }
}

void executeMelody(int option){
  switch (option) {
    case 0:
      player.stop();
      melodyPlaying = false;
      Serial.println("Sin melodias seleccionadas");
      break;

    case 1:
      Serial.println("Cancion feliz");
      player.play(1);   // archivo 0001.mp3
      melodyPlaying = true;
      break;

    case 2:
      Serial.println("Cancion relajante");
      player.play(2);   // archivo 0001.mp3
      melodyPlaying = true;
      break;

    case 3:
      Serial.println("Cancion melancolica");
      player.play(3);   // archivo 0001.mp3
      melodyPlaying = true;
      break;

    case 4:
      Serial.println("Cancion animada");
      player.play(4);   // archivo 0001.mp3
      melodyPlaying = true;
      break;
  }
}

void executeAnimation(int option){
  turnDownLed();

  switch (option) {
    case 0:
      Serial.println("Sin expresiones seleccionadas");
      break;

    case 1:
      Serial.println("Expresion Dizzy");
      dizzyEyes(1);
      dizzyEyes(0);
      break;

    case 2:
      Serial.println("Guiño de ojo");
      openedEye(0);
      blinkedEye(1);
      break;

    case 3:
      Serial.println("Expresion enamorado");
      heartEyes(0);
      heartEyes(1);
      break;

    case 4:
      Serial.println("Expresion triste");
      leftSadEye(1);
      rightSadEye(0);
      break;

    case 5:
      Serial.println("Expresion Feliz");
      openedEye(0);
      openedEye(1);
      break;

    case 6:
      Serial.println("Expresion molesta");
      angryEyes(0);
      angryEyes(1);
      break;
  }
}

// ================= SEQUENCE SCHEDULER =================

bool stepRunning = false;
bool sequenceRunning = false;

unsigned long stepStart = 0;

int executingIndex = 0;
int executingIteration = 0;

//ejecucion del paso
void executeStep(step s){ 
  executeMovement(s.movement);
  executeLight(s.light);
  executeMelody(s.melody);
  executeAnimation(s.animation);
}

void startSequence(){

  if(stepCount==0) return;
  if(sequenceRunning) return;

  executingIndex = 0;
  executingIteration = 0;

  stepRunning = false;   // ← IMPORTANTE
  stepStart = millis();  // ← sincroniza reloj

  sequenceRunning = true;

  Serial.println("Secuencia iniciada");
}

void updateSequence() {

  // Si no hay secuencia corriendo → no hacer nada
  if (!sequenceRunning) return;


  // --- INICIAR PASO NUEVO ---
  if (!stepRunning) {

    // Si ya terminamos todos los pasos
    if (executingIndex >= stepCount) {

      executingIteration++;

      // ¿Ya terminamos todas las iteraciones?
      if (executingIteration >= iterations) {

        Serial.println("Secuencia terminada");

        sequenceRunning = false;

        resetHardwareState();
        resetProgram();

        return;
      }

      // Reiniciar para la siguiente iteración
      executingIndex = 0;
    }

    // 👇 DEBUG DEL PASO

    Serial.print("Paso ");
    Serial.print(executingIndex);
    Serial.print(" | Iteracion ");
    Serial.println(executingIteration + 1);

    // Ejecutar el paso actual
    executeStep(sequence[executingIndex]);

    stepStart = millis();
    stepRunning = true;
  }

  // --- VER SI EL PASO TERMINÓ ---
  if (stepRunning && millis() - stepStart >= STEP_DURATION) {

    stepRunning = false;
    executingIndex++;   // avanzar al siguiente paso

    stopMotors();   // ← solo motores entre pasos
  }
}

/*void executeSequence() {
  Serial.println("=== Ejecutando secuencia ===");

  for (int i = 0; i < stepCount; i++) {
    Serial.print("Paso ");
    Serial.println(i);

    executeStep(sequence[i]);   // Acciones concurrentes
    delay(STEP_DURATION);       // Mantener el paso activo
  }

  Serial.println("=== Fin de la secuencia ===");
}*/

/*========== REINICIO ===========*/

void resetProgram() {
  stepCount = 0;
  currentStepIndex = 0;

  currentStep.movement = 0;
  currentStep.light = 0;
  currentStep.melody = 0;
  currentStep.animation = 0;

  movementCounter = 0;
  lightCounter = 0;
  melodyCounter = 0;
  animationCounter = 0;

  resetHardwareState();

  Serial.println("Sistema reiniciado. Listo para nueva secuencia.");
}

void resetHardwareState() {
  led.setRGB(OFF);      // apagar luces led RGB
  executeMelody(0);     // detener sonido
  stopMotors();         // detener motores
  turnDownLed();       // apagar matrices

  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], LOW);
  }

}


/* --- Movimiento sincronizado base ---
void syncMove(int leftSteps, int rightSteps) {

  int steps = max(abs(leftSteps), abs(rightSteps));

  int left = (leftSteps > 0) ? 1 : -1;
  int right = (rightSteps > 0) ? 1 : -1;

  for (int i = 0; i < steps; i++) {

    if (i < abs(leftSteps))
      motorLeft.step(left * LEFT_MOTOR_INVERTED);

    if (i < abs(rightSteps))
      motorRight.step(right * RIGHT_MOTOR_INVERTED);
  }
}*/

//Lectura de los botones
bool pressedButton(int pin, buttonState &btn) {
  const unsigned long debounceTime = 40;
  bool read = digitalRead(pin);

  if (read != btn.lastRead) {
    btn.lastChange = millis();
  }

  if ((millis() - btn.lastChange) > debounceTime) {
    if (read != btn.stableState) {
      btn.stableState = read;
      if (btn.stableState == LOW) {
        btn.lastRead = read;
        return true;
      }
    }
  }

  btn.lastRead = read;
  return false;
}

/*void readEncoder() {

  static int lastEncoded = 0;
  static long encoderValue = 0;
  static int lastIterations = -1;   // 👈 para detectar cambios

  int MSB = digitalRead(encoderS1);
  int LSB = digitalRead(encoderS2);

  int encoded = (MSB << 1) | LSB;
  int sum  = (lastEncoded << 2) | encoded;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    encoderValue++;

  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    encoderValue--;

  lastEncoded = encoded;

  iterations = constrain(abs(encoderValue)/4 + 1, 1, 4);

  // 👇 imprimir solo cuando cambie
  if(iterations != lastIterations){
    Serial.print("Iteraciones seleccionadas: ");
    Serial.println(iterations);
    lastIterations = iterations;
  }
}*/


//Iteraciones por giro de perilla
void readEncoder() {
 int s1 = digitalRead(encoderS1);
  int s2 = digitalRead(encoderS2);
  int position = 0;

  if (s1 != lastS1State) {

    // Detectar sentido
    if (s2 == s1) {
      encoderStepCounter++;   // horario
    } else {
      encoderStepCounter--;   // antihorario
    }

    // Solo cambiar valor cuando se acumulan suficientes pasos
    if (encoderStepCounter >= ENCODER_STEPS_PER_CHANGE) {
      encoderPosition++;
      encoderStepCounter = 0;
    }
    else if (encoderStepCounter <= -ENCODER_STEPS_PER_CHANGE) {
      encoderPosition--;
      encoderStepCounter = 0;
    }

    // Normalizar vuelta
    if (encoderPosition >= ENCODER_STEPS_PER_REV) 
      encoderPosition = 0;
    if (encoderPosition < 0) 
      encoderPosition = ENCODER_STEPS_PER_REV - 1;

  // Solo primera mitad (0–180°)
    if (encoderPosition < ENCODER_STEPS_PER_REV / 2) {

      // Dividir en zonas (cada 3 pasos aprox)
      position = encoderPosition / ENCODER_STEPS_PER_CHANGE ;

      if (position > ENCODER_STEPS_PER_CHANGE) 
        position = ENCODER_STEPS_PER_CHANGE;

      iterations = position + 1;
    }
    else {
      // Segunda mitad → valor máximo
      iterations = MAX_ITERATIONS;
    }
    Serial.print("Iteraciones: ");
    Serial.println(iterations);
    
    actualizarLEDs();

  }

  lastS1State = s1;
}

void actualizarLEDs() {
  for (int i = 0; i < 4; i++) {
    if (i < iterations) {
      digitalWrite(ledPins[i], HIGH);
    } else {
      digitalWrite(ledPins[i], LOW);
    }
  }
}

/*============= SET UP ============*/

void setup() {
  Serial.begin(9600);  
  
  mp3.begin(9600);    // DFPlayer en TX1/RX1
  if (player.begin(mp3)) {
    Serial.println("DFPlayer OK");
  } else {
    Serial.println("DFPlayer no detectado, continuando sin audio");
  }

  //set botones
  pinMode(movementButton, INPUT_PULLUP);
  pinMode(lightButton, INPUT_PULLUP);
  pinMode(melodyButton, INPUT_PULLUP);
  pinMode(animationButton, INPUT_PULLUP);
  pinMode(startButton, INPUT_PULLUP);
  pinMode(nextStepButton, INPUT_PULLUP);
  pinMode(previousStepButton, INPUT_PULLUP);
  pinMode(encoderS1, INPUT_PULLUP);
  pinMode(encoderS2, INPUT_PULLUP);

  motorLeft.setSpeed(MOTOR_SPEED);
  motorRight.setSpeed(MOTOR_SPEED);

  //set matrices
  for (int d = 0; d < 2; d++) {
    face.shutdown(d, false);
    face.setIntensity(d, 5);
    face.clearDisplay(d);
  }

  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  actualizarLEDs();

  Serial.println("INICIO DEL PROGRAMA");
}


/*================= LOOP ====================*/

void loop() {
  
  /*================= BOTON MOVIMIENTO ==============*/

  if (pressedButton(movementButton, btnMove)){
    movementCounter++;      
    if (movementCounter > MOVEMENTS) movementCounter = 0;  
    currentStep.movement = movementCounter;
  }

  /*============= BOTON LUCES =================*/

  if (pressedButton(lightButton, btnLight)){
    lightCounter++; 
    if (lightCounter > LIGHTS) lightCounter = 0;    
    currentStep.light = lightCounter;
  }

  /*================ BOTON MELODIAS ==============*/

  if (pressedButton(melodyButton, btnMelody)){
    melodyCounter++;
    if (melodyCounter > MELODIES) melodyCounter = 0;    
    currentStep.melody = melodyCounter;
  }

  /*============== BOTON ANIMACIONES =================*/

  if (pressedButton(animationButton, btnAnimation)){
    animationCounter++;
    if (animationCounter > ANIMATIONS) animationCounter = 0;   
    currentStep.animation = animationCounter;
  }

  /*========= SIGUIENTE PASO ========*/

  if (pressedButton(nextStepButton, btnNextStep)) {

    saveCurrentStepAtIndex();

    if (currentStepIndex < maxConfiguredSteps - 1) {
      currentStepIndex++;

      if (currentStepIndex < stepCount) {
        loadStep(currentStepIndex);   // paso ya existente
      } else {
        // paso nuevo → limpio
        currentStep = {0, 0, 0, 0};
        movementCounter = lightCounter = melodyCounter = animationCounter = 0;
      }
    }
  }

  /*============== PASO ANTERIOR ============*/

  if (pressedButton(previousStepButton, btnPreviousStep)) {

    saveCurrentStepAtIndex();

    if (currentStepIndex > 0) {
      currentStepIndex--;
      loadStep(currentStepIndex);
    }
  }

  /*============= ITERACIONES ==========*/
  if (!sequenceRunning){
    readEncoder();   // siempre leer encoder
  }

  /*================ INICIO ==================*/
  if (pressedButton(startButton, btnStart)) {
    startSequence();
  }

  if (sequenceRunning){
    updateMotors();
    updateSequence();
    return;
  }


}
