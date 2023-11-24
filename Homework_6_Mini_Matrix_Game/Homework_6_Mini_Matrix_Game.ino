/*
    Mini Matrix Game (Bomberman)

    The classic bomberman game is simulated on the 8x8 matrix.
    The game has 3 types of elements:  player (blinks slowly), bombs (blinks fast), wall (doesn’tblink).
    A random map is generated each time the game starts. The player moves around using a
    joystick and places bombs that destroy the walls in a plus pattern (like the original bomberman game).

    The circuit:

      Input:
        1 x Joystick - used for player movement - pins A0, A1, 7
      
      Output:
        1 x MAX7219 Led matrix driver - pins 10, 11, 12
        1 x 8x8 Led matrix controlled by the MAX7219 driver
        1 x 10uF capacitor to reduce power spikes when the matrix is fully on
        1 x 0.1uF capacitor to filter the noise on 5V
        1 x 10K resitor for the ISET pin on the matrix driver

    Created 24.11.2023
    By Comardici Marian Bogdan

    https://github.com/bogdancomardici/IntroductionToRobotics
*/

#include "LedControl.h"
const byte driverDin = 12;    // pin 12 is connected to the MAX7219 pin 1
const byte driverClock = 11;  // pin 11 is connected to the CLK pin 13
const byte driverLoad = 10;   // pin 10 is connected to LOAD pin 12

const byte joystickAxisX = A0;
const byte joystickAxisY = A1;
const byte joystickButton = 7;

LedControl lc = LedControl(driverDin, driverClock, driverLoad, 1);
byte matrixBrightness = 2;
const byte matrixSize = 8;

const int playerBlinkInterval = 600;  // 0.6 seconds
unsigned long lastPlayerBlink = 0;
byte playerBlinkState = 1;
byte previousPlayerBlinkState = 1;

byte playerX = 3;
byte playerY = 3;

bool joyMoved = false;
int minJoyThreshold = 400;
int maxJoyThreshold = 600;

int xAxisValue = 0;
int yAxisValue = 0;

byte currentMovement = 0;
byte previousMovement = 0;

const int bombTimer = 3000;
unsigned long bombPlacedTime;
bool bombPlaced = false;

byte bombX = 0;
byte bombY = 0;

byte bombBlinkState = 0;
const int bombBlinkInterval = 100;  // 0.1 seconds
unsigned long lastBombBlink = 0;

byte debounceDelay = 100;
byte joyButtonState = 0;
byte joyButtonReading = 0;
unsigned long lastJoyPress = 0;

byte mapMatrix[matrixSize][matrixSize] = {
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
};

void setup() {

  lc.shutdown(0, false);
  lc.setIntensity(0, matrixBrightness);
  lc.clearDisplay(0);

  pinMode(joystickAxisX, INPUT);
  pinMode(joystickAxisY, INPUT);
  pinMode(joystickButton, INPUT_PULLUP);

  pinMode(driverDin, OUTPUT);
  pinMode(driverClock, OUTPUT);
  pinMode(driverLoad, OUTPUT);

  // analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  randomSeed(analogRead(0));

  generateMap();
}
void loop() {

  // read the joystick values
  xAxisValue = analogRead(joystickAxisX);
  yAxisValue = analogRead(joystickAxisY);
  joyButtonReading = digitalRead(joystickButton);

  joyButtonState = debounceInput(joystickButton, &joyButtonReading, &lastJoyPress, debounceDelay);

  if (joyButtonState) {
    placeBomb(playerX, playerY);
  }

  currentMovement = joyDirection(xAxisValue, yAxisValue, minJoyThreshold, maxJoyThreshold, &joyMoved);

  if (currentMovement != previousMovement) {
    movePlayer(currentMovement);
    previousMovement = currentMovement;
    renderMap();
  }

  if (millis() - lastPlayerBlink > playerBlinkInterval) {
    playerBlinkState = !playerBlinkState;
    lastPlayerBlink = millis();
  }

  // we render the bomb only when one is placed
  if (bombPlaced && (millis() - lastBombBlink > bombBlinkInterval)) {
    bombBlinkState = !bombBlinkState;
    lastBombBlink = millis();
    renderBomb();
  }

  // we render the map only when a change to the walls happens
  if (bombPlaced && (millis() - bombPlacedTime > bombTimer)) {
    detonateBomb();
    renderMap();
  }

  renderPlayer();
}

