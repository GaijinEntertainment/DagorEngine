//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <string.h>
#include <util/dag_hierBitMemPool.h>
#include <debug/dag_assert.h>


//! template for 2D bit map of constant size
template <int N_POW2>
struct ConstSizeBitMap2d
{
  static constexpr int SZ = 1 << N_POW2;
  static constexpr int SHBITS = N_POW2;
  static constexpr int FULL_SZ = SZ;
  static constexpr int FULL_SHBITS = SHBITS;

  inline void ctor() { reset(); }
  inline void ctor(const ConstSizeBitMap2d<N_POW2> &a) { memcpy(bits, a.bits, sizeof(bits)); }
  inline void dtor() {}

  inline void reset() { memset(bits, 0, sizeof(bits)); }
  inline void setAll() { memset(bits, 0xFF, sizeof(bits)); }

  inline void set(int x, int y)
  {
    int idx = (y << SHBITS) + x;
    bits[idx >> 5] |= (1 << (idx & 0x1F));
  }
  inline void clr(int x, int y)
  {
    int idx = (y << SHBITS) + x;
    bits[idx >> 5] &= ~(1 << (idx & 0x1F));
  }

  inline int get(int x, int y) const { return getByIdx((y << SHBITS) + x); }
  inline int getByIdx(int idx) const { return bits[idx >> 5] & (1 << (idx & 0x1F)); }
  inline const unsigned *getBits() const { return bits; }

  int getW() const { return SZ; }
  int getH() const { return SZ; }

  bool compare(const ConstSizeBitMap2d<N_POW2> &bm, int ox, int oy, int &minx, int &miny, int &maxx, int &maxy)
  {
    for (const unsigned *b = bits, *b_end = b + (SZ * SZ + 31) / 32, *b2 = bm.bits; b < b_end; b++, b2++)
      if (*b != *b2)
      {
        if (ox < minx)
          minx = ox;
        if (oy < miny)
          miny = oy;
        if (ox + SZ - 1 > maxx)
          maxx = ox + SZ - 1;
        if (oy + SZ - 1 > maxy)
          maxy = oy + SZ - 1;
        return true;
      }
    return false;
  }

  void addSetBitsMinMax(int ox, int oy, int &minx, int &miny, int &maxx, int &maxy)
  {
    for (unsigned int *b = bits, *b_end = b + (SZ * SZ + 31) / 32; b < b_end; b++)
      if (*b)
      {
        if (ox < minx)
          minx = ox;
        if (oy < miny)
          miny = oy;
        if (ox + SZ - 1 > maxx)
          maxx = ox + SZ - 1;
        if (oy + SZ - 1 > maxy)
          maxy = oy + SZ - 1;
        return;
      }
  }

protected:
  unsigned int bits[(SZ * SZ + 31) / 32];
};


//! template for hierarchical bit map of constant size
template <int N_POW2, class BitMap>
struct HierConstSizeBitMap2d
{
  static constexpr int SZ = 1 << N_POW2;
  static constexpr int SHBITS = N_POW2;
  static constexpr int FULL_SZ = SZ * BitMap::FULL_SZ;
  static constexpr int FULL_SHBITS = SHBITS + BitMap::FULL_SHBITS;

  HierConstSizeBitMap2d() { ctor(); }
  ~HierConstSizeBitMap2d() { reset(); }
  HierConstSizeBitMap2d(const HierConstSizeBitMap2d<N_POW2, BitMap> &a) { ctor(a); }

  HierConstSizeBitMap2d<N_POW2, BitMap> &operator=(const HierConstSizeBitMap2d<N_POW2, BitMap> &a)
  {
    reset();
    ctor(a);
    return *this;
  }

  inline void ctor() { memset(bitmaps, 0, sizeof(bitmaps)); }
  inline void ctor(const HierConstSizeBitMap2d<N_POW2, BitMap> &a)
  {
    for (int i = 0; i < SZ * SZ; i++)
      bitmaps[i] = a.bitmaps[i] ? hb_new_copy(*a.bitmaps[i]) : NULL;
  }
  inline void dtor() { reset(); }

