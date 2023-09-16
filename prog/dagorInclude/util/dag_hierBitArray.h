//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <string.h>
#include <util/dag_hierBitMemPool.h>

#ifdef _DEBUG_TAB_
#include <util/dag_globDef.h>

#define CONST_SIZE_BIT_ARRAY_VALIDATE_IDX                         \
  G_ASSERTF(((unsigned)idx < (SZ + 31) / 32),                     \
    "ConstSizeBitArray index out of bounds: "                     \
    "ConstSizeBitArrayBase[%d] num=%d this=0x%p bits=0x%p SZ=%d", \
    idx, ((SZ + 31) / 32), this, bits, SZ);
#else
#define CONST_SIZE_BIT_ARRAY_VALIDATE_IDX ((void)0)
#endif


//! template for bit array of constant size
template <int SZ_IMM>
struct ConstSizeBitArrayBase
{
  enum
  {
    SZ = SZ_IMM
  };

  inline void ctor() { reset(); }
  inline void ctor(const ConstSizeBitArrayBase<SZ_IMM> &a) { memcpy(bits, a.bits, sizeof(bits)); }
  inline void dtor() {}

  inline void reset() { memset(bits, 0, sizeof(bits)); }
  inline void setAll() { memset(bits, 0xFF, sizeof(bits)); }

  inline void set(int index)
  {
    int idx = index >> 5;
    CONST_SIZE_BIT_ARRAY_VALIDATE_IDX;
    bits[idx] |= (1 << (index & 0x1F));
  }

  inline void clr(int index)
  {
    int idx = index >> 5;
    CONST_SIZE_BIT_ARRAY_VALIDATE_IDX;
    bits[idx] &= ~(1 << (index & 0x1F));
  }
  inline void toggle(int index)
  {
    int idx = index >> 5;
    CONST_SIZE_BIT_ARRAY_VALIDATE_IDX;
    bits[idx] ^= (1 << (index & 0x1F));
  }

  inline int get(int index) const
  {
    int idx = index >> 5;
    CONST_SIZE_BIT_ARRAY_VALIDATE_IDX;
    return bits[idx] & (1 << (index & 0x1F));
  }

  inline int getIter(int x, int &inc) const
  {
    int idx = (x >> 5);
    CONST_SIZE_BIT_ARRAY_VALIDATE_IDX;

    unsigned w = bits[idx];
    x &= 0x1F;
    if (w)
    {
      w >>= x;
      inc = (w & ~1) ? 1 : 32 - x;
      return w & 1;
    }

    inc = 32 - x;
    return 0;
  }

  inline void setRange(int x0, int x1)
  {
#ifdef _DEBUG_TAB_
    G_ASSERT(x0 <= x1);
#endif
    int idx = x0 >> 5, idx1 = x1 >> 5;
    if (idx == idx1)
    {
      CONST_SIZE_BIT_ARRAY_VALIDATE_IDX;
      bits[idx] |= (0xFFFFFFFF << (x0 & 0x1F)) & (0xFFFFFFFF >> (0x1F - (x1 & 0x1F)));
    }
    else
    {
      CONST_SIZE_BIT_ARRAY_VALIDATE_IDX;
      bits[idx] |= (0xFFFFFFFF << (x0 & 0x1F));
      for (idx = idx + 1; idx < idx1; idx++)
      {
        CONST_SIZE_BIT_ARRAY_VALIDATE_IDX;
        bits[idx] = 0xFFFFFFFF;
      }
      CONST_SIZE_BIT_ARRAY_VALIDATE_IDX;
      bits[idx] |= (0xFFFFFFFF >> (0x1F - (x1 & 0x1F)));
    }
  }

protected:
  unsigned int bits[(SZ + 31) / 32];
};

template <int N_POW2>
struct ConstSizeBitArray : public ConstSizeBitArrayBase<1 << N_POW2>
{
  static constexpr int SZ = ConstSizeBitArrayBase<1 << N_POW2>::SZ;
  static constexpr int FULL_SZ = SZ;
  static constexpr int FULL_SHBITS = N_POW2;
};

//! template for hierarchical bit array of constant size
template <int N_POW2, class BitArr>
struct HierConstSizeBitArray
{
  static constexpr int SZ = 1 << N_POW2;
  static constexpr int SHBITS = N_POW2;
  static constexpr int FULL_SZ = SZ * BitArr::FULL_SZ;
  static constexpr int FULL_SHBITS = SHBITS + BitArr::FULL_SHBITS;

  HierConstSizeBitArray() { ctor(); }
  HierConstSizeBitArray(const HierConstSizeBitArray<N_POW2, BitArr> &a) { ctor(a); }
  ~HierConstSizeBitArray() { reset(); }

  HierConstSizeBitArray<N_POW2, BitArr> &operator=(const HierConstSizeBitArray<N_POW2, BitArr> &a)
  {
    reset();
    ctor(a);
    return *this;
  }

  inline void ctor() { memset(bitarr, 0, sizeof(bitarr)); }
  inline void ctor(const HierConstSizeBitArray<N_POW2, BitArr> &a)
  {
    for (int i = 0; i < SZ; i++)
      bitarr[i] = a.bitarr[i] ? hb_new_copy(*a.bitarr[i]) : NULL;
  }
  inline void dtor() { reset(); }

  inline void reset()
  {
    for (int i = SZ - 1; i >= 0; i--)
      if (bitarr[i])
      {
        hb_delete<BitArr>(bitarr[i], 1);
        bitarr[i] = NULL;
      }
  }
  inline void setAll()
  {
    for (int i = SZ - 1; i >= 0; i--)
    {
      if (!bitarr[i])
        bitarr[i] = hb_new<BitArr>(1);
      bitarr[i]->setAll();
    }
  }

