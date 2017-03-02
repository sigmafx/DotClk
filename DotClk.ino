// Library Includes
#include <SD.h>
#include <SPI.h>
#include <Time.h>

// Local Includes
#include "Dmd.h"
#include "Font.h"
#include "Button.h"
#include "Scene.h"
#include "Setup.h"
#include "Config.h"

// Internal Fonts
#include "./Fonts/Standard.h"
#include "./Fonts/System.h"
#include "./Fonts/Menu.h"

enum MODE {
  modeSetup = 1,
  modeClock = 2,
  modeOff = 3,
};

// Pin Assignments
const int pinEN = 19 ;
const int pinR1 = 20 ;
const int pinR2 = 21 ;
const int pinLA = 7 ;
const int pinLB = 6 ;
const int pinLC = 5 ;
const int pinLD = 4 ;
const int pinLT = 1 ;
const int pinSK = 0 ;
const int pinLED = 13;
const int pinBtnPlus = 28;
const int pinBtnMinus = 29;
const int pinBtnEnter = 27;
const int pinBtnMenu = 30;
const int pinGND[] = { 16, 17, 18, 22, 23 } ;

// Fonts
Font fontStandard;
Font fontSystem;
Font fontMenu;
Font fontUser;
Font& fontClock = fontStandard;

// Buttons
Button btnMenu(pinBtnMenu);
Button btnPlus(pinBtnPlus);
Button btnMinus(pinBtnMinus);
Button btnEnter(pinBtnEnter);

// Scene directory
File dirScenes;

//----------------
// Function: setup
//----------------
void setup()
{  
  Dotmap dmpFont;

  // Set GND
  for (int nGnd = 0; nGnd < (sizeof(pinGND) / sizeof(int)); nGnd++)
  {
    digitalWrite(pinGND[nGnd], LOW);
    pinMode(pinGND[nGnd], OUTPUT);
  }
  
  // Serial debug
  Serial.begin(9600);
  //while(!Serial);

  // Set brightness from config
  int cfgBrightness;
  config.GetCfgItem(CFG_BRIGHTNESS, &cfgBrightness, sizeof(cfgBrightness));
  dmd.SetBrightness(cfgBrightness);

  // RTC Compensation
  // adjust is the amount of crystal error to compensate, 1 = 0.1192 ppm
  // For example, adjust = -100 is slows the clock by 11.92 ppm
  
  Teensy3Clock.compensate(0);
    
  // Initialise the LED pin as an output.
  pinMode(pinLED, OUTPUT);

  // System Font
  dmpFont.Create(564, 7);
  dmpFont.SetDotsFromRaw(SYSTEMFontDots, sizeof(SYSTEMFontDots));
  dmpFont.SetMaskFromRaw(SYSTEMFontMask, sizeof(SYSTEMFontMask));
  fontSystem.Create(94, dmpFont);
  fontSystem.SetCharInfoFromRaw(SYSTEMFontCharInfo, sizeof(SYSTEMFontCharInfo));

  // Menu Font
  dmpFont.Create(341, 11);
  dmpFont.SetDotsFromRaw(MENUFontDots, sizeof(MENUFontDots));
  dmpFont.SetMaskFromRaw(MENUFontMask, sizeof(MENUFontMask));
  fontMenu.Create(43, dmpFont);
  fontMenu.SetCharInfoFromRaw(MENUFontCharInfo, sizeof(MENUFontCharInfo));

  // Standard Font
  dmpFont.Create(202, 21);
  dmpFont.SetDotsFromRaw(STANDARDFontDots, sizeof(STANDARDFontDots));
  dmpFont.SetMaskFromRaw(STANDARDFontMask, sizeof(STANDARDFontMask));
  fontStandard.Create(15, dmpFont);
  fontStandard.SetCharInfoFromRaw(STANDARDFontCharInfo, sizeof(STANDARDFontCharInfo));

  // Initialise the DMD
  dmd.Initialise(pinEN, pinR1, pinR2, pinLA, pinLB, pinLC, pinLD, pinLT, pinSK);

  // Start the DMD
  dmd.Start();

  // Boot
  Boot();

  //File fileFontTrek = SD.open("Fonts/TREK.fnt");
  //fontUser.Create(fileFontTrek);
  //fileFontTrek.close();

  //fontClock = fontStandard;
  
  return;
}

