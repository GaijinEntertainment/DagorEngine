//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_bounds2.h>
#include <math/dag_TMatrix.h>
#include <de3_objEntity.h>
#include <de3_interface.h>
#include <de3_landClassData.h>
#include <de3_grassPlanting.h>
#include <EditorCore/ec_interface.h>

namespace objgenerator
{
#define ADD_ENTITY_BYMASK(rect_test, mask, rect, ent, ret_word)                                 \
  pos.x = mx + (ou + _frnd(seed)) * gstepx;                                                     \
  pos.y = 0;                                                                                    \
  pos.z = my + (ov + _frnd(seed)) * gstepy;                                                     \
  if ((rect_test && !(rect & Point2(pos.x, pos.z))) ||                                          \
      !mask.getClamped((pos.x - world0_x) * world2sampler, (pos.z - world0_y) * world2sampler)) \
    ret_word;                                                                                   \
  if (!is_place_allowed(pos.x + entity_ofs_x, pos.z + entity_ofs_z))                            \
    ret_word;                                                                                   \
  grassEnt->addGrassPoint(pos.x + entity_ofs_x, pos.z + entity_ofs_z);

static __forceinline void _rnd_ivec2_mbit2(int &seed, int &x, int &y)
{
  static const int CMASK = landclass::DensMapLeaf::SZ - 1;
  unsigned int a = (unsigned)seed * 0x41C64E6D + 0x3039, b;
  b = (unsigned)a * 0x41C64E6D + 0x3039;
  x = signed(a >> 16) & CMASK;
  y = signed(b >> 16) & CMASK;
  seed = (int)b;
}

namespace internal2
{
static const int CSZ = landclass::DensMapLeaf::SZ;
static IGrassPlanting *grassEnt;
static BBox2 rect;
static const void *pMask;
static float world2sampler, world0_x, world0_y, entity_ofs_x, entity_ofs_z;
static int skip_test_rect;
} // namespace internal2

template <class BitMask>
struct GenGrassCB
{
  const landclass::GrassDensity &grass;
  float density, gstepx, gstepy, x0, y0;

  GenGrassCB(const landclass::GrassDensity &g) : grass(g)
  {
    density = grass.density * grass.densMapCellSz.x * grass.densMapCellSz.y / 100.0;
    gstepx = grass.densMapCellSz.x;
    gstepy = grass.densMapCellSz.y;
  }