  inline void set(int x)
  {
    int idx = (x >> BitArr::FULL_SHBITS);
    if (!bitarr[idx])
      bitarr[idx] = hb_new<BitArr>(1);
    bitarr[idx]->set(x & (BitArr::FULL_SZ - 1));
  }
  inline void clr(int x)
  {
    int idx = (x >> BitArr::FULL_SHBITS);
    if (bitarr[idx])
      bitarr[idx]->clr(x & (BitArr::FULL_SZ - 1));
  }

  inline void setRange(int x0, int x1)
  {
    int idx0 = (x0 >> BitArr::FULL_SHBITS);
    int idx1 = (x1 >> BitArr::FULL_SHBITS);
    if (idx0 == idx1)
    {
      if (!bitarr[idx0])
        bitarr[idx0] = hb_new<BitArr>(1);

      BitArr &b = *bitarr[idx0];
      idx1 = x1 & (BitArr::FULL_SZ - 1);
      for (idx0 = x0 & (BitArr::FULL_SZ - 1); idx0 <= idx1; idx0++)
        b.set(idx0);
    }
    else
    {
      for (int i = idx0 + 1; i < idx1; i++)
      {
        if (!bitarr[i])
          bitarr[i] = hb_new<BitArr>(1);
        bitarr[i]->setAll();
      }

      if (!bitarr[idx0])
        bitarr[idx0] = hb_new<BitArr>(1);
      if (!bitarr[idx1])
        bitarr[idx1] = hb_new<BitArr>(1);

      BitArr &b = *bitarr[idx0];
      for (idx0 = x0 & (BitArr::FULL_SZ - 1); idx0 < BitArr::FULL_SZ; idx0++)
        b.set(idx0);

      BitArr &b1 = *bitarr[idx1];
      idx1 = x1 & (BitArr::FULL_SZ - 1);
      for (idx0 = 0; idx0 <= idx1; idx0++)
        b1.set(idx0);
    }
  }

  inline int get(int x) const
  {
    int idx = (x >> BitArr::FULL_SHBITS);
    if (bitarr[idx])
      return bitarr[idx]->get(x & (BitArr::FULL_SZ - 1));
    return 0;
  }

  inline int getIter(int x, int &inc) const
  {
    int idx = (x >> BitArr::FULL_SHBITS);
    if (bitarr[idx])
      return bitarr[idx]->getIter(x & (BitArr::FULL_SZ - 1), inc);

    inc = ((idx + 1) << BitArr::FULL_SHBITS) - x;
    return 0;
  }

protected:
  BitArr *bitarr[SZ];
};


//! template for hierarchical bit map of variable size
template <class BitArr>
struct HierBitArray
{
  HierBitArray()
  {
    sz = 0;
    bitarr = NULL;
  }
  HierBitArray(int _sz) { allocMem(_sz); }
  HierBitArray(const HierBitArray<BitArr> &a)
  {
    sz = 0;
    bitarr = NULL;
    operator=(a);
  }
  ~HierBitArray()
  {
    reset();
    hb_free<BitArr *>(bitarr, sz);
  }

  HierBitArray<BitArr> &operator=(const HierBitArray<BitArr> &a)
  {
    resize(a.getSz());
    for (int i = 0; i < sz; i++)
      bitarr[i] = a.bitarr[i] ? hb_new_copy<BitArr>(*a.bitarr[i]) : NULL;
    return *this;
  }

  inline void resize(int _sz)
  {
    reset();
    hb_free<BitArr *>(bitarr, sz);
    allocMem(_sz);
  }

  inline void reset()
  {
    for (int i = sz - 1; i >= 0; i--)
      if (bitarr[i])
      {
        hb_delete<BitArr>(bitarr[i], 1);
        bitarr[i] = NULL;
      }
  }
  inline void setAll()
  {
    for (int i = sz - 1; i >= 0; i--)
    {
      if (!bitarr[i])
        bitarr[i] = hb_new<BitArr>(1);
      bitarr[i]->setAll();
    }
  }

  inline void set(int x)
  {
    int idx = (x >> BitArr::FULL_SHBITS);
    if (!bitarr[idx])
      bitarr[idx] = hb_new<BitArr>(1);
    bitarr[idx]->set(x & (BitArr::FULL_SZ - 1));
  }
  inline void clr(int x)
  {
    int idx = (x >> BitArr::FULL_SHBITS);
    if (bitarr[idx])
      bitarr[idx]->clr(x & (BitArr::FULL_SZ - 1));
  }

  inline int get(int x) const
  {
    int idx = (x >> BitArr::FULL_SHBITS);
    if (bitarr[idx])
      return bitarr[idx]->get(x & (BitArr::FULL_SZ - 1));
    return 0;
  }

  inline int getIter(int x, int &inc) const
  {
    int idx = (x >> BitArr::FULL_SHBITS);
    if (bitarr[idx])
      return bitarr[idx]->getIter(x & (BitArr::FULL_SZ - 1), inc);

    inc = ((idx + 1) << BitArr::FULL_SHBITS) - x;
    return 0;
  }

  int getSz() const { return sz * BitArr::FULL_SZ; }

protected:
  int sz;
  BitArr **bitarr;

  void allocMem(int _sz)
  {
    sz = (_sz + BitArr::FULL_SZ - 1) >> BitArr::FULL_SHBITS;
    bitarr = hb_alloc_clr<BitArr *>(sz);
  }
};
