/*
    Elevator Simulator

    The scope of this project is to simulate a three floor elevator control system.
    Each floor is represented by one LED, the current floor LED lights up to represent
    the elevator moving through the floors. An additional LED is used to indicate the elevator
    status. This LED remains static while the elevator si stationary and blinks while the elevator 
    is moving. 
    Three buttons are used to call the elevator. The movement to each floor is simulated through cycling 
    through the coresponding floors LEDs
    A buzzer is used to play sounds such as: the elevator doors closing, the elevator moving and the "cling"
    sound an elevator makes when it reaches the desired floor.
    If the elevator is already at the selected floor pressing the corresponding button has no effect.
    Additionally, when the elevator is traveling, button presses are ignored
    To avoid unintentional button presses we use a debouncing technique.

    Pins 2, 3, 4 are used to light up the LEDs coresponding to each of the three floors.
    Pin 5 is used for the elevator status indicator LED
    Pins 8, 9, 10 are used to read the inputs from the three buttons of the elevator
    Pin 11 is used for the buzzer that plays the various elevator sounds.

    The circuit:

      Input:
        3 x Push Button - pins 8, 9, 10

      Output:
        3 x White LEDs - pins 2, 3, 4
        3 X 220ohm resistors - to lower the voltage to the 3 LEDs
        1 x Buzzer - pin 11
        1 x Red LED - pin 5

    Created 28.10.2023
    By Comardici Marian Bogdan

    https://github.com/bogdancomardici/IntroductionToRobotics
*/

const int floor1Indicator = 2;
const int floor2Indicator = 3;
const int floor3Indicator = 4;

const int elevatorStateIndicator = 5;
const int soundOutput = 11;

const int floor1Input = 8;
const int floor2Input = 9;
const int floor3Input = 10;

byte floor1Reading = LOW;
byte floor2Reading = LOW;
byte floor3Reading = LOW;

byte floor1LastReading = LOW;
byte floor2LastReading = LOW;
byte floor3LastReading = LOW;

bool floor1Selected = false;
bool floor2Selected = false;
bool floor3Selected = false;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

bool elevatorMoving = false;
int currentFloor = 1;
int nextFloor = 1;

unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
const long floorChangeInterval = 2000; // 2 seconds

unsigned long elevatorTimer = 0;
bool isMovingUp = false;
bool isMovingDown = false;

bool indicatorState = LOW;
unsigned long indicatorBlinkInterval = 100;

int elevatorMovingTone = 50;

int alarmTone = 3000;
bool soundAlarm = false;
unsigned long alarmStartTime = 0;
int alarmSoundTime = 700;

bool closeDoors = false;
unsigned long closeDoorsStartTime = 0;
int closeDoorsDuration = 1000;
int doorsClosingTone = 100;

void setup() {

  pinMode(floor1Indicator, OUTPUT);
  pinMode(floor2Indicator, OUTPUT);
  pinMode(floor3Indicator, OUTPUT);

  pinMode(elevatorStateIndicator, OUTPUT);
  pinMode(soundOutput, OUTPUT);

  pinMode(floor1Input, INPUT_PULLUP);
  pinMode(floor2Input, INPUT_PULLUP);
  pinMode(floor3Input, INPUT_PULLUP);
}

void loop() {
  // light up the led coresponding to the current floor
  // we increment by 1 because the pins we used for the floor indicators start from 2
  digitalWrite(currentFloor + 1, HIGH);

  // read the debounced input for each floor
  floor1Reading = digitalRead(floor1Input);
  floor2Reading = digitalRead(floor2Input);
  floor3Reading = digitalRead(floor3Input);

  bool floor1Selected = debounceInput(floor1Input, &floor1Reading, &lastDebounceTime, debounceDelay);
  bool floor2Selected = debounceInput(floor2Input, &floor2Reading, &lastDebounceTime, debounceDelay);
  bool floor3Selected = debounceInput(floor3Input, &floor3Reading, &lastDebounceTime, debounceDelay);
  
  // select the next floor
  // only valid if the elevator is not moving and the selected floor is not the current floor
  if (floor1Selected == true && currentFloor != 1 && elevatorMoving == false) {
    nextFloor = 1;
    elevatorMoving = true;
    closeDoorsStartTime = millis();
  } else if (floor2Selected == true && currentFloor != 2 && elevatorMoving == false) {
    nextFloor = 2;
    elevatorMoving = true;
    closeDoorsStartTime = millis();
  } else if (floor3Selected == true && currentFloor != 3 && elevatorMoving == false) {
    nextFloor = 3;
    elevatorMoving = true;
    closeDoorsStartTime = millis();
  }

  if (elevatorMoving) {

    currentMillis = millis();

    // play the doors closing sound when the elevator starts moving
    if(currentMillis - closeDoorsStartTime < closeDoorsDuration) {
      tone(soundOutput, doorsClosingTone);
    }
    else {
      // play the elevator moving sound while the elevator is traveling between floors
      tone(soundOutput, elevatorMovingTone);
    }
    
    if (currentMillis - elevatorTimer >= floorChangeInterval) {
      elevatorTimer = currentMillis;
      
      // travel to the next floor in ascending or descending order
      if (isMovingUp && currentFloor < nextFloor) {
        digitalWrite(currentFloor + 1, LOW);
        currentFloor++;
      } else if (isMovingDown && currentFloor > nextFloor) {
        digitalWrite(currentFloor + 1, LOW);
        currentFloor--;
      }

      // when the desired floor is reached the elevator stops moving and an alarm sound is triggered
      if (currentFloor == nextFloor) {
        soundAlarm = true;
        elevatorMoving = false;
        isMovingUp = false;
        isMovingDown = false;
      }

      // check if the elevator is still moving and light up the floor indicators as needed
      if (currentFloor < nextFloor) {
        isMovingUp = true;
        isMovingDown = false;
        digitalWrite(currentFloor + 1, HIGH);
      } else if (currentFloor > nextFloor) {
        isMovingDown = true;
        isMovingUp = false;
        digitalWrite(currentFloor + 1, HIGH);
      }
    }

    // blink the elevator status led while the elevator is moving
    if (currentFloor != nextFloor) {
      if (currentMillis - previousMillis >= indicatorBlinkInterval) {
        previousMillis = currentMillis;
        indicatorState = !indicatorState;
        digitalWrite(elevatorStateIndicator, indicatorState);
      }
    } else {
      digitalWrite(elevatorStateIndicator, LOW);
    }
  }
  else {
    digitalWrite(elevatorStateIndicator, HIGH);
    currentMillis = millis();
    // sound the alarm when the desired floor is reached
    if(soundAlarm == true) {
      alarmStartTime = millis();
      soundAlarm = false;
    }

    if(currentMillis - alarmStartTime < alarmSoundTime) {
      tone(soundOutput, alarmTone);
    }
    else {
      noTone(soundOutput);
    }
  }
}

// function to debounce the input of the elevator buttons
bool debounceInput(int pin, byte *lastReading, unsigned long *lastDebounceTime, unsigned long debounceDelay) {
  bool selected = false;
  int reading = digitalRead(pin);

  if (reading != *lastReading) {
    *lastDebounceTime = millis();
  }

  if ((millis() - *lastDebounceTime) > debounceDelay) {
    if (reading == LOW) {
      selected = true;
    } else if (reading == HIGH) {
      selected = false;
    }
  }

  *lastReading = reading;
  return selected;
}
