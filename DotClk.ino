// Library Includes
#include <SD.h>
#include <SPI.h>
#include <TimeLib.h>

// Local Includes
#include "Dmd.h"
#include "Font.h"
#include "Button.h"
#include "Scene.h"
#include "Setup.h"
#include "Config.h"
#include "Utils.h"
#include "Version.h"

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
const int pinG1 = 3 ;
const int pinG2 = 2 ;
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
Font *fontUser = NULL;
Font *fontClock;

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

  // Serial debug
  Serial.begin(9600);
  //while(!Serial);

  // Set GND for unused pins
  for (int nGnd = 0; nGnd < (int)(sizeof(pinGND) / sizeof(int)); nGnd++)
  {
    digitalWrite(pinGND[nGnd], LOW);
    pinMode(pinGND[nGnd], OUTPUT);
  }
  
  // Initialise the LED pin as an output.
  pinMode(pinLED, OUTPUT);

  // Detect button presses for configuring the DMD type
  InitDmdType();
    
  // Create internal fonts
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
  dmd.SetDmdType(config.GetCfgItems().cfgDmdType);
  dmd.Initialise(pinEN, pinR1, pinR2, pinG1, pinG2, pinLA, pinLB, pinLC, pinLD, pinLT, pinSK);

  // Set DMD brightness from config
  dmd.SetBrightness(config.GetCfgItems().cfgBrightness);

  // Set DMD colour from config
  dmd.SetColour(config.GetCfgItems().cfgDotColour);

  // Start the DMD
  dmd.Start();

  // Boot
  Boot();
  
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
      if(btnEnter.Read() == Button::Hold)
      {
        // From Clock mode to Off mode
        mode = modeOff;
        dmd.Stop();
      }
      else
      if(btnMenu.Read() == Button::Rising)
      {
        // From Clock mode to Setup mode
        mode = modeSetup;
        doSetup(true);
      }
      else
      {
        // Clock mode
        doClock();
      }
      break;

    case modeSetup:
      if(!doSetup(false))
      {
        // Returning from Setup mode
        mode = modeClock;

        // Need to force a refresh of the clock font as it may have been changed by the user
        InitClockFont();
      }
      break ;

    case modeOff:
      if(btnEnter.Read() == Button::Rising)
      {
        while(btnEnter.Read() != Button::Off);

        // From Off mode to Clock mode
        mode = modeClock;
        dmd.Start();
      }
      break ;

    default:
      // Default position is to Clock mode
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
  static uint16_t curScene = 0;
  
  DmdFrame frame;
  Dotmap dmpFrame ;
  Dotmap dmpClock;
  unsigned long millisNow = millis();
  const char *blanking;
  char clock[15 + 1];
  time_t timeNow = NowDST();

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

  if(!dirScenes)
  {
    // Scenes dir not open, try to init the SD Card
    Serial.println("Attempting to InitSD");
    InitSD();
  }

  // 'Scenes' directory exists and is open?
  // This means we can start using the scenes files in the directory
  if(dirScenes)
  {
    // Scene file open yet?
    if(!fileScene)
    {
      uint16_t skip ;
      uint16_t divider ;

      curScene++;
      switch(config.GetCfgItems().cfgShowBrand)
      {
        default:
        case Config::CFG_SB_NEVER:
          divider = 0;
          break ;
      
        case Config::CFG_SB_EVERY2:
          divider = 2;
          break ;

        case Config::CFG_SB_EVERY5:
          divider = 5;
          break ;

        case Config::CFG_SB_EVERY10:
          divider = 10;
          break ;

        case Config::CFG_SB_EVERY20:
          divider = 20;
          break ;
      }

      if((divider > 0 && (curScene % divider) > 0) || !SD.exists("/Scenes/brand.scn"))
      {
        // Open the next random scene file
        skip = random(0, 30);
        for(int scene = 0; scene < skip; scene++)
        {
          // This is a slow way to skip files in the directory, so I've limited it to skipping max 30 scenes.
          fileScene = dirScenes.openNextFile();
          if(!fileScene)
          {
            // End of the directory start again
            dirScenes.rewindDirectory();
            fileScene = dirScenes.openNextFile();
          }
  
          // Close if not last scene file as this is the one we want to use
          if(scene < (skip - 1))
          {
            fileScene.close();
          }
        }
      }
      else
      {
        fileScene = SD.open("/Scenes/brand.scn");
      }
      
      if(fileScene)
      {
        if(!scene.Create(fileScene))
        {
          // Couldn't read the scene - we can't continue to use it
          dirScenes.close();
        }
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
  switch(config.GetCfgItems().cfgTimeFormat)
  {
    default:
    case Config::CFG_TF_24HOUR:
      sprintf(clock, "%02d:%02d", hour(timeNow), minute(timeNow));
      break;

    case Config::CFG_TF_12HOUR:
      sprintf(clock, "%s%d:%02d", hourFormat12(timeNow) > 9 ? "" : " ", hourFormat12(timeNow), minute(timeNow));
      break;

    case Config::CFG_TF_12HAMPM:
      sprintf(clock, "%s%d:%02d%s", hourFormat12(timeNow) > 9 ? "" : " ", hourFormat12(timeNow), minute(timeNow),isAM() ? "AM" : "PM");
      break;
  }
  
  int cfgClockDelay;
  switch(config.GetCfgItems().cfgClockDelay)
  {
    default:
    case Config::CFG_CD_5SECS:
      cfgClockDelay = 5000;
      break;
    case Config::CFG_CD_10SECS:
      cfgClockDelay = 10000;
      break;
    case Config::CFG_CD_15SECS:
      cfgClockDelay = 15000;
      break;
    case Config::CFG_CD_30SECS:
      cfgClockDelay = 30000;
      break;
    case Config::CFG_CD_1MIN:
      cfgClockDelay = 60000;
      break;
    case Config::CFG_CD_2MINS:
      cfgClockDelay = 120000;
      break;
    case Config::CFG_CD_5MINS:
      cfgClockDelay = 300000;
      break;
  }

  if(millisNow - millisSceneStart < (uint16_t)cfgClockDelay)
  {
    // Generate clock dotmap
    fontClock->DmpFromString(dmpClock, clock, blanking);

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
        int xClock, yClock;

        // Generate clock dotmap
        switch(scene.GetClockStyle())
        {
          default:
          case Scene::ClockStyleStd:
            // Generate clock dotmap
            fontClock->DmpFromString(dmpClock, clock, blanking);
            xClock = (127 - dmpClock.GetWidth()) / 2;
            break;

          case Scene::ClockStyle1:
            clock[5] = '\0'; // Remove am/pm
            fontMenu.DmpFromString(dmpClock, clock, blanking);
            xClock = (42 - dmpClock.GetWidth()) / 2;
            break;

          case Scene::ClockStyle2:
            clock[5] = '\0'; // Remove am/pm
            fontMenu.DmpFromString(dmpClock, clock, blanking);
            xClock = ((42 - dmpClock.GetWidth()) / 2) + 43;
            break;

          case Scene::ClockStyle3:
            clock[5] = '\0'; // Remove am/pm
            fontMenu.DmpFromString(dmpClock, clock, blanking);
            xClock = ((42 - dmpClock.GetWidth()) / 2) + 87;
            break;

        case Scene::ClockStyle4:
            clock[5] = '\0'; // Remove am/pm
            fontMenu.DmpFromString(dmpClock, clock, blanking);
            xClock = (64 - dmpClock.GetWidth()) / 2;
            break;

          case Scene::ClockStyle5:
            clock[5] = '\0'; // Remove am/pm
            fontMenu.DmpFromString(dmpClock, clock, blanking);
            xClock = ((64 - dmpClock.GetWidth()) / 2) + 64;
            break;
        }
        
        // Determine y position of clock
        yClock = (31 - dmpClock.GetHeight()) / 2;

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
          frame.DotBlt(dmpClock, 0, 0, dmpClock.GetWidth(), dmpClock.GetHeight(), xClock, yClock);
          frame.DotBlt(dmpFrame, 0, 0, dmpFrame.GetWidth(), dmpFrame.GetHeight(), 0, 0);
        }
        else
        {
          // Clock sits above the animation frame
          frame.DotBlt(dmpFrame, 0, 0, dmpFrame.GetWidth(), dmpFrame.GetHeight(), 0, 0);
          frame.DotBlt(dmpClock, 0, 0, dmpClock.GetWidth(), dmpClock.GetHeight(), xClock, yClock);
        }

        // If debug on display the scene file name in the top left
        if(config.GetCfgItems().cfgDebug != 0)
        {
          Dotmap dmpFilename ;

          fontSystem.DmpFromString(dmpFilename, fileScene.name());
          dmpFilename.ClearMask();
          frame.DotBlt(dmpFilename, 0, 0, dmpFilename.GetWidth(), dmpFilename.GetHeight(), 0, 0);          
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
void Boot()
{
  DmdFrame frame;
  Dotmap dmpBootMsg;
  char bootMsg[16 + 1];
  
  // Show the version number of the firmware
  sprintf(bootMsg, "DOTCLK V%s", VERSION);
  fontSystem.DmpFromString(dmpBootMsg, bootMsg);
  frame.DotBlt(dmpBootMsg, 0, 0, dmpBootMsg.GetWidth(), dmpBootMsg.GetHeight(), (128 - dmpBootMsg.GetWidth())/2, (32 - dmpBootMsg.GetHeight())/2);

  // Update the DMD
  dmd.WaitSync();
  dmd.SetFrame(frame);

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

  // RTC Compensation
  // adjust is the amount of crystal error to compensate, 1 = 0.1192 ppm
  // For example, adjust = -100 is slows the clock by 11.92 ppm
  Teensy3Clock.compensate(0);

  // Init the SD Card
  InitSD();
  
  // Init the clock font
  InitClockFont();

  // Pause before returning
  delay(2000);
}

//-----------------
// Function: InitSD
//-----------------
void InitSD()
{
  if(dirScenes)
  {
    dirScenes.close();
  }
  
  // Connect to SD Card
  SD.begin(BUILTIN_SDCARD);

  // Open the 'Scenes' directory
  dirScenes = SD.open("/Scenes");
}

//------------------------
// Function: InitClockFont
//------------------------
void InitClockFont()
{
  // Check the SD is available
  if(SD.exists("/Fonts"))
  {
    // Delete any previous user font loaded
    if(fontUser != NULL)
    {
      delete fontUser;
      fontUser = NULL;
    }

    // Do we need to find the font?
    if(strcmp(config.GetCfgItems().cfgClockFont, "STANDARD") != 0)
    {
      // Open the 'Fonts' directory
      File dirFonts = SD.open("/Fonts");    
      if(dirFonts)
      {
        do
        {
          // Open each font file
          File fileFont = dirFonts.openNextFile();
          if(fileFont)
          {
            FONTNAME fontName ;

            // Get the font name and compare against the user selected one in config
            Font::GetFontName(fileFont, fontName);
            if(strcmp(config.GetCfgItems().cfgClockFont, fontName) == 0)
            {
              // Found our font, now load it for use
              fontUser = new Font();

              // Load the font
              fontUser->Create(fileFont);
              break;
            }

            fileFont.close();
          }
          else
          {
            // Problem reading a font file so quit the loop
            break;
          }
        }
        while(true);

        if(fontUser != NULL)
        {
          // Use the user font requested
          fontClock = fontUser;
        }
        else
        {
          // Configured font entry not found - Default to STANDARD
          fontClock = &fontStandard;
        }

        dirFonts.close();
      }
      else
      {
        // No Fonts directory found - Default to STANDARD
        fontClock = &fontStandard;
      }
    }
    else
    {
      // Standard font is in use - this is a built in font
      fontClock = &fontStandard;
    }
  }
  else
  {
    // No SD Card found - Default to STANDARD
    fontClock = &fontStandard;
  }
}

//----------------------
// Function: InitDmdType
//----------------------
void InitDmdType()
{
  ConfigItems cfgItems = config.GetCfgItems();

  if(btnMenu.ReadRaw() == Button::On)
  {
    cfgItems.cfgDmdType = 0;
    config.SetCfgItems(cfgItems);
  }
  else
  if(btnMinus.ReadRaw() == Button::On)
  {
    cfgItems.cfgDmdType = 1;
    config.SetCfgItems(cfgItems);
  }

  // Wait for all buttons to be released
  while(btnMenu.ReadRaw() == Button::On ||
    btnMinus.ReadRaw() == Button::On ||
    btnPlus.ReadRaw() == Button::On ||
    btnEnter.ReadRaw() == Button::On) ;
}

//-------------------------
// Function: getTeensy3Time
//-------------------------
time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

