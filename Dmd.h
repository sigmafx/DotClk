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

    volatile bool active;

    int pinEN;
    int pinR1;
    int pinR2;
    int pinLA;
    int pinLB;
    int pinLC;
    int pinLD;
    int pinLT;
    int pinSK;
  
    int UpdateRow();

  public:
    Dmd();
    void Initialise(int pinEN, int pinR1, int pinR2, int pinLA, int pinLB, int pinLC, int pinLD, int pinLT, int pinSK);
    void Start();
    void Stop();
    bool IsActive();
    bool SetBrightness(int set);
    int GetBrightness();
    void SetFrame(DmdFrame& frame);
    bool WaitSync(uint32_t timeout = 0);  
    void IsrDmd();
    
};

extern Dmd dmd ;

#endif

