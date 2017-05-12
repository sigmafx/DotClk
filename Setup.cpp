#include <Arduino.h>
#include <Time.h>

#include "Globals.h"

#include "Setup.h"
#include "Font.h"
#include "DmdFrame.h"
#include "Dotmap.h"
#include "Utils.h"

typedef void (*FEEDBACK)(int value);

struct Menu
{
  const char *menuTitle;
  int cntMenuItems;
  const char **menuItems ;
  const char *menuButtons[4];

  Menu(int cntMenuItems) { menuItems = new const char *[cntMenuItems]; this->cntMenuItems = cntMenuItems;}
  ~Menu() { if(cntMenuItems > 0) delete[] menuItems;}
};

// Function prototypes
static void PaintTitle(DmdFrame& frame, const char *titleText);
static void PaintButtons(DmdFrame& frame, const char *btnText[4]);
static int HandleStandard(DmdFrame& frame, Menu& menu, bool isInit, int& initValue, FEEDBACK feedback = NULL);
static void FeedbackDotColour(int value);
static bool HandleBrightness(DmdFrame& frame, bool isInit, int& initValue);
static int HandleSetTime(DmdFrame& frame, const char *title, bool tick, bool isInit, time_t& initValue);
static int HandleTimeCorrect(DmdFrame& frame, bool tick, bool isInit, int& initValue);

// Menu IDs and running order
enum
{
  MENU_SETTIME = 0,
  MENU_DST,
  MENU_TIMEFORMAT,
  MENU_TIMECORRECT,
  MENU_SLEEPTIME,
  MENU_WAKETIME,
  MENU_BRIGHTNESS,
  MENU_CLOCKDELAY,
  MENU_CLOCKFONT,
  MENU_DOTCOLOUR,
  MENU_BTNMAP,
  MENU_SHOWBRAND,
  MENU_DEBUG,
};

// Standard menu structs
struct MenuMainMenu : Menu
{
  MenuMainMenu() : Menu(13)
  {
    menuTitle = "MAIN MENU";
    menuItems[0] = "SET TIME";
    menuItems[1] = "DST";
    menuItems[2] = "TIME FORMAT";
    menuItems[3] = "TIME CORRECT";
    menuItems[4] = "SLEEP TIME";
    menuItems[5] = "WAKE TIME";
    menuItems[6] = "BRIGHTNESS";
    menuItems[7] = "CLOCK DELAY";
    menuItems[8] = "CLOCK FONT";
    menuItems[9] = "DOT COLOUR";
    menuItems[10] = "BUTTON MAPPING";
    menuItems[11] = "SHOW BRAND";
    menuItems[12] = "DEBUG";
    menuButtons[0] = "Exit";
    menuButtons[1] = "Prev";
    menuButtons[2] = "Next";
    menuButtons[3] = "Edit";
  }
};

struct MenuDST : Menu
{
  MenuDST() : Menu(2)
  {
    menuTitle = "DST";
    menuItems[0] = "OFF";
    menuItems[1] = "ON";
    menuButtons[0] = "Back";
    menuButtons[1] = "Prev";
    menuButtons[2] = "Next";
    menuButtons[3] = "Save";    
  }
};

struct MenuTimeFormat : Menu
{
  MenuTimeFormat() : Menu(3) 
  {
    menuTitle = "TIME FORMAT";
    menuItems[0] = "24 HOUR";
    menuItems[1] = "12 HOUR";
    menuItems[2] = "12H WITH AM/PM";
    menuButtons[0] = "Back";
    menuButtons[1] = "Prev";
    menuButtons[2] = "Next";
    menuButtons[3] = "Save";
  }
};

struct MenuClockDelay : Menu
{
  MenuClockDelay() : Menu(7) 
  {
    menuTitle = "CLOCK DELAY";
    menuItems[0] = "5 SECONDS";
    menuItems[1] = "10 SECONDS";
    menuItems[2] = "15 SECONDS";
    menuItems[3] = "30 SECONDS";
    menuItems[4] = "1 MINUTE";
    menuItems[5] = "2 MINUTES";
    menuItems[6] = "5 MINUTES";
    menuButtons[0] = "Back";
    menuButtons[1] = "Prev";
    menuButtons[2] = "Next";
    menuButtons[3] = "Save";
  }
};

