/*
    Potentiometer Controlled RGB LED

    The scope of this project is to control the individual channels of a
    RGB LED using three potentiometers coresponding to the three color
    channels of the LED.

    Pins A0, A1, A2 are used to read the analog value from the potentiometers
    Pins 9, 10, 11 are used to write the analog value coresponding to the
    color intensity of each channel on the RGB LED

    The circuit:

      Input:
        3 x B50K potentiometers - pins A0, A1, A2
        3 x 104 (100.000pF) capacitors - filter the input from the potentiometers

      Output:
        1 x RGB LED - pins 9, 10, 11
        3 X 220ohm resistors - to lower the voltage to the 3 pins of the RGB LED
        
    Created 20.10.2023
    By Comardici Marian Bogdan

    https://github.com/bogdancomardici/IntroductionToRobotics
*/
const int inputRed = A0;
const int inputGreen = A1;
const int inputBlue = A2;

const int outputLedRed = 9;
const int outputLedGreen = 10;
const int outputLedBlue = 11;

int redValue = 0;
int greenValue = 0;
int blueValue = 0;

int redValuePWM = 0;
int greenValuePWM = 0;
int blueValuePWM = 0;

void setup() {

  pinMode(inputRed, INPUT);
  pinMode(inputGreen, INPUT);
  pinMode(inputBlue, INPUT);
  Serial.begin(9600);
}

void loop() {

  // read the values from the potentiometers
  redValue = analogRead(inputRed);
  greenValue = analogRead(inputGreen);
  blueValue = analogRead(inputBlue);

  // map the potentiometer values to the output values for the LED
  redValuePWM = map(redValue, 0, 1023, 0, 255);
  greenValuePWM = map(greenValue, 0, 1023, 0, 255);
  blueValuePWM = map(blueValue, 0, 1023, 0, 255);

  // write the analog values to the LED channels
  analogWrite(outputLedRed, redValuePWM);
  analogWrite(outputLedBlue, greenValuePWM);
  analogWrite(outputLedGreen, blueValuePWM);

  delay(10);
}
