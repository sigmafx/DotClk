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

// Force Wake flag
bool forceWake = false;

// Scene counts
uint16_t cntScenes = 0;

// SD Card
SdFs* sdfs = NULL;