struct MenuDotColour : Menu
{
  MenuDotColour() : Menu(7) 
  {
    menuTitle = "DOT COLOUR";
    menuItems[0] = "RED";
    menuItems[1] = "GREEN";
    menuItems[2] = "YELLOW";
    menuItems[3] = "BLUE";
    menuItems[4] = "MAGENTA";
    menuItems[5] = "CYAN";
    menuItems[6] = "WHITE";
    menuButtons[0] = "Back";
    menuButtons[1] = "Prev";
    menuButtons[2] = "Next";
    menuButtons[3] = "Save";
  }
};

struct MenuBtnMap : Menu
{
  MenuBtnMap() : Menu(2) 
  {
    menuTitle = "BUTTON MAPPING";
    menuItems[0] = "NORMAL";
    menuItems[1] = "REVERSE";
    menuButtons[0] = "Back";
    menuButtons[1] = "Prev";
    menuButtons[2] = "Next";
    menuButtons[3] = "Save";
  }
};

struct MenuShowBrand : Menu
{
  MenuShowBrand() : Menu(5) 
  {
    menuTitle = "SHOW BRAND";
    menuItems[0] = "NEVER";
    menuItems[1] = "EVERY 2";
    menuItems[2] = "EVERY 5";
    menuItems[3] = "EVERY 10";
    menuItems[4] = "EVERY 20";
    menuButtons[0] = "Back";
    menuButtons[1] = "Prev";
    menuButtons[2] = "Next";
    menuButtons[3] = "Save";
  }
};

struct MenuDebug : Menu
{
  MenuDebug() : Menu(2) 
  {
    menuTitle = "DEBUG";
    menuItems[0] = "OFF";
    menuItems[1] = "ON";
    menuButtons[0] = "Back";
    menuButtons[1] = "Prev";
    menuButtons[2] = "Next";
    menuButtons[3] = "Save";
  }
};

// Standard menu objects
MenuMainMenu menuMainMenu;
  MenuDST menuDST;
  MenuTimeFormat menuTimeFormat;
  MenuClockDelay menuClockDelay;
  Menu *menuClockFont = NULL;
  MenuDotColour menuDotColour;
  MenuBtnMap menuBtnMap;
  MenuShowBrand menuShowBrand;
  MenuDebug menuDebug;

FONTNAME *fontUserNames = NULL;

// Config Items
time_t DateTime ;
ConfigItems setItems;
int cfgClockFont;

