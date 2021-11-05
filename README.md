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
https://www.pjrc.com/teensy/loader.html

## How to Use The Source Code
To compile the source code you'll need to download the Arduino IDE and the Teensy extensions:
https://www.pjrc.com/teensy/tutorial.html

You can then grab the code from the repository and compile / update the code locally as you need.

N.B. You'll need to use the SD library in the following repository:
https://github.com/sigmafx/SD

This includes changes for fast directory listing using newly added function getNextFile.

When compiling the code use the following CPU Speeds (from Arduino IDE menu Tools->CPU Speed):
Teensy 3.6: 180 MHz
Teensy 3.5: 168 MHz (overclocked)

## How to Use DotClk
The code is designed for use with the DotClk interface board. The code is well documented for the pin assignments for the various connections to the screen and the control buttons.

Pressing the Menu button will display the menu. Use Prev and Next buttons and the Edit / Back buttons to setup the clock.

Press and hold the Back button to turn off the display. Press and release the Back button to turn on again.

## Where are the Animations Held?
On the SD Card there needs to be a directory called Scenes. Place the animation scene files (.scn) in this directory.

## Where are the Fonts Held?
On the SD Card there needs to be a directory called Fonts. Place the font files (.fnt) in this directory.

Get in touch if you need any help - I hang out on PinballInfo.com, user DrPinball.

## If the clock keeps crashing / screen goes blank
There has been an issue where the clock appears to stop working - the screen remains blank and it is necessary to turn off, then on to restart the clock, although once this has started happening the clock will show a blank screen again after a short while.

This issue has now been fixed - if you are having this problem please follow these steps:
1. Remove the SD card from the clock and delete all files on it. Copy over the Scenes and Fonts folders by following the instructions here:
https://github.com/sigmafx/DotClk-Resources

2. Update the software installed on the clock by using the latest DotClk.ino.TEENSY35.hex file. To do this follow the instruections here:
https://github.com/sigmafx/DotClk#how-to-use-the-hex-file

It will be necessary to connect a micro usb cable to the Teensy micro controller in the clock and use the Teensy loader application to select the hex file and upload to the board.

If you are having issues with this process please send an email to dotclk@drpinball.co.uk.