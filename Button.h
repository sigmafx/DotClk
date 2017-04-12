#ifndef __BUTTON_H__
#define __BUTTON_H__

// Return values from Read function
#define BTN_OFF     0
#define BTN_RISING  1
#define BTN_ON      2
#define BTN_ON_HOLD 3
#define BTN_FALLING 4

class Button
{
  private:
    int pinBtn;
    unsigned long timeDebounce;
    unsigned long timeHold;
    unsigned long timeLast;
    
    int readLast;
    bool risen;
    
  public:
    Button(int pinBtn);
    int Read();
    int ReadRaw();
};

#endif