//------------------
// Function: doSetup
//------------------
bool doSetup(bool isInit)
{
  static bool showMainMenu;
  static bool initMainMenu;
  static int idxSubMenu;

  DmdFrame frame ;
  bool ret = true;  

  // First time in, initialise
  if(isInit)
  {
    // Retrieve the config items as they set
    setItems = config.GetCfgItems();
    cfgClockFont = 0;

    // Generate the Clock Font menu
    if(menuClockFont != NULL)
    {
      // Delete the previous menu
      delete menuClockFont;
    }

    // Determine number of user fonts
    byte fontCount = Font::GetFontCount();

    // Allocate space for the names and read
    if(fontUserNames != NULL)
    {
      delete[] fontUserNames;
      fontUserNames = NULL;
    }
    fontUserNames = new FONTNAME[fontCount];

    File dirFonts = SD.open("/Fonts");
    if(dirFonts)
    {
      int fontCur ;

      // Default to STANDARD until we find a matching font
      cfgClockFont = 0;

      for(fontCur = 0; fontCur < fontCount; fontCur++)
      {
        File fileFont = dirFonts.openNextFile();
        if(fileFont)
        {
          Font::GetFontName(fileFont, fontUserNames[fontCur]);
          fileFont.close();

          if(strcmp(setItems.cfgClockFont, fontUserNames[fontCur]) == 0)
          {
            cfgClockFont = fontCur + 1;
          }
        }
        else
        {
          // Problem opening the font file
          fontCount = 0;
          cfgClockFont = 0;
          break;
        }
      }

      dirFonts.close();
    }
    else
    {
      // Couldn't open the Fonts directory
      fontCount = 0;
      cfgClockFont = 0;
    }
    
    // Allocate the new menu
    menuClockFont = new Menu(fontCount + 1);
    
    // Populate it
    menuClockFont->menuTitle = "CLOCK FONT";
    // STANDARD is always entry 0
    menuClockFont->menuItems[0] = "STANDARD";
    // Add any user fonts from entry 1 onwards
    for(int fontCur = 0; fontCur < fontCount; fontCur++)
    {
      menuClockFont->menuItems[fontCur + 1] = fontUserNames[fontCur];
    }
    menuClockFont->menuButtons[0] = "Back";
    menuClockFont->menuButtons[1] = "Prev";
    menuClockFont->menuButtons[2] = "Next";
    menuClockFont->menuButtons[3] = "Save";

    // Init sub menu
    idxSubMenu = MENU_SETTIME;

    showMainMenu = true;
    initMainMenu = true;
  }

  if(showMainMenu)
  {
    int menuRet = HandleStandard(frame, menuMainMenu, initMainMenu, idxSubMenu);
    initMainMenu = false;
    
    if(menuRet != 0)
    {
      if(menuRet == -1)
      {
        // No menu item selected, quit
        ret = false;
      }
      else
      {
        initMainMenu = true;
        showMainMenu = false;

        // Menu item selected
        switch(idxSubMenu)
        {
          case MENU_SETTIME: // Set Time
            DateTime = NowDST();
            HandleSetTime(frame, menuMainMenu.menuItems[idxSubMenu], false, true, DateTime);
            break;

          case MENU_DST: // DST
            HandleStandard(frame, menuDST, true, setItems.cfgDST);
            break ;

          case MENU_TIMEFORMAT: // Time Format
            HandleStandard(frame, menuTimeFormat, true, setItems.cfgTimeFormat);
            break ;
          
          case MENU_TIMECORRECT: // Time Correct
            HandleTimeCorrect(frame, false, true, setItems.cfgTimeCorrect);
            break ;

          case MENU_SLEEPTIME: // Sleep Time
            HandleSetTime(frame, menuMainMenu.menuItems[idxSubMenu], false, true, setItems.cfgSleepTime);
            break;

          case MENU_WAKETIME: // Wake Time
            HandleSetTime(frame, menuMainMenu.menuItems[idxSubMenu], false, true, setItems.cfgWakeTime);
            break;

          case MENU_BRIGHTNESS: // Brightness
            HandleBrightness(frame, true, setItems.cfgBrightness);
            break ;

          case MENU_CLOCKDELAY: // Clock Delay
            HandleStandard(frame, menuClockDelay, true, setItems.cfgClockDelay);
            break ;

          case MENU_CLOCKFONT: // Clock Font
            HandleStandard(frame, *menuClockFont, true, cfgClockFont);
            break ;

          case MENU_DOTCOLOUR: // Dot Colour
            HandleStandard(frame, menuDotColour, true, setItems.cfgDotColour);
            break ;

          case MENU_BTNMAP: // Button Mapping
            HandleStandard(frame, menuBtnMap, true, setItems.cfgBtnMap);
            break ;

          case MENU_SHOWBRAND: // Show Brand
            HandleStandard(frame, menuShowBrand, true, setItems.cfgShowBrand);
            break ;

          case MENU_DEBUG: // Debug
            HandleStandard(frame, menuDebug, true, setItems.cfgDebug);
            break ;

          default: // ERROR
            showMainMenu = true;
            break ;
        }
      }
    }
  }
  else
  {
    switch(idxSubMenu)
    {
      case MENU_SETTIME: // Set Time
      {
        int menuRet = HandleSetTime(frame, menuMainMenu.menuItems[idxSubMenu], (millis()/500)%2, false, DateTime);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
            if(setItems.cfgDST == Config::CFG_DST_ON)
            {
              // DST On so rewind time by an hour
              DateTime -= SECS_PER_HOUR;
            }
            
            // Set the time
            setTime(DateTime);
            Teensy3Clock.set(DateTime);
          }
          
          showMainMenu = true;
        }
        break;
      }
        
      case MENU_DST: // Daylight Saving Time
      {
        int menuRet = HandleStandard(frame, menuDST, false, setItems.cfgDST);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
            config.SetCfgItems(setItems);
          }
          
          showMainMenu = true;
        }
        break ;
      }

      case MENU_TIMEFORMAT: // Time Format
      {
        int menuRet = HandleStandard(frame, menuTimeFormat, false, setItems.cfgTimeFormat);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
            config.SetCfgItems(setItems);
          }
          showMainMenu = true;
        }
        break ;        
      }
        
      case MENU_TIMECORRECT: // Time Correct
      {
        int menuRet = HandleTimeCorrect(frame, (millis()/500)%2, false, setItems.cfgTimeCorrect);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
            config.SetCfgItems(setItems);
            Teensy3Clock.compensate(setItems.cfgTimeCorrect);
          }
          showMainMenu = true;
        }
        break ;        
      }
        
      case MENU_SLEEPTIME: // Sleep Time
      {
        int menuRet = HandleSetTime(frame, menuMainMenu.menuItems[idxSubMenu], (millis()/500)%2, false, setItems.cfgSleepTime);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
            config.SetCfgItems(setItems);
            forceWake = false;
          }
          
          showMainMenu = true;
        }
        break;
      }

      case MENU_WAKETIME: // Wake Time
      {
        int menuRet = HandleSetTime(frame, menuMainMenu.menuItems[idxSubMenu], (millis()/500)%2, false, setItems.cfgWakeTime);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
            config.SetCfgItems(setItems);
            forceWake = false;
          }
          
          showMainMenu = true;
        }
        break;
      }

      case MENU_BRIGHTNESS: // Brightness
        if(!HandleBrightness(frame, false, setItems.cfgBrightness))
        {
          config.SetCfgItems(setItems);
          showMainMenu = true;
        }
        break;
      
      case MENU_CLOCKDELAY: // Clock Delay
      {
        int menuRet = HandleStandard(frame, menuClockDelay, false, setItems.cfgClockDelay);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
            config.SetCfgItems(setItems);
          }
          showMainMenu = true;
        }
        break;
      }
      
      case MENU_CLOCKFONT: // Clock Font
      {
        int menuRet = HandleStandard(frame, *menuClockFont, false, cfgClockFont);
        if(menuRet != 0)
        {
          if(menuRet == 1)
          {
            strcpy(setItems.cfgClockFont, menuClockFont->menuItems[cfgClockFont]);
            config.SetCfgItems(setItems);
          }
          showMainMenu = true;
        }
        break ;
      }

      case MENU_DOTCOLOUR: // Dot Colour
      {        
        int menuRet = HandleStandard(frame, menuDotColour, false, setItems.cfgDotColour, FeedbackDotColour);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
            config.SetCfgItems(setItems);
          }
          else
          {
            // Reset colour back on 'Back' button
            dmd.SetColour(setItems.cfgDotColour);
          }
          
          showMainMenu = true;
        }
        break;
      }
      
      case MENU_BTNMAP: // Button Mapping
      {
        int menuRet = HandleStandard(frame, menuBtnMap, false, setItems.cfgBtnMap);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
            SetBtnMapping(setItems.cfgBtnMap);
            config.SetCfgItems(setItems);
          }          
          showMainMenu = true;
        }
        break;
      }

      case MENU_SHOWBRAND: // Show Brand
      {
        int menuRet = HandleStandard(frame, menuShowBrand, false, setItems.cfgShowBrand);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
            config.SetCfgItems(setItems);
          }          
          showMainMenu = true;
        }
        break;
      }

      case MENU_DEBUG: // Debug
      {        
        int menuRet = HandleStandard(frame, menuDebug, false, setItems.cfgDebug);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
            config.SetCfgItems(setItems);
          }          
          showMainMenu = true;
        }
        break;
      }

      default:
        showMainMenu = true;
        break;
    }
  }
  
  // Update the DMD
  dmd.WaitSync();
  dmd.SetFrame(frame);

  return ret;
}

