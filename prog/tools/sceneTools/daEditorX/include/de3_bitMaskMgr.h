//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <string.h>

class IBitMaskImageMgr
{
public:
  static constexpr unsigned HUID = 0xEA2182A6u; // IBitMaskImageMgr

  struct BitmapMask
  {
    int getWidth() const { return w; }
    int getHeight() const { return h; }

    int getBitsPerPixel() const { return bpp; }
    bool isImage32() const { return isImage; }

    // get pixel value family
    inline unsigned getImagePixel(int x, int y) const;
    inline unsigned char getMaskPixel1(int x, int y) const;
    inline unsigned char getMaskPixel2(int x, int y) const;
    inline unsigned char getMaskPixel4(int x, int y) const;
    inline unsigned char getMaskPixel8(int x, int y) const;
    inline unsigned short getMaskPixel16(int x, int y) const;
    inline unsigned getMaskPixel32(int x, int y) const;
    inline unsigned getMaskPixelAnyBpp(int x, int y) const;

    // set pixel value family
    inline void setImagePixel(int x, int y, unsigned c);
    inline void setMaskPixel1(int x, int y, unsigned char c);
    inline void setMaskPixel2(int x, int y, unsigned char c);
    inline void setMaskPixel4(int x, int y, unsigned char c);
    inline void setMaskPixel8(int x, int y, unsigned char c);
    inline void setMaskPixel16(int x, int y, unsigned short c);
    inline void setMaskPixel32(int x, int y, unsigned c);

  protected:
    int w, h, tileSz, wTiles;

    // bits per pixel can be 1, 2, 4, 8, 16, 32 for masks or 32 for images
    unsigned short int bmStride; // row stride (for bitmask only)
    unsigned short int bpp : 16, isImage : 1;
    void *bitsTile[32 * 32];

    const void *tileRd(int &x, int &y) const
    {
      if (!tileSz)
        return bitsTile[0];
      int idx = x / tileSz + (y / tileSz) * wTiles;
      x %= tileSz;
      y %= tileSz;
      return bitsTile[idx];
    }
    void *tileWr(int &x, int &y, bool force)
    {
      if (!tileSz)
        return bitsTile[0];
      int idx = x / tileSz + (y / tileSz) * wTiles;
      x %= tileSz;
      y %= tileSz;
      if (force && !bitsTile[idx])
      {
        int sz = !isImage ? tileSz * bmStride : tileSz * tileSz * bpp / 8;
        bitsTile[idx] = memalloc(sz, midmem);
        memset(bitsTile[idx], 0, sz);
      }
      return bitsTile[idx];
    }
  };

  virtual bool createImage32(BitmapMask &img, int w, int h) = 0;
  virtual bool createBitMask(BitmapMask &img, int w, int h, int bpp) = 0;
  virtual bool resampleBitMask(BitmapMask &img, int new_w, int new_h, int new_bpp) = 0;
  virtual void clearBitMask(BitmapMask &img) = 0;
  virtual void calcBitMaskMD5(BitmapMask &img, void *md5_state) = 0;
  virtual void destroyImage(BitmapMask &img) = 0;

  // load and save bitmask image
  //   filename construction rules:
  //     folder!=NULL :  $(Project)/$(folder)/$(image_name).tif
  //     folder==NULL :  $(Develop)/$(image_name).tif
  virtual bool loadImage(BitmapMask &img, const char *folder, const char *image_name) = 0;
  virtual bool saveImage(BitmapMask &img, const char *folder, const char *image_name) = 0;
  virtual bool checkBitMask(const char *folder, const char *image_name, int w, int h, int bpp) = 0;
  virtual bool getBitMaskProps(const char *folder, const char *image_name, int &out_w, int &out_h, int &out_bpp) = 0;
};


