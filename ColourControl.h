#ifndef __COLOURCONTROL_H__

#include <Arduino.h>
#include <IntervalTimer.h>

class ColourControl
{
public:
    ColourControl();
    void SetColour(byte colour);

private:
    byte m_colour;
    IntervalTimer m_timerColour;
};

#endif