//---------------
// Function: loop
//---------------
void loop()
{
  static int mode = modeClock ;

  switch(mode)
  {
    case modeClock:
      if(btnEnter.Read() == BTN_ON_HOLD)
      {
        mode = modeOff;
        dmd.Stop();
      }
      else
      if(btnMenu.Read() == BTN_RISING)
      {
        // Switch to setup mode
        mode = modeSetup;
        doSetup(true);
      }
      else
      {
        doClock();
      }
      break;

    case modeSetup:
      if(!doSetup(false))
      {
        mode = modeClock;
      }
      break ;

    case modeOff:
      if(btnEnter.Read() == BTN_RISING)
      {
        while(btnEnter.Read() != BTN_OFF);
      
        mode = modeClock;
        dmd.Start();
      }
      break ;

    default:
      mode = modeClock;
      break;
  }

  // Keep millis running
  delay(10);
}

//------------------
// Function: doClock
//------------------
void doClock()
{
  static File fileScene ;
  static Scene scene;
  static unsigned long millisClockBeat = 0;
  static unsigned long millisSceneStart = 0;
  static unsigned long millisSceneFrameDelay = 0;
  
  DmdFrame frame;
  Dotmap dmpFrame ;
  Dotmap dmpClock;
  unsigned long millisNow = millis();
  const char *blanking;
  char clock[15 + 1];
  time_t timeNow = now();

  // Initialise the clock
  if(millisClockBeat == 0)
  {
    millisClockBeat = millisNow;
  }

  // Initialise the scene start
  if(millisSceneStart == 0)
  {
    millisSceneStart = millisNow;
  }

  // 'Scenes' directory exists and is open?
  // This means we can start using the scenes files in the directory
  if(dirScenes)
  {
    // Scene file open yet?
    if(!fileScene)
    {
      // Open the next scene file
      fileScene = dirScenes.openNextFile();
      if(!fileScene)
      {
        // End of the directory start again
        dirScenes.rewindDirectory();
        fileScene = dirScenes.openNextFile();
      }

      if(fileScene)
      {
        scene.Create(fileScene);
      }
      else
      {
        // Problem occurred, close the Scenes directory as we can't continue to use it
        dirScenes.close();
      }
    }
  }

  // Generate the clock dotmap as this is always used whether hidden behind a scene frame or part of it through the mask or on top
  // Second beat
  if((((millisNow) / 500) % 2) == 0)
  {
    // Show the second dots
    blanking = "       ";
    digitalWrite(pinLED, HIGH);
  }
  else
  {
    // Hide the second dots
    blanking = "  -    ";
    digitalWrite(pinLED, LOW);
  }

  // Create the clock dotmap  
  int cfgTimeFormat ;
  config.GetCfgItem(CFG_TIMEFORMAT, &cfgTimeFormat, sizeof(cfgTimeFormat));

  switch(cfgTimeFormat)
  {
    default:
    case CFG_TF_24HOUR:
      sprintf(clock, "%02d:%02d", hour(timeNow), minute(timeNow));
      break;

    case CFG_TF_12HOUR:
      sprintf(clock, "%s%d:%02d", hourFormat12(timeNow) > 9 ? "" : " ", hourFormat12(timeNow), minute(timeNow));
      break;

    case CFG_TF_12HAMPM:
      sprintf(clock, "%s%d:%02d%s", hourFormat12(timeNow) > 9 ? "" : " ", hourFormat12(timeNow), minute(timeNow),isAM() ? "AM" : "PM");
      break;
  }
  
  fontClock.DmpFromString(dmpClock, clock, blanking);

  int cfgClockDelay;
  config.GetCfgItem(CFG_CLOCKDELAY, &cfgClockDelay, sizeof(cfgClockDelay));
  switch(cfgClockDelay)
  {
    default:
    case CFG_CD_5SECS:
      cfgClockDelay = 5000;
      break;
    case CFG_CD_10SECS:
      cfgClockDelay = 10000;
      break;
    case CFG_CD_15SECS:
      cfgClockDelay = 15000;
      break;
    case CFG_CD_30SECS:
      cfgClockDelay = 30000;
      break;
    case CFG_CD_1MIN:
      cfgClockDelay = 60000;
      break;
    case CFG_CD_2MINS:
      cfgClockDelay = 120000;
      break;
    case CFG_CD_5MINS:
      cfgClockDelay = 300000;
      break;
  }

  if(millisNow - millisSceneStart < (uint16_t)cfgClockDelay)
  {
    // Only showing the clock between animations
    frame.Clear();
    frame.DotBlt(dmpClock, 0, 0, dmpClock.GetWidth(), dmpClock.GetHeight(), (127 - dmpClock.GetWidth()) / 2, (31 - dmpClock.GetHeight())/2);

    // Update the DMD
    dmd.WaitSync();
    dmd.SetFrame(frame);
  }
  else
  {  
    // At least one scene file exists
    if(fileScene)
    {
      // At the end of the scene?
      if(!scene.Eof())
      {
        // Get the next scene frame?
        if(millisSceneFrameDelay == 0 || (millisNow - millisSceneFrameDelay) > scene.GetFrameDelay())
        {
          // First frame or next frame
          scene.NextFrame(fileScene);
          millisSceneFrameDelay = millisNow;
        }

        // Get the frame dotmap
        dmpFrame = scene.GetFrameDotmap();

        frame.Clear();
        if(scene.GetFrameLayer() == 0)
        {
          // Clock sits behind the animation frame
          frame.DotBlt(dmpClock, 0, 0, dmpClock.GetWidth(), dmpClock.GetHeight(), (127 - dmpClock.GetWidth()) / 2, (31 - dmpClock.GetHeight())/2);
          frame.DotBlt(dmpFrame, 0, 0, dmpFrame.GetWidth(), dmpFrame.GetHeight(), 0, 0);
        }
        else
        {
          // Clock sits above the animation frame
          frame.DotBlt(dmpFrame, 0, 0, dmpFrame.GetWidth(), dmpFrame.GetHeight(), 0, 0);
          frame.DotBlt(dmpClock, 0, 0, dmpClock.GetWidth(), dmpClock.GetHeight(), (127 - dmpClock.GetWidth()) / 2, (31 - dmpClock.GetHeight())/2);
        }

        // Update the DMD
        dmd.WaitSync();
        dmd.SetFrame(frame);
      }
      else
      {
        // Finished the scene, show the clock again
        fileScene.close();
        millisSceneStart = millisNow;
        millisSceneFrameDelay = 0;
      }
    }
    else
    {
      // No scenes to show so revert to the clock
      millisSceneStart = millisNow;
    }
  }
}

