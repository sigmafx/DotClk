#include <Arduino.h>
#include <Time.h>

#include "Globals.h"

#include "Utils.h"

//------------------------
// Function: SetBtnMapping
//------------------------
void SetBtnMapping(bool reverse)
{
  if(reverse)
  {
    btnMenu.SetPin(pinBtnEnter);
    btnPlus.SetPin(pinBtnMinus);
    btnMinus.SetPin(pinBtnPlus);
    btnEnter.SetPin(pinBtnMenu);
  }
  else
  {
    btnMenu.SetPin(pinBtnMenu);
    btnPlus.SetPin(pinBtnPlus);
    btnMinus.SetPin(pinBtnMinus);
    btnEnter.SetPin(pinBtnEnter);
  }
}

//-----------------
// Function: nowDST
//-----------------
time_t NowDST()
{
  time_t timeNow = now();

  if(config.GetCfgItems().cfgDST == Config::CFG_DST_ON)
  {
    timeNow += SECS_PER_HOUR; // Add on an hour
  }

  return timeNow;
}

//---------------------
// Function: CpuRestart
//---------------------
void CpuRestart()
{
  uint32_t *addrRestart = (uint32_t *)0xE000ED0C;

  // Force restart of CPU
  *addrRestart = 0x5FA0004;
}

//-------------------------
// Function: getTeensy3Time
//-------------------------
time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

//------------------
// Function: FreeRam
//------------------
uint32_t FreeRam()
{
    uint32_t stackTop;
    uint32_t heapTop;

    // current position of the stack.
    stackTop = (uint32_t) &stackTop;

    // current position of heap.
    void* hTop = malloc(1);
    heapTop = (uint32_t) hTop;
    free(hTop);

    // The difference is the free, available ram.
    return stackTop - heapTop;
}

