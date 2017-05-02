// Library Includes
#include <SD.h>
#include <SPI.h>
#include <TimeLib.h>

// Local Includes
#include "Globals.h"

#include "Font.h"
#include "Scene.h"
#include "Setup.h"
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
  modeSleep = 4,
};

typedef char FILENAME[8+1+3+1];

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
const int pinGND[] = { 16, 17, 18, 22, 23 } ;

// Fonts
Font fontStandard;
Font *fontUser = NULL;
Font *fontClock;

// Scene files list
FILENAME *sceneNames = NULL;
uint16_t cntScenes = 0;
uint16_t curScene = 0;

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

  // Set button mapping from config value
  SetBtnMapping(config.GetCfgItems().cfgBtnMap);

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
  static int mode = modeClock;
  
  time_t tNow = NowDST();
  time_t tWake = config.GetCfgItems().cfgWakeTime;

  // Reset the forceWake
  if(forceWake && hour(tNow) == hour(tWake) && minute(tNow) == minute(tWake))
  {
    forceWake = false;
  }
    
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
        time_t tSleep = config.GetCfgItems().cfgSleepTime;
        
        if(!forceWake &&  // Haven't been forcibly woken
            !(hour(tWake) == hour(tSleep) && minute(tWake) == minute(tSleep)) && // Wake and Sleep times aren't the same
            (hour(tNow) == hour(tSleep) && minute(tNow) == minute(tSleep))) // Time to sleep
        {
          // From Clock mode to Sleep mode
          mode = modeSleep;
          dmd.Stop();
        }
        else
        {
          // Clock mode
          doClock();
        }
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

    case modeSleep:
    {
      time_t tNow = NowDST();
      time_t tWake = config.GetCfgItems().cfgWakeTime;
        
      if(btnMenu.Read() == Button::Rising ||
          btnPlus.Read() == Button::Rising ||
          btnMinus.Read() == Button::Rising ||
          btnEnter.Read() == Button::Rising)
      {
        // Any button forces wake up
        forceWake = true;
      }
      
      if(forceWake || (hour(tNow) == hour(tWake) && minute(tNow) == minute(tWake)))
      {
        // From Sleep mode to Clock mode
        mode = modeClock;
        dmd.Start();
      }
      
      break;
    }

    default:
      // Default position is to Clock mode
      mode = modeClock;
      break;
  }
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

  if(cntScenes == 0)
  {
    // Scenes dir not open, try to init the SD Card
    if(InitSD())
    {
      // SD Card now available - reboot
      CpuRestart();
    }
  }

  // Scenes to open and scene file not open yet?
  if(cntScenes > 0 && !fileScene)
  {
    char pathScene[255 + 1];
    uint16_t divider ;

    // Determine when and if the Brand scene should be shown
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

    if(divider == 0 || curScene % divider > 0 || !SD.exists("/Scenes/brand.scn"))
    {
      // Open the next scene file
      sprintf(pathScene, "/Scenes/%s", sceneNames[curScene % cntScenes]);
    }
    else
    {
      // Use the brand scene
      strcpy(pathScene, "/Scenes/brand.scn");
    }
    
    if(SD.exists(pathScene))
    {
      fileScene = SD.open(pathScene);
    }

    if(fileScene)
    {
      // Creste the scene object from the scene file
      if(!scene.Create(fileScene))
      {
        // SD Card not inserted
        cntScenes = 0;
      }
    }
    else
    {
      // SD Card not inserted
      cntScenes = 0;
    }
    
    // Keep track of the scene count
    curScene++;
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

  if(cntScenes == 0 || millisNow - millisSceneStart < (uint16_t)cfgClockDelay)
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
      // Get the next scene frame?
      if(millisSceneFrameDelay == 0 || (millisNow - millisSceneFrameDelay) > scene.GetFrameDelay())
      {
        // At the end of the scene?
        if(!scene.Eof())
        {
          // First frame or next frame
          scene.NextFrame(fileScene);
          millisSceneFrameDelay = millisNow;
        }
        else
        {
          // Finished the scene close it
          fileScene.close();
        }
      }

      // Still open after next frame processing?
      if(fileScene)
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
            yClock = (31 - dmpClock.GetHeight()) / 2;
            break;

          case Scene::ClockStyle1:
            clock[5] = '\0'; // Remove am/pm
            fontMenu.DmpFromString(dmpClock, clock, blanking);
            xClock = (42 - dmpClock.GetWidth()) / 2;
            yClock = (31 - dmpClock.GetHeight()) / 2;
            break;

          case Scene::ClockStyle2:
            clock[5] = '\0'; // Remove am/pm
            fontMenu.DmpFromString(dmpClock, clock, blanking);
            xClock = ((42 - dmpClock.GetWidth()) / 2) + 43;
            yClock = (31 - dmpClock.GetHeight()) / 2;
            break;

          case Scene::ClockStyle3:
            clock[5] = '\0'; // Remove am/pm
            fontMenu.DmpFromString(dmpClock, clock, blanking);
            xClock = ((42 - dmpClock.GetWidth()) / 2) + 87;
            yClock = (31 - dmpClock.GetHeight()) / 2;
            break;

        case Scene::ClockStyle4:
            clock[5] = '\0'; // Remove am/pm
            fontMenu.DmpFromString(dmpClock, clock, blanking);
            xClock = (64 - dmpClock.GetWidth()) / 2;
            yClock = (31 - dmpClock.GetHeight()) / 2;
            break;

          case Scene::ClockStyle5:
            clock[5] = '\0'; // Remove am/pm
            fontMenu.DmpFromString(dmpClock, clock, blanking);
            xClock = ((64 - dmpClock.GetWidth()) / 2) + 64;
            yClock = (31 - dmpClock.GetHeight()) / 2;
            break;

        case Scene::ClockStyle6:
            clock[5] = '\0'; // Remove am/pm
            fontMenu.DmpFromString(dmpClock, clock, blanking);
            xClock = scene.GetCustomX() - (dmpClock.GetWidth() / 2);
            yClock = scene.GetCustomY();
            break;
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

        // If debug on, display the scene file name in the top left
        if(config.GetCfgItems().cfgDebug != 0)
        {
          Dotmap dmpFilename ;
          FILENAME sceneName ;
          char *dot ;

          // Extract file name and truncate at fullstop
          strcpy(sceneName, fileScene.name());
          dot = strstr(sceneName, ".");
          if(dot != NULL)
          {
            *dot = '\0';
          }

          fontSystem.DmpFromString(dmpFilename, sceneName);
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
          millisSceneStart = millisNow;        
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
  unsigned long timeBootStart = millis();
  unsigned long timeBootDurn ;
  
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
  if(InitSD())
  {  
    // Init the scene file name list
    InitScenes();
  }

  // Init the clock font - SD card is optional as STANDARD font can be used
  InitClockFont();

  // Pause at least 2 seconds
  timeBootDurn = millis() - timeBootStart;
  if(timeBootDurn < 2000)
  {
    // Pause before returning
    delay(2000 - timeBootDurn);
  }
}

//-----------------
// Function: InitSD
//-----------------
bool InitSD()
{
  // Connect to SD Card
  SD.begin(BUILTIN_SDCARD);

  // Return whether we can see the Scenes directory
  return SD.exists("/Scenes") ? true : false;
}

//---------------------
// Function: InitScenes
//---------------------
void InitScenes()
{
  File dirScenes = SD.open("/Scenes");
  
  // Scene directory exists?
  if(dirScenes)
  {
    // Previous list exists?
    if(sceneNames != NULL)
    {
      // Free it
      free(sceneNames);

      cntScenes = 0;
      sceneNames = NULL;
    }

    // Rewind the directory
    dirScenes.rewindDirectory();

    // Loop on each scene file
    while(true)
    {
      File fileScene;
  
      // Open the next scene file
      fileScene = dirScenes.openNextFile();
      if(fileScene)
      {
        // Don't want to store the branding scene
        if(strcmp(fileScene.name(), "BRAND.SCN") != 0)
        {
          if(sceneNames == NULL)
          {
            // First time, malloc
            cntScenes = 1;        
            sceneNames = (FILENAME *)malloc(sizeof(FILENAME));
          }
          else
          {
            // Subsequent time, realloc
            cntScenes++;
            sceneNames = (FILENAME *)realloc(sceneNames, sizeof(FILENAME) * (cntScenes));
          }

          // Copy the file name into the ever expanding array
          strcpy(sceneNames[cntScenes - 1], fileScene.name());
        }

        // Close the file
        fileScene.close();
      }
      else
      {
        // End of directory
        break;
      }
    }

    // Close the directory
    dirScenes.close();
  }
}

//------------------------
// Function: InitClockFont
//------------------------
void InitClockFont()
{
  // Check the SD is available with Fonts directory
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

