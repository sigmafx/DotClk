#include <Arduino.h>

#include "DmdFrame.h"

DmdFrame::DmdFrame()
{
  width = 128;
  height = 32;
  Clear();
}

byte DmdFrame::GetDot(int x, int y)
{
  byte value ;

  if(!CheckRange(x, y))
  {
    return 0x00;
  }

  value = frame.dots[y][x / 2];
  if(x % 2)
  {
    // Odd dot
    value >>= 4;
  }
  else
  {
    // Even dot
    value &= 0x0F;
  }

  return value;
}

void DmdFrame::SetDot(int x, int y, byte value)
{
  byte dot;

  if(!CheckRange(x, y))
  {
    return;
  }

  value &= 0x0F;
  
  dot = frame.dots[y][x / 2];
  if(x % 2)
  {
    dot &= 0x0F;
    dot |= (value << 4);
  }
  else
  {
    dot &= 0xF0;
    dot |= value;
  }

  frame.dots[y][x / 2] = dot;
}

void DmdFrame::Clear(byte value)
{
  memset(frame.dots, (value & 0x0F )| (value << 4), sizeof(frame.dots));
}

void DmdFrame::DotBlt(Dotmap& dmp, int sourceX, int sourceY, int sourceWidth, int sourceHeight, int destX, int destY)
{
  int bltSrcX, bltSrcY, bltDestX, bltDestY;
  
  for (bltSrcX = sourceX, bltDestX = destX; bltSrcX < (sourceX + sourceWidth); bltSrcX++, bltDestX++)
  {
    for(bltSrcY = sourceY, bltDestY = destY; bltSrcY < (sourceY + sourceHeight); bltSrcY++, bltDestY++)
    {
      byte bltDot ;

      // Apply mask
      if(dmp.GetMask(bltSrcX, bltSrcY))
      {
        bltDot = this->GetDot(bltDestX, bltDestY);
      }
      else
      {
        bltDot = dmp.GetDot(bltSrcX, bltSrcY);
      }

      // Set dot
      this->SetDot(bltDestX, bltDestY, bltDot);
    }
  }
}

//--------
//--------
// PRIVATE
//--------
//--------
//---------------------
// Function: CheckRange
//---------------------
bool DmdFrame::CheckRange(int x, int y)
{
  if(x < 0 || y < 0 || x >= width || y >= height)
  {
    // Out of range, return
    return false;
  }
  else
  {
    return true;
  }
}

