// Library Includes
#include <SdFat.h>
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
// HUB08
#ifdef HUB08
const int pinEN = 19 ; // B2
const int pinR1 = 20 ; // D5
const int pinR2 = 21 ; // D6
const int pinG1 = 17 ; // B1
const int pinG2 = 18 ; // B3
const int pinB1 = 22 ;
const int pinB2 = 23 ;
const int pinLA = 7 ;  // D2
const int pinLB = 6 ;  // D4
const int pinLC = 5 ;  // D7
const int pinLD = 4 ;  // A13
const int pinLT = 1 ;  // B17
const int pinSK = 0 ;  // B16
const int pinLED = 13;
const int pinGND[] = { 2, 3, 16, 17, 18, 22, 23 } ;
#endif

#ifdef HUB75
// HUB75
const int pinEN = 23; // A9
const int pinR1 = 16; // A2
const int pinR2 = 18; // A4
const int pinG1 = 17;  // A3
const int pinG2 = 19;  // A5
const int pinB1 = 7;  // 
const int pinB2 = 5;  // 
const int pinLA = 20;  // A6
const int pinLB = 3;  // 
const int pinLC = 21;  // A7 
const int pinLD = 2;  // 
const int pinLT = 1;  // 
const int pinSK = 22;  // A8
const int pinLED = 13;
const int pinGND[] = { 6, 4, 0 } ;
#endif

// Fonts
Font fontStandard;
Font *fontUser = NULL;
Font *fontClock;

// Scene files list
FILENAME *sceneNames = NULL;
uint16_t cntScenes = 0;
uint16_t curScene = 0;

// SD Card
SdFatSdio SD;

