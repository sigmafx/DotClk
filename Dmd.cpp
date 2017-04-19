#include <Arduino.h>

#include "Dmd.h"

// Funtion Prototypes
extern "C"
{
static void isrDmd();
}

// One and only instance
Dmd dmd;

//----------------------
// Function: Constructor
//----------------------
Dmd::Dmd()
{
  // Set buffer pointers
  bufferActive = &buffer1;
  bufferInactive = &buffer2;
  bufferInUse = &buffer1;

  // Init other variables
  frame = 0;
  row = 0;
  active = false;
  brightness = 0;
  colour = 0x00;

  // Set interrupt priority
  timerDmd.priority(200);
}

//---------------------
// Function: Initialise
//---------------------
void Dmd::Initialise(int pinEN, int pinR1, int pinR2, int pinG1, int pinG2, int pinLA, int pinLB, int pinLC, int pinLD, int pinLT, int pinSK)
{
  // Screen pin outputs
  pinMode(pinEN, OUTPUT);
  pinMode(pinR1, OUTPUT);
  pinMode(pinR2, OUTPUT);
  pinMode(pinG1, OUTPUT);
  pinMode(pinG2, OUTPUT);
  pinMode(pinLA, OUTPUT);
  pinMode(pinLB, OUTPUT);
  pinMode(pinLC, OUTPUT);
  pinMode(pinLD, OUTPUT);
  pinMode(pinLT, OUTPUT);
  pinMode(pinSK, OUTPUT);

  //Set all to LOW
  digitalWrite(pinEN, LOW);
  digitalWrite(pinR1, LOW);
  digitalWrite(pinR2, LOW);
  digitalWrite(pinG1, LOW);
  digitalWrite(pinG2, LOW);
  digitalWrite(pinLA, LOW);
  digitalWrite(pinLB, LOW);
  digitalWrite(pinLC, LOW);
  digitalWrite(pinLD, LOW);
  digitalWrite(pinLT, LOW);
  digitalWrite(pinSK, LOW);

  // Store the pin assignments
  this->pinEN = pinEN;
  this->pinR1 = pinR1;
  this->pinR2 = pinR2;
  this->pinG1 = pinG1;
  this->pinG2 = pinG2;
  this->pinLA = pinLA;
  this->pinLB = pinLB;
  this->pinLC = pinLC;
  this->pinLD = pinLD;
  this->pinLT = pinLT;
  this->pinSK = pinSK;  
}

//----------------
// Function: Start
//----------------
void Dmd::Start()
{
  // Start the interrupts
  active = true;
  timerDmd.begin(isrDmd, 1);
}

//---------------
// Function: Stop
//---------------
void Dmd::Stop()
{
  // Stop the display
  active = false;
}

//-------------------
// Function: IsActive
//-------------------
bool Dmd::IsActive()
{
  return active;
}

//------------------------
// Function: SetBrightness
//------------------------
bool Dmd::SetBrightness(int set)
{
  bool ret ;

   // Check range
  if(set < 0 || set > 63)
  {
    // Out of range, return
    ret = false;
  }
  else
  {
    // Set brightness
    brightness = set;
    ret = true;
  }

  return ret;
}

//------------------------
// Function: GetBrightness
//------------------------
int Dmd::GetBrightness()
{
  return brightness;
}

bool Dmd::SetColour(byte set)
{
  bool ret ;

   // Check range
  if(set > 2)
  {
    // Out of range, return
    ret = false;
  }
  else
  {
    // Set brightness
    colour = set;
    ret = true;
  }

  return ret;  
}

byte Dmd::GetColour()
{
  return colour;  
}

//-------------------
// Function: SetFrame
//-------------------
void Dmd::SetFrame(DmdFrame& source)
{
  DmdFrameRaw *bufferTemp;

  // Copy the source frame to the inactive buffer
  memcpy(bufferInactive->dots, source.frame.dots, sizeof(bufferInactive->dots));

  // Switch inactive to active
  bufferTemp = bufferActive;  
  bufferActive = bufferInactive;
  bufferInactive = bufferTemp;
}

