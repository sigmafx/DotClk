#include <Arduino.h>

#include "Dotmap.h"

//-------
//-------
// PUBLIC
//-------
//-------
//-----------------------
// Function: Constructor 
//-----------------------
Dotmap::Dotmap()
{
  dots = NULL;
  mask = NULL;

  width = 0;
  height = 0;
  widthBytesDots = 0;
  widthBytesMask = 0;
}

//---------------------
// Function: Destructor
//---------------------
Dotmap::~Dotmap()
{
  Delete();
}

//---------------------
// Function: operator =
//---------------------
Dotmap& Dotmap::operator=(const Dotmap& rhs)
{
  Create(rhs.width, rhs.height);
  
  if(rhs.dots == NULL)
  {
    dots = NULL;
  }
  else
  {
    SetDotsFromRaw(rhs.dots, widthBytesDots * height * sizeof(byte));    
  }

  if(rhs.mask == NULL)
  {
    mask = NULL;
  }
  else
  {
    SetMaskFromRaw(rhs.mask, widthBytesMask * height * sizeof(byte));
  }

  return *this;
}

//-----------------
// Function: Create
//-----------------
void Dotmap::Create(int width, int height)
{
  // Ensure any previous data has been freed
  Delete();
  
  // Determine byte widths of the dot and mask arrays
  widthBytesDots = (width / 2) + (width % 2 ? 1 : 0);
  widthBytesMask = (width / 8) + (width % 8 ? 1 : 0);

  this->width = width;
  this->height = height;

  // Allocate memory for the arrays
  dots = new byte[widthBytesDots * height];
  mask = new byte[widthBytesMask * height];

  ClearDots();
  ClearMask();
}

//-----------------
// Function: Create
//-----------------
void Dotmap::Create(File& fileDotmap)
{
  uint16_t dotsWidth;
  uint16_t dotsHeight;
  uint16_t dotsBpp;
  uint16_t hasMask;
//  uint16_t maskWidth;
//  uint16_t maskHeight;
//  uint16_t maskBpp;
  
  // Read the dotmap item header
  fileDotmap.read(&dotsWidth, sizeof(dotsWidth));
  fileDotmap.read(&dotsHeight, sizeof(dotsHeight));
  fileDotmap.read(&dotsBpp, sizeof(dotsBpp));
  fileDotmap.read(&hasMask, sizeof(hasMask));
//  fileDotmap.read(&maskWidth, sizeof(maskWidth));
//  fileDotmap.read(&maskHeight, sizeof(maskHeight));
//  fileDotmap.read(&maskBpp, sizeof(maskBpp));

  Create(dotsWidth, dotsHeight);
  
  // Read the dots data
  fileDotmap.read(dots, widthBytesDots * height * sizeof(byte));

  // Read the optional mask data
  if(hasMask)
  {
    fileDotmap.read(mask, widthBytesMask * height * sizeof(byte));
  }
}

//-------------------------
// Function: SetDotsFromRaw
//-------------------------
bool Dotmap::SetDotsFromRaw(const byte *data, uint16_t len)
{
  bool ret = false;

  if(len != (widthBytesDots * height))
  {
    // ERROR Incorrect buffer size
    goto ERROR_EXIT;
  }
  
  // Copy the data
  memcpy(dots, data, len);  

ERROR_EXIT:
  return ret;
}

//-------------------------
// Function: SetMaskFromRaw
//-------------------------
bool Dotmap::SetMaskFromRaw(const byte *data, uint16_t len)
{
  bool ret = false;

  if(len != (widthBytesMask * height))
  {
    // ERROR Incorrect buffer size
    Serial.println("Error in mask from raw");
    goto ERROR_EXIT;
  }
  
  // Copy the data
  memcpy(mask, data, len);  

ERROR_EXIT:
  return ret;
}

