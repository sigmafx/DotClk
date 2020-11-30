#include <IntervalTimer.h>
#include <Arduino.h>

#include "Globals.h"

// Funtion Prototypes
extern "C"
{
static void isrDmdType0();
static void isrDmdType1();
}

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
  brightness = 0;
  colour = 0x00;
  
  // Set interrupt priority
  timerDmd.priority(200);
}

//---------------------
// Function: Initialise
//---------------------
void Dmd::Initialise(int pinEN, int pinR1, int pinR2, int pinG1, int pinG2, int pinB1, int pinB2, int pinLA, int pinLB, int pinLC, int pinLD, int pinLT, int pinSK)
{
  // Screen pin outputs
  pinMode(pinEN, OUTPUT);
  pinMode(pinR1, OUTPUT);
  pinMode(pinR2, OUTPUT);
  pinMode(pinG1, OUTPUT);
  pinMode(pinG2, OUTPUT);
  #ifdef HUB75
  pinMode(pinB1, OUTPUT);
  pinMode(pinB2, OUTPUT);
  #endif
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
  #ifdef HUB75
  digitalWrite(pinB1, LOW);
  digitalWrite(pinB2, LOW);
  #endif
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
  this->pinB1 = pinB1;
  this->pinB2 = pinB2;
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
  timerDmd.begin(dmdType == 1 ? isrDmdType1 : isrDmdType0, 1);
}

