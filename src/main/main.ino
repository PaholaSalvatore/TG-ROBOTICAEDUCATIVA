/*============= LIBRERIAS =================*/
#include <LedControl.h>
#include <Stepper.h>
#include <RGBLED.h>
#include <DFRobotDFPlayerMini.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include "SdFat.h"

/*============= CONFIGURACIONES GLOBALES ================*/

#define STEPS_PER_REV 2048    // Pasos por vuelta
#define HALF_STEPS_PER_REV STEPS_PER_REV/2
#define MOTOR_SPEED 10        // Velocidad RPM

#define SECONDS_PER_MINUTE 60UL
#define MS_PER_SECOND 1000UL
#define MS_PER_MINUTE (SECONDS_PER_MINUTE * MS_PER_SECOND)

#define REVOLUTION_TIME_MS (MS_PER_MINUTE / MOTOR_SPEED)
#define HALF_REV_TIME_MS (REVOLUTION_TIME_MS / 2)

// Corrección física de la orientacion de los motores
#define LEFT_MOTOR_INVERTED  1
#define RIGHT_MOTOR_INVERTED -1

#define STEP_DURATION 8000    // tiempo de cada paso en milisegundos
#define MAX_STEPS 6           // Cantidad de pasos maxima permitida
#define MIN_STEPS 4           // Cantidad de pasos minima permitida

#define MOVEMENTS 6   // Opciones de movimientos que puede ejecutar el robot
#define LIGHTS 7      // Opciones de luces que puede ejecutar el robot
#define MELODIES 7    // Opciones de melodias que puede reproducir el robot
#define ANIMATIONS 7  // Opciones de animacion que puede hacer el robot

#define MAX_ITERATIONS 4              // Iteraciones maximas ejecutables
#define MIN_ITERATIONS 1              // Iteraciones minimas ejecutables
#define ENCODER_STEPS_PER_CHANGE 3    // Regulacion de pulsos del encoder para seleccionar iteraciones
#define ENCODER_STEPS_PER_REV 15

// Colores RGB 
#define RED    255, 0, 0
#define GREEN   0, 255, 0
#define BLUE    0, 0, 255
#define YELLOW 255, 255, 0
#define PINK 148, 0, 211
#define PURPLE 255, 0, 255
#define ORANGE 200, 140, 0
#define WHITE  255, 255, 255
#define OFF 0, 0, 0

// Pines TFT
const uint8_t SD_CS_PIN = 10;
const uint8_t SOFT_MISO_PIN = 12;
const uint8_t SOFT_MOSI_PIN = 11;
const uint8_t SOFT_SCK_PIN  = 13;

// Soft SPI
SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(0), &softSpi)

MCUFRIEND_kbv tft;
SdFat sd;
File bmpFile;

/* ============== ESTADOS BARRA DE PROGRESO UI ==========*/

bool barTransitionActive = false;
int barTransitionStep = 0;
unsigned long barTransitionTime = 0;

const int barTransitionFrames = 15;   // suavidad
const int barTransitionInterval = 15; // velocidad ms

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

DFRobotDFPlayerMini player;
HardwareSerial &mp3 = Serial1;

bool melodyPlaying = false;

// Dimensiones y posiciones de la iconografia
const int sizeIcon = 100;
const int sizeBarWidth = 220;
const int sizeBarHeight = 50;
const int leftX  = 10;
const int rightX = 130;
const int row1 = 20;
const int row2 = ((2 * row1) + sizeIcon);
const int row3 = 260;

/*=========== VARIABLES DE CONTROL =============*/

//Variables de control del encoder
int iterations = MIN_ITERATIONS;
int lastS1State = HIGH;
int encoderStepCounter = 0;
int encoderPosition = 0;
const int ledPins[4] = {31, 33, 35, 37};

bool blinkActive = false;
int blinkLed = 0;
int blinkCount = 0;

unsigned long blinkTimer = 0;

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

/* ====== CAMBIOS DEL BOTON DE INICIO ======*/
unsigned long startPressTime = 0;
bool startWasPressed = false;

const unsigned long LONG_PRESS_TIME = 1000; // 1 segundo
bool sequencePaused = false;

unsigned long pauseStartTime = 0;

/*========== MODELO DE PASOS ==========*/

// Modelo de datos para los pasos
struct step{
  uint8_t movement;
  uint8_t light;
  uint8_t melody;
  uint8_t animation;
};

const int stepSelector = 47;

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
  startMove(-HALF_STEPS_PER_REV, HALF_STEPS_PER_REV);
}

