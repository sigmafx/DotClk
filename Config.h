#ifndef __CONFIG_H__
#define __CONFIG_H__

class Config
{
  private:
    int cfgConfigItems;
    int cfgDST;
    int cfgTimeFormat;
    int cfgBrightness;
    int cfgClockDelay;

    int readBytes(int address, byte *data, size_t len);
    int writeBytes(int address, byte *data, size_t  len);
    bool writeCfgItem(int idxItem);

  public:
    Config();
    bool Read();
    bool Write();
    bool GetCfgItem(int idxItem, void *data, size_t len);
    bool SetCfgItem(int idxItem, void *data, size_t  len, bool write = true);
};

// Index of config items
enum {
  CFG_CONFIGITEMS = 0,
  CFG_DST = 1,
  CFG_TIMEFORMAT = 2,
  CFG_BRIGHTNESS = 3,
  CFG_CLOCKDELAY = 4,
};

// DST
enum {
  CFG_DST_OFF = 0,
  CFG_DST_ON = 1,
};

// TIME FORMAT
enum {
  CFG_TF_24HOUR = 0,
  CFG_TF_12HOUR = 1,
  CFG_TF_12HAMPM = 2,  
};

// CLOCK DELAY
enum {
  CFG_CD_5SECS = 0,
  CFG_CD_10SECS,
  CFG_CD_15SECS,
  CFG_CD_30SECS,
  CFG_CD_1MIN,
  CFG_CD_2MINS,
  CFG_CD_5MINS,
};

extern Config config;

#endif
