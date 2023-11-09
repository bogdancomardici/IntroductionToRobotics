/*
    Stopwatch Timer

    The scope of this project is to build a simple stopwatch with the following functionalities:
      - start/stop the stopwatch
      - reset the stopwatch
      - save up to four lap times
      - view and reset the lap times

    A four digit 7 segment display along with a 74HC595N shift register is used to display the timer,
    with a tenth of a second precision
    Three buttons are used to control the stopwatch - start/pause, reset, save/cycle lap time

    If the lap button is pressed during the timer counter the lap time is saved. We can only save the last
    lap time, we overwrite the oldest one.

    The reset button only works while the timer is paused

    In pause mode, the lap button cycles through the saved lap times

    Pressing the reset button in lap cycle mode, resets the saved laps.
    Pressing the reset button in timer mode (paused) resets the timer.

    Interrupts are used on the start/pause and reset buttons to increase timing precision.

    Pins 10, 11, 12 are used to control the shift register clock, latch and data inputs.
    Pins 4, 5, 6, 7 are used to light up the four digits of the 7 segment display using the shift register.
    Pins 2, 3, 9 are used for the control buttons on the stopwatch.

    The circuit:

      Input:
        3 x Push Button - pins 2, 3, 9

      Output:
        1 X 4 digit 7 segment display - to display the current timer time, pins 4, 5, 6, 7
        8 X 220ohm resistors - to lower the voltage to the 3 LEDs

      Input and Output:
        1 X 74HC595N shift register - to display the time on the 7 segment display, pins 10, 11, 12 for input

    Created 10.11.2023
    By Comardici Marian Bogdan

    https://github.com/bogdancomardici/IntroductionToRobotics
*/
const int clockPin = 10;
const int latchPin = 11;
const int dataPin = 12;

const int segD1 = 4;
const int segD2 = 5;
const int segD3 = 6;
const int segD4 = 7;

const int startPauseTrigger = 2;
const int resetTrigger = 3;
const int lapTrigger = 9;

int displayDigits[] = { segD1, segD2, segD3, segD4 };
const int displayCount = 4;

const int encodingsNumber = 10;

byte byteEncodings[encodingsNumber] = {
  //A B C D E F G DP
  B11111100,  // 0
  B01100000,  // 1
  B11011010,  // 2
  B11110010,  // 3
  B01100110,  // 4
  B10110110,  // 5
  B10111110,  // 6
  B11100000,  // 7
  B11111110,  // 8
  B11110110   // 9
};

unsigned long currentMillis = 0;
unsigned long lastIncrement = 0;
unsigned long delayCount = 100;  // count in tenths of a second


const int debounceDelay = 200;
unsigned long lastDebounceTime = 0;
unsigned long currentTime = 0;

const int noLaps = 4;
unsigned long lapTimes[noLaps] = { 0, 0, 0, 0 };
int lastLapNumber = 0;

volatile bool stopwatchRunning = false;
bool enterLapCycle = false;

int lapTriggerState;
int lastLapTriggerState = LOW;
int lapTriggerValue;

void setup() {

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  pinMode(startPauseTrigger, INPUT_PULLUP);
  pinMode(resetTrigger, INPUT_PULLUP);
  pinMode(lapTrigger, INPUT_PULLUP);

  for (int i = 0; i < displayCount; i++) {
    pinMode(displayDigits[i], OUTPUT);
    digitalWrite(displayDigits[i], LOW);
  }

  attachInterrupt(digitalPinToInterrupt(startPauseTrigger), startPauseTimer, FALLING);
  attachInterrupt(digitalPinToInterrupt(resetTrigger), resetTimer, FALLING);
}

void loop() {

  currentMillis = millis();
  lapTriggerValue = digitalRead(lapTrigger);

  // debounce the lap button input
  if (lapTriggerValue != lastLapTriggerState) {
    lastDebounceTime = currentMillis;
  }

  if ((currentMillis - lastDebounceTime) > debounceDelay) {

    if (lapTriggerValue != lapTriggerState) {

      lapTriggerState = lapTriggerValue;

      if (lapTriggerState == LOW) {
        // if the stopwatch is running we save the lap time
        if (stopwatchRunning)
          saveLap(currentTime);
        else {
          // if the stopwatch is paused we can cycle through the saved lap times
          enterLapCycle = true;
          if (lastLapNumber < noLaps - 1)
            lastLapNumber++;
          else
            lastLapNumber = 0;
        }
      }
    }
  }

  lastLapTriggerState = lapTriggerValue;

  if (stopwatchRunning) {
    if (currentMillis - lastIncrement > delayCount) {
      currentTime++;
      currentTime %= 10000;
      lastIncrement = millis();
    }

    // we write the current time or the current lap time based on the state of the stopwatch
  }
  if (enterLapCycle) {
    writeNumber(lapTimes[lastLapNumber]);
  } else
    writeNumber(currentTime);
}

void writeReg(int digit) {

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, digit);
  digitalWrite(latchPin, HIGH);
}

void activateDisplay(int displayNumber) {

  for (int i = 0; i < displayCount; i++) {
    digitalWrite(displayDigits[i], HIGH);
  }
  digitalWrite(displayDigits[displayNumber], LOW);
}

void writeNumber(int number) {

  int currentNumber = number;
  int displayDigit = 3;
  int lastDigit = 0;

  while (currentNumber != 0) {

    lastDigit = currentNumber % 10;
    activateDisplay(displayDigit);

    // add the decimal point on the second display
    if (displayDigit == 2)
      writeReg(byteEncodings[lastDigit] + 1);
    else
      writeReg(byteEncodings[lastDigit]);

    displayDigit--;
    currentNumber /= 10;

    writeReg(B00000000);
  }

  // fill the rest of the display with 0
  while (displayDigit >= 0) {
    activateDisplay(displayDigit);
    if (displayDigit == 2)
      writeReg(byteEncodings[0] + 1);
    else
      writeReg(byteEncodings[0]);
    displayDigit--;
    writeReg(B00000000);
  }
}

void startPauseTimer() {

  // start and pause the stopwatch
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > debounceDelay) {
    stopwatchRunning = !stopwatchRunning;
    enterLapCycle = false;
  }

  lastInterruptTime = interruptTime;
}

void resetTimer() {

  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > debounceDelay && stopwatchRunning == false) {

    // if we are in the lap cycle mode we reset the lap times
    if (enterLapCycle == true) {
      for (int i = 0; i < noLaps; i++)
        lapTimes[i] = 0;
    // if we are in the stopwatch mode we reset the current time
    } else {
      currentTime = 0;
      enterLapCycle = false;
    }
  }

  lastInterruptTime = interruptTime;
}

void saveLap(int lapTime) {

  // save the latest lap and oveeride the last one
  if (lastLapNumber < noLaps) {
    lapTimes[lastLapNumber] = currentTime % 10000;
    lastLapNumber++;
  }

  if (lastLapNumber >= noLaps)
    lastLapNumber = 0;
}