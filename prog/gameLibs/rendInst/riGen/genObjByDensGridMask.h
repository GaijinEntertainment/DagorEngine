// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "landClassData.h"
#include "genObjUtil.h"

#include <math/dag_bounds2.h>
#include <math/dag_TMatrix.h>


namespace rendinst::gen
{
static __forceinline void _rnd_ivec2_mbit(int &seed, int &x, int &y)
{
  static constexpr int MASK = rendinst::gen::land::DensMapLeaf::SZ - 1;
  unsigned int a = (unsigned)seed * 0x41C64E6D + 0x3039, b;
  b = (unsigned)a * 0x41C64E6D + 0x3039;
  x = signed(a >> 16) & MASK;
  y = signed(b >> 16) & MASK;
  seed = (int)b;
}
static __forceinline void _rnd_ivec2(int &seed, int &x, int &y)
{
  unsigned int a = (unsigned)seed * 0x41C64E6D + 0x3039, b;
  b = (unsigned)a * 0x41C64E6D + 0x3039;
  x = signed(a >> 16) & 0x7FFF;
  y = signed(b >> 16) & 0x7FFF;
  seed = (int)b;
}

static __forceinline void _rnd_fvec2(int &seed, float &x, float &y)
{
  int ix, iy;
  _rnd_ivec2(seed, ix, iy);
  y = iy / 32768.0f;
  x = ix / 32768.0f;
}

static __forceinline void _skip6_rnd(int &seed)
{
  unsigned int a = (unsigned)seed * 0x41C64E6D + 0x3039, b, c, d, e, f;
  b = (unsigned)a * 0x41C64E6D + 0x3039;
  c = (unsigned)b * 0x41C64E6D + 0x3039;
  d = (unsigned)c * 0x41C64E6D + 0x3039;
  e = (unsigned)d * 0x41C64E6D + 0x3039;
  f = (unsigned)e * 0x41C64E6D + 0x3039;
  seed = (int)f;
}
__forceinline real getRandom(int &s, const Point2 &r) { return r.x + r.y * _srnd(s); }


#define ADD_ENTITY_BYMASK(rect_test, mask, rect, ent, ret_word)                                                  \
  float rndx, rndy;                                                                                              \
  _rnd_fvec2(seed, rndx, rndy);                                                                                  \
  pos.x = (ou + rndx) * gstepx + (mx + entity_ofs_x);                                                            \
  pos.y = init_y0;                                                                                               \
  pos.z = (ov + rndy) * gstepy + (my + entity_ofs_z);                                                            \
  align_place_xz(pos);                                                                                           \
  int mtx = int(floorf(pos.x * world2sampler) - floorf(world2sampler * entity_ofs_x));                           \
  int mtz = int(floorf(pos.z * world2sampler) - floorf(world2sampler * entity_ofs_z));                           \
  if ((rect_test && !(rect & Point2::xz(pos))) || !mask.getClamped(mtx, mtz) || !is_place_allowed(pos.x, pos.z)) \
  {                                                                                                              \
    _skip6_rnd(seed);                                                                                            \
    _rnd(seed);                                                                                                  \
    ret_word;                                                                                                    \
  }                                                                                                              \
                                                                                                                 \
  float w = floorf(_frnd(seed) * sgeg.sumWeight * 8192.0f + 0.5f) / 8192.0f;                                     \
  int objId, entIdx = -1;                                                                                        \
  bool posInst = false;                                                                                          \
  bool paletteRotation = false;                                                                                  \
  for (objId = 0; objId < sgeg.obj.size(); ++objId)                                                              \
  {                                                                                                              \
    w -= sgeg.obj[objId].weight;                                                                                 \
    if (w <= 0)                                                                                                  \
    {                                                                                                            \
      entIdx = sgeg.obj[objId].entityIdx;                                                                        \
      posInst = sgeg.obj[objId].posInst;                                                                         \
      paletteRotation = sgeg.obj[objId].paletteRotation;                                                         \
      break;                                                                                                     \
    }                                                                                                            \
  }                                                                                                              \
  if (objId == sgeg.obj.size())                                                                                  \
  {                                                                                                              \
    objId = 0;                                                                                                   \
    entIdx = sgeg.obj[0].entityIdx;                                                                              \
    posInst = sgeg.obj[0].posInst;                                                                               \
    paletteRotation = sgeg.obj[0].paletteRotation;                                                               \
  }                                                                                                              \
  if (entIdx < 0)                                                                                                \
    ret_word;                                                                                                    \
  rendinst::gen::SingleEntityPool &pool = pools[entIdx];                                                         \
  int paletteId = 0;                                                                                             \
  if (pool.tryAdd(pos.x, pos.z))                                                                                 \
  {                                                                                                              \
    Point3 norm(0, 1, 0);                                                                                        \
    custom_get_height(pos, sgeg.obj[objId].orientType == sgeg.obj[objId].ORIENT_WORLD ? nullptr : &norm);        \
    MpPlacementRec mppRec;                                                                                       \
    bool is_multi_place = sgeg.obj[objId].mpRec.mpOrientType != MpPlacementRec::MP_ORIENT_NONE;                  \
    if (is_multi_place)                                                                                          \
    {                                                                                                            \
      TMatrix rot_tm = TMatrix::IDENT;                                                                           \
      RotationPaletteManager::Palette palette;                                                                   \
      if (paletteRotation)                                                                                       \
        palette = get_rotation_palette_manager()->getPalette({layerIdx, int(ent_remap[entIdx])});                \
      calc_matrix_33(sgeg.obj[objId], rot_tm, seed, palette, paletteId);                                         \
      mppRec = sgeg.obj[objId].mpRec;                                                                            \
      place_multipoint(mppRec, pos, rot_tm, sgeg.aboveHt);                                                       \
    }                                                                                                            \
                                                                                                                 \
    if (sgeg.obj[objId].orientType == sgeg.obj[objId].ORIENT_WORLD)                                              \
    {                                                                                                            \
      if (!is_multi_place)                                                                                       \
        place_on_ground(pos, sgeg.aboveHt);                                                                      \
      tm.identity();                                                                                             \
    }                                                                                                            \
    else                                                                                                         \
    {                                                                                                            \
      place_on_ground(pos, norm, sgeg.aboveHt);                                                                  \
      if (sgeg.obj[objId].orientType == sgeg.obj[objId].ORIENT_NORMAL_XZ)                                        \
        tm.setcol(0, norm % Point3(-norm.z, 0, norm.x));                                                         \
      else if (sgeg.obj[objId].orientType == sgeg.obj[objId].ORIENT_WORLD_XZ)                                    \
      {                                                                                                          \
        if (fabsf(norm.x) + fabsf(norm.z) > 1e-5f)                                                               \
          tm.setcol(0, Point3(0, 1, 0) % Point3(-norm.z, 0, norm.x));                                            \
        else                                                                                                     \
          tm.setcol(0, Point3(1, 0, 0));                                                                         \
        norm.set(0, 1, 0);                                                                                       \
      }                                                                                                          \
      else                                                                                                       \
        tm.setcol(0, norm % Point3(0, 0, 1));                                                                    \
      float xAxisLength = length(tm.getcol(0));                                                                  \
      if (xAxisLength <= 1.192092896e-06F)                                                                       \
        tm.setcol(0, normalize(norm % Point3(1, 0, 0)));                                                         \
      else                                                                                                       \
        tm.setcol(0, tm.getcol(0) / xAxisLength);                                                                \
      tm.setcol(1, norm);                                                                                        \
      tm.setcol(2, normalize(tm.getcol(0) % norm));                                                              \
    }                                                                                                            \
    if (is_multi_place)                                                                                          \
      rotate_multipoint(tm, mppRec);                                                                             \
    RotationPaletteManager::Palette palette;                                                                     \
    if (paletteRotation)                                                                                         \
      palette = get_rotation_palette_manager()->getPalette({layerIdx, int(ent_remap[entIdx])});                  \
    calc_matrix_33(sgeg.obj[objId], tm, seed, palette, paletteId);                                               \
    pos.y += getRandom(seed, sgeg.obj[objId].yOffset);                                                           \
    tm.setcol(3, pos);                                                                                           \
    if (destrExcl.isMarked(pos.x, pos.z))                                                                        \
    {                                                                                                            \
      tm.setcol(0, Point3(0, 0, 0));                                                                             \
      tm.setcol(1, Point3(0, 0, 0));                                                                             \
      tm.setcol(2, Point3(0, 0, 0));                                                                             \
      paletteId = 0;                                                                                             \
    }                                                                                                            \
    pool.addEntity(tm, posInst, ent_remap[entIdx], paletteRotation ? paletteId : -1);                            \
  }                                                                                                              \
  else                                                                                                           \
    _skip6_rnd(seed);

namespace internal
{
static constexpr int CSZ = rendinst::gen::land::DensMapLeaf::SZ;

static dag::Span<rendinst::gen::SingleEntityPool> pools;
static BBox2 rect;
static const void *pMask;
static float world2sampler, world0_x, world0_y, entity_ofs_x, entity_ofs_z;
static int subtype, skip_test_rect;
static float init_y0;
int16_t *ent_remap;
} // namespace internal

template <class BitMask>
struct GenObjCB
{
  const rendinst::gen::land::SingleGenEntityGroup &sgeg;
  float density, gstepx, gstepy, x0, y0;
  int layerIdx;