//----------------
// Function: setup
//----------------
void setup()
{  
  Dotmap dmpFont;

  // Set low voltage level to HIGH
  PMC_LVDSC1 |= PMC_LVDSC1_LVDV(1);

  // Serial debug
  Serial.begin(9600);
  //while(!Serial);

  // Randomize the generator
  randomSeed(analogRead(32));

  // Set GND for unused pins
  for (int nGnd = 0; nGnd < (int)(sizeof(pinGND) / sizeof(int)); nGnd++)
  {
    pinMode(pinGND[nGnd], OUTPUT_OPENDRAIN);
    digitalWrite(pinGND[nGnd], LOW);
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
  dmpFont.Create(351, 11);
  dmpFont.SetDotsFromRaw(MENUFontDots, sizeof(MENUFontDots));
  dmpFont.SetMaskFromRaw(MENUFontMask, sizeof(MENUFontMask));
  fontMenu.Create(44, dmpFont);
  fontMenu.SetCharInfoFromRaw(MENUFontCharInfo, sizeof(MENUFontCharInfo));

  // Standard Font
  dmpFont.Create(202, 21);
  dmpFont.SetDotsFromRaw(STANDARDFontDots, sizeof(STANDARDFontDots));
  dmpFont.SetMaskFromRaw(STANDARDFontMask, sizeof(STANDARDFontMask));
  fontStandard.Create(15, dmpFont);
  fontStandard.SetCharInfoFromRaw(STANDARDFontCharInfo, sizeof(STANDARDFontCharInfo));

  // Initialise the DMD
  dmd.SetDmdType(config.GetCfgItems().cfgDmdType);
  dmd.Initialise(pinEN, pinR1, pinR2, pinG1, pinG2, pinB1, pinB2, pinLA, pinLB, pinLC, pinLD, pinLT, pinSK);

  // Set DMD brightness from config
  dmd.SetBrightness(config.GetCfgItems().cfgBrightness);

  // Set DMD colour from config
  dmd.SetColour(config.GetCfgItems().cfgDotColour);

  // Start the DMD
  dmd.Start();

  // Boot
  Boot();

  // Display test if 'Enter' button pressed
  DisplayTest();
  
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
  static SdFile fileScene ;
  static Scene scene;
  static unsigned long millisSceneStart = millis();
  static unsigned long millisSceneFrameDelay = 0;
  static uint16_t curScene = 0;
  static unsigned long sceneStart = 0, sceneDuration = 0;
  static unsigned long cfgClockDelayValue = 0;
  
  DmdFrame frame;
  Dotmap dmpFrame ;
  Dotmap dmpClock;
  unsigned long millisNow = millis();
  const char *blanking;
  char clock[15 + 1];
  time_t timeNow = NowDST();
  ConfigItems cfgItems = config.GetCfgItems();
  
  if(cntScenes == 0)
  {
    // Scenes dir not open, try to init the SD Card
    if(InitSD())
    {
      // SD Card now available - reboot
      CpuRestart();
    }
  }
  else if(!fileScene.isOpen())
  {
    char pathScene[255 + 1];
    int cfgShowBrandValue = config.GetShowBrandValue();

    cfgClockDelayValue = config.GetClockDelayValue();

    if(cfgShowBrandValue == 0 || curScene % cfgShowBrandValue > 0 || !SD.exists("/Scenes/brand.scn"))
    {
      // Open the next scene file
      if(cfgItems.cfgDebug  == 0)
      {
        // Not debug, so play random scene
        sprintf(pathScene, "/Scenes/%s", sceneNames[random(cntScenes)]);
      }
      else
      {
        // In debug mode, play in alphabetical order
        sprintf(pathScene, "/Scenes/%s", sceneNames[curScene % cntScenes]);
      }
    }
    else
    {
      // Use the brand scene
     strcpy(pathScene, "/Scenes/brand.scn");
    }

    // Open the scene file
    if(fileScene.open(pathScene, O_RDONLY))
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

  // Generate the clock string and blanking as this is always used whether hidden behind a scene frame or part of it through the mask or on top
  // Second beat
  if((((millisNow) / 500) % 2) == 0)
  {
    // Show the second dots
    blanking = "       ";
    digitalWriteFast(pinLED, HIGH);
  }
  else
  {
    // Hide the second dots
    blanking = "  -    ";
    digitalWriteFast(pinLED, LOW);
  }

  // Create the clock string
  switch(cfgItems.cfgTimeFormat)
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
  
  if(cntScenes == 0 || millisNow - millisSceneStart < cfgClockDelayValue)
  {
    // Generate clock dotmap
    fontClock->DmpFromString(dmpClock, clock, blanking);

    // Only showing the clock between animations
    frame.Clear();
    frame.DotBlt(dmpClock, 0, 0, dmpClock.GetWidth(), dmpClock.GetHeight(), (127 - dmpClock.GetWidth()) / 2, (31 - dmpClock.GetHeight())/2);

    if(cfgItems.cfgDebug != 0)
    {
      char textDurn[63 + 1];
      Dotmap dmpDuration ;
      
      sprintf(textDurn, "%lu", sceneDuration);
      fontSystem.DmpFromString(dmpDuration, textDurn);
      dmpDuration.ClearMask();
      frame.DotBlt(dmpDuration, 0, 0, dmpDuration.GetWidth(), dmpDuration.GetHeight(), 0, 0);         
    }
  }
  else
  {  
    if(sceneStart == 0)
    {
      sceneStart = millis();
    }
    
    // Get the next scene frame?
    if(millisSceneFrameDelay == 0 || millisNow - millisSceneFrameDelay > scene.GetFrameDelay())
    {
      // At the end of the scene?
      if(!scene.Eof())
      {
        if(millisSceneFrameDelay == 0)
        {
          // First frame, start timing from now
          millisSceneFrameDelay = millisNow;
        }
        else
        {
          // Add on the frame delay so  that we keep a good track of time
          millisSceneFrameDelay += scene.GetFrameDelay();
        }

        // First frame or next frame
        scene.NextFrame(fileScene);
      }
      else
      {
        // Finished the scene close it
        fileScene.close();
      }
    }

    // Still open after next frame processing?
    if(fileScene.isOpen())
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

        case Scene::ClockStyleCustom:
          clock[5] = '\0'; // Remove am/pm
          fontMenu.DmpFromString(dmpClock, clock, blanking);
          xClock = scene.GetCustomX() - (dmpClock.GetWidth() / 2);
          yClock = scene.GetCustomY() - (dmpClock.GetHeight() / 2);
          break;
      }

      // Get the frame dotmap
      dmpFrame = scene.GetFrameDotmap();
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
      if(cfgItems.cfgDebug != 0 && fileScene.isOpen())
      {
        Dotmap dmpFilename ;
        FILENAME sceneName ;
        char *dot ;

        // Extract file name and truncate at fullstop
        fileScene.getName(sceneName, sizeof(sceneName));
        dot = strstr(sceneName, ".");
        if(dot != NULL)
        {
          *dot = '\0';
        }
        
        fontSystem.DmpFromString(dmpFilename, sceneName);
        dmpFilename.ClearMask();
        frame.DotBlt(dmpFilename, 0, 0, dmpFilename.GetWidth(), dmpFilename.GetHeight(), 0, 0);          
      }
    }
    else
    {
      // Finished the scene, show the clock again
      millisSceneStart = millisNow;
      millisSceneFrameDelay = 0;

      // Reset frame duration timer
      sceneDuration = millis() - sceneStart;
      sceneStart = 0;
        
      // Generate clock dotmap
      fontClock->DmpFromString(dmpClock, clock, blanking);
  
      // Only showing the clock between animations
      frame.Clear();
      frame.DotBlt(dmpClock, 0, 0, dmpClock.GetWidth(), dmpClock.GetHeight(), (127 - dmpClock.GetWidth()) / 2, (31 - dmpClock.GetHeight())/2);
    }
  }

  // Update the DMD
  dmd.SetFrame(frame);
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
  frame.DotBlt(dmpBootMsg, 0, 0, dmpBootMsg.GetWidth(), dmpBootMsg.GetHeight(), (128 - dmpBootMsg.GetWidth())/2, (16 - dmpBootMsg.GetHeight()) - 1);

  // Show the version number of the firmware
  ConfigItems cfgItems = config.GetCfgItems();
  sprintf(bootMsg, "SCREEN TYPE:%d", cfgItems.cfgDmdType);
  fontSystem.DmpFromString(dmpBootMsg, bootMsg);
  frame.DotBlt(dmpBootMsg, 0, 0, dmpBootMsg.GetWidth(), dmpBootMsg.GetHeight(), (128 - dmpBootMsg.GetWidth())/2, 16 + 1);

  // Update the DMD
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
  Teensy3Clock.compensate(config.GetCfgItems().cfgTimeCorrect);

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
  // Return whether we can see the Scenes directory
  return SD.begin() && SD.exists("/Scenes");
}

