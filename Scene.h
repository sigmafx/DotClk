#ifndef __SCENE_H__
#define __SCENE_H__

#include <SdFat.h>
#include "Dotmap.h"

class Scene
{
  private:
    uint16_t firstFrameDelay;
    uint16_t firstFrameLayer;
    uint16_t firstBlank;
    uint16_t frameDelay;
    uint16_t frameLayer;
    uint16_t lastFrameDelay;
    uint16_t lastFrameLayer;
    uint16_t lastBlank;
    uint16_t cntFrames;
    uint16_t curFrame;
    byte clockStyle;
    byte customX;
    byte customY;
    short doFirst;
    short doLast;
    Dotmap dmpFrame;

  public:
    enum {
      ClockStyleStd = 0,
      ClockStyleCustom,
    };
    
  public:
    Scene();
    ~Scene();

    bool Create(FsFile& fileScene);
    void Clear();
    bool Eof();
    bool NextFrame(FsFile& fileScene);
    Dotmap& GetFrameDotmap();
    unsigned long GetFrameDelay();
    uint16_t GetFrameLayer();
    byte GetClockStyle();
    byte GetCustomX();
    byte GetCustomY();
};

#endif
