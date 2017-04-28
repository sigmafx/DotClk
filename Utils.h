#ifndef __UTILS_H__
#define __UTILS_H__

extern "C" {

  void SetBtnMapping(bool reverse);
  time_t NowDST();
  void CpuRestart();
  time_t getTeensy3Time();
  uint32_t FreeRam();
}

#endif