  GenObjCB(const rendinst::gen::land::SingleGenEntityGroup &g, int layer_idx) : sgeg(g), x0(0.f), y0(0.f), layerIdx(layer_idx)
  {
    density = sgeg.density * sgeg.densMapCellSz.x * sgeg.densMapCellSz.y * 0.01f;
    gstepx = sgeg.densMapCellSz.x;
    gstepy = sgeg.densMapCellSz.y;
  }

  bool checkDensity() const
  {
    using namespace internal;
    return CSZ * CSZ * density >= 1.f / 8192.f;
  }

  void onNonEmptyL1Block(const rendinst::gen::land::DensMapLeaf *l, int u0, int v0) const
  {
    using namespace internal;

    int numc = 0;
    DECL_ALIGN16(uint8_t, cu[CSZ][CSZ]);
    enum : uint8_t
    {
      FREE,
      ADD,
      ADDED
    };

    if (ptrdiff_t(l) == 1)
    {
      memset(cu, ADD, sizeof(cu));
      numc = CSZ * CSZ;
    }
    else
    {
      uint8_t *curCu = cu[0];
      const uint32_t *bits = l->getBits();
      for (const uint32_t *endBits = bits + (CSZ * CSZ / 32); bits != endBits; bits++, curCu += 32)
        for (int i = 0; i < 32; i++)
        {
          bool isSet = (*bits >> i) & 1;
          numc += isSet;
          curCu[i] = ADD * isSet;
        }
    }
    if (!numc)
      return;

    Point3 pos;
    TMatrix tm;

    int seed1 = u0 + 97, seed2 = v0 + 79;
    _rnd(seed1);
    _rnd(seed2);
    int seed = (sgeg.rseed ^ seed1 ^ seed2) + seed1 + seed2;

    int numu = 0;
    float num = floorf((numc * density) * 1024.0f + 0.5f) / 1024.0f;
    float mx = gstepx * u0 + x0, my = gstepy * v0 + y0;

    for (; num >= 1; num -= 1)
    {
      int ou, ov;
      for (;;)
      {
        _rnd_ivec2_mbit(seed, ou, ov);
        if (cu[ov][ou] == ADD)
          break;
      }
      cu[ov][ou] = ADDED;
      if (++numu >= numc)
      {
        numu = 0;
        for (int i = 0; i < CSZ * CSZ; ++i)
          if (cu[i / CSZ][i % CSZ] == ADDED)
            cu[i / CSZ][i % CSZ] = ADD;
      }
      ADD_ENTITY_BYMASK(!skip_test_rect, (*(BitMask *)pMask), rect, ent, continue);
    }

    if (num > 0)
    {
      int ou, ov;
      for (;;)
      {
        _rnd_ivec2_mbit(seed, ou, ov);
        if (cu[ov][ou] == ADD)
          break;
      }
      if (_rnd(seed) < ((ptrdiff_t(l) == 1 || l->get(ov, ou)) ? (int)floorf(num * 32768) + 1 : 0))
      {
        ADD_ENTITY_BYMASK(!skip_test_rect, (*(BitMask *)pMask), rect, ent, return);
      }
    }
  }

private:
  GenObjCB(const GenObjCB &cb);
  GenObjCB &operator=(const GenObjCB &cb);
};

template <class BitMask>
inline void generatePlantedEntitiesInMaskedRect(const rendinst::gen::land::PlantedEntities &planted, int layerIdx,
  dag::Span<rendinst::gen::SingleEntityPool> pools, const BitMask &mask, float world2sampler, float world0_x, float world0_y,
  float box, float entity_ofs_x, float entity_ofs_z, float init_y0, int16_t *ent_remap, float dens_map_pivot_x, float dens_map_pivot_z)
{
  Point3 pos;
  TMatrix tm;

  tm.identity();
  internal::pools.set(pools.data(), pools.size());
  internal::pMask = &mask;
  internal::world2sampler = world2sampler;
  internal::world0_x = world0_x;
  internal::world0_y = world0_y;
  internal::entity_ofs_x = entity_ofs_x;
  internal::entity_ofs_z = entity_ofs_z;
  internal::init_y0 = init_y0;
  internal::rect[0] = Point2(world0_x + entity_ofs_x, world0_y + entity_ofs_z);
  internal::rect[1] = Point2(internal::rect[0].x + box, internal::rect[0].y + box);
  internal::ent_remap = ent_remap;

  for (int i = 0; i < planted.data.size(); i++)
  {
    const rendinst::gen::land::SingleGenEntityGroup &sgeg = planted.data[i];
    if (sgeg.density <= 0)
      continue;

    if (!sgeg.densityMap)
    {
      using internal::CSZ;
      G_STATIC_ASSERT(CSZ <= 16);
      float dx = box;
      float dy = box;
      float gstepx = box / CSZ;
      float gstepy = box / CSZ;
      float mx = world0_x, my = world0_y;
      unsigned short cu2[CSZ];

      memset(cu2, 0, sizeof(cu2));

      int seed1 = int(mx / dx) + 97, seed2 = int(my / dy) + 79;
      _rnd(seed1);
      _rnd(seed2);
      int seed = (sgeg.rseed ^ seed1 ^ seed2) + seed1 + seed2;

      int numu = 0;
      float density = sgeg.density * 0.01f;
      float num = floorf((dx * dy * density) * 128.0f + 0.5f) / 128.0f;

      for (; num >= 1; num -= 1)
      {
        int ou, ov;
        for (;;)
        {
          _rnd_ivec2_mbit(seed, ou, ov);
          if (!(cu2[ov] & (1 << ou)))
            break;
        }
        cu2[ov] |= (1 << ou);
        if (++numu >= CSZ * CSZ)
        {
          numu = 0;
          memset(cu2, 0, sizeof(cu2));
        }
        ADD_ENTITY_BYMASK(1, mask, internal::rect, planted.ent, continue);
      }

      if (num > 0)
      {
        int ou, ov;
        for (;;)
        {
          _rnd_ivec2_mbit(seed, ou, ov);
          if (!(cu2[ov] & (1 << ou)))
            break;
        }
        if (_rnd(seed) < (int)floorf(num * 32768) + 1)
        {
          ADD_ENTITY_BYMASK(1, mask, internal::rect, planted.ent, continue);
        }
      }
      continue;
    }

    GenObjCB<BitMask> cb(sgeg, layerIdx);
    if (!cb.checkDensity())
      continue;

    real dx = sgeg.densMapSize.x;
    real dy = sgeg.densMapSize.y;
    real gstepxRcp = 1.f / cb.gstepx;
    real gstepyRcp = 1.f / cb.gstepy;
    float densMapOfsX = sgeg.densMapOfs.x + dens_map_pivot_x - entity_ofs_x;
    float densMapOfsZ = sgeg.densMapOfs.y + dens_map_pivot_z - entity_ofs_z;
    real tx0 = floorf((internal::rect[0].x - (entity_ofs_x + densMapOfsX)) / dx) * dx + densMapOfsX;
    real ty0 = floorf((internal::rect[0].y - (entity_ofs_z + densMapOfsZ)) / dy) * dy + densMapOfsZ;
    real tx1 = ceilf((internal::rect[1].x - (entity_ofs_x + densMapOfsX)) / dx) * dx + densMapOfsX;
    real ty1 = ceilf((internal::rect[1].y - (entity_ofs_z + densMapOfsZ)) / dy) * dy + densMapOfsZ;

    IBBox2 densMapClip;
    densMapClip[0].x = densMapClip[0].y = 0;
    densMapClip[1].x = int(sgeg.densMapSize.x * gstepxRcp);
    densMapClip[1].y = int(sgeg.densMapSize.y * gstepyRcp);

    for (real mx = tx0; mx < tx1; mx += dx)
      for (real my = ty0; my < ty1; my += dy)
      {
        IBBox2 bb;
        bb[0].x = (int)floorf((world0_x - mx) * gstepxRcp);
        bb[0].y = (int)floorf((world0_y - my) * gstepyRcp);
        bb[1].x = (int)ceilf((internal::rect[1].x - (entity_ofs_x + mx)) * gstepxRcp - 1);
        bb[1].y = (int)ceilf((internal::rect[1].y - (entity_ofs_z + my)) * gstepyRcp - 1);
        densMapClip.clipBox(bb);

        if (!bb.isEmpty())
        {
          cb.x0 = mx;
          cb.y0 = my;
          internal::skip_test_rect = (bb == densMapClip);
          sgeg.densityMap->enumerateL1Blocks(cb, bb);
        }
      }
  }
}
#undef ADD_ENTITY_BYMASK
} // namespace rendinst::gen