  void onNonEmptyL1Block(const landclass::DensMapLeaf *l, int u0, int v0) const
  {
    using namespace internal2;

    int numc = 0, i;
    char cu[CSZ][CSZ];

    if (ptrdiff_t(l) == 1)
    {
      memset(cu, 1, sizeof(cu));
      numc = CSZ * CSZ;
    }
    else
      for (i = 0; i < CSZ * CSZ; ++i)
        if (l->getByIdx(i))
        {
          cu[0][i] = 1;
          ++numc;
        }
        else
          cu[0][i] = 0;

    if (!numc)
      return;

    Point3 pos, norm;
    TMatrix tm;
    tm.identity();

    int seed1 = u0 + 97, seed2 = v0 + 79;
    _rnd(seed1);
    _rnd(seed2);
    int seed = (grass.rseed ^ seed1 ^ seed2) + _rnd(seed1) + _rnd(seed2);
    int numu = 0;
    float num = floorf(numc * density * 1024.0f + 0.5f) / 1024.0f;
    float mx = gstepx * u0 + x0, my = gstepy * v0 + y0;

    for (; num >= 1; num -= 1)
    {
      int ou, ov;
      for (;;)
      {
        _rnd_ivec2_mbit2(seed, ou, ov);
        if (cu[ov][ou] == 1)
          break;
      }
      cu[ov][ou] = 2;
      if (++numu >= numc)
      {
        numu = 0;
        for (i = 0; i < CSZ * CSZ; ++i)
          if (cu[0][i] == 2)
            cu[0][i] = 1;
      }
      ADD_ENTITY_BYMASK(!skip_test_rect, (*(BitMask *)pMask), rect, ent, continue);
    }

    if (num > 0)
    {
      int ou, ov;
      for (;;)
      {
        _rnd_ivec2_mbit2(seed, ou, ov);
        if (cu[ov][ou] == 1)
          break;
      }
      if (_rnd(seed) < ((ptrdiff_t(l) == 1 || l->get(ov, ou)) ? int(num * 32768) + 1 : 0))
      {
        ADD_ENTITY_BYMASK(!skip_test_rect, (*(BitMask *)pMask), rect, ent, return);
      }
    }
  }
};

template <class BitMask>
inline void generateGrassInMaskedRect(const landclass::GrassDensity &grass, IGrassPlanting *grassEnt, const BitMask &mask,
  float world2sampler, float world0_x, float world0_y, float box_x, float box_y, float entity_ofs_x = 0, float entity_ofs_z = 0)
{
  Point3 pos;

  internal2::rect[0] = Point2(world0_x, world0_y);
  internal2::rect[1] = Point2(world0_x + box_x, world0_y + box_y);

  if (!grass.densityMap)
  {
    using internal::CSZ;
    float dx = box_x;
    float dy = box_y;
    float gstepx = box_x / CSZ;
    float gstepy = box_y / CSZ;
    float mx = floor(world0_x / dx) * dx, my = floor(world0_y / dy) * dy;
    unsigned short cu2[CSZ];
    G_STATIC_ASSERT(CSZ <= 16);

    memset(cu2, 0, sizeof(cu2));

    int seed1 = int(mx / dx) + 97, seed2 = int(my / dy) + 79;
    _rnd(seed1);
    _rnd(seed2);
    int seed = (grass.rseed ^ seed1 ^ seed2) + _rnd(seed1) + _rnd(seed2);
    int numu = 0;

    float cellSize = grassEnt->getCellSize();
    float num = (grass.density * 0.01) * (dx * dy);
    float max_num = (dx / cellSize) * (dy / cellSize);
    if (num > max_num)
      num = max_num;

    for (; num >= 1; num -= 1)
    {
      int ou, ov;
      for (;;)
      {
        _rnd_ivec2_mbit2(seed, ou, ov);
        if (!(cu2[ov] & (1 << ou)))
          break;
      }
      cu2[ov] |= (1 << ou);
      if (++numu >= CSZ * CSZ)
      {
        numu = 0;
        memset(cu2, 0, sizeof(cu2));
      }
      ADD_ENTITY_BYMASK(1, mask, internal2::rect, planted.ent, continue);
    }

    if (num > 0)
    {
      int ou, ov;
      for (;;)
      {
        _rnd_ivec2_mbit2(seed, ou, ov);
        if (!(cu2[ov] & (1 << ou)))
          break;
      }
      if (_frnd(seed) <= num)
      {
        ADD_ENTITY_BYMASK(1, mask, internal2::rect, planted.ent, return);
      }
    }
    return;
  }

  internal2::grassEnt = grassEnt;
  internal2::pMask = &mask;
  internal2::world2sampler = world2sampler;
  internal2::world0_x = world0_x;
  internal2::world0_y = world0_y;
  internal2::entity_ofs_x = entity_ofs_x;
  internal2::entity_ofs_z = entity_ofs_z;

  GenGrassCB<BitMask> cb(grass);
  real dx = grass.densMapSize.x;
  real dy = grass.densMapSize.y;
  real tx0 = floorf((internal::rect[0].x - grass.densMapOfs.x) / dx) * dx + grass.densMapOfs.x;
  real ty0 = floorf((internal::rect[0].y - grass.densMapOfs.y) / dy) * dy + grass.densMapOfs.y;
  real tx1 = ceilf((internal::rect[1].x - grass.densMapOfs.x) / dx) * dx + grass.densMapOfs.x;
  real ty1 = ceilf((internal::rect[1].y - grass.densMapOfs.y) / dy) * dy + grass.densMapOfs.y;

  IBBox2 densMapClip;
  densMapClip[0].x = densMapClip[0].y = 0;
  densMapClip[1].x = grass.densMapSize.x / cb.gstepx;
  densMapClip[1].y = grass.densMapSize.y / cb.gstepy;

  for (real mx = tx0; mx < tx1; mx += dx)
    for (real my = ty0; my < ty1; my += dy)
    {
      IBBox2 bb;
      bb[0].x = floor((world0_x - mx) / cb.gstepx);
      bb[0].y = floor((world0_y - my) / cb.gstepy);
      bb[1].x = ceil((internal2::rect[1].x - mx) / cb.gstepx);
      bb[1].y = ceil((internal2::rect[1].y - my) / cb.gstepy);
      densMapClip.clipBox(bb);

      if (!bb.isEmpty())
      {
        cb.x0 = mx;
        cb.y0 = my;
        internal2::skip_test_rect = (bb == densMapClip);
        grass.densityMap->enumerateL1Blocks(cb, bb);
      }
    }
}
#undef ADD_ENTITY_BYMASK
} // namespace objgenerator
