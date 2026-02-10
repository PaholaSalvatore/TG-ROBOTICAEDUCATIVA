#include <LedControl.h>
#include <RGBLED.h>

#define STEP_DURATION 5000  // tiempo de cada paso en milisegundos
#define MAX_STEPS 8         //Cantidad de pasos maxima permitida
#define MOVEMENTS 3
#define LIGHTS 4
#define MELODIES 3
#define ANIMATIONS 6
#define MAX_ITERATIONS 4
#define MIN_ITERATIONS 1
#define ENCODER_STEPS_PER_CHANGE 3

#define RED    255, 0, 0
#define GREEN   0, 255, 0
#define BLUE    0, 0, 255
#define WHITE  255, 255, 255
#define OFF 0, 0, 0

// luces led rgb
RGBLED led(9, 10, 11); 

//matrices led
LedControl face = LedControl(51, 52, 53, 2);

// Botones y encoder
const int movementButton = 22;      //boton para seleccionar los movimientos
const int lightButton = 24;         //Boton para seleccionar las luces
const int melodyButton = 26;        //Boton para seleccionar las melodia
const int animationButton = 28;     //Boton para seleccionar las animacion de ojos y boca
const int previousStepButton = 23;  //Boton para regresar al paso anterior
const int nextStepButton = 25;      //Boton para avanzar al siguiente paso

const int startButton = 30;         //Boton para iniciar ejecucion de pasos
const int encoderS1 = 31;           //PIN S1
const int encoderS2 = 32;           //PIN S2

//Variables de control del encoder
int iterations = MIN_ITERATIONS;
int lastS1State = HIGH;
int encoderStepCounter = 0;

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

/* -------- Estrucutra logica de concurrencia --------*/

// Modelo de datos para los pasos
struct step{
  uint8_t movement;
  uint8_t light;
  uint8_t melody;
  uint8_t animation;
};

step currentStep;             //paso actual
step sequence[MAX_STEPS];     // Arreglo para la Secuencia de pasos
int maxConfiguredSteps = 2;   // Configuracion del selector de pasos 4, 6 u 8 
int stepCount = 0;            // Cuántos pasos ya se han guardado (programado por el usuario)
int currentStepIndex = 0;

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

//Sonrisa
void leftSmile(int d) {
  int puntos[][2] = {
    {2,0},{2,1},{2,2},{2,3},{2,4},{2,5},{2,6},{2,7},
    {3,0},{3,1},{3,2},{3,3},{3,4},{3,5},{3,6},{3,7},
    {4,0},{4,1},{4,2},{4,3},{4,4},{4,5},{4,6},{4,7},
    {5,1},{5,2},{5,3},{5,4},{5,5},{5,6},{5,7},
    {6,2},{6,3},{6,4},{6,5},{6,6},{6,7},
    {7,3},{7,4},{7,5},{7,6},{7,7}
  };
  
  for (auto &p : puntos) face.setLed(d, p[0], p[1], true);
}

void rightSmile(int d) {
  int puntos[][2] = {
    {2,0},{2,1},{2,2},{2,3},{2,4},{2,5},{2,6},{2,7},
    {3,0},{3,1},{3,2},{3,3},{3,4},{3,5},{3,6},{3,7},
    {4,0},{4,1},{4,2},{4,3},{4,4},{4,5},{4,6},{4,7},
    {5,0},{5,1},{5,2},{5,3},{5,4},{5,5},{5,6},
    {6,0},{6,1},{6,2},{6,3},{6,4},{6,5},{6,6},
    {7,0},{7,1},{7,2},{7,3},{7,4}
  };
  
  for (auto &p : puntos) face.setLed(d, p[0], p[1], true);
}

//Linea
void seriousMouth(int d) {
  for (int d = 0; d < 2; d++) {
    for (int r = 3; r < 5; r++) {
      for (int c = 0; c < 8; c++) {
        face.setLed(d, r, c, true);
      }
    }
  }
}

//Sonrisa triste
void leftSadMouth(int d) {
  int puntos[][2] = {
    {2,4},{2,5},{2,6},{2,7},
    {3,3},
    {4,2},
    {5,2}
  };
  
  for (auto &p : puntos) face.setLed(d, p[0], p[1], true);
}