  inline void reset()
  {
    for (int i = SZ * SZ - 1; i >= 0; i--)
      if (bitmaps[i])
      {
        hb_delete<BitMap>(bitmaps[i], 1);
        bitmaps[i] = NULL;
      }
  }
  inline void setAll()
  {
    for (int i = SZ * SZ - 1; i >= 0; i--)
    {
      if (!bitmaps[i])
        bitmaps[i] = hb_new<BitMap>(1);
      bitmaps[i]->setAll();
    }
  }

  inline void set(int x, int y)
  {
    int idx = ((y >> BitMap::FULL_SHBITS) << SHBITS) + (x >> BitMap::FULL_SHBITS);
    if (!bitmaps[idx])
      bitmaps[idx] = hb_new<BitMap>(1);
    bitmaps[idx]->set(x & (BitMap::FULL_SZ - 1), y & (BitMap::FULL_SZ - 1));
  }
  inline void clr(int x, int y)
  {
    int idx = ((y >> BitMap::FULL_SHBITS) << SHBITS) + (x >> BitMap::FULL_SHBITS);
    if (bitmaps[idx])
      bitmaps[idx]->clr(x & (BitMap::FULL_SZ - 1), y & (BitMap::FULL_SZ - 1));
  }

  inline int get(int x, int y) const
  {
    int idx = ((y >> BitMap::FULL_SHBITS) << SHBITS) + (x >> BitMap::FULL_SHBITS);
    if (bitmaps[idx])
      return bitmaps[idx]->get(x & (BitMap::FULL_SZ - 1), y & (BitMap::FULL_SZ - 1));
    return 0;
  }

  int getW() const { return SZ * BitMap::FULL_SZ; }
  int getH() const { return SZ * BitMap::FULL_SZ; }

  bool compare(const HierConstSizeBitMap2d<N_POW2, BitMap> &bm, int ox, int oy, int &minx, int &miny, int &maxx, int &maxy)
  {
    bool differs = false;

    BitMap *const __restrict *b1 = bitmaps;
    BitMap *const __restrict *b2 = bm.bitmaps;

    for (int y = 0; y < SZ; y++, oy += BitMap::FULL_SZ)
      for (int x = 0; x < SZ; x++, b1++, b2++)
        if (b1[0] && b2[0])
          differs |= b1[0]->compare(*b2[0], ox + x * BitMap::FULL_SZ, oy, minx, miny, maxx, maxy);
        else if (b1[0] && !b2[0])
        {
          b1[0]->addSetBitsMinMax(ox + x * BitMap::FULL_SZ, oy, minx, miny, maxx, maxy);
          differs = true;
        }
        else if (!b1[0] && b2[0])
        {
          b2[0]->addSetBitsMinMax(ox + x * BitMap::FULL_SZ, oy, minx, miny, maxx, maxy);
          differs = true;
        }

    return differs;
  }

  void addSetBitsMinMax(int ox, int oy, int &minx, int &miny, int &maxx, int &maxy)
  {
    BitMap *const __restrict *b = bitmaps;

    for (int y = 0; y < SZ; y++, oy += BitMap::FULL_SZ)
      for (int x = 0; x < SZ; x++, b++)
        if (b[0])
          b[0]->addSetBitsMinMax(ox + x * BitMap::FULL_SZ, oy, minx, miny, maxx, maxy);
  }

protected:
  BitMap *bitmaps[SZ * SZ];
};

//! template for hierarchical bit map of variable size
template <class BitMap>
struct HierBitMap2d
{
  HierBitMap2d()
  {
    w = 0;
    h = 0;
    bitmaps = NULL;
  }
  HierBitMap2d(int _w, int _h) { allocMem(_w, _h); }
  HierBitMap2d(const HierBitMap2d<BitMap> &a)
  {
    w = h = 0;
    bitmaps = NULL;
    operator=(a);
  }
  ~HierBitMap2d()
  {
    reset();
    hb_free<BitMap *>(bitmaps, w * h);
  }

  HierBitMap2d<BitMap> &operator=(const HierBitMap2d<BitMap> &a)
  {
    resize(a.getW(), a.getH());
    for (int i = 0; i < w * h; i++)
      bitmaps[i] = a.bitmaps[i] ? hb_new_copy<BitMap>(*a.bitmaps[i]) : NULL;
    return *this;
  }

  inline void resize(int _w, int _h)
  {
    reset();
    hb_free<BitMap *>(bitmaps, w * h);
    allocMem(_w, _h);
  }

