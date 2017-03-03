# DotClk
This is an Arduino compatible application for driving Dot Matrix Displays to display clock and pinball animations.
Whilst it is developed on the Arduino IDE, it is intended for use on the Teensy 3.6 MCU.

## Why Teensy 3.6?
The Teensy 3.6 comprises all of the necessary features to support a DMD clock:
* On board Real Time Clock (RTC)
* SD Card reader - allows update of pinball animations and fonts
* Plenty of GPIOs
* Vast library of existing Arduino code and examples
* Performance - 180MHz clock speed easily allows DMD clock speed of 4-5MHz
* Small package size
* Cheap and easily available

## How to Use the Hex File
Included in this repository is the hex file built from the latest source code. To upload the hex file to the Teensy you just need to download the loader application:
[] : https://www.pjrc.com/teensy/loader.html

## How to Use The Source Code
