#include <Arduino.h>

#include "Font.h"
#include "Dotmap.h"

Font::Font()
{
  charInfo = NULL;
}

Font::~Font()
{
  Delete();
}

void Font::Create(uint16_t chars, Dotmap& font)
{
  Delete();
  
  this->dmpFont = font;
  this->chars = chars;

  charInfo = new FontCharInfo[chars] ;
}

void Font::Create(File& fileFont)
{
  uint16_t Version;
  uint16_t CntFontInfo;
  byte FontNameLen ;
  char FontName[255 + 1] ;

  // Rewind the file
  fileFont.seek(0);
  
  // Read the font header
  fileFont.read(&Version, sizeof(Version));
  fileFont.read(&FontNameLen, sizeof(FontNameLen));
  memset(FontName, '\0', sizeof(FontName));
  fileFont.read(FontName, FontNameLen);
  fileFont.read(&CntFontInfo, sizeof(CntFontInfo));
  
  // Read the font item info structure
  Delete();
  charInfo = new FontCharInfo[CntFontInfo] ;
  fileFont.read(charInfo, sizeof(FontCharInfo) * CntFontInfo);

  // Read the font bitmap
  dmpFont.Create(fileFont);
  chars = CntFontInfo;
}

bool Font::SetCharInfoFromRaw(const byte *data, uint16_t len)
{
  bool ret = false;

  if(len != (chars * sizeof(FontCharInfo)))
  {
    // ERROR - invalid len
    Serial.print("Invalid len - ");
    Serial.println(len);    
    goto ERROR_EXIT;
  }

  memcpy(charInfo, data, len);

  ret = true;

ERROR_EXIT:
  return ret;
}

bool Font::SetCharInfo(int idx, char ascii, uint16_t width, uint16_t kerning)
{
  bool ret = false;
  
  if(idx < 0 || idx >= chars || charInfo == NULL)
  {
    // ERROR Out of range
    goto ERROR_EXIT;
  }

  charInfo[idx].ascii = ascii;
  charInfo[idx].width = width;
  charInfo[idx].kerning = kerning;

  ret = true;
  
ERROR_EXIT:
  return ret;
}

Dotmap& Font::DmpFromString(Dotmap& dmp, const char *string, const char *blanking)
{
  int widthString;
  int destXOffset;
  int destXOffsetRetainMask;
  int thisChar;
  
  // Determine total width of the returned dotmap from the chars being requested
  widthString = 0;
  for (thisChar = 0; thisChar < (int)(strlen(string) - 1); thisChar++)
  {
    widthString += GetCharWidth(string[thisChar]);
    widthString -= GetCharKerning(string[thisChar]);
  }

  widthString += GetCharWidth(string[thisChar]);

  // Create the dotmap
  dmp.Create(widthString, dmpFont.GetHeight());

  // Now build up the dotmap
  destXOffset = 0;
  destXOffsetRetainMask = 0;
  for (int thisChar = 0; thisChar < (int)strlen(string); thisChar++)
  {
    int thisCharWidth = GetCharWidth(string[thisChar]);
    int thisCharOffset = GetCharOffset(string[thisChar]);
    
    for(int srcY = 0, destY = 0; srcY < dmpFont.GetHeight(); srcY++, destY++)
    {
      for(int srcX = thisCharOffset, destX = destXOffset; srcX < (thisCharOffset + thisCharWidth); srcX++, destX++)
      {
        if(blanking != NULL && blanking[thisChar] == '-')
        {
          dmp.SetDot(destX, destY, 0x00);
          if(destX >= destXOffsetRetainMask)
          {
            dmp.SetMask(destX, destY, 0x01);
          }
        }
        else
        {
          dmp.SetDot(destX, destY, dmpFont.GetDot(srcX, srcY));
          if(destX < destXOffsetRetainMask)
          {
            dmp.SetMask(destX, destY, dmpFont.GetMask(srcX, srcY) & dmp.GetMask(destX, destY));
          }
          else
          {
            dmp.SetMask(destX, destY, dmpFont.GetMask(srcX, srcY));
          }
        }
      }
    }

    // Move to next character position
    destXOffset += thisCharWidth;

    // Adjust for kerning
    destXOffsetRetainMask = destXOffset;
    destXOffset -= GetCharKerning(string[thisChar]);
  }
  
  return dmp;
}

byte Font::GetFontCount()
{
  byte ret = 0;

  // Open the 'Fonts' directory
  File dirFonts = SD.open("/Fonts");    
  if(dirFonts)
  {
    do
    {
      File fileFont = dirFonts.openNextFile();
      if(fileFont)
      {
        ret++;
        fileFont.close();
      }
      else
      {
        break;
      }
    }
    while(true);

    dirFonts.close();
  }

  return ret;
}

byte Font::GetFontName(File& fileFont, FONTNAME fontName)
{
  uint16_t Version;
  byte FontNameLen ;

  fileFont.seek(0);
  fileFont.read(&Version, sizeof(Version));
  fileFont.read(&FontNameLen, sizeof(FontNameLen));

  FontNameLen = min((int)(FontNameLen + 1), (int)sizeof(FONTNAME));
  
  fileFont.read(fontName, FontNameLen);
  fontName[FontNameLen - 1] = '\0';

  return FontNameLen;
}

//--------
//--------
// PRIVATE
//--------
//--------
uint16_t Font::GetCharWidth(char asciiGet)
{
  uint16_t ret = 0;
  
  for (uint16_t thisChar = 0; thisChar < chars; thisChar++)
  {    
    if(charInfo[thisChar].ascii == asciiGet)
    {
      ret = charInfo[thisChar].width;
      break ;
    }
  }

  return ret;
}

uint16_t Font::GetCharOffset(char asciiGet)
{
  uint16_t ret = 0;
  
  for (uint16_t thisChar = 0; thisChar < chars; thisChar++)
  {
    if(charInfo[thisChar].ascii == asciiGet)
    {
      break ;
    }
    else
    {
      ret += charInfo[thisChar].width;
    }
  }

  return ret;
}

uint16_t Font::GetCharKerning(char asciiGet)
{
  uint16_t ret = 0;
  
  for (uint16_t thisChar = 0; thisChar < chars; thisChar++)
  {    
    if(charInfo[thisChar].ascii == asciiGet)
    {
      ret = charInfo[thisChar].kerning;
      break ;
    }
  }

  return ret;
}

void Font::Delete()
{
  if(charInfo != NULL)
  {
    delete[] charInfo;
  }
}