//---------------------
// Function: PaintTitle
//---------------------
static void PaintTitle(DmdFrame& frame, const char *titleText)
{
  Dotmap dmpTitle;
  char clock[5 + 1];
  time_t time = NowDST();
  
  // Title Text
  dmpTitle.Create(128, 9);
  dmpTitle.Fill(0x01);
  frame.DotBlt(dmpTitle, 0, 0, dmpTitle.GetWidth(), dmpTitle.GetHeight(), 0, 0);
  fontSystem.DmpFromString(dmpTitle, titleText);
  frame.DotBlt(dmpTitle, 0, 0, dmpTitle.GetWidth(), dmpTitle.GetHeight(), 1, 1);
  // Clock
  sprintf(clock, "%02d:%02d", hour(time), minute(time));
  fontSystem.DmpFromString(dmpTitle, clock);
  frame.DotBlt(dmpTitle, 0, 0, dmpTitle.GetWidth(), dmpTitle.GetHeight(), 128 - dmpTitle.GetWidth(), 1);  
}

//-----------------------
// Function: PaintButtons
//-----------------------
static void PaintButtons(DmdFrame& frame, const char *btnText[4])
{
  Dotmap dmpButtons;

  // Background
  dmpButtons.Create(28, 9);
  dmpButtons.Fill(0x01);
  frame.DotBlt(dmpButtons, 0, 0, dmpButtons.GetWidth(), dmpButtons.GetHeight(), 2, 23);
  frame.DotBlt(dmpButtons, 0, 0, dmpButtons.GetWidth(), dmpButtons.GetHeight(), 34, 23);
  frame.DotBlt(dmpButtons, 0, 0, dmpButtons.GetWidth(), dmpButtons.GetHeight(), 66, 23);
  frame.DotBlt(dmpButtons, 0, 0, dmpButtons.GetWidth(), dmpButtons.GetHeight(), 98, 23);
  
  // Button Text
  fontSystem.DmpFromString(dmpButtons, btnText[0]);
  frame.DotBlt(dmpButtons, 0, 0, dmpButtons.GetWidth(), dmpButtons.GetHeight(), 2 + ((28 - dmpButtons.GetWidth()) / 2), 24);
  fontSystem.DmpFromString(dmpButtons, btnText[1]);
  frame.DotBlt(dmpButtons, 0, 0, dmpButtons.GetWidth(), dmpButtons.GetHeight(), 34 + ((28 - dmpButtons.GetWidth()) / 2), 24);
  fontSystem.DmpFromString(dmpButtons, btnText[2]);
  frame.DotBlt(dmpButtons, 0, 0, dmpButtons.GetWidth(), dmpButtons.GetHeight(), 66 + ((28 - dmpButtons.GetWidth()) / 2), 24);
  fontSystem.DmpFromString(dmpButtons, btnText[3]);
  frame.DotBlt(dmpButtons, 0, 0, dmpButtons.GetWidth(), dmpButtons.GetHeight(), 98 + ((28 - dmpButtons.GetWidth()) / 2), 24);
}