  inline void reset()
  {
    for (int i = w * h - 1; i >= 0; i--)
      if (bitmaps[i])
      {
        hb_delete<BitMap>(bitmaps[i], 1);
        bitmaps[i] = NULL;
      }
  }
  inline void setAll()
  {
    for (int i = w * h - 1; i >= 0; i--)
    {
      if (!bitmaps[i])
        bitmaps[i] = hb_new<BitMap>(1);
      bitmaps[i]->setAll();
    }
  }

  inline void set(int x, int y)
  {
    int idx = (y >> BitMap::FULL_SHBITS) * w + (x >> BitMap::FULL_SHBITS);
    if (!bitmaps[idx])
      bitmaps[idx] = hb_new<BitMap>(1);
    bitmaps[idx]->set(x & (BitMap::FULL_SZ - 1), y & (BitMap::FULL_SZ - 1));
  }
  inline void clr(int x, int y)
  {
    int idx = (y >> BitMap::FULL_SHBITS) * w + (x >> BitMap::FULL_SHBITS);
    if (bitmaps[idx])
      bitmaps[idx]->clr(x & (BitMap::FULL_SZ - 1), y & (BitMap::FULL_SZ - 1));
  }

  inline int get(int x, int y) const
  {
    int idx = (y >> BitMap::FULL_SHBITS) * w + (x >> BitMap::FULL_SHBITS);
    if (bitmaps[idx])
      return bitmaps[idx]->get(x & (BitMap::FULL_SZ - 1), y & (BitMap::FULL_SZ - 1));
    return 0;
  }

  inline int getClamped(int x, int y) const
  {
    if (x < 0)
      x = 0;
    else if (x >= w * BitMap::FULL_SZ)
      x = w * BitMap::FULL_SZ - 1;
    if (y < 0)
      y = 0;
    else if (y >= h * BitMap::FULL_SZ)
      y = h * BitMap::FULL_SZ - 1;
    return get(x, y);
  }

  int getW() const { return w * BitMap::FULL_SZ; }
  int getH() const { return h * BitMap::FULL_SZ; }

  bool isEmpty() const
  {
    for (int i = w * h - 1; i >= 0; i--)
      if (bitmaps[i])
        return false;
    return true;
  }

  bool compare(const HierBitMap2d<BitMap> &bm, int ox, int oy, int &minx, int &miny, int &maxx, int &maxy)
  {
    G_ASSERT(w * h == bm.w * bm.h);
    bool differs = false;

    BitMap *__restrict *b1 = bitmaps;
    BitMap *__restrict *b2 = bm.bitmaps;

    for (int y = 0; y < h; y++, oy += BitMap::FULL_SZ)
      for (int x = 0; x < w; x++, b1++, b2++)
        if (b1[0] && b2[0])
          differs |= b1[0]->compare(*b2[0], ox + x * BitMap::FULL_SZ, oy, minx, miny, maxx, maxy);
        else if (b1[0] && !b2[0])
        {
          b1[0]->addSetBitsMinMax(ox + x * BitMap::FULL_SZ, oy, minx, miny, maxx, maxy);
          differs = true;
        }
        else if (!b1[0] && b2[0])
        {
          b2[0]->addSetBitsMinMax(ox + x * BitMap::FULL_SZ, oy, minx, miny, maxx, maxy);
          differs = true;
        }

    return differs;
  }

  void addSetBitsMinMax(int ox, int oy, int &minx, int &miny, int &maxx, int &maxy)
  {
    BitMap *__restrict *b = bitmaps;

    for (int y = 0; y < h; y++, oy += BitMap::FULL_SZ)
      for (int x = 0; x < w; x++, b++)
        if (b[0])
          b[0]->addSetBitsMinMax(ox + x * BitMap::FULL_SZ, oy, minx, miny, maxx, maxy);
  }

protected:
  int w, h;
  BitMap **bitmaps;

  void allocMem(int _w, int _h)
  {
    w = (_w + BitMap::FULL_SZ - 1) >> BitMap::FULL_SHBITS;
    h = (_h + BitMap::FULL_SZ - 1) >> BitMap::FULL_SHBITS;
    bitmaps = hb_alloc_clr<BitMap *>(w * h);
  }
};
