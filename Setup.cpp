#include <Arduino.h>
#include <Time.h>

#include "Button.h"
#include "Font.h"
#include "DmdFrame.h"
#include "Dotmap.h"
#include "Dmd.h"
#include "Config.h"

// Menu Helper
struct Menu;
typedef void (*INITFN)(class Menu *);
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
void PaintTitle(DmdFrame& frame, const char *titleText);
void PaintButtons(DmdFrame& frame, const char *btnText[4]);
int HandleStandard(DmdFrame& frame, Menu& menu, bool isInit, int& initValue);
bool HandleBrightness(DmdFrame& frame, bool isInit, int& initValue);
int HandleSetTime(DmdFrame& frame, bool tick, bool isInit, time_t& initValue);

// External objects - see DmdClock source
extern Font fontMenu;
extern Font fontSystem;
extern Button btnMenu;
extern Button btnPlus;
extern Button btnMinus;
extern Button btnEnter;
extern Dmd dmd;

// Menu IDs
enum
{
  MENU_SETTIME = 0,
  MENU_DST = 1,
  MENU_TIMEFORMAT = 2,
  MENU_BRIGHTNESS = 3,
  MENU_CLOCKDELAY = 4,
};

// Standard menu structs
struct MenuMainMenu : Menu
{
  MenuMainMenu() : Menu(5)
  {
    menuTitle = "MAIN MENU";
    menuItems[0] = "SET TIME";
    menuItems[1] = "DST";
    menuItems[2] = "TIME FORMAT";
    menuItems[3] = "BRIGHTNESS";
    menuItems[4] = "CLOCK DELAY";
    menuButtons[0] = "Back";
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

// Standard menu objects
MenuMainMenu menuMainMenu;
MenuDST menuDST;
MenuTimeFormat menuTimeFormat;
MenuClockDelay menuClockDelay;

// Config Items
time_t DateTime ;
int cfgDST;
int cfgTimeFormat;
int Brightness ;
int cfgClockDelay;

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
    // Initialise values from config
    config.GetCfgItem(CFG_DST, &cfgDST, sizeof(cfgDST));
    config.GetCfgItem(CFG_TIMEFORMAT, &cfgTimeFormat, sizeof(cfgTimeFormat));
    config.GetCfgItem(CFG_BRIGHTNESS, &Brightness, sizeof(Brightness));
    config.GetCfgItem(CFG_CLOCKDELAY, &cfgClockDelay, sizeof(cfgClockDelay));

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
            DateTime = now();
            HandleSetTime(frame, false, true, DateTime);
            break;

          case MENU_DST: // DST
            HandleStandard(frame, menuDST, true, cfgDST);
            break ;

          case MENU_TIMEFORMAT: // Time Format
            HandleStandard(frame, menuTimeFormat, true, cfgTimeFormat);
            break ;
          
          case MENU_BRIGHTNESS: // Brightness
            Brightness = dmd.GetBrightness();
            HandleBrightness(frame, true, Brightness);
            break ;

          case MENU_CLOCKDELAY: // Clock Delay
            HandleStandard(frame, menuClockDelay, true, cfgClockDelay);
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
        int menuRet = HandleSetTime(frame, (millis()/500)%2, false, DateTime);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
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
        int menuRet = HandleStandard(frame, menuDST, false, cfgDST);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
            // Write the DST to config
            config.SetCfgItem(CFG_DST, &cfgDST, sizeof(cfgDST));
          }
          
