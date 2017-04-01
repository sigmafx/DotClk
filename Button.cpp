#include <Arduino.h>

#include "Button.h"

Button::Button(int pinBtn)
{
  // Set the button pint to input pull up
  // Button is expected to be grounded to be ON
  pinMode(pinBtn, INPUT_PULLUP);

  // Initialise member variables
  this->pinBtn = pinBtn ;
  timeDebounce = 100;
  timeHold = 1000;
  risen = false;
  readLast = HIGH ;
}

int Button::Read()
{
  unsigned long timeNow = millis();
  int ret = BTN_OFF;
  
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
            ret = BTN_ON_HOLD;
          }
          else
          {
            // Button ON but not HOLD
            ret = BTN_ON;
          }
        }
        else
        {
          // Btn RISING
          ret = BTN_RISING;
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

