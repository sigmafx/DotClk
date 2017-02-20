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
  cfgConfigItems = 0;
  cfgDST = 0;
  cfgTimeFormat = 0;
  cfgBrightness = 0;
  cfgClockDelay = 0;

  if(!Read())
  {
    // No config data, so setup some defaults and save
    cfgDST = CFG_DST_OFF;
    cfgTimeFormat = CFG_TF_24HOUR;
    cfgBrightness = 10;
    cfgClockDelay = CFG_CD_5SECS;

    Write();
  }
}

bool Config::Read()
{
  bool ret;
  int address = 0;

  // Read the config item count
  address += readBytes(address, (byte*)&cfgConfigItems, sizeof(cfgConfigItems));
  if(cfgConfigItems > 0)
  {
    // Read the remaining items
    address += readBytes(address, (byte*)&cfgDST, sizeof(cfgDST));
    address += readBytes(address, (byte*)&cfgTimeFormat, sizeof(cfgTimeFormat));
    address += readBytes(address, (byte*)&cfgBrightness, sizeof(cfgBrightness));
    address += readBytes(address, (byte*)&cfgClockDelay, sizeof(cfgClockDelay));

    ret = true;
  }
  else
  {
    // No items to read
    ret = false;
  }

  return ret;
}

bool Config::Write()
{
  bool ret = true;
  int address = 0;

  // Default the config item count
  cfgConfigItems = 4;

  // Write all config items
  address += writeBytes(address, (byte*)&cfgConfigItems, sizeof(cfgConfigItems));
  address += writeBytes(address, (byte*)&cfgDST, sizeof(cfgDST));
  address += writeBytes(address, (byte*)&cfgTimeFormat, sizeof(cfgTimeFormat));
  address += writeBytes(address, (byte*)&cfgBrightness, sizeof(cfgBrightness));
  address += writeBytes(address, (byte*)&cfgClockDelay, sizeof(cfgClockDelay));

  return ret;
}

//---------------------
// Function: GetCfgItem
//---------------------
bool Config::GetCfgItem(int idxItem, void *data, size_t len)
{
  bool ret;
  
  if(cfgConfigItems == 0)
  {
    // No items currently so try to read from EEPROM
    ret = Read();
    if(cfgConfigItems == 0)
    {
      // EEPROM not setup yet
      ret = false;
    }
  }
  else
  {
    ret = true;
  }

  if(ret)
  {
    switch(idxItem)
    {
      case CFG_DST:
        if(len == sizeof(cfgDST))
        {
          *(int *)data = cfgDST;
        }
        else
        {
          ret = false;
        }
        break ;
  
      case CFG_TIMEFORMAT:
        if(len == sizeof(cfgTimeFormat))
        {
          *(int *)data = cfgTimeFormat;
        }
        else
        {
          ret = false;
        }
        break ;
  
      case CFG_BRIGHTNESS:
        if(len == sizeof(cfgBrightness))
        {
          *(int *)data = cfgBrightness;
        }
        else
        {
          ret = false;
        }
        break ;
  
      case CFG_CLOCKDELAY:
        if(len == sizeof(cfgClockDelay))
        {
          *(int *)data = cfgClockDelay;
        }
        else
        {
          ret = false;
        }
        break ;

      default:
        ret = false;
        break ;
    }
  }
  
  return ret;
}

//---------------------
// Function: SetCfgItem
//---------------------
bool Config::SetCfgItem(int idxItem, void *data, size_t len, bool write)
{
  bool ret = true;

  switch(idxItem)
  {
    case CFG_DST:
      if(len == sizeof(cfgDST))
      {
        cfgDST = *(int *)data;
      }
      else
      {
        ret = false;
      }
      break ;

    case CFG_TIMEFORMAT:
      if(len == sizeof(cfgTimeFormat))
      {
        cfgTimeFormat = *(int *)data;
      }
      else
      {
        ret = false;
      }
      break ;

    case CFG_BRIGHTNESS:
      if(len == sizeof(cfgBrightness))
      {
        cfgBrightness = *(int *)data;
      }
      else
      {
        ret = false;
      }
      break ;

    case CFG_CLOCKDELAY:
      if(len == sizeof(cfgClockDelay))
      {
        cfgClockDelay = *(int *)data;
      }
      else
      {
        ret = false;
      }
      break ;

    default:
      ret = false;
      break ;
  }

  if(ret && write)
  {
    // All good and been asked to write to EEPROM
    ret = writeCfgItem(idxItem);
  }
  
  return ret;
}

//--------
//--------
// PRIVATE
//--------
//--------

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

//-----------------------
// Function: writeCfgItem
//-----------------------
bool Config::writeCfgItem(int idxItem)
{
  int address[] = { 0x0000,     // ConfigItems
                      0x0004,   // DST
                      0x0008,   // Time Format
                      0x000C,   // Brightness
                      0x0010,   // Clock Delay
                      }; 
  byte *data;
  size_t len;
  bool ret;

  switch(idxItem)
  {
    case CFG_CONFIGITEMS:
      data = (byte*)&cfgConfigItems ;
      len = sizeof(cfgConfigItems);
      break;
      
    case CFG_DST:
      data = (byte*)&cfgDST;
      len = sizeof(cfgDST);
      break;
      
    case CFG_TIMEFORMAT:
      data = (byte*)&cfgTimeFormat;
      len = sizeof(cfgTimeFormat);
      break;
      
    case CFG_BRIGHTNESS:
      data = (byte*)&cfgBrightness;
      len = sizeof(cfgBrightness);
      break;

    case CFG_CLOCKDELAY:
      data = (byte*)&cfgClockDelay;
      len = sizeof(cfgClockDelay);
      break;

    default:
      data = NULL;
      len = 0;
      break;
  }

  if(len > 0)
  {
    ret = writeBytes(address[idxItem], data, len);
  }
  else
  {
    ret = false;
  }
  
  return ret;
}

// End of file

