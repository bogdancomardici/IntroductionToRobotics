/*
    Mini Matrix Game (Bomberman)

    The classic bomberman game is simulated on the 8x8 matrix.
    Various stats and game information are shown on a LCD Display (player status, welcome and
    end game messages, number of lives left, etc...)
    The game has 3 types of elements:  player (blinks slowly), bombs (blinks fast), wall (doesn’t blink).
    A random map is generated each time the game starts. The player moves around using a
    joystick and places bombs that destroy the walls in a plus pattern (like the original bomberman game).
    Every game is timed and the score is calculated by how fast the player destroys all the walls,
    the player has 3 lives that he can lose when positioned in the blast radius of a bomb when detonating.
    The game has an interactive menu with a different sections such as: about - details about the game and it's 
    creator), settings (change lcd brightness, matrix brightness, etc...).
    Sound effects such as the bomb timer are provided by a buzzer.


    The circuit:

      Input:
        1 x Joystick - used for player movement - pins A0, A1, 13
      
      Output:
        1 x MAX7219 Led matrix driver - pins 10, 11, 12
        1 x 8x8 Led matrix controlled by the MAX7219 driver
        1 x 10uF capacitor to reduce power spikes when the matrix is fully on
        1 x 0.1uF capacitor to filter the noise on 5V
        1 x 10K resitor for the ISET pin on the matrix driver
        1 x LCD Display for displaying various info - pins 3, 4, 5, 6, 7, 8, 9
        1 x 50K Resistors to adjust LCD Contrast

    Menu structure:

    1. Start game
    2. Settings
      2.1. LCD brightness control
      2.2. Matrix brightness control
    3. About

    Created 24.11.2023
    By Comardici Marian Bogdan

    https://github.com/bogdancomardici/IntroductionToRobotics
*/

#include "LedControl.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>

const byte driverDin = 12;    // pin 12 is connected to the MAX7219 pin 1
const byte driverClock = 11;  // pin 11 is connected to the CLK pin 13
const byte driverLoad = 10;   // pin 10 is connected to LOAD pin 12

const byte joystickAxisX = A0;
const byte joystickAxisY = A1;
const byte joystickButton = 13;

// LCD pins
const byte lcdRs = 3;
const byte lcdEn = 8;
const byte lcdD4 = 7;
const byte lcdD5 = 6;
const byte lcdD6 = 5;
const byte lcdD7 = 4;
const byte lcdBacklight = 9;
const byte buzzerPin = 2;

const int bombTone = 700;

LiquidCrystal lcd(lcdRs, lcdEn, lcdD4, lcdD5, lcdD6, lcdD7);
byte lcdBrightness = 10;
byte previousLcdBrightness = 10;
byte lcdLineWidth = 16;
int lcdScrollInterval = 400;

LedControl lc = LedControl(driverDin, driverClock, driverLoad, 1);
const byte matrixSize = 8;
byte matrixBrightness = 2;
byte previousMatrixBrightness = 2;

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

bool inAbout = false;

String aboutString = " Bomberman by Bogdan Comardici - github.com/bogdancomardici";
byte aboutStringPos = 0;
byte aboutStringLen = 56;


bool inSettings = false;
byte settingsPosition = 1;
byte previousSettingsPosition = 0;

bool inSettingsInput = false;
byte settingsInput = 0;

bool inEndGameScreen = false;

bool inMenu = true;

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

byte arrowLeftChar[8] = {
  0b00000,
  0b00000,
  0b00100,
  0b01100,
  0b11100,
  0b01100,
  0b00100,
  0b00000
};

