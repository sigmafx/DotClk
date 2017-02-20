#ifndef __DMDFRAME_H__
#define __DMDFRAME_H__

#include "DmdFrameRaw.h"
#include "Dotmap.h"

class DmdFrame
{
  private:
    DmdFrameRaw frame;
    int width;
    int height;

    bool CheckRange(int x, int y);
    
  public:
    DmdFrame();
    byte GetDot(int x, int y);
    void SetDot(int x, int y, byte value);
    void Clear(byte value = 0x00);
    void DotBlt(Dotmap& dmp, int sourceX, int sourceY, int sourceWidth, int sourceHeight, int destX, int destY);

    friend class Dmd;
};

#endif

