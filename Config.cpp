#include <Arduino.h>
#include <EEPROM.h>

#include "Globals.h"

//----------------------
// Function: Constructor
//----------------------
Config::Config()
{
  cfgCntItems = 0;
  memset(&cfgItems, 0, sizeof(cfgItems));
  
  if(!readEeprom())
  {
    // No config data, so setup some defaults and save
    cfgItems.cfgDST = CFG_DST_OFF;
    cfgItems.cfgTimeFormat = CFG_TF_24HOUR;
    cfgItems.cfgTimeCorrect = 0;
    cfgItems.cfgBrightness = 10;
    cfgItems.cfgClockDelay = CFG_CD_5SECS;
    strcpy(cfgItems.cfgClockFont, "STANDARD");
    cfgItems.cfgDotColour = CFG_DC_RED;
    cfgItems.cfgDmdType = 0;
    cfgItems.cfgShowBrand = CFG_SB_NEVER;
    cfgItems.cfgDebug = CFG_DBG_NO;
    cfgItems.cfgBtnMap = CFG_BM_NORMAL;
    cfgItems.cfgSleepTime = 0;
    cfgItems.cfgWakeTime = 0;

    setValues();

    writeEeprom();
  }
}

//----------------------
// Function: GetCfgItems
//----------------------
const ConfigItems& Config::GetCfgItems()
{
  if(cfgCntItems != CntItems)
  {
    // No items currently so try to read from EEPROM
    readEeprom();
  }
  
  return cfgItems;
}

//---------------------
// Function: SetCfgItem
//---------------------
bool Config::SetCfgItems(ConfigItems& cfgItems, bool write)
{
  bool ret = true;

  // Copy the passed data items
  this->cfgItems = cfgItems ;

  if(write)
  {
    // Write to EEPROM
    ret = writeEeprom();
  }

  setValues();
  
  return ret;
}

int Config::GetShowBrandValue()
{
  return cfgShowBrandValue;
}

int Config::GetClockDelayValue()
{
  return cfgClockDelayValue;
}

//--------
//--------
// PRIVATE
//--------
//--------
//---------------
// Function: Read
//---------------
bool Config::readEeprom()
{
  bool ret;
  int address = 0;

  // Read the config item count
  address += readBytes(address, (byte *)&cfgCntItems, sizeof(cfgCntItems));
  if(cfgCntItems == CntItems)
  {
    // Read the remaining items
    readBytes(address, (byte*)&cfgItems, sizeof(cfgItems));    
    setValues();
    ret = true;
  }
  else
  {
    // No items to read
    ret = false;
  }

  return ret;
}

//----------------------
// Function: writeEeprom
//----------------------
bool Config::writeEeprom()
{
  bool ret = true;
  int address = 0;

  // Default the config item count
  cfgCntItems = CntItems;

  // Write all config items
  address += writeBytes(address, (byte*)&cfgCntItems, sizeof(cfgCntItems));
  writeBytes(address, (byte*)&cfgItems, sizeof(cfgItems));

  return ret;
}

//--------------------
// Function: readBytes
//--------------------
int Config::readBytes(int address, byte *data, size_t len)
{
  size_t dataByte;
  
  for(dataByte = 0; dataByte < len && address < EEPROM.length(); dataByte++, address++)
  {
    data[dataByte] = EEPROM.read(address);
  }

  return (int)dataByte;
}

//---------------------
// Function: writeBytes
//---------------------
int Config::writeBytes(int address, byte *data, size_t len)
{
  size_t dataByte;
  
  for(dataByte = 0; dataByte < len && address < EEPROM.length(); dataByte++, address++)
  {
    EEPROM.write(address, data[dataByte]);
  }

  return (int)dataByte;
}

//--------------------
// Function: setValues
//--------------------
void Config::setValues()
{
  // Convert cfgShowBrand to Values
  switch(cfgItems.cfgShowBrand)
  {
    default:
    case Config::CFG_SB_NEVER:
      cfgShowBrandValue = 0;
      break ;
    case Config::CFG_SB_EVERY2:
      cfgShowBrandValue = 2;
      break ;
    case Config::CFG_SB_EVERY5:
      cfgShowBrandValue = 5;
      break ;
    case Config::CFG_SB_EVERY10:
      cfgShowBrandValue = 10;
      break ;
    case Config::CFG_SB_EVERY20:
      cfgShowBrandValue = 20;
      break ;
  }

  // Convert cfgClockDelay to Values
  switch(cfgItems.cfgClockDelay)
  {
    default:
    case Config::CFG_CD_5SECS:
      cfgClockDelayValue = 5000;
      break;
    case Config::CFG_CD_10SECS:
      cfgClockDelayValue = 10000;
      break;
    case Config::CFG_CD_15SECS:
      cfgClockDelayValue = 15000;
      break;
    case Config::CFG_CD_30SECS:
      cfgClockDelayValue = 30000;
      break;
    case Config::CFG_CD_1MIN:
      cfgClockDelayValue = 60000;
      break;
    case Config::CFG_CD_2MINS:
      cfgClockDelayValue = 120000;
      break;
    case Config::CFG_CD_5MINS:
      cfgClockDelayValue = 300000;
      break;
  }
}

// End of file