//-------------------
// Function: WaitSync
//-------------------
bool Dmd::WaitSync(uint32_t timeout)
{
  // Wait until the current Active buffer has started to be used
  while(bufferInUse == bufferInactive)
  {
    delayMicroseconds(1);
  }
  
  return true;
}

//-----------------
// Function: IsrDmd
//-----------------
void Dmd::IsrDmd()
{
  int isrDelay;

  // Disable the timer interrupt
  timerDmd.end();

  // Update a Dmd row
  isrDelay = UpdateRow();

  // Set up next interrupt
  if(active)
  {
    timerDmd.begin(isrDmd, isrDelay);
  }
  else
  {
    // Disable display
    digitalWriteFast(pinEN, HIGH);
  }
}

//---------------------
// Function: SetDmdType
//---------------------
void Dmd::SetDmdType(int dmdType)
{
  this->dmdType = dmdType;
}

//--------
//--------
// PRIVATE
//--------
//--------
//--------------------
// Function: UpdateRow
//--------------------
int Dmd::UpdateRow()
{
  int col;
  int colBit ;
  int ret ;
  
  // Process each column, 2 at a time
  for(col = 0; col < 64; col++)
  {
    // Disable the display at the appropriate column, thereby setting the brightness
    if(col >=  brightness)
      digitalWriteFast(pinEN, HIGH);
  
    for(colBit = 0; colBit < 2; colBit++)
    {
      byte  data1,
            data2;
      byte colourbits = colour + 1;

      // Extract the 2 data rows
      data1 = bufferInUse->dots[row][col];
      data1 = colBit & 1 ? (data1 >> 4) : (data1 & 0x0F);
      data1 &= (1 << frame);

      data2 = bufferInUse->dots[row + 16][col];
      data2 = colBit & 1 ? ( data2 >> 4) : (data2 & 0x0F);
      data2 &= (1 << frame);

      if(dmdType == 1)
      {
        data1 = !data1;
        data2 = !data2;
      }

      // Clock LOW
      digitalWriteFast(pinSK, LOW);

      // Set data
      // Red
      digitalWriteFast(pinR1, colourbits & 0x01 ? data1 : 0);
      digitalWriteFast(pinR2, colourbits & 0x01 ? data2 : 0);
      // Green
      digitalWriteFast(pinG1, (colourbits & 0x02) ? data1 : 0);
      digitalWriteFast(pinG2, (colourbits & 0x02) ? data2 : 0);
      // Blue
      //digitalWriteFast(pinB1, (colourbits & 0x04) ? data1 : 0);
      //digitalWriteFast(pinB2, (colourbits & 0x04) ? data2 : 0);

      // Clock HIGH
      digitalWriteFast(pinSK, HIGH);
    }
  }

  // Data latch LOW
  digitalWriteFast(pinLT, HIGH);

  // Set row address
  digitalWriteFast(pinLA, row & 0b0001);
  digitalWriteFast(pinLB, row & 0b0010);
  digitalWriteFast(pinLC, row & 0b0100);
  digitalWriteFast(pinLD, row & 0b1000);

  // Data latch HIGH
  digitalWriteFast(pinLT, LOW);

  // Enable display
  digitalWriteFast(pinEN, LOW);

  // Return value is row pause to create the dot intensities
  switch(frame)
  {
    default:
    case 0:
      ret = 1;
      break ;

    case 1:
      ret = 2;
      break ;

    case 2:
      ret = 30 ;
      break ;

    case 3:
      ret = 45;
      break ;
  }

  // Next row
  row++;
  if(row == 16)
  {
    // Finished a frame
    row = 0;

    // Next frame
    frame++;
    if(frame == 4)
    {
      // Finished a full frame
      frame = 0;

      // Select the in-use buffer
      bufferInUse = bufferActive;
    }
  }

  return ret;
}

//-----------------
// Function: isrDmd
//-----------------
static void isrDmd()
{
  // Call the class instance isr routine
  dmd.IsrDmd();
}

// End of file