//-------------------------
// Function: HandleStandard
//-------------------------
static int HandleStandard(DmdFrame& frame, Menu& menu, bool isInit, int& initValue, FEEDBACK feedback)
{
  static int value ;

  Dotmap dmpSetup;
  int ret = 0;

  int btnMenuRead = btnMenu.Read();
  int btnPlusRead = btnPlus.Read();
  int btnMinusRead = btnMinus.Read();
  int btnEnterRead = btnEnter.Read();

  if(isInit)
  {
    value = initValue;
  }

  if(btnMenuRead == Button::Rising)
  {
    // Back
    ret = -1;
  }

  if(btnPlusRead == Button::Rising)
  {
    // Next
    if(value < (menu.cntMenuItems - 1))
    {
      value++;
      if(feedback != NULL)
      {
        feedback(value);
      }
    }
  }

  if(btnMinusRead == Button::Rising)
  {
    // Prev
    if(value > 0)
    {
      value-- ;
      if(feedback != NULL)
      {
        feedback(value);
      }
    }
  }

  if(btnEnterRead == Button::Rising)
  { 
    // Save
    initValue = value;    
    ret = 1;
  }

  // Title
  PaintTitle(frame, menu.menuTitle);
  
  // Body
  if(value > 0)
  {
    fontMenu.DmpFromString(dmpSetup, "<");
    frame.DotBlt(dmpSetup, 0, 0, dmpSetup.GetWidth(), dmpSetup.GetHeight(), 0, 11);
  }
  
  if(value < (menu.cntMenuItems - 1))
  {
    fontMenu.DmpFromString(dmpSetup, ">");
    frame.DotBlt(dmpSetup, 0, 0, dmpSetup.GetWidth(), dmpSetup.GetHeight(), 128 - dmpSetup.GetWidth(), 11);
  }
  
  // Menu Item
  fontMenu.DmpFromString(dmpSetup, menu.menuItems[value]);
  frame.DotBlt(dmpSetup, 0, 0, dmpSetup.GetWidth(), dmpSetup.GetHeight(), (128 - dmpSetup.GetWidth()) / 2, 11);

  // Buttons
  PaintButtons(frame, menu.menuButtons);
  
  return ret;
}

