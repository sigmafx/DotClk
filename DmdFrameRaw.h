#ifndef __DMDFRAMERAW_H__
#define __DMDFRAMERAW_H__

typedef byte DmdFrameRow[64];

class DmdFrameRaw
{
  public:
    DmdFrameRow dots[32];
};

#endif

