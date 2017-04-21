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
  bool ret = true;
  uint16_t version;
  uint16_t cntItemDotmap;
  uint16_t cntItemStoryboard;

  // version
  ret &= fileScene.read(&version, sizeof(version)) > -1;

  // cntItemDotmap
  ret &= fileScene.read(&cntItemDotmap, sizeof(cntItemDotmap)) > -1;
  
  // cntItemStoryboard
  ret &= fileScene.read(&cntItemStoryboard, sizeof(cntItemStoryboard)) > -1;

  // Only read the scene if we read the header successfully
  if(ret)
  {
    // Read storyboard
    for (int idxItem = 0; idxItem < cntItemStoryboard; idxItem++)
    {
      // First Frame Delay
      ret &= fileScene.read(&firstFrameDelay, sizeof(firstFrameDelay)) > -1;
      // First Frame Layer
      ret &= fileScene.read(&firstFrameLayer, sizeof(firstFrameLayer)) > -1;
      // First Blank
      ret &= fileScene.read(&firstBlank, sizeof(firstBlank)) > -1;
  
      // Frame Delay
      ret &= fileScene.read(&frameDelay, sizeof(frameDelay)) > -1;
      //Frame Layer
      ret &= fileScene.read(&frameLayer, sizeof(frameLayer)) > -1;
  
      // Last Frame Delay
      ret &= fileScene.read(&lastFrameDelay, sizeof(lastFrameDelay)) > -1;    
      // Last  Frame Layer
      ret &= fileScene.read(&lastFrameLayer, sizeof(lastFrameLayer)) > -1;
      // Last Blank
      ret &= fileScene.read(&lastBlank, sizeof(lastBlank)) > -1;
  
      // Clock Style
      ret &= fileScene.read(&clockStyle, sizeof(clockStyle)) > -1;
  
      // Skip the 19 bytes for future features
      byte space[19] ;
      ret &= fileScene.read(space, sizeof(space)) > -1;
    }
  }
  
  // First and Last frame special processing?
  doFirst = (firstFrameDelay == 0 ? NA : TODO);
  doLast = (lastFrameDelay == 0 ? NA : TODO);
  
  cntFrames = cntItemDotmap;
  curFrame = 0;
  
  return ret;  
}

bool Scene::Eof()
{
  return (curFrame == cntFrames && (doLast == DONE || doLast == NA));
}

bool Scene::NextFrame(File& fileScene)
{
  bool ret = true;
  
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
    ret = dmpFrame.Create(fileScene);
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
      ret = false;
      goto ERROR_EXIT;
    }
  }

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

