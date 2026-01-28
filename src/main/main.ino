// Botones y encoder
const int movementButton = 22;      //boton para seleccionar los movimientos
const int lightButton = 24;         //Boton para seleccionar las luces
const int melodyButton = 26;        //Boton para seleccionar las melodia
const int animationButton = 28;     //Boton para seleccionar las animacion de ojos y boca
const int startButton = 30;         //Boton para iniciar ejecucion de pasos

int movementCounter = 0;
int lightCounter = 0;
int melodyCounter = 0; 
int animationCounter = 0;

struct step{
  uint8_t movement;
  uint8_t light;
  uint8_t melody;
  uint8_t animation;
};

step currentStep;

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
  switch (option) {
    case 0:
      Serial.println("Sin opciones seleccionadas");
      break;

    case 1:
      Serial.println("Feliz");
      break;

    case 2:
      Serial.println("Triste");
      break;

    case 3:
      Serial.println("Molesto");
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

void setup() {
  //set botones
  pinMode(movementButton, INPUT_PULLUP);
  pinMode(lightButton, INPUT_PULLUP);
  pinMode(melodyButton, INPUT_PULLUP);
  pinMode(animationButton, INPUT_PULLUP);
  pinMode(startButton, INPUT_PULLUP);

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
      
      if (animationCounter > 3) animationCounter = 0;
      
      currentStep.animation = animationCounter;
      
      while (digitalRead(animationButton) == LOW);
      delay(40);
    }
  }

  if (digitalRead(startButton) == LOW) {
  
    delay(40);
  
    if (digitalRead(startButton) == LOW) {
      Serial.println("START");
      executeStep(currentStep);
      while (digitalRead(startButton) == LOW);   
      delay(40);
   }
  }


}