//---------------
// Function: Boot
//---------------
bool Boot()
{
  bool ret = false;
  Dotmap    dmpBootText;
  DmdFrame  frameBoot;

  // Title
  fontSystem.DmpFromString(dmpBootText, "DotClk");
  frameBoot.DotBlt(dmpBootText, 0, 0, dmpBootText.GetWidth(), dmpBootText.GetHeight(), 1, 1);
  
  // Version
  #define VERSION "Dave Robbo"
  fontSystem.DmpFromString(dmpBootText, VERSION);
  frameBoot.DotBlt(dmpBootText, 0, 0, dmpBootText.GetWidth(), dmpBootText.GetHeight(), 128 - dmpBootText.GetWidth(), 1);

  // Set up RTC
  setSyncProvider(getTeensy3Time);
  setSyncInterval(60); // Seconds
  delay(100);
  if (timeStatus()!= timeSet)
  {
    Serial.println("Unable to sync with the RTC");
  }
  else
  {
    Serial.println("RTC has set the system time");
  }

  // Connect to SD Card
  Serial.print("Initializing SD card...");
  if (!SD.begin(BUILTIN_SDCARD))
  {
    Serial.println("initialization failed!");
  }
  else
  {
    Serial.println("initialization done.");
  }

  // Open the 'Scenes' directory
  dirScenes = SD.open("/Scenes");

  // Check the contents of the 'Scenes' directory - must only contain valid .scn files

  // SD Status
  fontSystem.DmpFromString(dmpBootText, "SD CARD: OK");
  frameBoot.DotBlt(dmpBootText, 0, 0, dmpBootText.GetWidth(), dmpBootText.GetHeight(), 1, 9);

  // RTC Status
  fontSystem.DmpFromString(dmpBootText, "EEPROM: OK");
  frameBoot.DotBlt(dmpBootText, 0, 0, dmpBootText.GetWidth(), dmpBootText.GetHeight(), 1, 17);

  // Display
  dmd.WaitSync();
  dmd.SetFrame(frameBoot);

  // Wait for 2s
  delay(100);
  
  goto ERROR_EXIT;

  ret = true;
ERROR_EXIT:
  return ret;
}

//-------------------------
// Function: getTeensy3Time
//-------------------------
time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

//------------------
// Function: FreeRam
//------------------
uint32_t FreeRam()
{
    uint32_t stackTop;
    uint32_t heapTop;

    // current position of the stack.
    stackTop = (uint32_t) &stackTop;

    // current position of heap.
    void* hTop = malloc(1);
    heapTop = (uint32_t) hTop;
    free(hTop);

    // The difference is the free, available ram.
    return stackTop - heapTop;
}

