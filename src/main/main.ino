#include <LedControl.h>
#define STEP_DURATION 1500  // tiempo de cada paso en milisegundos
#define MAX_STEPS 8         //Cantidad de pasos maxima permitida

unsigned long lastPressTime = 0;
const unsigned long doubleClickDelay = 400; // ms
bool waitingSecondPress = false;

//matrices led
LedControl face = LedControl(51, 52, 53, 2);

// Botones y encoder
const int movementButton = 22;      //boton para seleccionar los movimientos
const int lightButton = 24;         //Boton para seleccionar las luces
const int melodyButton = 26;        //Boton para seleccionar las melodia
const int animationButton = 28;     //Boton para seleccionar las animacion de ojos y boca
const int startButton = 30;         //Boton para iniciar ejecucion de pasos

//Variables de estados relacionadas a los botones de accion
int movementCounter = 0;
int lightCounter = 0;
int melodyCounter = 0; 
int animationCounter = 0;

/* -------- Estrucutra logica de concurrencia --------*/

// Modelo de datos para los pasos
struct step{
  uint8_t movement;
  uint8_t light;
  uint8_t melody;
  uint8_t animation;
};

step currentStep;   //paso actual
step secuency[MAX_STEPS];     // Arreglo para la Secuencia de pasos
int maxConfiguredSteps = 4;   // Configuracion del selector de pasos 4, 6 u 8 
int stepCount = 0;            // Cuántos pasos ya se han guardado (programado por el usuario)

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
void saveCurrentStep() {
  if (stepCount < maxConfiguredSteps) {
    secuency[stepCount] = currentStep;
    Serial.print("Paso guardado en posicion: ");
    Serial.println(stepCount);

    stepCount++;

    // Reiniciar el paso actual para el siguiente
    currentStep.movement = 0;
    currentStep.light = 0;
    currentStep.melody = 0;
    currentStep.animation = 0;

    // Reiniciar los contadores
    movementCounter = 0;
    lightCounter = 0;
    melodyCounter = 0;
    animationCounter = 0;
    
    // Si ya se alcanzó el número configurado de pasos,
    // mostramos toda la secuencia
    if (stepCount == maxConfiguredSteps) {
      printSequence();
    }
  } else {
    Serial.println("Secuencia completa");
  }
}

void printSequence() {
  Serial.println("=== Contenido de la secuencia ===");

  for (int i = 0; i < stepCount; i++) {
    Serial.print("Paso ");
    Serial.print(i);
    Serial.print(" -> ");

    Serial.print("M:");
    Serial.print(secuency[i].movement);
    Serial.print(" | L:");
    Serial.print(secuency[i].light);
    Serial.print(" | Me:");
    Serial.print(secuency[i].melody);
    Serial.print(" | A:");
    Serial.println(secuency[i].animation);
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
      Serial.println("Sin opciones seleccionadas");
      break;

    case 1:
      Serial.println("Azul");
      break;

    case 2:
      Serial.println("Rojo");
      break;

    case 3:
      Serial.println("Amarillo");
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

    executeStep(secuency[i]);   // Acciones concurrentes
    delay(STEP_DURATION);       // Mantener el paso activo
  }

  Serial.println("=== Fin de la secuencia ===");

  // Volver al estado inicial del sistema
  resetProgram();
}

void resetProgram() {
  stepCount = 0;

  currentStep.movement = 0;
  currentStep.light = 0;
  currentStep.melody = 0;
  currentStep.animation = 0;

  movementCounter = 0;
  lightCounter = 0;
  melodyCounter = 0;
  animationCounter = 0;

  waitingSecondPress = false;

  resetHardwareState();

  Serial.println("Sistema reiniciado. Listo para nueva secuencia.");
}

void resetHardwareState() {
  executeMovement(0);   // detener motores
  executeLight(0);      // apagar luces
  executeMelody(0);     // detener sonido
  executeAnimation(0);  // mostrar expresión neutra o apagar matriz
}


void setup() {
  //set botones
  pinMode(movementButton, INPUT_PULLUP);
  pinMode(lightButton, INPUT_PULLUP);
  pinMode(melodyButton, INPUT_PULLUP);
  pinMode(animationButton, INPUT_PULLUP);
  pinMode(startButton, INPUT_PULLUP);

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
  
  if (digitalRead(movementButton) == LOW) {
    
    delay(40);
    
    if (digitalRead(movementButton) == LOW) {
      
      movementCounter++;
      
      if (movementCounter > 3) movementCounter = 0;
      
      currentStep.movement = movementCounter;
      
      while (digitalRead(movementButton) == LOW);
      delay(40);
    }
  }

  // -------- BOTÓN light --------
  if (digitalRead(lightButton) == LOW) {

    delay(40);

    if (digitalRead(lightButton) == LOW) {
      
      lightCounter++;
      
      if (lightCounter > 3) lightCounter = 0;
      
      currentStep.light = lightCounter;
      
      while (digitalRead(lightButton) == LOW);
      delay(40);
    }
  }
    
  // -------- BOTÓN melodia --------
  if (digitalRead(melodyButton) == LOW) {

    delay(40);

    if (digitalRead(melodyButton) == LOW) {
      
      melodyCounter++;
      
      if (melodyCounter > 3) melodyCounter = 0;
      
      currentStep.melody = melodyCounter;
      
      while (digitalRead(melodyButton) == LOW);
      delay(40);
    }
  }

    // -------- BOTÓN animacion de ojos y boca --------
  if (digitalRead(animationButton) == LOW) {

    delay(40);

    if (digitalRead(animationButton) == LOW) {
      
      animationCounter++;
      
      if (animationCounter > 6) animationCounter = 0;
      
      currentStep.animation = animationCounter;
      
      while (digitalRead(animationButton) == LOW);
      delay(40);
    }
  }
  
  if (digitalRead(startButton) == LOW) {
    delay(30);
    if (digitalRead(startButton) == LOW) {

      unsigned long now = millis();

      if (!waitingSecondPress) {
        // Primer pulso detectado
        waitingSecondPress = true;
        lastPressTime = now;
      } else {
        // Segundo pulso dentro del tiempo permitido → ejecutar secuencia
        if (now - lastPressTime <= doubleClickDelay) {
          Serial.println("DOBLE PULSO -> Ejecutar secuencia");
          executeSequence();

          // Reiniciar para nueva programación
          stepCount = 0;
          waitingSecondPress = false;
        }
      }

      while (digitalRead(startButton) == LOW);
      delay(30);
    }
  }

  // Si pasó el tiempo y no hubo segundo pulso → es pulso simple
  if (waitingSecondPress && (millis() - lastPressTime > doubleClickDelay)) {
    Serial.println("PULSO SIMPLE -> Guardar paso");
    saveCurrentStep();
    waitingSecondPress = false;
  }


}