byte arrowRightChar[8] = {
  0b00000,
  0b00000,
  0b00100,
  0b00110,
  0b00111,
  0b00110,
  0b00100,
  0b00000,
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
  lc.clearDisplay(0);

  pinMode(joystickAxisX, INPUT);
  pinMode(joystickAxisY, INPUT);
  pinMode(joystickButton, INPUT_PULLUP);

  pinMode(driverDin, OUTPUT);
  pinMode(driverClock, OUTPUT);
  pinMode(driverLoad, OUTPUT);

  pinMode(buzzerPin, OUTPUT);

  pinMode(lcdRs, OUTPUT);
  pinMode(lcdEn, OUTPUT);
  pinMode(lcdD4, OUTPUT);
  pinMode(lcdD5, OUTPUT);
  pinMode(lcdD6, OUTPUT);
  pinMode(lcdD7, OUTPUT);
  pinMode(lcdBacklight, OUTPUT);

  // read data from eeprom;
  EEPROM.get(0, lcdBrightness);
  EEPROM.get(sizeof(byte), matrixBrightness);

  // set lcd and matrix brightness
  lc.setIntensity(0, matrixBrightness);
  analogWrite(lcdBacklight, lcdBrightness * 25);

  // initialize the LCD
  lcd.createChar(0, arrowDownChar);
  lcd.createChar(1, arrowUpAndDownChar);
  lcd.createChar(2, arrowUpChar);
  lcd.createChar(3, heartChar);
  lcd.createChar(4, arrowLeftChar);
  lcd.createChar(5, arrowRightChar);
  lcd.begin(16, 2);
  welcomeMessage();
  // analog input pin 3 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  randomSeed(analogRead(A3));

  generateMap();

  Serial.begin(9600);
}
void loop() {
  // read the joystick values

  analogWrite(lcdBacklight, lcdBrightness * 25);
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

    if (bombBlinkState) {
      tone(buzzerPin, bombTone);
    } else {
      noTone(buzzerPin);
    }

    // we render the map only when a change to the walls happens
    if (bombPlaced && (millis() - bombPlacedTime > bombTimer)) {
      detonateBomb();
      renderMap();
    }

    renderPlayer();

    if (playerDead() || playerWin()) {
      // reset game
      if (playerWin()) {
        printWin();
        inEndGameScreen = true;
        inGame = false;
        inMenu = false;
      }

      else if (playerDead()) {
        printGameOver();
        inEndGameScreen = true;
        inGame = false;
        inMenu = false;
      }
    }
  } else if (inAbout) {
    if (currentMovement == 2) {
      inAbout = false;
      inMenu = true;
      lcd.clear();
      printMenu(menuPosition);
    } else {
      if (millis() - previousMillis > lcdScrollInterval) {
        previousMillis = millis();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.write(" ");  // to ignore scroll
        lcd.write((uint8_t)4);
        lcd.write("Back");
        lcd.setCursor(0, 1);
        for (byte i = aboutStringPos; i <= aboutStringPos + lcdLineWidth; i++)
          lcd.print(aboutString[i]);
        aboutStringPos++;
        if (aboutStringPos > aboutStringLen - lcdLineWidth)  // wrap string
          aboutStringPos = 0;
        lcd.scrollDisplayLeft();
      }
    }
  } else if (inSettings) {
    if (currentMovement != previousMovement) {
      Serial.println(currentMovement);
      if (currentMovement == 2) {
        lcd.clear();
        printMenu(menuPosition);
        inSettings = false;
        inSettingsInput = false;
        inAbout = false;
        inMenu = true;
      } else if (currentMovement == 3) {
        lcd.clear();
        printSettingsInput(settingsPosition);
        inSettingsInput = true;
        inSettings = false;
        inAbout = false;
      }
      if (currentMovement == 1 && settingsPosition < 2) {
        settingsPosition++;
      } else if (currentMovement == 0 && settingsPosition > 1) {
        settingsPosition--;
      }

      if (settingsPosition != previousSettingsPosition) {
        printSettings(settingsPosition);
        previousSettingsPosition = settingsPosition;
      }
      previousMovement = currentMovement;
    }
  } else if (inSettingsInput) {
    if (currentMovement == 2) {
      inSettingsInput = false;
      inAbout = false;
      inSettings = true;
      inMenu = false;
      lcd.clear();
      printSettings(menuPosition);
      EEPROM.put(0, lcdBrightness);
      EEPROM.put(sizeof(byte), matrixBrightness);
    }
    if (currentMovement != previousMovement) {
      if (settingsPosition == 2) {
        if (currentMovement == 0 && matrixBrightness < 15) {
          matrixBrightness++;
        } else if (currentMovement == 1 && matrixBrightness > 1) {
          matrixBrightness--;
        }

        if (matrixBrightness != previousMatrixBrightness) {
          printSettingsInput(settingsPosition);
          previousMatrixBrightness = matrixBrightness;
          lc.setIntensity(0, matrixBrightness);
        }
      } else if (settingsPosition == 1) {
        if (currentMovement == 0 && lcdBrightness < 10) {
          lcdBrightness++;
        } else if (currentMovement == 1 && lcdBrightness > 1) {
          lcdBrightness--;
        }

        if (lcdBrightness != previousLcdBrightness) {
          printSettingsInput(settingsPosition);
          previousLcdBrightness = lcdBrightness;
          analogWrite(lcdBacklight, lcdBrightness * 25);
        }
      }
      previousMovement = currentMovement;
    }
  } else if (inMenu) {
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

    if (currentMovement == 3) {
      menuActions(menuPosition);
    }
  } else if (inEndGameScreen) {
    if (currentMovement != previousMovement) {
      inEndGameScreen = false;
      resetGame();
    }
    previousMovement = currentMovement;
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

    // check if player is in blast radius
    for (int blastRadius = -1; blastRadius <= 1; blastRadius++) {
      if (playerX == bombX + blastRadius && playerY == bombY) {
        noLives--;
        break;
      }
      if (playerY == bombY + blastRadius && playerX == bombX) {
        noLives--;
        break;
      }
    }
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
      lcd.write("Start Game");
      lcd.write((uint8_t)5);
      lcd.write("    ");
      lcd.write((uint8_t)0);
      break;
    case 2:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Settings");
      lcd.write((uint8_t)5);
      lcd.write("      ");
      lcd.write((uint8_t)1);
      break;
    case 3:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("About");
      lcd.write((uint8_t)5);
      lcd.write("         ");
      lcd.write((uint8_t)2);
      break;
    default:
      break;
  }
}