//---------------------
// Function: InitScenes
//---------------------
void InitScenes()
{
  SdFile dirScenes;
  SdFile file ;

  // Scene directory exists?
  if(dirScenes.open("/Scenes", O_RDONLY))
  {
    // Previous list exists?
    if(sceneNames != NULL)
    {
      // Free it
      free(sceneNames);

      cntScenes = 0;
      sceneNames = NULL;
    }

    // Loop on each scene file
    while(file.openNext(&dirScenes, O_RDONLY))
    {
        FILENAME filename ;

        if(file.getName(filename, sizeof(filename)))
        {
          // Don't want to store the branding scene
          if(strcmp(filename, "BRAND.SCN") != 0)
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
            strcpy(sceneNames[cntScenes - 1], filename);
        }
      }

      file.close();
    }

    // Sort the list alphabetically
    if(sceneNames != NULL && cntScenes > 0)
    {
      // Sort the list of filenames alphabetically
      qsort(sceneNames, cntScenes, sizeof(sceneNames[0]), (__compar_fn_t)strcmp);
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
      SdFile dirFonts;
      
      if(dirFonts.open("/Fonts", O_RDONLY))
      {
        SdFile fileFont;
        
        while(fileFont.openNext(&dirFonts, O_RDONLY))
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

  // Wait for buttons to be released
  while(btnMenu.ReadRaw() == Button::On ||
    btnMinus.ReadRaw() == Button::On) ;
}

//----------------------
// Function: DisplayTest
//----------------------
void DisplayTest()
{
  DmdFrame frame ;
    
  if(btnEnter.ReadRaw() == Button::On)
  {
    // Show test image - all dots on
    frame.Clear(0x0F);
    dmd.SetFrame(frame);

    // Wait for button to be released
    while(btnEnter.ReadRaw() == Button::On);

    // Wait for button to be pressed again
    while(btnEnter.Read() != Button::Rising);
  }
}

