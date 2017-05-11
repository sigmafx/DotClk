#include <Arduino.h>

#include "Button.h"

Button::Button(int pinBtn)
{
  SetPin(pinBtn);
  
  timeDebounce = 10;
  timeHold = 1000;
  risen = false;
  readLast = HIGH ;
}

void Button::SetPin(int pinBtn)
{
  // Set the button pint to input pull up
  // Button is expected to be grounded to be ON
  pinMode(pinBtn, INPUT_PULLUP);

  // Store the button pin assignment
  this->pinBtn = pinBtn ;
}

int Button::ReadRaw()
{
  return digitalRead(pinBtn) == LOW ? On : Off ;
}

int Button::Read()
{
  unsigned long timeNow = millis();
  int ret = Button::Off;
  
  if(digitalRead(pinBtn) == LOW)
  {
    if(readLast == HIGH)
    {
      // Transition from OFF to ON, not debounced
      timeLast = millis();
    }
    else
    {
      if((timeNow - timeLast) > timeDebounce)
      {
        // Debounced
        if(risen)
        {
          if((timeNow - timeLast) > timeHold)
          {
            // Button ON and HOLD
            ret = Button::Hold;
          }
          else
          {
            // Button ON but not HOLD
            ret = Button::On;
          }
        }
        else
        {
          // Btn RISING
          ret = Button::Rising;
          risen = true;
        }
      }
    }

    // Keep track of where we are
    readLast = LOW;
  }
  else
  {
    // Button not pressed, reset everything
    timeLast = 0;    
    risen = false;

    // Keep track of where we are
    readLast = HIGH;
  }

  return ret;
}