void printSettings(byte settingsOption) {

  switch (settingsOption) {
    case 1:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write((uint8_t)4);
      lcd.write("Back          ");
      lcd.write((uint8_t)0);
      lcd.setCursor(0, 1);
      lcd.write("LCD Brightness");
      lcd.write((uint8_t)5);
      break;
    case 2:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write((uint8_t)4);
      lcd.write("Back   Matrix ");
      lcd.write((uint8_t)2);
      lcd.setCursor(0, 1);
      lcd.write("    Brightness");
      lcd.write((uint8_t)5);
      break;
    default:
      break;
  }
}

void printSettingsInput(byte settingsOption) {
  switch (settingsOption) {
    case 1:
      lcd.clear();
      lcd.write((uint8_t)4);
      lcd.write("Back");
      lcd.setCursor(0, 1);
      lcd.write("Brightness: ");
      char lcdBrightnessChar[2];
      lcd.write(itoa(lcdBrightness, lcdBrightnessChar, 10));
      if (lcdBrightness == 1)
        lcd.write((uint8_t)2);
      else if (lcdBrightness == 10)
        lcd.write((uint8_t)0);
      else
        lcd.write((uint8_t)1);
      break;
    case 2:
      lcd.clear();
      lcd.write((uint8_t)4);
      lcd.write("Back");
      lcd.setCursor(0, 1);
      lcd.write("Brightness: ");
      char matrixBrightnessChar[2];
      lcd.write(itoa(matrixBrightness, matrixBrightnessChar, 10));
      if (matrixBrightness == 1)
        lcd.write((uint8_t)2);
      else if (matrixBrightness == 15)
        lcd.write((uint8_t)0);
      else
        lcd.write((uint8_t)1);
      break;
    default:
      break;
  }
}
void menuActions(byte menuOption) {
  switch (menuOption) {
    case 1:
      inGame = true;
      inMenu = false;
      renderMap();
      break;
    case 2:
      inSettings = true;
      inMenu = false;
      lcd.clear();
      printSettings(settingsPosition);
      break;
    case 3:
      inAbout = true;
      inMenu = false;
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

bool playerDead() {
  if (noLives == 0)
    return true;

  return false;
}

bool playerWin() {
  for (byte i = 0; i < matrixSize; i++)
    for (byte j = 0; j < matrixSize; j++)
      if (mapMatrix[i][j] == 1)
        return false;

  return true;
}
void printGameOver() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write("## GAME OVER! ##");
  lcd.setCursor(0, 1);
  lcd.write((uint8_t)4);
  lcd.write("EXIT   RESTART");
  lcd.write((uint8_t)5);
}
void printWin() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write("### YOU WIN! ###");
  lcd.setCursor(0, 1);
  lcd.write((uint8_t)4);
  lcd.write("EXIT   RESTART");
  lcd.write((uint8_t)5);
}

void resetGame() {
  inGame = false;
  inMenu = true;
  generateMap();
  lcd.clear();
  playerX = 3;
  playerY = 3;
  noLives = 3;
  playTime = 0;
  printMenu(menuPosition);
}