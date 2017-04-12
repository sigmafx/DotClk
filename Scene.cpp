#include <Arduino.h>
#include "Scene.h"

enum SpecialProc
{
  NA = 0,
  TODO = 1,
  INPROC = 2,
  DONE = 3
};

Scene::Scene()
{
  cntFrames = 0;
  curFrame = 0;
}

Scene::~Scene()
{
}

bool Scene::Create(File& fileScene)
{
  bool ret = false;
  uint16_t version;
  uint16_t cntItemDotmap;
  uint16_t cntItemStoryboard;

  // version
  fileScene.read(&version, sizeof(version));

  // cntItemDotmap
  fileScene.read(&cntItemDotmap, sizeof(cntItemDotmap));
  
  // cntItemStoryboard
  fileScene.read(&cntItemStoryboard, sizeof(cntItemStoryboard));

  // Read storyboard
  for (int idxItem = 0; idxItem < cntItemStoryboard; idxItem++)
  {
    // First Frame Delay
    fileScene.read(&firstFrameDelay, sizeof(firstFrameDelay));
    // First Frame Layer
    fileScene.read(&firstFrameLayer, sizeof(firstFrameLayer));
    // First Blank
    fileScene.read(&firstBlank, sizeof(firstBlank));

    // Frame Delay
    fileScene.read(&frameDelay, sizeof(frameDelay));
    //Frame Layer
    fileScene.read(&frameLayer, sizeof(frameLayer));

    // Last Frame Delay
    fileScene.read(&lastFrameDelay, sizeof(lastFrameDelay));    
    // Last  Frame Layer
    fileScene.read(&lastFrameLayer, sizeof(lastFrameLayer));
    // Last Blank
    fileScene.read(&lastBlank, sizeof(lastBlank));

    // Clock Style
    fileScene.read(&clockStyle, sizeof(clockStyle));

    // Skip the 19 bytes for future features
    byte space[19] ;
    fileScene.read(space, sizeof(space));
  }

  // First and Last frame special processing?
  doFirst = (firstFrameDelay == 0 ? NA : TODO);
  doLast = (lastFrameDelay == 0 ? NA : TODO);
  
  cntFrames = cntItemDotmap;
  curFrame = 0;
  
  ret = true;

  return ret;  
}

bool Scene::Eof()
{
  return (curFrame == cntFrames && (doLast == DONE || doLast == NA));
}

bool Scene::NextFrame(File& fileScene)
{
  bool ret = false;
  
  if(doFirst == TODO)
  {
    doFirst = INPROC;
  }
  else
  {
    doFirst = DONE;
  }

  // Last Frame?
  if(curFrame == cntFrames)
  {
    if(doLast == TODO)
    {
      doLast = INPROC;
    }
    else
    {
      doLast = DONE;
    }
  }

  if(doFirst == INPROC && firstBlank == 0x01)
  {
    // Set dotmap and mask to blank
    dmpFrame.ClearDots();
    dmpFrame.ClearMask();
  }
  else
  if(curFrame < cntFrames)
  {
    dmpFrame.Create(fileScene);
    curFrame++;
  }
  else
  {
    if(doLast == INPROC && lastBlank == 0x01)
    {
      // Set dotmap and mask to blank
      dmpFrame.ClearDots();
      dmpFrame.ClearMask();
    }
    else
    {
      // End of file
      goto ERROR_EXIT;
    }
  }

  ret = true;

ERROR_EXIT:

  return ret;
}

Dotmap& Scene::GetFrameDotmap()
{
  return dmpFrame;
}
    
unsigned long Scene::GetFrameDelay()
{
  uint16_t ret;
  
  if(doFirst == INPROC)
  {
    ret = firstFrameDelay;
  }
  else
  if(doLast == INPROC)
  {
    ret = lastFrameDelay;
  }
  else
  {
    ret = frameDelay;
  }
  
  return ret;
}

uint16_t Scene::GetFrameLayer()
{
  uint16_t ret;
  
  if(doFirst == INPROC)
  {
    ret = firstFrameLayer;
  }
  else
  if(doLast == INPROC || doLast == DONE)
  {
    ret = lastFrameLayer;
  }
  else
  {
    ret = frameLayer;
  }
  
  return ret;
}

byte Scene::GetClockStyle()
{
  return clockStyle;
}

