#include "Globals.h"

static void isrTimer()
{
  static byte colour = 0x00;

  colour++;
  if(colour > 0x06)
  {
    colour = 0x00;
  }

  dmd.SetColour(colour);
}

ColourControl::ColourControl()
{
    m_colour = 0x00;
}

void ColourControl::SetColour(byte colour)
{
  if(colour != m_colour)
  {
    m_colour = colour;
    if(m_colour == 0x07)
    {
      // Start timer
      m_timerColour.begin(isrTimer, 60000000);
    }
    else
    {
      // Stop timer
      m_timerColour.end();
      dmd.SetColour(m_colour);
    }
  }
}
