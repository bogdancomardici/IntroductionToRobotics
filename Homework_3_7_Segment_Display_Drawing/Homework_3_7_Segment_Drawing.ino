/*
    7 Segment Display Drawing

    The scope of this project is to "draw" on a 7 segment display.
    The initial position of the drawing cursor is on the decimal point.
    We use a joystick to move from one segment to it's neighbours.
    The segment that is currently selected blinks regardles of it's state.
    Short pressing the joystick toggles the segment ON or OFF and long pressing
    the joystick resets all the segments and moves the cursor back to it's starting position,
    the decimal point.
    Interrupts are used to toggle the segments ON or OFF.
    The joystick acts as a toggle to avoid multiple inputs while holding it in one direction.

    The circuit:

      Input:
        1 x Joystick - pins 2, A0, A1
      
      Output:
        1 x 7 segment display with decimal point
        8 x 220ohm resistors - to lower the voltage to the display segments

    Created 02.11.2023
    By Comardici Marian Bogdan

    https://github.com/bogdancomardici/IntroductionToRobotics
*/

const int segmentA = 12;
const int segmentB = 10;
const int segmentC = 9;
const int segmentD = 8;
const int segmentE = 7;
const int segmentF = 6;
const int segmentG = 5;
const int segmentDP = 4;

const int actionTrigger = 2;
const int axisX = A0;
const int axisY = A1;

const int segSize = 8;
int segments[segSize] = { 
  segmentA, segmentB, segmentC, segmentD, segmentE, segmentF, segmentG, segmentDP
};

int xAxisValue;
int yAxisValue;

bool joyMoved = false;
int minThreshold = 400;
int maxThreshold = 600;

int currentCursorPosition = 7;
int cursorMovement = 0;
int previousCursorMovement = 0;

int blinkInterval = 400;
int cursorBlinkState = LOW;

unsigned long previousMillis = 0;
unsigned long currentMillis = 0;

int currentActionTriggerState;
int previousActionTriggerState = LOW;

unsigned long actionInitTime = 0;
unsigned long actionReleasedTime = 0;

int longPressDuration = 1000;
int debounceTime = 200;

volatile byte segmentStates[segSize] = {0, 0, 0, 0, 0, 0, 0, 0};
int possibleMoves[segSize][4] = {
// Possible moves for every segment, -1 if the move is not possible
// or the index in the segments array of the possible segment to move to
// UP DOWN  LEFT  RIGHT
  {-1, 6, 5, 1},  // a
  {0, 6, 5, -1},  // b
  {6, 3, 4, 7},   // c
  {6, -1, 4, 2},  // d
  {6, 3, -1, 2},  // e
  {0, 6, -1, 1},  // f
  {0, 3, -1, -1}, // g
  {-1, -1, 2, -1} // dp
};

void setup() {

  for (int i = 0; i < segSize; i++) {
    pinMode(segments[i], OUTPUT);
  }

  pinMode(actionTrigger, INPUT_PULLUP);
  pinMode(axisX, INPUT);
  pinMode(axisY, INPUT);

  attachInterrupt(digitalPinToInterrupt(actionTrigger), changeSegmentState, FALLING);
}


void loop() {
  
  for (int i = 0; i < segSize; i++) {
    if (i != currentCursorPosition) {
      digitalWrite(segments[i], segmentStates[i]);
    }
  }

  currentMillis = millis();

  // make the cursor blink on the current segment
  if (currentMillis - previousMillis >= blinkInterval) {
        previousMillis = currentMillis;
        cursorBlinkState = !cursorBlinkState;
        digitalWrite(segments[currentCursorPosition], cursorBlinkState);
  }

  xAxisValue = analogRead(axisX);
  yAxisValue = analogRead(axisY);

  currentActionTriggerState = digitalRead(actionTrigger);

  // detect long press of trigger button
  if (currentActionTriggerState == LOW) {
    if (actionInitTime == 0) {
      actionInitTime = millis();
    } else {
      // if the button is long pressed we turn off all the segments and move
      // the cursor back to the decimal point
      if (millis() - actionInitTime >= longPressDuration) {
        for (int i = 0; i < segSize; i++)
          segmentStates[i] = 0;
        currentCursorPosition = 7;
      }
    }
  } else {
    actionInitTime = 0;
  }

  cursorMovement = joyDirection(xAxisValue, yAxisValue, minThreshold, maxThreshold, &joyMoved);

  if (cursorMovement != previousCursorMovement) {
        // get the next move and check if it is valid
    switch (cursorMovement) {
      case 0:
        if (possibleMoves[currentCursorPosition][0] != -1)
          currentCursorPosition = possibleMoves[currentCursorPosition][0];
        break;
      case 1:
        if (possibleMoves[currentCursorPosition][1] != -1)
          currentCursorPosition = possibleMoves[currentCursorPosition][1];
        break;
      case 2:
        if (possibleMoves[currentCursorPosition][2] != -1)
          currentCursorPosition = possibleMoves[currentCursorPosition][2];
        break;
      case 3:
        if (possibleMoves[currentCursorPosition][3] != -1)
          currentCursorPosition = possibleMoves[currentCursorPosition][3];
        break;
      default:
        break;
    }
  }

  previousCursorMovement = cursorMovement;
}

int joyDirection(int xAxisValue, int yAxisValue, int minThreshold, int maxThreshold, bool *joyMoved) {
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

  // return -1 if the joystick didn't move
  if (xAxisValue >= minThreshold && xAxisValue <= maxThreshold && yAxisValue <= maxThreshold && yAxisValue >= minThreshold) {
    *joyMoved = false;
    return -1;
  }
}

// function to change segment state on the current cursor position
void changeSegmentState() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > debounceTime) {
    segmentStates[currentCursorPosition] = !segmentStates[currentCursorPosition];
  }
  lastInterruptTime = interruptTime;
}