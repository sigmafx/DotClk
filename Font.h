#ifndef __FONT_H__
#define __FONT_H__

#include "SD.h"
#include "Dotmap.h"

#pragma pack(1)
typedef struct tagFontCharInfo
{
  char ascii ;
  uint16_t width ;
  uint16_t kerning;
} FontCharInfo ;
#pragma pack()

class Font
{
  private:
    Dotmap dmpFont;
    FontCharInfo *charInfo;
    uint16_t chars;

    uint16_t GetCharWidth(char charGet);
    uint16_t GetCharOffset(char charGet);
    uint16_t GetCharKerning(char charGet);
    void Delete();
    
  public:
    Font();
    ~Font();
    void Create(uint16_t chars, Dotmap& font);
    void Create(File& fileFont);
    bool SetCharInfoFromRaw(const byte *data, uint16_t len);
    bool SetCharInfo(int idx, char ascii, uint16_t width, uint16_t kerning);
    Dotmap& DmpFromString(Dotmap& dmp, const char *string, const char *blanking = NULL);
};

#endif