//----------------------------
// Function: FeedbackDotColour
//----------------------------
static void FeedbackDotColour(int value)
{
  dmd.SetColour(value);  
}

//---------------------------
// Function: HandleBrightness
//---------------------------
static bool HandleBrightness(DmdFrame& frame, bool isInit, int& initValue)
{
  static int value ;
  bool ret = true;
  Dotmap dmpBrightness ;
  const char *btnText[] = {"Back", "Down", "Up", "Save", };
  
  int btnMenuRead = btnMenu.Read();
  int btnPlusRead = btnPlus.Read();
  int btnMinusRead = btnMinus.Read();
  int btnEnterRead = btnEnter.Read();

  if(isInit)
  {
    value = initValue;
  }
  
  if(btnMenuRead == Button::Rising)
  {
    // Back
    dmd.SetBrightness(initValue);
    ret = false;
  }

  if(btnPlusRead == Button::Rising || btnPlusRead == Button::Hold)
  {
    // Up
    if(value < 63)
    {
      value++;
      dmd.SetBrightness(value);
    }
  }
        
  if(btnMinusRead == Button::Rising || btnMinusRead == Button::Hold)
  {
    // Down
    if(value > 0)
    {
      value--;
      dmd.SetBrightness(value);
    }
  }
  
  if(btnEnterRead == Button::Rising)
  { 
    // Save
    initValue = value;
    ret = false;
  }

  // Title
  PaintTitle(frame, "BRIGHTNESS");

  // Brightness Bar Graph
  dmpBrightness.Create(68, 8);
  dmpBrightness.Fill(5);
  frame.DotBlt(dmpBrightness, 0, 0, dmpBrightness.GetWidth(), dmpBrightness.GetHeight(), 30, 12);

  dmpBrightness.Create(66, 6);
  dmpBrightness.Fill(0);
  frame.DotBlt(dmpBrightness, 0, 0, dmpBrightness.GetWidth(), dmpBrightness.GetHeight(), 31, 13);

  dmpBrightness.Create(64, 4);
  dmpBrightness.Fill(15);
  frame.DotBlt(dmpBrightness, 0, 0, (dmpBrightness.GetWidth()/64)*(value + 1), dmpBrightness.GetHeight(), 32, 14);

  // Buttons
  PaintButtons(frame, btnText);

  return ret;
}