//-----------------
// Function: SetDot
//-----------------
void Dotmap::SetDot(int x, int y, byte dotSet)
{
  int   idxOffset ;
  byte  set ;

  if(!CheckRange(x, y))
  {
    // Out of range, return
    return;
  }

  // Determine byte offset into bitmap
  idxOffset = (x / 2) + (y * widthBytesDots);

  // Retrieve current value
  set = dots[idxOffset];

  // Set new value
  if(x % 2)
  {
    // Odd numbered column
    set &= 0x0F;
    set |= (dotSet << 4);
  }
  else
  {
    // Even numbered column
    set &= 0xF0;
    set |= (dotSet & 0x0F);
  }    

  // Set back into dotmap
  dots[idxOffset] = set;
}

//-----------------
// Function: GetDot
//-----------------
byte Dotmap::GetDot(int x, int y)
{
  int   idxOffset ;
  byte  get ;

  if(!CheckRange(x, y))
  {
    // Out of range, return
    return 0x00;
  }

  // Determine byte offset into bitmap
  idxOffset = (x / 2) + (y * widthBytesDots);

  // Retrieve current value
  get = dots[idxOffset];

  // Get new value
  if(x % 2)
  {
    // Odd numbered column
    get >>= 4;
  }
  else
  {
    // Even numbered column
    get &= 0x0F;
  }    

  return get;
}

//------------------
// Function: SetMask
//------------------
void Dotmap::SetMask(int x, int y, byte maskSet)
{
  int   idxOffset ;
  byte  set ;

  if(!CheckRange(x, y))
  {
    // Out of range, return
    return;
  }

  // Determine byte offset into bitmap
  idxOffset = (x / 8) + (y * widthBytesMask);

  // Retrieve current value
  set = mask[idxOffset];

  // Set new value
  if(maskSet)
  {
    set |= (1 << (x % 8));
  }
  else
  {
    set &= ~(1 << (x % 8));
  }

  // Set backj into dotmap
  mask[idxOffset] = set;
}

//------------------
// Function: GetMask
//------------------
byte Dotmap::GetMask(int x, int y)
{
  int   idxOffset ;
  byte  get ;

  if(!CheckRange(x, y))
  {
    // Out of range, return
    return 0x01;
  }

  if(mask == NULL)
  {
    return 0x01;
  }

  // Determine byte offset into bitmap
  idxOffset = (x / 8) + (y * widthBytesMask);

  // Retrieve current value
  get = mask[idxOffset] & (1 << (x % 8));

  return get ? 1 : 0;
}

//-------------------
// Function: GetWidth
//-------------------
int Dotmap::GetWidth()
{
  return width;
}

//--------------------
// Function: GetHeight
//--------------------
int Dotmap::GetHeight()
{
  return height;
}

//---------------
// Function: Fill
//---------------
void Dotmap::Fill(byte dot)
{
  memset(dots, (dot & 0x0F) | (dot << 4), widthBytesDots * height * sizeof(byte));
}

//---------------
// Function: Fill
//---------------
void Dotmap::Fill(int x, int y, int width, int height, byte dot)
{

}

//--------------------
// Function: ClearDots
//--------------------
void Dotmap::ClearDots()
{
  memset(dots, 0x00, widthBytesDots * height * sizeof(byte));  
}

//--------------------
// Function: ClearMask
//--------------------
void Dotmap::ClearMask()
{
  memset(mask, 0x00, widthBytesMask * height * sizeof(byte));  
}

//--------
//--------
// PRIVATE
//--------
//--------
//---------------------
// Function: CheckRange
//---------------------
bool Dotmap::CheckRange(int x, int y)
{
  if(x < 0 || y < 0 || x >= width || y >= height)
  {
    // Out of range, return
    return false;
  }
  else
  {
    return true;
  }
}

//-----------------
// Function: Delete
//-----------------
void Dotmap::Delete()
{
  if(dots != NULL)
  {
    delete[] dots;
  }

  if(mask != NULL)
  {
    delete[] mask;
  }
}
// End of file

