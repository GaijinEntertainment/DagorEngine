/*
 * DaEditor3
 * Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
 * Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
 */

#ifndef _DAGORTECH_TOOLS_DAEDITOR3__DE3_BITMASKMGR_H_
#define _DAGORTECH_TOOLS_DAEDITOR3__DE3_BITMASKMGR_H_
#pragma once


class IBitMaskImageMgr
{
public:
  struct BitmapMask
  {
    int getWidth() const { return w; }
    int getHeight() const { return h; }
    void *getBits() const { return bits; }

    int getBitsPerPixel() const { return bpp; }
    bool isImage32() const { return bpp == 32; }
    unsigned getBitsSize() const { return w * h * bpp / 8; }

    // get pixel value family
    inline unsigned getImagePixel(int x, int y) const;
    inline unsigned char getMaskPixel1(int x, int y) const;
    inline unsigned char getMaskPixel2(int x, int y) const;
    inline unsigned char getMaskPixel4(int x, int y) const;
    inline unsigned char getMaskPixel8(int x, int y) const;

    // set pixel value family
    inline void setImagePixel(int x, int y, unsigned c);
    inline void setMaskPixel1(int x, int y, unsigned char c);
    inline void setMaskPixel2(int x, int y, unsigned char c);
    inline void setMaskPixel4(int x, int y, unsigned char c);
    inline void setMaskPixel8(int x, int y, unsigned char c);

    inline unsigned char getMaxMaskPixelValue() const { return 0xFF << (8 - bpp); }

  protected:
    void *bits;
    int w, h;

    // bits per pixel can be 1, 2, 4, 8 for masks or 32 for images
    short int bmStride; // row stride (for bitmask only)
    short int bpp;
  };

  virtual bool createImage32(BitmapMask &img, int w, int h) = 0;
  virtual bool createBitMask(BitmapMask &img, int w, int h, int bpp) = 0;
  virtual void destroyImage(BitmapMask &img) = 0;
  virtual bool saveImage(BitmapMask &img, const char *fname) = 0;
};


//
// IBitMaskImageMgr::BitmapMask inline methods implementation
//
inline unsigned IBitMaskImageMgr::BitmapMask::getImagePixel(int x, int y) const
{
  // valid for image32 only!
  return ((unsigned *)bits)[y * w + x];
}
inline unsigned char IBitMaskImageMgr::BitmapMask::getMaskPixel1(int x, int y) const
{
  // valid for 1-bit bitmasks only!
  unsigned char byte = ((unsigned char *)bits)[y * bmStride + (x >> 3)];
  return (byte << (x & 7)) & 0x80;
}
inline unsigned char IBitMaskImageMgr::BitmapMask::getMaskPixel2(int x, int y) const
{
  // valid for 2-bit bitmasks only!
  unsigned char byte = ((unsigned char *)bits)[y * bmStride + (x >> 2)];
  return (byte << ((x & 3) * 2)) & 0xC0;
}
inline unsigned char IBitMaskImageMgr::BitmapMask::getMaskPixel4(int x, int y) const
{
  // valid for 4-bit bitmasks only!
  unsigned char byte = ((unsigned char *)bits)[y * bmStride + (x >> 1)];
  return (byte << ((x & 1) * 4)) & 0xF0;
}
inline unsigned char IBitMaskImageMgr::BitmapMask::getMaskPixel8(int x, int y) const
{
  // valid for 8-bit bitmasks only!
  return ((unsigned char *)bits)[y * bmStride + x];
}

inline void IBitMaskImageMgr::BitmapMask::setImagePixel(int x, int y, unsigned c)
{
  // valid for image32 only!
  ((unsigned *)bits)[y * w + x] = c;
}
inline void IBitMaskImageMgr::BitmapMask::setMaskPixel1(int x, int y, unsigned char c)
{
  // valid for 1-bit bitmasks only!
  unsigned char &byte = ((unsigned char *)bits)[y * bmStride + (x >> 3)];
  byte &= ~(0x80 >> (x & 7));
  byte |= (c & 0x80) >> (x & 7);
}
inline void IBitMaskImageMgr::BitmapMask::setMaskPixel2(int x, int y, unsigned char c)
{
  // valid for 2-bit bitmasks only!
  unsigned char &byte = ((unsigned char *)bits)[y * bmStride + (x >> 2)];
  byte &= ~(0xC0 >> (x & 3));
  byte |= (c & 0xC0) >> (x & 3);
}
inline void IBitMaskImageMgr::BitmapMask::setMaskPixel4(int x, int y, unsigned char c)
{
  // valid for 4-bit bitmasks only!
  unsigned char &byte = ((unsigned char *)bits)[y * bmStride + (x >> 1)];
  byte &= ~(0xF0 >> (x & 1));
  byte |= (c & 0xF0) >> (x & 1);
}
inline void IBitMaskImageMgr::BitmapMask::setMaskPixel8(int x, int y, unsigned char c)
{
  // valid for 8-bit bitmasks only!
  ((unsigned char *)bits)[y * bmStride + x] = c;
}

IBitMaskImageMgr *get_tiff_bit_mask_image_mgr();

#endif
