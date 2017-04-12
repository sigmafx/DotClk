#include <Arduino.h>
#include <EEPROM.h>

#include "Config.h"

// One and only instance
Config config;

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
    cfgItems.cfgBrightness = 10;
    cfgItems.cfgClockDelay = CFG_CD_5SECS;
    strcpy(cfgItems.cfgClockFont, "STANDARD");
    cfgItems.cfgDotColour = CFG_DC_RED;

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
  
  return ret;
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

// End of file

