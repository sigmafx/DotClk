#ifndef __SCENE_H__
#define __SCENE_H__

#include <SD.h>
#include "Dotmap.h"

class Scene
{
  public:
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
    short doFirst;
    short doLast;

    Dotmap dmpFrame;

  public:
    Scene();
    ~Scene();

    bool Create(File& fileScene);
    bool Eof();
    bool NextFrame(File& fileScene);
    Dotmap& GetFrameDotmap();
    unsigned long GetFrameDelay();
    uint16_t GetFrameLayer();
};

#endif

