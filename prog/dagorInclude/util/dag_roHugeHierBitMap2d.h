//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_patchTab.h>
#include <util/dag_hierBitMap2d.h>
#include <ioSys/dag_genIo.h>
#include <math/integer/dag_IBBox2.h>


template <int L1_N_P2, int L2_N_P2>
struct RoHugeHierBitMap2d
{
  struct L2Bmp
  {
    typedef ConstSizeBitMap2d<L1_N_P2> L1Bmp;
    static constexpr int SZ = 1 << L2_N_P2;
    static constexpr int SHBITS = L2_N_P2;
    static constexpr int FULL_SZ = SZ * L1Bmp::FULL_SZ;
    static constexpr int FULL_SHBITS = SHBITS + L1Bmp::FULL_SHBITS;

    inline int get(int x, int y, const void *base) const
    {
      int idx = ((y >> L1Bmp::FULL_SHBITS) << SHBITS) + (x >> L1Bmp::FULL_SHBITS);
      if (bitmapOfs[idx])
        return (bitmapOfs[idx] != 1) ? bitmap(idx, base).get(x & (L1Bmp::FULL_SZ - 1), y & (L1Bmp::FULL_SZ - 1)) : 1;
      return 0;
    }

    int getW() const { return SZ * L1Bmp::FULL_SZ; }
    int getH() const { return SZ * L1Bmp::FULL_SZ; }

    const uint32_t bitmapOfs[SZ * SZ];

    const L1Bmp *bitmapPtr(int ofs, const void *base) const { return (const L1Bmp *)(ptrdiff_t(base) + ofs); }
    const L1Bmp &bitmap(int i, const void *base) const { return *bitmapPtr(bitmapOfs[i], base); }

  private:
    L2Bmp() {}
  };

public:
  static RoHugeHierBitMap2d *create(void *mem)
  {
    RoHugeHierBitMap2d *bm = new (mem, _NEW_INPLACE) RoHugeHierBitMap2d;
    bm->bitmapOfs.patch(mem);
    return bm;
  }
  static RoHugeHierBitMap2d *create(IGenLoad &crd)
  {
    int l1 = crd.readInt(), l2 = crd.readInt();
    if (l1 != L1_N_P2 || l2 != L2_N_P2)
      return NULL;
    int dump_sz = crd.readInt();
    char *dump = (char *)defaultmem->tryAlloc(dump_sz);
    if (!dump)
      return NULL;
    crd.read(dump, dump_sz);
    if (RoHugeHierBitMap2d *bm = create(dump))
      return bm;
    defaultmem->free(dump);
    return NULL;
  }

  inline int get(int x, int y) const
  {
    int idx = (y >> L2Bmp::FULL_SHBITS) * w + (x >> L2Bmp::FULL_SHBITS);
    if (bitmapOfs[idx])
      return bitmapOfs[idx] != 1 ? bitmap(idx).get(x & (L2Bmp::FULL_SZ - 1), y & (L2Bmp::FULL_SZ - 1), this) : 1;
    return 0;
  }

  inline int getClamped(int x, int y) const
  {
    if (x < 0)
      x = 0;
    else if (x >= w * L2Bmp::FULL_SZ)
      x = w * L2Bmp::FULL_SZ - 1;
    if (y < 0)
      y = 0;
    else if (y >= h * L2Bmp::FULL_SZ)
      y = h * L2Bmp::FULL_SZ - 1;
    return get(x, y);
  }

  int getW() const { return w * L2Bmp::FULL_SZ; }
  int getH() const { return h * L2Bmp::FULL_SZ; }

  template <typename EnumL1CB>
  void enumerateL1Blocks(const EnumL1CB &cb, const IBBox2 &bx) const
  {
    int x0 = max(bx.lim[0].x >> L2Bmp::FULL_SHBITS, 0), x1 = min(bx.lim[1].x >> L2Bmp::FULL_SHBITS, w - 1);
    int y = max(bx.lim[0].y >> L2Bmp::FULL_SHBITS, 0), y1 = min(bx.lim[1].y >> L2Bmp::FULL_SHBITS, h - 1);

    for (; y <= y1; y++)
      for (int x = x0; x <= x1; x++)
        if (uint32_t b2o = bitmapOfs[y * w + x])
        {
          int sx = x << L2Bmp::FULL_SHBITS, sy = y << L2Bmp::FULL_SHBITS;
          int bx0 = max((bx.lim[0].x - sx) >> L2Bmp::L1Bmp::FULL_SHBITS, 0),
              bx1 = min((bx.lim[1].x - sx) >> L2Bmp::L1Bmp::FULL_SHBITS, int(L2Bmp::SZ - 1));
          int by = max((bx.lim[0].y - sy) >> L2Bmp::L1Bmp::FULL_SHBITS, 0),
              by1 = min((bx.lim[1].y - sy) >> L2Bmp::L1Bmp::FULL_SHBITS, int(L2Bmp::SZ - 1));

          const L2Bmp *b2 = bitmapPtr(b2o);
          for (; by <= by1; by++)
            for (int bx2 = bx0; bx2 <= bx1; bx2++)
              if (uint32_t b1o = (b2o != 1) ? b2->bitmapOfs[by * L2Bmp::SZ + bx2] : 1)
                cb.onNonEmptyL1Block(b1o != 1 ? b2->bitmapPtr(b1o, this) : (const typename L2Bmp::L1Bmp *)1,
                  sx + bx2 * L2Bmp::L1Bmp::SZ, sy + by * L2Bmp::L1Bmp::SZ);
        }
  }

protected:
  int w, h;
  PatchablePtr<uint32_t> bitmapOfs;

  RoHugeHierBitMap2d() {} //-V730
  const L2Bmp *bitmapPtr(int ofs) const { return (const L2Bmp *)(ptrdiff_t(this) + ofs); }
  const L2Bmp &bitmap(int i) const { return *bitmapPtr(bitmapOfs[i]); }
};
