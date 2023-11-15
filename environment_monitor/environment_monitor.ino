#include <EEPROM.h>
#define FLOAT_SIZE sizeof(float)
const int ledRed = 9;
const int ledBlue = 10;
const int ledGreen = 11;

const int ultrasonicTrigger = 12;
const int ultrasonicEcho = 13;

const int ldrInput = A0;

String menurgbString;
int menuOption = 0;

String submenurgbString;
int submenuOption = 0;

String submenuValueString;
int submenuValue = 0;

bool inMenu = true;
bool inSubMenu = false;
bool inSettingsInputMode = false;
bool showInstructions = false;

int sensorSamplingInterval = 2;
int ultrasonicAlertThreshold = 10;
int ldrAlertThreshold = 300;

bool displayCurrentSensorsReadings = false;

bool ledAutomaticMode = true;
int ledRedValue = 255;
int ledGreenValue = 0;
int ledBlueValue = 200;

unsigned long lastReading = 0;
String sensorReadingsInput;
float ultrasonicDistanceValue = 0.0;
float lightLevelValue = 0.0;

bool alertActive = false;

float ultrasonicLogging[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
float ldrLogging[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void setup() {

  Serial.begin(9600);

  pinMode(ledRed, OUTPUT);
  pinMode(ledBlue, OUTPUT);
  pinMode(ledGreen, OUTPUT);

  pinMode(ultrasonicTrigger, OUTPUT);
  pinMode(ultrasonicEcho, INPUT);

  pinMode(ldrInput, INPUT);

  // read EEPROM Data

  for (int i = 0; i < 10; i++){
    EEPROM.get(i * FLOAT_SIZE, ultrasonicLogging[i]);
  }

  for(int i = 10; i < 20; i++) {
    EEPROM.get(i * FLOAT_SIZE, ldrLogging[i - 10]);
  }
  printMenu();
}

void loop() {

  if (millis() - lastReading > sensorSamplingInterval * 1000) {
    alertActive = false;
    ultrasonicDistanceValue = getDistance();
    lightLevelValue = getLightLevel();
    addUltrasonicLog(ultrasonicDistanceValue);
    addLdrLog(lightLevelValue);
    lastReading = millis();

    // update EEPROM

    for (int i = 0; i < 10; i++) {
      EEPROM.put(i * FLOAT_SIZE, ultrasonicLogging[i]);
    }

    for (int i = 10; i < 20; i++) {
      EEPROM.put(i * FLOAT_SIZE, ldrLogging[i - 10]);
    }
    if (displayCurrentSensorsReadings) {
      printSensorReadings(ultrasonicDistanceValue, lightLevelValue);
    }
    if (ultrasonicDistanceValue < ultrasonicAlertThreshold) {
      Serial.println("ALERT! Object too close!");
      alertActive = true;
    }
    if (lightLevelValue < ldrAlertThreshold) {
      Serial.println("ALERT! Light too low!");
      alertActive = true;
    }
    if (ledAutomaticMode) {
      if (alertActive) {
        displayLedColor(255, 0, 0);
      } else {
        displayLedColor(0, 255, 0);
      }
    } else {
      displayLedColor(ledRedValue, ledGreenValue, ledBlueValue);
    }
  }

  if (inMenu) {
    if (Serial.available() > 0) {
      menurgbString = Serial.readStringUntil('\n');
      menuOption = menurgbString.toInt();

      if (menuOption >= 1 && menuOption <= 4) {
        if (menuOption == 2) {
          goToInputMode();
        } else {
          goToSubMenu();
        }

        printSubMenu(menuOption);
      } else {
        Serial.println("Invalid option. Please try again.");
      }
    }
  } else if (inSubMenu) {
    if (Serial.available() > 0) {
      submenurgbString = Serial.readStringUntil('\n');
      submenuOption = submenurgbString.toInt();
      if (expectsInput(menuOption, submenuOption)) {
        goToInputMode();
        showInstructions = true;
      } else {
        executeFunctions(menuOption, submenuOption, -1);
      }
    }
  } else if (inSettingsInputMode) {
    if (showInstructions) {
      printInstructions(menuOption, submenuOption);
      showInstructions = false;
    }
    if (Serial.available() > 0) {
      submenuValueString = Serial.readStringUntil('\n');

      // convert to string for the manual led control
      if (menuOption == 4 && submenuOption == 1) {
        if (setLedColor(submenuValueString, ledRedValue, ledGreenValue, ledBlueValue)) {
          goBackToSubMenu();
        } else {
          Serial.println("Invalid input. Please try again.");
        }
      } else {
        submenuValue = submenuValueString.toInt();
        executeFunctions(menuOption, submenuOption, submenuValue);
      }
    }
  } else if (displayCurrentSensorsReadings) {

    if (Serial.available() > 0) {
      sensorReadingsInput = Serial.readStringUntil('\n');
      if (sensorReadingsInput.equals("#")) {
        displayCurrentSensorsReadings = false;
        goBackToMenu();
      }
    }
  }
}


float getDistance() {

  digitalWrite(ultrasonicTrigger, LOW);
  delayMicroseconds(2);

  digitalWrite(ultrasonicTrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(ultrasonicTrigger, LOW);

  float duration = pulseIn(ultrasonicEcho, HIGH);
  float distance = duration * 0.034 / 2;

  return distance;
}

float getLightLevel() {

  float lightLevel = analogRead(ldrInput);
  return lightLevel;
}

bool setLedColor(String& rgbString, int& ledRedValue, int& ledGreenValue, int& ledBlueValue) {
  // Count the number of commas in the string
  int commaCount = 0;
  for (int i = 0; i < rgbString.length(); i++) {
    if (rgbString[i] == ',') {
      commaCount++;
    }
  }

  // Check if the string starts with "(" and ends with ")" and contains exactly two commas
  if (rgbString.startsWith("(") && rgbString.endsWith(")") && commaCount == 2) {
    // Create a temporary buffer to store the numbers
    char buffer[rgbString.length()];

    // Copy the string to the buffer, excluding the parentheses
    rgbString.substring(1, rgbString.length() - 1).toCharArray(buffer, rgbString.length() - 1);

    // Use sscanf to extract the three integers
    if (sscanf(buffer, "%d,%d,%d", &ledRedValue, &ledGreenValue, &ledBlueValue) != 3) {
      // If sscanf did not successfully convert all three numbers, set them to -1
      ledRedValue = ledGreenValue = ledBlueValue = 0;
      return false;
    }
  } else {
    // If the input format is incorrect, set all numbers to -1 (or handle it according to your needs)
    ledRedValue = ledGreenValue = ledBlueValue = 0;
    return false;
  }

  if (ledRedValue < 0 || ledRedValue > 255 || ledGreenValue < 0 || ledGreenValue > 255 || ledBlueValue < 0 || ledBlueValue > 255) {
    return false;
  }
  return true;
}

void displayLedColor(int redValue, int greenValue, int blueValue) {
  analogWrite(ledRed, redValue);
  analogWrite(ledGreen, greenValue);
  analogWrite(ledBlue, blueValue);
}

void printMenu() {

  Serial.println("Main Menu");
  Serial.println("1. Sensor Settings");
  Serial.println("2. Reset Logger Data");
  Serial.println("3. System Status");
  Serial.println("4. RGB LED Control");
}

void printSubMenu(int menuItem) {

  switch (menuItem) {
    case 1:
      Serial.println("Sensor Settings");
      Serial.println("1. Sensors Sampling Interval");
      Serial.println("2. Ultrasonic Alert Threshold");
      Serial.println("3. LDR Alert Threshold");
      Serial.println("4. Back");
      break;

    case 2:
      Serial.println("Reset Logger Data");
      Serial.println("Are you sure?");
      Serial.println("1. Yes");
      Serial.println("2. No");
      break;

    case 3:
      Serial.println("System Status");
      Serial.println("1. Current Sensor Readings");
      Serial.println("2. Current Sensor Settings");
      Serial.println("3. Display Logged Data");
      Serial.println("4. Back");
      break;

    case 4:
      Serial.println("RGB LED Control");
      Serial.println("Automatic Mode: " + String(ledAutomaticMode ? "ON" : "OFF"));
      Serial.println("1. Manual Color Control");
      Serial.println("2. LED: Toggle Automatic ON/OFF");
      Serial.println("3. Back");
      break;

    default:
      Serial.println("Invalid option. Please try again.");
      break;
  }
}

void printInstructions(int menuOption, int submenuOption) {
  switch (menuOption) {
    case 1:
      switch (submenuOption) {
        case 1:
          Serial.println("Sensor Sampling Interval (must be between 1 and 10 seconds)");
          Serial.println("Current Sampling Interval: " + String(sensorSamplingInterval) + " seconds");
          Serial.println("Enter the new value, please.");
          break;
        case 2:
          Serial.println("Ultrasonic Alert Threshold");
          Serial.println("Current Minimum Ultrasonic Threshold: " + String(ultrasonicAlertThreshold) + " cm");
          Serial.println("Enter the new value, please.");
          break;
        case 3:
          Serial.println("LDR Alert Threshold");
          Serial.println("Current LDR Alert Threshold: " + String(ldrAlertThreshold));
          Serial.println("Enter the new value, please.");
          break;
        default:
          break;
      }
      break;
    case 4:
      switch (submenuOption) {
        case 1:
          Serial.println("Set the color of the RGB LED in the (rrr,ggg,bbb) format, all values must be between 0 and 255");
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

bool expectsInput(int menuOption, int submenuOption) {
  switch (menuOption) {
    case 1:
      switch (submenuOption) {
        case 1:
          return true;
          break;
        case 2:
          return true;
          break;
        case 3:
          return true;
          break;
        case 4:
          return false;
          break;
        default:
          return false;
          break;
      }
      break;
    case 2:
      return true;
      break;
    case 3:
      return false;
      break;
    case 4:
      switch (submenuOption) {
        case 1:
          return true;
          break;
        default:
          break;
      }

    default:
      return false;
      break;
  }
}
void executeFunctions(int menuOption, int submenuOption, int value) {

  switch (menuOption) {
    case 1:
      switch (submenuOption) {
        case 1:
          if (submenuValue < 1 || submenuValue > 10) {
            Serial.println("Invalid Sampling Interval. Please try again.");
          } else {
            sensorSamplingInterval = submenuValue;
            Serial.println("The new sampling interval is: " + String(sensorSamplingInterval) + " seconds");
            goBackToSubMenu();
          }
          break;
        case 2:
          ultrasonicAlertThreshold = submenuValue;
          Serial.println("The new ultrasonic alert threshold is: " + String(ultrasonicAlertThreshold) + " cm");
          goBackToSubMenu();
          break;
        case 3:
          ldrAlertThreshold = submenuValue;
          Serial.println("The new LDR alert threshold is: " + String(ldrAlertThreshold));
          goBackToSubMenu();
          break;
        case 4:
          goBackToMenu();
          break;
        default:
          Serial.println("Invalid option. Please try again.");
          break;
      }
      break;

    case 2:
      switch (submenuValue) {
        case 1:
          resetLoggerData();
          Serial.println("The logger data has been reset!");
          goBackToMenu();
          break;
        case 2:
          goBackToMenu();
          break;
        default:
          Serial.println("Invalid option. Please try again.");
          break;
      }
      break;
    case 3:
      switch (submenuOption) {
        case 1:
          goToSensorsDisplay();
          Serial.println("Reading sensor input...");
          Serial.println("Current sensor readings, enter # to exit");
          break;
        case 2:
          printSensorSettings();
          goBackToSubMenu();
          break;
        case 3:
          printLoggedData();
          goBackToSubMenu();
          break;
        case 4:
          goBackToMenu();
          break;
        default:
          Serial.println("Invalid option. Please try again.");
          break;
      }
      break;
    case 4:
      switch (submenuOption) {
        case 1:
          break;
        case 2:
          ledAutomaticMode = !ledAutomaticMode;
          Serial.println("LED Automatic Mode is now: " + String(ledAutomaticMode ? "ON" : "OFF"));
          goBackToSubMenu();
          break;
        case 3:
          goBackToMenu();
          break;
        default:
          Serial.println("Invalid option. Please try again.");
          break;
      }
      break;
    default:
      Serial.println("Invalid option. Please try again.");
      break;
  }
}

void goBackToMenu() {
  inMenu = true;
  inSubMenu = false;
  inSettingsInputMode = false;
  displayCurrentSensorsReadings = false;
  printMenu();
}

void goBackToSubMenu() {
  inSubMenu = true;
  inSettingsInputMode = false;
  inMenu = false;
  displayCurrentSensorsReadings = false;
  printSubMenu(menuOption);
}

void goToInputMode() {
  inSettingsInputMode = true;
  inSubMenu = false;
  inMenu = false;
  displayCurrentSensorsReadings = false;
}

void goToSubMenu() {
  inMenu = false;
  inSubMenu = true;
  inSettingsInputMode = false;
  displayCurrentSensorsReadings = false;
}

void goToSensorsDisplay() {
  inMenu = false;
  inSubMenu = false;
  inSettingsInputMode = false;
  displayCurrentSensorsReadings = true;
}

void resetLoggerData() {
  for (int i = 0; i < 10; i++) {
    ultrasonicLogging[i] = 0.0;
    ldrLogging[i] = 0.0;
  }
}

void printSensorReadings(float distance, float lightLevel) {
  Serial.println("Ultrasonic: " + String(distance) + " cm");
  Serial.println("Light Level: " + String(lightLevel));
  Serial.println("-----------");
}

void printSensorSettings() {
  Serial.println("Current Sensor Settings:");
  Serial.println("Sensors Sampling Interval = " + String(sensorSamplingInterval));
  Serial.println("Ultrasonic Alert Threshold = " + String(ultrasonicAlertThreshold));
  Serial.println("LDR Alert Threshold = " + String(ldrAlertThreshold));
}

void printLoggedData() {
  Serial.println("Logged Data: ");

  Serial.println("-- Ultrasonic --");

  float loggedData;
  for (int i = 0; i < 10; i++) {
    EEPROM.get(i * FLOAT_SIZE, loggedData);
    Serial.println(String(i + 1) + ". " + String(loggedData) + " cm");
  }

  Serial.println("-- LDR --");
  for (int i = 10; i < 20; i++) {
    EEPROM.get(i * FLOAT_SIZE, loggedData);
    Serial.println(String(i - 9) + ". " + String(loggedData));
  }
}

void addUltrasonicLog(float ultrasonicValue) {

  // pop the last element

  for (int i = 9; i >= 1; i--) {
    ultrasonicLogging[i] = ultrasonicLogging[i - 1];
  }

  // apend to the front of the queue

  ultrasonicLogging[0] = ultrasonicValue;
}

void addLdrLog(float ldrValue) {

  // pop the last element

  for (int i = 9; i >= 1; i--) {
    ldrLogging[i] = ldrLogging[i - 1];
  }

  // apend to the front of the queue

  ldrLogging[0] = ldrValue;
}