//------------------------
// Function: HandleSetTime
//------------------------
static int HandleSetTime(DmdFrame& frame, const char *title, bool tick, bool isInit, time_t& initValue)
{
  static TimeElements value ;
  static int position ;
  
  int ret = 0;
  Dotmap dmpSetTime;
  const char *btnText[] = {"Back", "-", "+", "" };

  char setTimeStr[9 + 1] ;
  const char *blankingPos0, *blankingPos1 ;

  int btnMenuRead = btnMenu.Read();
  int btnPlusRead = btnPlus.Read();
  int btnMinusRead = btnMinus.Read();
  int btnEnterRead = btnEnter.Read();

  if(isInit)
  {
    breakTime(initValue, value);
    position = 0;
  }
  
  if(btnMenuRead == Button::Rising)
  {
    // Back
    ret = -1;
  }

  if(btnPlusRead == Button::Rising || btnPlusRead == Button::Hold)
  {
    // +
    if(position == 0)
    {
      // Hour
      value.Hour++ ;
      if (value.Hour > 23) value.Hour = 0;
    }
    else
    {
      value.Minute++;
      if (value.Minute > 59) value.Minute = 0;
    }
  }
        
  if(btnMinusRead == Button::Rising || btnMinusRead == Button::Hold)
  {
    // -
    if(position == 0)
    {
      // Hour
      value.Hour-- ;
      if (value.Hour > 23) value.Hour = 23;
    }
    else
    {
      value.Minute--;
      if (value.Minute > 59) value.Minute = 59;
    }
  }
  
  if(btnEnterRead == Button::Rising)
  {
    // Next / Save
    if(position == 0)
    {
      position = 1;
    }
    else
    {
      value.Second = 0;
      initValue = makeTime(value);
      ret = 1;
    }
  }

  // Title
  PaintTitle(frame, title);

  // Clock
  blankingPos0 = tick ? "-     -" : "      -";
  blankingPos1 = tick ? "-     -" : "-      ";
  
  sprintf(setTimeStr, ">%02d:%02d<", value.Hour, value.Minute);
  fontMenu.DmpFromString(dmpSetTime, setTimeStr, position == 0 ? blankingPos0 : blankingPos1);
  frame.DotBlt(dmpSetTime, 0, 0, dmpSetTime.GetWidth(), dmpSetTime.GetHeight(), (128 - dmpSetTime.GetWidth()) / 2, 11);

  // Buttons
  btnText[3] = (position == 0 ? "Next" : "Save");
  PaintButtons(frame, btnText);

  return ret;
}

//------------------------------------
// Function: HandleSetTimeCompensation
//------------------------------------
static int HandleTimeCorrect(DmdFrame& frame, bool tick, bool isInit, int& initValue)
{
  static int value ;
  
  int ret = 0;
  Dotmap dmpSet;
  const char *btnText[] = {"Back", "-", "+", "Save" };

  char setStr[9 + 1] ;
  const char *blanking;

  int btnMenuRead = btnMenu.Read();
  int btnPlusRead = btnPlus.Read();
  int btnMinusRead = btnMinus.Read();
  int btnEnterRead = btnEnter.Read();

  if(isInit)
  {
    value = initValue;
  }
  
  if(btnMenuRead == Button::Rising)
  {
    // Back
    ret = -1;
  }

  if(btnPlusRead == Button::Rising || btnPlusRead == Button::Hold)
  {
    // +
    value++;
    if(value > 999)
    {
      value = 999;
    }
  }
        
  if(btnMinusRead == Button::Rising || btnMinusRead == Button::Hold)
  {
    // -
    value--;
    if(value < -999)
    {
      value = -999;
    }
  }
  
  if(btnEnterRead == Button::Rising)
  {
    // Save
    initValue = value;
    ret = 1;
  }

  // Title
  PaintTitle(frame, "TIME CORRECT");

  // Clock
  blanking = tick ? "-      -" : "        ";
  sprintf(setStr, "> %+04d <", value);

  fontMenu.DmpFromString(dmpSet, setStr, blanking);
  frame.DotBlt(dmpSet, 0, 0, dmpSet.GetWidth(), dmpSet.GetHeight(), (128 - dmpSet.GetWidth()) / 2, 11);

  // Buttons
  PaintButtons(frame, btnText);

  return ret;
}

