#ifndef __BUTTON_H__
#define __BUTTON_H__

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
    enum {
      Off = 0,
      Rising,
      On,
      Hold,
      Falling,  
    };
    
  public:
    Button(int pinBtn);
    int Read();
    int ReadRaw();
};

#endif

