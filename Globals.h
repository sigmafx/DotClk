#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "Dmd.h"
#include "Config.h"
#include "Button.h"

// Constants
const int pinBtnPlus = 28;
const int pinBtnMinus = 29;
const int pinBtnEnter = 27;
const int pinBtnMenu = 30;

// Dmd Screen
extern Dmd dmd ;

// Configuration
extern Config config ;

// Buttons
extern Button btnMenu;
extern Button btnPlus;
extern Button btnMinus;
extern Button btnEnter;

// Fonts
extern Font fontMenu;
extern Font fontSystem;

#endif
