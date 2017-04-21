#ifndef __DOTMAP_H__
#define __DOTMAP_H__
#include <SD.h>

class Dotmap
{
  private:
    byte *dots;
    byte *mask;

    uint16_t width;
    uint16_t height;
    uint16_t widthBytesDots;
    uint16_t widthBytesMask;

    bool CheckRange(int x, int y);
    void Delete();
    
  public:
    Dotmap();
    ~Dotmap();

    Dotmap& operator=(const Dotmap& other);
    void Create(const int width, const int height);
    bool Create(File& fileDotmap);

    bool SetDotsFromRaw(const byte *data, uint16_t len);
    bool SetMaskFromRaw(const byte *data, uint16_t len);

    void SetDot(int x, int y, byte dot);
    byte GetDot(int x, int y);
    void SetMask(int x, int y, byte mask);
    byte GetMask(int x, int y);
    int GetWidth();
    int GetHeight();

    void Fill(byte dot);
    void Fill(int x, int y, int width, int height, byte dot);

    void ClearDots();
    void ClearMask();
};

#endif

