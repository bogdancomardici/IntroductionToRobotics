# Introduction to Robotics (2023 - 2024)

_Introduction to Robotics laboratory assignments completed during the third year at the Faculty of Mathematics and Computer Science, University of Bucharest. Each assignment entails specific instructions, implementation particulars, as well as code and image files._


[![Static Badge](https://img.shields.io/badge/Faculty_of_Mathematics_and_Computer_Science-blue?style=for-the-badge&link=https%3A%2F%2Ffmi.unibuc.ro%2F)](https://fmi.unibuc.ro)

[![Static Badge](https://img.shields.io/badge/Unibuc_Robotics-blue?style=for-the-badge&logo=facebook&color=%23d3d3d3&link=https%3A%2F%2Fwww.facebook.com%2Funibuc.robotics)](https://www.facebook.com/unibuc.robotics)

[![Static Badge](https://img.shields.io/badge/unibuc.robotics-blue?style=for-the-badge&logo=instagram&color=pink&link=https%3A%2F%2Fwww.instagram.com%2Funibuc.robotics%2F)](https://www.instagram.com/unibuc.robotics)

## Homework #1 - RGB LED

This assignment focuses on controlling each channel (Red, Green, and Blue) of  an  RGB  LED  using  individual  potentiometers.
Three Potentiometers, one RGB LED and coresponding wires and resistors were used to build this project. Capacitors were also used to _filter_ the noisy input of the potentiometers. The potentiometers’s analog values are read with an Arduino UNO and then the mamapped values are written to the LED pins.

Video: https://youtu.be/MSEW_lssp1M?si=4kzzFp8WuwO-5N67

<p align = center>
  <img src="Homework_1_RGB_LED/rgb_led.jpg" width = 50%>
</p>

## Homework #2 - Elevator Simulator

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
Additionally, when the elevator is traveling, button presses are ignored.
To avoid unintentional button presses we use a debouncing technique.

Video: https://youtu.be/foRM9pGMEos

<p align = center>
  <img src="Homework_2_Elevator_Simulator/elevator_simulator.jpg" width = 50%>
</p>

## Homework #3 - 7 Segment Drawing

The scope of this project is to "draw" on a 7 segment display.
The initial position of the drawing cursor is on the decimal point.
We use a joystick to move from one segment to it's neighbours.
The segment that is currently selected blinks regardles of it's state.
Short pressing the joystick toggles the segment ON or OFF and long pressing
the joystick resets all the segments and moves the cursor back to it's starting position,
the decimal point.
Interrupts are used to toggle the segments ON or OFF.
The joystick acts as a toggle to avoid multiple inputs while holding it in one direction.

Video: https://youtu.be/rTrH3HMgBuk

<p align = center>
  <img src="Homework_3_7_Segment_Display_Drawing/7segment_drawing.jpg" width = 50%>
</p>