//
// IBitMaskImageMgr::BitmapMask inline methods implementation
//
inline unsigned IBitMaskImageMgr::BitmapMask::getImagePixel(int x, int y) const
{
  const void *bits = tileRd(x, y);
  if (!bits)
    return 0;
  // valid for image32 only!
  return ((unsigned *)bits)[y * (tileSz ? tileSz : w) + x];
}
inline unsigned char IBitMaskImageMgr::BitmapMask::getMaskPixel1(int x, int y) const
{
  const void *bits = tileRd(x, y);
  if (!bits)
    return 0;
  // valid for 1-bit bitmasks only!
  unsigned char byte = ((unsigned char *)bits)[y * bmStride + (x >> 3)];
  return (byte << (x & 7)) & 0x80;
}
inline unsigned char IBitMaskImageMgr::BitmapMask::getMaskPixel2(int x, int y) const
{
  const void *bits = tileRd(x, y);
  if (!bits)
    return 0;
  // valid for 2-bit bitmasks only!
  unsigned char byte = ((unsigned char *)bits)[y * bmStride + (x >> 2)];
  return (byte << ((x & 3) * 2)) & 0xC0;
}
inline unsigned char IBitMaskImageMgr::BitmapMask::getMaskPixel4(int x, int y) const
{
  const void *bits = tileRd(x, y);
  if (!bits)
    return 0;
  // valid for 4-bit bitmasks only!
  unsigned char byte = ((unsigned char *)bits)[y * bmStride + (x >> 1)];
  return (byte << ((x & 1) * 4)) & 0xF0;
}
inline unsigned char IBitMaskImageMgr::BitmapMask::getMaskPixel8(int x, int y) const
{
  const void *bits = tileRd(x, y);
  if (!bits)
    return 0;
  // valid for 8-bit bitmasks only!
  return ((unsigned char *)bits)[y * bmStride + x];
}
inline unsigned short IBitMaskImageMgr::BitmapMask::getMaskPixel16(int x, int y) const
{
  const void *bits = tileRd(x, y);
  if (!bits)
    return 0;
  // valid for 16-bit bitmasks only!
  return *(unsigned short *)(y * bmStride + x * 2 + (char *)bits);
}
inline unsigned IBitMaskImageMgr::BitmapMask::getMaskPixel32(int x, int y) const
{
  const void *bits = tileRd(x, y);
  if (!bits)
    return 0;
  // valid for 32-bit bitmasks only!
  return *(unsigned *)(y * bmStride + x * 4 + (char *)bits);
}
inline unsigned IBitMaskImageMgr::BitmapMask::getMaskPixelAnyBpp(int x, int y) const
{
  if (bpp == 1)
    return getMaskPixel1(x, y);
  if (bpp == 2)
    return getMaskPixel2(x, y);
  if (bpp == 4)
    return getMaskPixel4(x, y);
  if (bpp == 8)
    return getMaskPixel8(x, y);
  if (bpp == 16)
    return getMaskPixel16(x, y);
  if (bpp == 32 && !isImage)
    return getMaskPixel32(x, y);
  return 0;
}

inline void IBitMaskImageMgr::BitmapMask::setImagePixel(int x, int y, unsigned c)
{
  void *bits = tileWr(x, y, c != 0);
  if (!bits)
    return;
  // valid for image32 only!
  ((unsigned *)bits)[y * (tileSz ? tileSz : w) + x] = c;
}
inline void IBitMaskImageMgr::BitmapMask::setMaskPixel1(int x, int y, unsigned char c)
{
  void *bits = tileWr(x, y, c != 0);
  if (!bits)
    return;
  // valid for 1-bit bitmasks only!
  unsigned char &byte = ((unsigned char *)bits)[y * bmStride + (x >> 3)];
  int shift = (x & 7);
  byte &= ~(0x80 >> shift);
  byte |= (c & 0x80) >> shift;
}
inline void IBitMaskImageMgr::BitmapMask::setMaskPixel2(int x, int y, unsigned char c)
{
  void *bits = tileWr(x, y, c != 0);
  if (!bits)
    return;
  // valid for 2-bit bitmasks only!
  unsigned char &byte = ((unsigned char *)bits)[y * bmStride + (x >> 2)];
  int shift = (x & 3) * 2;
  byte &= ~(0xC0 >> shift);
  byte |= (c & 0xC0) >> shift;
}
inline void IBitMaskImageMgr::BitmapMask::setMaskPixel4(int x, int y, unsigned char c)
{
  void *bits = tileWr(x, y, c != 0);
  if (!bits)
    return;
  // valid for 4-bit bitmasks only!
  unsigned char &byte = ((unsigned char *)bits)[y * bmStride + (x >> 1)];
  int shift = (x & 1) * 4;
  byte &= ~(0xF0 >> shift);
  byte |= (c & 0xF0) >> shift;
}
inline void IBitMaskImageMgr::BitmapMask::setMaskPixel8(int x, int y, unsigned char c)
{
  void *bits = tileWr(x, y, c != 0);
  if (!bits)
    return;
  // valid for 8-bit bitmasks only!
  ((unsigned char *)bits)[y * bmStride + x] = c;
}
inline void IBitMaskImageMgr::BitmapMask::setMaskPixel16(int x, int y, unsigned short c)
{
  void *bits = tileWr(x, y, c != 0);
  if (!bits)
    return;
  // valid for 16-bit bitmasks only!
  *(unsigned short *)(y * bmStride + x * 2 + (char *)bits) = c;
}
inline void IBitMaskImageMgr::BitmapMask::setMaskPixel32(int x, int y, unsigned c)
{
  void *bits = tileWr(x, y, c != 0);
  if (!bits)
    return;
  // valid for 32-bit bitmasks only!
  *(unsigned *)(y * bmStride + x * 4 + (char *)bits) = c;
}
