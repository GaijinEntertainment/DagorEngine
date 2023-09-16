//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_roHugeHierBitMap2d.h>

struct EditableHugeBitMap2d : public RoHugeHierBitMap2d<4, 3>
{
  enum
  {
    L2_SZ = L2Bmp::SZ,
    L2_SH = L2Bmp::SHBITS,
    L2_FSZ = L2Bmp::FULL_SZ,
    L2_FSH = L2Bmp::FULL_SHBITS,

    L1_SZ = L2Bmp::L1Bmp::SZ,
    L1_FSH = L2Bmp::L1Bmp::FULL_SHBITS,
  };

public:
  static EditableHugeBitMap2d *create(int _w, int _h)
  {
    int w = (_w + L2_FSZ - 1) >> L2_FSH;
    int h = (_h + L2_FSZ - 1) >> L2_FSH;

    const size_t ofs0 = sizeof(EditableHugeBitMap2d);
    const size_t ofs1 = ofs0 + (w * h) * sizeof(unsigned);
    const size_t ofs2 = ofs1 + (w * h) * (L2_SZ * L2_SZ) * sizeof(unsigned);
    const size_t sz = ofs2 + (w * h) * (L2_SZ * L2_SZ) * sizeof(L2Bmp::L1Bmp);

    char *mem = (char *)memalloc(sz, tmpmem);
    EditableHugeBitMap2d *bm = new (mem, _NEW_INPLACE) EditableHugeBitMap2d;
    bm->w = w;
    bm->h = h;
    bm->bitmapOfs.setPtr(mem + ofs0);
    for (int i = 0; i < w * h; i++)
    {
      bm->bitmapOfs[i] = unsigned(ofs1 + i * (L2_SZ * L2_SZ) * sizeof(unsigned));
      for (int j = 0; j < L2_SZ * L2_SZ; j++)
        const_cast<uint32_t *>(bm->bitmap(i).bitmapOfs)[j] = unsigned(ofs2 + (i * L2_SZ * L2_SZ + j) * sizeof(L2Bmp::L1Bmp));
    }
    memset(mem + ofs2, 0, sz - ofs2);
    return bm;
  }

  inline unsigned getL1Ofs(int x, int y)
  {
    const size_t ofs2 = sizeof(EditableHugeBitMap2d) + (w * h) * (1 + (L2_SZ * L2_SZ)) * sizeof(unsigned);
    int idx = (y >> L2_FSH) * w + (x >> L2_FSH);
    int l2_x = x & (L2_FSZ - 1), l2_y = y & (L2_FSZ - 1);
    int l2_idx = ((l2_y >> L1_FSH) << L2_SH) + (l2_x >> L1_FSH);
    return unsigned(ofs2 + (idx * L2_SZ * L2_SZ + l2_idx) * sizeof(L2Bmp::L1Bmp));
  }
  inline void set(int x, int y)
  {
    reinterpret_cast<L2Bmp::L1Bmp *>(ptrdiff_t(this) + getL1Ofs(x, y))->set(x & (L1_SZ - 1), y & (L1_SZ - 1));
  }
  inline void clr(int x, int y)
  {
    reinterpret_cast<L2Bmp::L1Bmp *>(ptrdiff_t(this) + getL1Ofs(x, y))->clr(x & (L1_SZ - 1), y & (L1_SZ - 1));
  }

protected:
  EditableHugeBitMap2d() {}
};
