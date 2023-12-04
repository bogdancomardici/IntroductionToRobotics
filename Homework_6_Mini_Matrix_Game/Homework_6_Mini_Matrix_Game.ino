/*
    Mini Matrix Game (Bomberman)

    The classic bomberman game is simulated on the 8x8 matrix.
    Various stats and game information are shown on a LCD Display (player status, welcome and
    end game messages, etc...)
    The game has 3 types of elements:  player (blinks slowly), bombs (blinks fast), wall (doesn’tblink).
    A random map is generated each time the game starts. The player moves around using a
    joystick and places bombs that destroy the walls in a plus pattern (like the original bomberman game).
    Every game is timed and the score is calculated by how fast the player destroys all the walls,
    the player has 3 lives that he can lose when positioned in the blast radius of a bomb when detonating.


    The circuit:

      Input:
        1 x Joystick - used for player movement - pins A0, A1, 13
      
      Output:
        1 x MAX7219 Led matrix driver - pins 10, 11, 12
        1 x 8x8 Led matrix controlled by the MAX7219 driver
        1 x 10uF capacitor to reduce power spikes when the matrix is fully on
        1 x 0.1uF capacitor to filter the noise on 5V
        1 x 10K resitor for the ISET pin on the matrix driver
        1 x LCD Display for displaying various info - pins 4, 5, 6, 7, 8, 9
        1 x 50K Resistors to adjust LCD Contrast

    Created 24.11.2023
    By Comardici Marian Bogdan

    https://github.com/bogdancomardici/IntroductionToRobotics
*/

#include "LedControl.h"
#include <LiquidCrystal.h>

const byte driverDin = 12;    // pin 12 is connected to the MAX7219 pin 1
const byte driverClock = 11;  // pin 11 is connected to the CLK pin 13
const byte driverLoad = 10;   // pin 10 is connected to LOAD pin 12

const byte joystickAxisX = A0;
const byte joystickAxisY = A1;
const byte joystickButton = 13;

// LCD pins
const byte lcdRs = 9;
const byte lcdEn = 8;
const byte lcdD4 = 7;
const byte lcdD5 = 6;
const byte lcdD6 = 5;
const byte lcdD7 = 4;
LiquidCrystal lcd(lcdRs, lcdEn, lcdD4, lcdD5, lcdD6, lcdD7);

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

bool inGame = false;
byte menuPosition = 1;
byte previousMenuPosition = 0;

byte noLives = 3;
byte previousLives = 3;

unsigned long playTime = 0;
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;

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

byte arrowDownChar[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b01110,
  0b00100
};

byte arrowUpAndDownChar[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b00000,
  0b00000,
  0b11111,
  0b01110,
  0b00100
};

byte arrowUpChar[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

byte heartChar[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
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

  // initialize the LCD
  lcd.createChar(0, arrowDownChar);
  lcd.createChar(1, arrowUpAndDownChar);
  lcd.createChar(2, arrowDownChar);
  lcd.createChar(3, heartChar);
  lcd.begin(16, 2);
  welcomeMessage();
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
  currentMovement = joyDirection(xAxisValue, yAxisValue, minJoyThreshold, maxJoyThreshold, &joyMoved);

  if (inGame) {

    currentMillis = millis();

    if (currentMillis - previousMillis > 1000) {  // count seconds
      playTime++;
      playTime %= 1000;
      previousMillis = millis();
      printGameStats(noLives, playTime);
    }

    if (noLives != previousLives) {
      printGameStats(noLives, playTime);
      previousLives = noLives;
    }

    if (joyButtonState) {
      placeBomb(playerX, playerY);
    }

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
  } else {
    if (currentMovement != previousMovement) {
      if (currentMovement == 1 && menuPosition < 3) {
        menuPosition++;
      } else if (currentMovement == 0 && menuPosition > 1) {
        menuPosition--;
      }

      if (menuPosition != previousMenuPosition) {
        printMenu(menuPosition);
        previousMenuPosition = menuPosition;
      }
      previousMovement = currentMovement;
    }

    if (joyButtonState) {
      menuActions(menuPosition);
    }
  }
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

    if(playerX == bombX && playerY == bombY)
      noLives--;
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

void welcomeMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write("## WELCOME TO ###");
  lcd.setCursor(0, 1);
  lcd.write("## BOMBERMAN! ##");
  delay(3000);
  lcd.clear();
}

void printMenu(byte menuOption) {
  switch (menuOption) {
    case 1:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Start Game     ");
      lcd.write((uint8_t)0);
      break;
    case 2:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Settings       ");
      lcd.write((uint8_t)1);
      break;
    case 3:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("About          ");
      lcd.write((uint8_t)2);
      break;
    default:
      break;
  }
}

void menuActions(byte menuOption) {
  switch (menuOption) {
    case 1:
      inGame = true;
      renderMap();
      break;
    case 2:
      break;
    case 3:
      break;
    default:
      break;
  }
}

void printGameStats(byte noLives, unsigned long playTime) {
  lcd.clear();
  lcd.setCursor(0, 0);
  for (int i = 0; i < noLives; i++)
    lcd.write((uint8_t)3);
  lcd.setCursor(0, 1);
  lcd.write("Play time: ");
  char playTimeChar[4];
  lcd.write(itoa(playTime, playTimeChar, 10));
}