//---------------
// Function: Stop
//---------------
void Dmd::Stop()
{
  // Stop the interrupts
  timerDmd.end();

  // Disable display
  digitalWriteFast(pinEN, HIGH);
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

//--------------------
// Function: SetColour
//--------------------
bool Dmd::SetColour(byte set)
{
  bool ret ;

   // Check range
  if(set > 7)
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

//--------------------
// Function: GetColour
//--------------------
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

//------------------
// Function: IsrDmd0
//------------------
void Dmd::IsrDmdType0()
{
  int isrDelay;

  // Disable the timer interrupt
  timerDmd.end();

  // Update a Dmd row
  isrDelay = UpdateRowType0();

  // Set up next interrupt
  timerDmd.begin(isrDmdType0, isrDelay);
}

//------------------
// Function: IsrDmd1
//------------------
void Dmd::IsrDmdType1()
{
  int isrDelay;

  // Disable the timer interrupt
  timerDmd.end();

  // Update a Dmd row
  isrDelay = UpdateRowType1();

  // Set up next interrupt
  timerDmd.begin(isrDmdType1, isrDelay);
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
//-------------------------
// Function: UpdateRowType0
//-------------------------
int Dmd::UpdateRowType0()
{
  int ret ;
  int col;
  byte *rowTop, *rowBottom ;
  byte mask, maskR, maskG, maskB;

  rowTop = bufferInUse->dots[row];
  rowBottom = bufferInUse->dots[row + 16];

  mask = 0x01 << frame;
  maskR = (colour + 1) & 0x01 ? 0xFF : 0x00;
  maskG = (colour + 1) & 0x02 ? 0xFF : 0x00;
  maskB = (colour + 1) & 0x04 ? 0xFF : 0x00;
  
  // Process each column, 2 at a time
  for(col = 0; col < 128; )
  {
    byte  data1,
          data2;

    // Disable the display at the appropriate column, thereby setting the brightness
    if(col ==  brightness)
    {
      digitalWriteFast(pinEN, HIGH);
    }

    // Extract the 2 data rows
    data1 = *rowTop & mask;
    data2 = *rowBottom & mask;
    
    // Clock LOW
    digitalWriteFast(pinSK, LOW);

    // Set data
    // Red
    digitalWriteFast(pinR1, data1 & maskR);
    digitalWriteFast(pinR2, data2 & maskR);
    // Green
    digitalWriteFast(pinG1, data1 & maskG);
    digitalWriteFast(pinG2, data2 & maskG);

    #ifdef HUB75
    // Blue
    digitalWriteFast(pinB1, data1 & maskB);
    digitalWriteFast(pinB2, data2 & maskB);
    #else
    // Stop compile warning
    maskB = maskB;
    #endif

    // Clock HIGH
    digitalWriteFast(pinSK, HIGH);

    rowTop++;
    rowBottom++;
    col++;
    
    // Disable the display at the appropriate column, thereby setting the brightness
    if(col ==  brightness)
    {
      digitalWriteFast(pinEN, HIGH);
    }

    // Extract the 2 data rows
    data1 = *rowTop & mask;
    data2 = *rowBottom & mask;
    
    // Clock LOW
    digitalWriteFast(pinSK, LOW);

    // Set data
    // Red
    digitalWriteFast(pinR1, data1 & maskR);
    digitalWriteFast(pinR2, data2 & maskR);
    // Green
    digitalWriteFast(pinG1, data1 & maskG);
    digitalWriteFast(pinG2, data2 & maskG);

    #ifdef HUB75
    // Blue
    digitalWriteFast(pinB1, data1 & maskB);
    digitalWriteFast(pinB2, data2 & maskB);
    #endif

    // Clock HIGH
    digitalWriteFast(pinSK, HIGH);

    rowTop++;
    rowBottom++;
    col++;
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
  int rets[] = { 1, 2, 30, 45};
  ret = rets[frame];
  
  // Next row
  row++;
  if(row & 0x10)
  {
    // Finished a frame
    row = 0;

    // Next frame
    frame++;
    if(frame & 0x04)
    {
      // Finished a full frame
      frame = 0;

      // Select the in-use buffer
      bufferInUse = bufferActive;
    }
  }

  return ret;
}

//-------------------------
// Function: UpdateRowType1
//-------------------------
int Dmd::UpdateRowType1()
{
  int ret ;
  int col;
  byte *rowTop, *rowBottom ;
  byte mask, maskR, maskG, maskB;

  rowTop = bufferInUse->dots[row];
  rowBottom = bufferInUse->dots[row + 16];

  mask = 0x01 << frame;
  maskR = (colour + 1) & 0x01 ? 0xFF : 0x00;
  maskG = (colour + 1) & 0x02 ? 0xFF : 0x00;
  maskB = (colour + 1) & 0x04 ? 0xFF : 0x00;
  
  // Process each column, 2 at a time
  // Code has been flattened out to improve performance
  for(col = 0; col < 128; )
  {
    byte  data1,
          data2;

    // Disable the display at the appropriate column, thereby setting the brightness
    if(col ==  brightness)
    {
      digitalWriteFast(pinEN, HIGH);
    }

    // Extract the 2 data rows
    data1 = *rowTop & mask;
    data2 = *rowBottom & mask;

    data1 = !data1;
    data2 = !data2;
    
    // Clock LOW
    digitalWriteFast(pinSK, LOW);

    // Set data
    // Red
    digitalWriteFast(pinR1, data1 & maskR);
    digitalWriteFast(pinR2, data2 & maskR);
    // Green
    digitalWriteFast(pinG1, data1 & maskG);
    digitalWriteFast(pinG2, data2 & maskG);

    #ifdef HUB75
    // Blue
    digitalWriteFast(pinB1, data1 & maskB);
    digitalWriteFast(pinB2, data2 & maskB);
    #else
    // Stop compile warning
    maskB = maskB;
    #endif

    // Clock HIGH
    digitalWriteFast(pinSK, HIGH);

    rowTop++;
    rowBottom++;
    col++;

    // Disable the display at the appropriate column, thereby setting the brightness
    if(col ==  brightness)
    {
      digitalWriteFast(pinEN, HIGH);
    }

    // Extract the 2 data rows
    data1 = *rowTop & mask;
    data2 = *rowBottom & mask;

    data1 = !data1;
    data2 = !data2;
    
    // Clock LOW
    digitalWriteFast(pinSK, LOW);

    // Set data
    // Red
    digitalWriteFast(pinR1, data1 & maskR);
    digitalWriteFast(pinR2, data2 & maskR);
    // Green
    digitalWriteFast(pinG1, data1 & maskG);
    digitalWriteFast(pinG2, data2 & maskG);

    #ifdef HUB75
    // Blue
    digitalWriteFast(pinB1, data1 & maskB);
    digitalWriteFast(pinB2, data2 & maskB);
    #endif

    // Clock HIGH
    digitalWriteFast(pinSK, HIGH);

    rowTop++;
    rowBottom++;
    col++;
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
  int rets[] = { 1, 2, 30, 45};
  ret = rets[frame];

  // Next row
  row++;
  if(row & 0x10)
  {
    // Finished a frame
    row = 0;

    // Next frame
    frame++;
    if(frame & 0x04)
    {
      // Finished a full frame
      frame = 0;

      // Select the in-use buffer
      bufferInUse = bufferActive;
    }
  }

  return ret;
}

//------------------
// Function: isrDmd0
//------------------
static void isrDmdType0()
{
  // Call the class instance isr routine
  noInterrupts();
  dmd.IsrDmdType0();
  interrupts();
}

//------------------
// Function: isrDmd1
//------------------
static void isrDmdType1()
{
  // Call the class instance isr routine
  noInterrupts();
  dmd.IsrDmdType1();
  interrupts();
}

// End of file