void turnRight() {
  Serial.println("Girar derecha");
  startMove(HALF_STEPS_PER_REV, -HALF_STEPS_PER_REV);
}

void spin360() {
  Serial.println("Giro 360");
  startMove(STEPS_PER_REV*2, STEPS_PER_REV*2);
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

//Ojos felices
void happyEyes(int d) {
  int puntos[][2] = {
                {1,2},{1,3},{1,4},{1,5},
          {2,1},                        {2,6},
          {3,1},                        {3,6}                          
  };

  for (auto &p : puntos) face.setLed(d, p[0], p[1], true);
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
          {2,1},{2,2},{2,3},            {2,6},
          {3,1},{3,2},{3,3},            {3,6},
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
          {0,1},{0,2},            {0,5},{0,6},
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
    {1,0},                                    {1,7},
    {2,0},                                    {2,7},
          {3,1},                        {3,6},
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
void leftAngryEye(int d) {
  int puntos[][2] = {
    {0,0},{0,1},{0,2},{0,3},
    {1,0},{1,1},{1,2},{1,3},{1,4},
    {2,0},{2,1},{2,2},{2,3},{2,4},{2,5},
    {3,0},{3,1},{3,2},{3,3},{3,4},{3,5},{3,6},
    {4,0},{4,1},{4,2},{4,3},{4,4},{4,5},{4,6},{4,7},
    {5,0},{5,1},{5,2},{5,3},{5,4},            {5,7},
    {6,0},{6,1},{6,2},{6,3},{6,4},            {6,7},
    {7,0},{7,1},{7,2},{7,3},{7,4},{7,5},{7,6},{7,7}
  };
  
  for (auto &p : puntos) face.setLed(d, p[0], p[1], true);
}

void rightAngryEye(int d){
    int puntos[][2] = {
                            {0,4},{0,5},{0,6},{0,7},
                      {1,3},{1,4},{1,5},{1,6},{1,7},
                {2,2},{2,3},{2,4},{2,5},{2,6},{2,7},
          {3,1},{3,2},{3,3},{3,4},{3,5},{3,6},{3,7},
    {4,0},{4,1},{4,2},{4,3},{4,4},{4,5},{4,6},{4,7},
    {5,0},{5,1},{5,2},{5,3},{5,4},            {5,7},
    {6,0},{6,1},{6,2},{6,3},{6,4},            {6,7},
    {7,0},{7,1},{7,2},{7,3},{7,4},{7,5},{7,6},{7,7}
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

void updateStepMode() {

  static int lastState = digitalRead(stepSelector);

  int state = digitalRead(stepSelector);

  if (state != lastState) {

    delay(20);
    state = digitalRead(stepSelector);

    if (state != lastState) {

      //Cambiar modo
      if (state == LOW) {
        maxConfiguredSteps = MIN_STEPS;   // 4
      } else {
        maxConfiguredSteps = MAX_STEPS;   // 6
      }

      Serial.print("Modo pasos: ");
      Serial.println(maxConfiguredSteps);
      Serial.println(digitalRead(stepSelector));

      //AJUSTAR ÍNDICE
      currentStepIndex = constrain(currentStepIndex, 0, maxConfiguredSteps - 1);

      // AJUSTAR stepCount (MUY IMPORTANTE)
      if (stepCount > maxConfiguredSteps) {
        stepCount = maxConfiguredSteps;
      }

      //LIMPIAR PASOS EXTRA (de 6 → 4)
      for (int i = maxConfiguredSteps; i < MAX_STEPS; i++) {
        sequence[i] = {0,0,0,0};
      }

      //REDIBUJAR UI COMPLETA
      updateIcons(currentStep);
      startBarTransition();

      lastState = state;
    }
  }
}

void initStepMode() {

  int state = digitalRead(stepSelector);

  if (state == LOW) {
    maxConfiguredSteps = MIN_STEPS;   // 4
  } else {
    maxConfiguredSteps = MAX_STEPS;   // 6
  }

  Serial.print("Modo inicial: ");
  Serial.println(maxConfiguredSteps);
}

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
      led.setRGB(PINK);
      Serial.println("ROSADO");
      break;
      
    case 5:
      led.setRGB(PURPLE);
      Serial.println("MORADO");
      break;

    case 6:
      led.setRGB(YELLOW);
      Serial.println("AMARILLO");
      break;

    case 7:
      led.setRGB(ORANGE);
      Serial.println("NARANJA");
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
      Serial.println("Cancion abecedario");
      player.play(1);   // archivo 0001.mp3
      melodyPlaying = true;
      break;

    case 2:
      Serial.println("Cancion de navidad");
      player.play(2);   // archivo 0001.mp3
      melodyPlaying = true;
      break;

    case 3:
      Serial.println("Cancion alegre");
      player.play(3);   // archivo 0001.mp3
      melodyPlaying = true;
      break;

    case 4:
      Serial.println("Cancion de autobus");
      player.play(4);   // archivo 0001.mp3
      melodyPlaying = true;
      break;
    
    case 5:
      Serial.println("Cancion de SC");
      player.play(5);   // archivo 0001.mp3
      melodyPlaying = true;
      break;
      
    case 6:
      Serial.println("Cancion de Clementine");
      player.play(6);   // archivo 0001.mp3
      melodyPlaying = true;
      break;
    
    case 7:
      Serial.println("Cancion de HB");
      player.play(7);   // archivo 0001.mp3
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
      Serial.println("Feliz");
      happyEyes(0);
      happyEyes(1);
      break;

    case 2:
      Serial.println("Guiño de ojo");
      openedEye(0);
      blinkedEye(1);
      break;

    case 3:
      Serial.println("Triste");
      leftSadEye(1);
      rightSadEye(0);
      break;

    case 4:
      Serial.println("Enojado");
      leftAngryEye(1);
      rightAngryEye(0);
      break;

    case 5:
      Serial.println("Enamorado");
      heartEyes(0);
      heartEyes(1);
    break;

    case 6:
      Serial.println("Curioso");
      openedEye(0);
      openedEye(1);
      break;

    case 7:
      Serial.println("Dizzy");
      dizzyEyes(0);
      dizzyEyes(1);
    break;
  }

}

// ================= SEQUENCE SCHEDULER =================

bool stepRunning = false;
bool sequenceRunning = false;

unsigned long stepStart = 0;
unsigned long stepDuration = 0;

int executingIndex = 0;
int executingIteration = 0;

//ejecucion del paso
void executeStep(step s){ 
  executeMovement(s.movement);
  executeLight(s.light);
  executeMelody(s.melody);
  executeAnimation(s.animation);
}

unsigned long getMovementDuration(uint8_t movement){

  switch (movement){

    case 0: return STEP_DURATION;

    case 1: return REVOLUTION_TIME_MS;        // forward
    case 2: return REVOLUTION_TIME_MS;        // backward
    case 3: return HALF_REV_TIME_MS;   // turn
    case 4: return HALF_REV_TIME_MS;
    case 5: return REVOLUTION_TIME_MS;        // spin

    default: return STEP_DURATION;
  }
}

void startSequence(){
  if(sequenceRunning) return;

  // GUARDAR SIEMPRE EL PASO ACTUAL
  sequence[currentStepIndex] = currentStep;

  if (currentStepIndex == stepCount && stepCount < maxConfiguredSteps) {
    stepCount++;
  }

  if(stepCount==0) return;

  executingIndex = 0;
  executingIteration = 0;
  startBlink(0);

  stepRunning = false;   // ← IMPORTANTE
  stepStart = millis();  // ← sincroniza reloj

  sequenceRunning = true;

  Serial.println("Secuencia iniciada");
}

void updateSequence() {

  // Si no hay secuencia corriendo → no hacer nada
  if (!sequenceRunning) return;

  //si esta pausado --> no avanza
  if (sequencePaused) return;

  // --- INICIAR PASO NUEVO ---
  if (!stepRunning) {

    // Si ya terminamos todos los pasos
    if (executingIndex >= stepCount) {

      executingIteration++;
      startBlink(executingIteration);

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

    Serial.print("Paso ");
    Serial.print(executingIndex);
    Serial.print(" | Iteracion ");
    Serial.println(executingIteration);

    // Ejecutar el paso actual
    step current = sequence[executingIndex];
    updateIcons(current);
    updateBar(executingIndex);

    executeStep(current);    
    stepDuration = getMovementDuration(current.movement);

    stepStart = millis();
    stepRunning = true;
  }

  // --- VER SI EL PASO TERMINÓ ---
  if (stepRunning && millis() - stepStart >= stepDuration) {

    stepRunning = false;
    executingIndex++;   // avanzar al siguiente paso

    stopMotors();   // ← solo motores entre pasos
  }
}

void startBlink(int ledIndex){

  blinkActive = true;
  blinkLed = ledIndex;
  blinkCount = 0;

  blinkTimer = millis();
}

void updateBlink(){

  if(!blinkActive) return;

  if(millis() - blinkTimer > 120){

    blinkTimer = millis();

    digitalWrite(ledPins[blinkLed], !digitalRead(ledPins[blinkLed]));

    blinkCount++;

    if(blinkCount >= 6){   // 3 parpadeos

      digitalWrite(ledPins[blinkLed], HIGH);
      blinkActive = false;
    }
  }
}

/*========== REINICIO ===========*/

void resetProgram() {
  stepCount = 0;
  currentStepIndex = 0;

  currentStep.movement = 0;
  currentStep.light = 0;
  currentStep.melody = 0;
  currentStep.animation = 0;

  for (int i=0 ; i < maxConfiguredSteps; i++){
    sequence[i] = currentStep;
  }

  movementCounter = 0;
  lightCounter = 0;
  melodyCounter = 0;
  animationCounter = 0;

  updateIcons(currentStep);
  updateBar(currentStepIndex);

  blinkActive = 0;
  blinkCount = 0;

  actualizarLEDs();

  Serial.println("Sistema reiniciado. Listo para nueva secuencia.");
}

void resetHardwareState() {
  led.setRGB(OFF);      // apagar luces led RGB
  player.stop();
    melodyPlaying = false;
  stopMotors();         // detener motores
  turnDownLed();        // apagar matrices

  //Apagar leds de iteraciones
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  Serial.println("Hardware reiniciado");

}

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

void handleStartButton() {

  bool read = digitalRead(startButton);

  // Detectar PRESIONADO
  if (read == LOW && !startWasPressed) {
    startWasPressed = true;
    startPressTime = millis();
  }

  // Detectar SOLTADO
  if (read == HIGH && startWasPressed) {

    startWasPressed = false;

    unsigned long pressDuration = millis() - startPressTime;

    // --------- PULSACIÓN LARGA ----------
    if (pressDuration > LONG_PRESS_TIME) {

      Serial.println("RESET");

      sequenceRunning = false;
      sequencePaused = false;

      resetProgram();
      resetHardwareState();
    }

    // --------- PULSACIÓN CORTA ----------
    else {

      if (!sequenceRunning) {
        Serial.println("START");
        startSequence();
      }
      else if (sequenceRunning && !sequencePaused) {
        Serial.println("PAUSA");
        sequencePaused = true;
        pauseStartTime = millis();  // GUARDAR MOMENTO DE PAUSA

        if(melodyPlaying){
          player.pause();
        }
      }
      else if (sequencePaused) {
        Serial.println("REANUDAR");
        sequencePaused = false;
        stepStart += millis() - pauseStartTime; // AJUSTAR EL TIEMPO DEL PASO

        if(melodyPlaying){
          player.start();
        }
      }
    }
  }
}

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

// ================= MOSTRAR IMAGENES ===============
void drawRAW(const char *filename, int x, int y, int w, int h) {

    File file = sd.open(filename);

    if (!file) {
        Serial.println("No se pudo abrir RAW");
        return;
    }

    uint16_t buffer[240];  

    for (int row = 0; row < h; row++) {
        // 🔥 POSICIÓN EXACTA EN EL ARCHIVO
        file.seek(row * w * 2);

        // leer fila exacta
        file.read((uint8_t*)buffer, w * 2);

        tft.setAddrWindow(x, y + row, x + w - 1, y + row);
        tft.pushColors(buffer, w, true);
    }

    file.close();
}

void updateMovementIcon(int value) {
  char filename[20];
  sprintf(filename, "move%d.raw", value);
  drawRAW(filename, leftX, row1, sizeIcon, sizeIcon);
}

void updateLightIcon(int value) {
  char filename[20];
  sprintf(filename, "color%d.raw", value);
  drawRAW(filename, leftX, row2, sizeIcon, sizeIcon);
}

void updateMelodyIcon(int value) {
  char filename[20];
  sprintf(filename, "music%d.raw", value);
  drawRAW(filename, rightX, row1, sizeIcon, sizeIcon);
}

void updateAnimationIcon(int value) {
  char filename[20];
  sprintf(filename, "eyes%d.raw", value);
  drawRAW(filename, rightX, row2, sizeIcon, sizeIcon);
}

void updateIcons(step s) {
  updateMovementIcon(s.movement);
  updateLightIcon(s.light);
  updateMelodyIcon(s.melody);
  updateAnimationIcon(s.animation);
}

void updateBar(int stepIndex) {

  char filename[20];
  sprintf(filename, "bar%d_%d.raw", maxConfiguredSteps, stepIndex);

  drawRAW(filename, leftX, row3, sizeBarWidth, sizeBarHeight);
}

void startBarTransition() {
  barTransitionActive = true;
  barTransitionStep = 0;
  barTransitionTime = millis();
}

void updateBarTransition() {

  // bloquear animación si el robot está ejecutando
  if (sequenceRunning) return;

  if (!barTransitionActive) return;

  if (millis() - barTransitionTime < barTransitionInterval) return;

  barTransitionTime = millis();

  int wipeWidth = map(barTransitionStep, 0, barTransitionFrames, 0, sizeBarWidth);

  tft.fillRect(leftX, row3, wipeWidth, sizeBarHeight, 0x0000);

  barTransitionStep++;

  if (barTransitionStep > barTransitionFrames) {

    updateBar(currentStepIndex);
    barTransitionActive = false;
  }
}

/*============= SET UP ============*/

void setup() {
  Serial.begin(9600);  

  pinMode(stepSelector, INPUT_PULLUP);
  initStepMode();

  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(0);   // vertical
  tft.fillScreen(0x0000);

  Serial.println("Iniciando SD...");

  if (!sd.begin(SD_CONFIG)) {
    Serial.println("Error SD");
    tft.setTextColor(0xF800);
    tft.println("SD Error");
    return;
  }

  Serial.println("SD OK");

  //Mostrar imagenes derecha
  updateIcons(sequence[currentStepIndex]);
  updateBar(currentStepIndex);
      
  mp3.begin(9600);    // DFPlayer en TX1/RX1
  if (player.begin(mp3)) {
    Serial.println("DFPlayer OK");
  } else {
    Serial.println("DFPlayer no detectado, continuando sin audio");
  }
  player.volume(30);  // Volumen de 0 a 30

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

  updateBarTransition();

  if (!sequenceRunning){  
    updateStepMode();

    /*================= BOTON MOVIMIENTO ==============*/

    if (pressedButton(movementButton, btnMove)){
      movementCounter++;      
      if (movementCounter > MOVEMENTS) movementCounter = 0;  
      currentStep.movement = movementCounter;

      updateMovementIcon(movementCounter);
    }

    /*============= BOTON LUCES =================*/

    if (pressedButton(lightButton, btnLight)){
      lightCounter++; 
      if (lightCounter > LIGHTS) lightCounter = 0;    
      currentStep.light = lightCounter;

      updateLightIcon(lightCounter);
    }

    /*================ BOTON MELODIAS ==============*/

    if (pressedButton(melodyButton, btnMelody)){
      melodyCounter++;
      if (melodyCounter > MELODIES) melodyCounter = 0;    
      currentStep.melody = melodyCounter;

      updateMelodyIcon(melodyCounter);
    }

    /*============== BOTON ANIMACIONES =================*/

    if (pressedButton(animationButton, btnAnimation)){
      animationCounter++;
      if (animationCounter > ANIMATIONS) animationCounter = 0;   
      currentStep.animation = animationCounter;

      updateAnimationIcon(animationCounter);
    }

    /*========= SIGUIENTE PASO ========*/

    if (pressedButton(nextStepButton, btnNextStep)) {

      saveCurrentStepAtIndex();

      if (currentStepIndex < maxConfiguredSteps - 1) {
        currentStepIndex++;

        if (currentStepIndex < stepCount) {
          loadStep(currentStepIndex);   // paso ya existente
          updateIcons(sequence[currentStepIndex]);
          updateBar(currentStepIndex);
        } else {
          // paso nuevo → limpio
          currentStep = {0, 0, 0, 0};
          movementCounter = lightCounter = melodyCounter = animationCounter = 0;
          updateIcons(sequence[currentStepIndex]);
          updateBar(currentStepIndex);
        }
      }
    }

    /*============== PASO ANTERIOR ============*/

    if (pressedButton(previousStepButton, btnPreviousStep)) {

      saveCurrentStepAtIndex();

      if (currentStepIndex > 0) {
        currentStepIndex--;
        loadStep(currentStepIndex);
        updateIcons(sequence[currentStepIndex]);
        updateBar(currentStepIndex);
      }
    }

    /*============= ITERACIONES ==========*/
    readEncoder();   // siempre leer encoder
  }

  /*================ INICIO ==================*/
  handleStartButton();
  updateBlink();

  if (sequenceRunning){
    updateMotors();
    updateSequence();
    return;
  }
}
