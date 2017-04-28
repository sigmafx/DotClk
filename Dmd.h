#ifndef __DMD_H__
#define __DMD_H__

#include "DmdFrame.h"
#include "DmdFrameRaw.h"

class Dmd
{
  private:
    DmdFrameRaw buffer1;
    DmdFrameRaw buffer2;
    DmdFrameRaw *bufferActive;
    DmdFrameRaw *bufferInactive;
    DmdFrameRaw *bufferInUse;
 
    IntervalTimer timerDmd ;

    int frame ;
    int row ;
    int brightness ;
    byte colour;
    int dmdType ;
    
    volatile bool active;

    int pinEN;
    int pinR1;
    int pinR2;
    int pinG1;
    int pinG2;
    int pinLA;
    int pinLB;
    int pinLC;
    int pinLD;
    int pinLT;
    int pinSK;
  
    int UpdateRow();

  public:
    Dmd();
    void Initialise(int pinEN, int pinR1, int pinR2, int pinG1, int pinG2, int pinLA, int pinLB, int pinLC, int pinLD, int pinLT, int pinSK);
    void Start();
    void Stop();
    bool IsActive();
    bool SetBrightness(int set);
    int GetBrightness();
    bool SetColour(byte colour);
    byte GetColour();
    void SetFrame(DmdFrame& frame);
    bool WaitSync(uint32_t timeout = 0);  
    void IsrDmd();
    void SetDmdType(int dmdType);
    
};

#endif

