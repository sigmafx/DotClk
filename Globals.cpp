#include <Arduino.h>

#include "Globals.h"

// Dmd Screen
Dmd dmd ;

// Configuration
Config config ;

// Buttons
Button btnMenu(pinBtnMenu);
Button btnPlus(pinBtnPlus);
Button btnMinus(pinBtnMinus);
Button btnEnter(pinBtnEnter);

// Fonts
Font fontSystem;
Font fontMenu;