void rightSadMouth(int d) {
  int puntos[][2] = {
    {2,0},{2,1},{2,2},{2,3},
    {3,4},
    {4,5},
    {5,5}
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

void printSequence() {
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
}

// Ejecuciones

void executeMovement(int option){
  switch (option) {
    case 0:
      Serial.println("Sin opciones seleccionadas");
      break;

    case 1:
      Serial.println("Adelante");
      break;

    case 2:
      Serial.println("Atras");
      break;

    case 3:
      Serial.println("Stop");
      break;
  }
}

void executeLight(int option){
  switch (option) {
    case 0:
      led.setRGB(OFF);
      Serial.println("Sin opciones seleccionadas");
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
      Serial.println("Sin opciones seleccionadas");
      break;

    case 1:
      Serial.println("Cancion feliz");
      break;

    case 2:
      Serial.println("Cancion relajante");
      break;

    case 3:
      Serial.println("Cancion melancolica");
      break;
  }
}

void executeAnimation(int option){
  turnDownLed();

  switch (option) {
    case 0:
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

//ejecucion del paso
void executeStep(step s){
  
  executeMovement(s.movement);
  executeLight(s.light);
  executeMelody(s.melody);
  executeAnimation(s.animation);
}

void executeSequence() {
  Serial.println("=== Ejecutando secuencia ===");

  for (int i = 0; i < stepCount; i++) {
    Serial.print("Paso ");
    Serial.println(i);

    executeStep(sequence[i]);   // Acciones concurrentes
    delay(STEP_DURATION);       // Mantener el paso activo
  }

  Serial.println("=== Fin de la secuencia ===");
}

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
  executeMovement(0);   // detener motores
  executeLight(0);      // apagar luces
  executeMelody(0);     // detener sonido
  executeAnimation(0);  // mostrar expresión neutra o apagar matriz
  led.setRGB(OFF);
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

//Iteraciones por giro de perilla
void readEncoder() {
  int s1 = digitalRead(encoderS1);
  int s2 = digitalRead(encoderS2);

  if (s1 != lastS1State) {

    // Detectar sentido
    if (s2 == s1) {
      encoderStepCounter++;   // horario
    } else {
      encoderStepCounter--;   // antihorario
    }

    // Solo cambiar valor cuando se acumulan suficientes pasos
    if (encoderStepCounter >= ENCODER_STEPS_PER_CHANGE) {
      iterations++;
      encoderStepCounter = 0;
    }
    else if (encoderStepCounter <= -ENCODER_STEPS_PER_CHANGE) {
      iterations--;
      encoderStepCounter = 0;
    }

    // CLAMP 1–4
    if (iterations < MIN_ITERATIONS) iterations = MIN_ITERATIONS;
    if (iterations > MAX_ITERATIONS) iterations = MAX_ITERATIONS;

    Serial.print("Iteraciones: ");
    Serial.println(iterations);
  }

  lastS1State = s1;
}

void setup() {
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

  //set matrices
  for (int d = 0; d < 2; d++) {
    face.shutdown(d, false);
    face.setIntensity(d, 5);
    face.clearDisplay(d);
  }

  Serial.begin(9600);  
}

void loop() {
  // -------- BOTÓN MOVEMENT --------
  
  if (pressedButton(movementButton, btnMove)){
    movementCounter++;      
    if (movementCounter > MOVEMENTS) movementCounter = 0;  
    currentStep.movement = movementCounter;
  }

/*----------- Boton de luces -----------*/
  if (pressedButton(lightButton, btnLight)){
    lightCounter++; 
    if (lightCounter > LIGHTS) lightCounter = 0;    
    currentStep.light = lightCounter;
  }

/*------------ Boton de melodias ------------*/
  if (pressedButton(melodyButton, btnMelody)){
    melodyCounter++;
    if (melodyCounter > MELODIES) melodyCounter = 0;    
    currentStep.melody = melodyCounter;
  }

/*-------- BOTÓN animacion de ojos y boca --------*/ 
  if (pressedButton(animationButton, btnAnimation)){
    animationCounter++;
    if (animationCounter > ANIMATIONS) animationCounter = 0;   
    currentStep.animation = animationCounter;
  }

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

  if (pressedButton(previousStepButton, btnPreviousStep)) {

    saveCurrentStepAtIndex();

    if (currentStepIndex > 0) {
      currentStepIndex--;
      loadStep(currentStepIndex);
    }
}

  readEncoder();   // siempre leer encoder

  if (pressedButton(startButton, btnStart)) {

    Serial.print("Ejecutando ");
    Serial.print(iterations);
    Serial.println(" iteraciones");

    for (int i = 0; i < iterations; i++) {
      Serial.print("Iteracion ");
      Serial.println(i);
      executeSequence();
    }
    resetProgram();
  }

}