// generate map in a random manner
void generateMap() {
  for (int row = 0; row < matrixSize; row++)
    for (int col = 0; col < matrixSize; col++)
      mapMatrix[row][col] = random(0, 2);

  // make space for the player to start
  mapMatrix[3][3] = 0;
  mapMatrix[3][4] = 0;
  mapMatrix[4][3] = 0;
  mapMatrix[4][4] = 0;
}

void renderMap() {
  for (int row = 0; row < matrixSize; row++)
    for (int col = 0; col < matrixSize; col++)
      lc.setLed(0, row, col, mapMatrix[row][col]);
}

void renderPlayer() {
  lc.setLed(0, playerX, playerY, playerBlinkState);
}

void renderBomb() {
  lc.setLed(0, bombX, bombY, bombBlinkState);
}

void placeBomb(byte xPosition, byte yPosition) {

  if (!bombPlaced) {
    bombX = xPosition;
    bombY = yPosition;
    bombPlaced = true;
    bombPlacedTime = millis();
  }
}

// detonate the bomb and destroy the walls in a plus pattern
void detonateBomb() {
  if (bombPlaced) {
    mapMatrix[bombX][bombY] = 0;
    if (bombX - 1 >= 0)
      mapMatrix[bombX - 1][bombY] = 0;

    if (bombX + 1 < matrixSize)
      mapMatrix[bombX + 1][bombY] = 0;

    if (bombY - 1 >= 0)
      mapMatrix[bombX][bombY - 1] = 0;

    if (bombY + 1 < matrixSize)
      mapMatrix[bombX][bombY + 1] = 0;
  }
  bombPlaced = false;
  bombX = 9;
  bombY = 9;
}

// move player based on joystick dirrection
void movePlayer(byte direction) {

  if (direction == 4)
    return;

  // move up
  if (direction == 0 && playerX > 0 && mapMatrix[playerX - 1][playerY] == 0) {
    playerX--;
    return;
  }

  // move down
  if (direction == 1 && playerX < matrixSize - 1 && mapMatrix[playerX + 1][playerY] == 0) {
    playerX++;
    return;
  }

  // move left
  if (direction == 2 && playerY > 0 && mapMatrix[playerX][playerY - 1] == 0) {
    playerY--;
    return;
  }

  // move right
  if (direction == 3 && playerY < matrixSize - 1 && mapMatrix[playerX][playerY + 1] == 0) {
    playerY++;
  }
}

byte joyDirection(int xAxisValue, int yAxisValue, int minThreshold, int maxThreshold, bool *joyMoved) {
  // return 0 for up cursorMovement
  if (yAxisValue < minThreshold && *joyMoved == false) {
    *joyMoved = true;
    return 0;
  }

  // return 1 for down cursorMovement
  if (yAxisValue > maxThreshold && *joyMoved == false) {
    *joyMoved = true;
    return 1;
  }

  // return 2 for left cursorMovement
  if (xAxisValue < minThreshold && *joyMoved == false) {
    *joyMoved = true;
    return 2;
  }

  // return 3 for right cursorMovement
  if (xAxisValue > maxThreshold && *joyMoved == false) {
    *joyMoved = true;
    return 3;
  }

  // return 4 if the joystick didn't move
  if (xAxisValue >= minThreshold && xAxisValue <= maxThreshold && yAxisValue <= maxThreshold && yAxisValue >= minThreshold) {
    *joyMoved = false;
    return 4;
  }
}

bool debounceInput(int pin, byte *lastReading, unsigned long *lastDebounceTime, unsigned long debounceDelay) {
  bool pressed = false;
  int reading = digitalRead(pin);

  if (reading != *lastReading) {
    *lastDebounceTime = millis();
  }

  if ((millis() - *lastDebounceTime) > debounceDelay) {
    if (reading == LOW) {
      pressed = true;
    }
  }

  *lastReading = reading;
  return pressed;
}