          showMainMenu = true;
        }
        break ;
      }

      case MENU_TIMEFORMAT: // Time Format
      {
        int menuRet = HandleStandard(frame, menuTimeFormat, false, cfgTimeFormat);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
            config.SetCfgItem(CFG_TIMEFORMAT, &cfgTimeFormat, sizeof(cfgTimeFormat));
          }
          showMainMenu = true;
        }
        break ;        
      }
        
      case MENU_BRIGHTNESS: // Brightness
        if(!HandleBrightness(frame, false, Brightness))
        {
          // Write the brightness to config
          config.SetCfgItem(CFG_BRIGHTNESS, &Brightness, sizeof(Brightness));
          
          showMainMenu = true;
        }
        break;
      
      case MENU_CLOCKDELAY: // Clock Delay
      {
        int menuRet = HandleStandard(frame, menuClockDelay, false, cfgClockDelay);
        if( menuRet != 0)
        {
          if(menuRet == 1)
          {
            config.SetCfgItem(CFG_CLOCKDELAY, &cfgClockDelay, sizeof(cfgClockDelay));
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
void PaintTitle(DmdFrame& frame, const char *titleText)
{
  Dotmap dmpTitle;
  char clock[5 + 1];
  time_t time = now();
  
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
void PaintButtons(DmdFrame& frame, const char *btnText[4])
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
  frame.DotBlt(dmpButtons, 0, 0, dmpButtons.GetWidth(), dmpButtons.GetHeight(), 4, 24);
  fontSystem.DmpFromString(dmpButtons, btnText[1]);
  frame.DotBlt(dmpButtons, 0, 0, dmpButtons.GetWidth(), dmpButtons.GetHeight(), 36, 24);
  fontSystem.DmpFromString(dmpButtons, btnText[2]);
  frame.DotBlt(dmpButtons, 0, 0, dmpButtons.GetWidth(), dmpButtons.GetHeight(), 68, 24);
  fontSystem.DmpFromString(dmpButtons, btnText[3]);
  frame.DotBlt(dmpButtons, 0, 0, dmpButtons.GetWidth(), dmpButtons.GetHeight(), 100, 24);
}

//-------------------------
// Function: HandleStandard
//-------------------------
int HandleStandard(DmdFrame& frame, Menu& menu, bool isInit, int& initValue)
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

  if(btnMenuRead == BTN_RISING)
  {
    // Back
    ret = -1;
  }

  if(btnPlusRead == BTN_RISING)
  {
    // Next
    if(value < (menu.cntMenuItems - 1))
    {
      value++;
    }
  }

  if(btnMinusRead == BTN_RISING)
  {
    // Prev
    if(value > 0)
    {
      value-- ;
    }
  }

  if(btnEnterRead == BTN_RISING)
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

//---------------------------
// Function: HandleBrightness
//---------------------------
bool HandleBrightness(DmdFrame& frame, bool isInit, int& initValue)
{
  static int value ;
  bool ret = true;
  Dotmap dmpBrightness ;
  const char *btnText[] = {"Back", "Down", " Up ", "Save", };
  
  int btnMenuRead = btnMenu.Read();
  int btnPlusRead = btnPlus.Read();
  int btnMinusRead = btnMinus.Read();
  int btnEnterRead = btnEnter.Read();

  if(isInit)
  {
    value = initValue;
  }
  
  if(btnMenuRead == BTN_RISING)
  {
    // Back
    dmd.SetBrightness(initValue);
    ret = false;
  }

  if(btnPlusRead == BTN_RISING || btnPlusRead == BTN_ON_HOLD)
  {
    // Up
    if(value < 63)
    {
      value++;
      dmd.SetBrightness(value);
    }
  }
        
  if(btnMinusRead == BTN_RISING || btnMinusRead == BTN_ON_HOLD)
  {
    // Down
    if(value > 0)
    {
      value--;
      dmd.SetBrightness(value);
    }
  }
  
  if(btnEnterRead == BTN_RISING)
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
int HandleSetTime(DmdFrame& frame, bool tick, bool isInit, time_t& initValue)
{
  static TimeElements value ;
  static int position ;
  
  int ret = 0;
  Dotmap dmpSetTime;
  const char *btnText[] = {"Back", " -  ", "  + ", "" };

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
  
  if(btnMenuRead == BTN_RISING)
  {
    // Back
    ret = -1;
  }

  if(btnPlusRead == BTN_RISING || btnPlusRead == BTN_ON_HOLD)
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
        
  if(btnMinusRead == BTN_RISING || btnMinusRead == BTN_ON_HOLD)
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
  
  if(btnEnterRead == BTN_RISING)
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
  PaintTitle(frame, "SET TIME");

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

