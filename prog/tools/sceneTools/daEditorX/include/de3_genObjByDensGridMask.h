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
#include <de3_genObjUtil.h>
#include <de3_heightmap.h>
#include <de3_randomSeed.h>

namespace objgenerator
{
static __forceinline void _rnd_ivec2_mbit(int &seed, int &x, int &y)
{
  static const int CMASK = landclass::DensMapLeaf::SZ - 1;
  unsigned int a = (unsigned)seed * 0x41C64E6D + 0x3039, b;
  b = (unsigned)a * 0x41C64E6D + 0x3039;
  x = signed(a >> 16) & CMASK;
  y = signed(b >> 16) & CMASK;
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


#define ADD_ENTITY_BYMASK(rect_test, mask, rect, ent, ret_word)                                                             \
  float rndx, rndy;                                                                                                         \
  _rnd_fvec2(seed, rndx, rndy);                                                                                             \
  pos.x = mx + (ou + rndx) * gstepx;                                                                                        \
  pos.y = init_y0;                                                                                                          \
  pos.z = my + (ov + rndy) * gstepy;                                                                                        \
  if ((rect_test && !(rect & Point2(pos.x, pos.z))) ||                                                                      \
      !mask.getClamped((pos.x - world0_x) * world2sampler, (pos.z - world0_y) * world2sampler) ||                           \
      !is_place_allowed(pos.x + entity_ofs_x, pos.z + entity_ofs_z))                                                        \
  {                                                                                                                         \
    _skip6_rnd(seed);                                                                                                       \
    _rnd(seed);                                                                                                             \
    ret_word;                                                                                                               \
  }                                                                                                                         \
                                                                                                                            \
  float w = floorf(_frnd(seed) * sgeg.sumWeight * 8192.0f + 0.5f) / 8192.0f;                                                \
  int objId, entIdx = -1;                                                                                                   \
  for (objId = 0; objId < sgeg.obj.size(); ++objId)                                                                         \
  {                                                                                                                         \
    w -= sgeg.obj[objId].weight;                                                                                            \
    if (w <= 0)                                                                                                             \
    {                                                                                                                       \
      entIdx = sgeg.obj[objId].entityIdx;                                                                                   \
      break;                                                                                                                \
    }                                                                                                                       \
  }                                                                                                                         \
  if (objId == sgeg.obj.size())                                                                                             \
  {                                                                                                                         \
    objId = 0;                                                                                                              \
    entIdx = sgeg.obj[0].entityIdx;                                                                                         \
  }                                                                                                                         \
  if (ent[entIdx] && ent[entIdx]->getAssetTypeId() == skip_asset_type)                                                      \
  {                                                                                                                         \
    _skip6_rnd(seed);                                                                                                       \
    ret_word;                                                                                                               \
  }                                                                                                                         \
  pos.x += entity_ofs_x;                                                                                                    \
  pos.z += entity_ofs_z;                                                                                                    \
  Point3 norm(0, 1, 0);                                                                                                     \
  hmap ? hmap->getHeightmapPointHt(pos, sgeg.obj[objId].orientType == sgeg.obj[objId].ORIENT_WORLD ? NULL : &norm) : false; \
  landclass::SingleEntityPool &pool = pools[entIdx];                                                                        \
  if (pool.entUsed < pool.entPool.size())                                                                                   \
    pool.entPool[pool.entUsed]->setSubtype(IObjEntity::ST_NOT_COLLIDABLE);                                                  \
                                                                                                                            \
  MpPlacementRec mppRec;                                                                                                    \
  bool is_multi_place = sgeg.obj[objId].mpRec.mpOrientType != MpPlacementRec::MP_ORIENT_NONE;                               \
  if (sgeg.colliders.size() && is_multi_place)                                                                              \
  {                                                                                                                         \
    TMatrix rot_tm = TMatrix::IDENT;                                                                                        \
    calc_matrix_33(sgeg.obj[objId], rot_tm, seed);                                                                          \
    mppRec = sgeg.obj[objId].mpRec;                                                                                         \
    place_multipoint(mppRec, pos, rot_tm, sgeg.aboveHt);                                                                    \
  }                                                                                                                         \
                                                                                                                            \
  if (sgeg.obj[objId].orientType == sgeg.obj[objId].ORIENT_WORLD)                                                           \
  {                                                                                                                         \
    if (sgeg.colliders.size() && !is_multi_place)                                                                           \
      place_on_ground(pos, sgeg.aboveHt);                                                                                   \
    tm.identity();                                                                                                          \
  }                                                                                                                         \
  else                                                                                                                      \
  {                                                                                                                         \
    if (sgeg.colliders.size())                                                                                              \
      place_on_ground(pos, norm, sgeg.aboveHt);                                                                             \
    if (sgeg.obj[objId].orientType == sgeg.obj[objId].ORIENT_NORMAL_XZ)                                                     \
      tm.setcol(0, norm % Point3(-norm.z, 0, norm.x));                                                                      \
    else if (sgeg.obj[objId].orientType == sgeg.obj[objId].ORIENT_WORLD_XZ)                                                 \
    {                                                                                                                       \
      if (fabsf(norm.x) + fabsf(norm.z) > 1e-5f)                                                                            \
        tm.setcol(0, Point3(0, 1, 0) % Point3(-norm.z, 0, norm.x));                                                         \
      else                                                                                                                  \
        tm.setcol(0, Point3(1, 0, 0));                                                                                      \
      norm.set(0, 1, 0);                                                                                                    \
    }                                                                                                                       \
    else                                                                                                                    \
      tm.setcol(0, norm % Point3(0, 0, 1));                                                                                 \
    float xAxisLength = length(tm.getcol(0));                                                                               \
    if (xAxisLength <= 1.192092896e-06F)                                                                                    \
      tm.setcol(0, normalize(norm % Point3(1, 0, 0)));                                                                      \
    else                                                                                                                    \
      tm.setcol(0, tm.getcol(0) / xAxisLength);                                                                             \
    tm.setcol(1, norm);                                                                                                     \
    tm.setcol(2, normalize(tm.getcol(0) % norm));                                                                           \
  }                                                                                                                         \
  if (is_multi_place)                                                                                                       \
    rotate_multipoint(tm, mppRec);                                                                                          \
  calc_matrix_33(sgeg.obj[objId], tm, seed);                                                                                \
  pos.y += getRandom(seed, sgeg.obj[objId].yOffset);                                                                        \
  tm.setcol(3, pos);                                                                                                        \
  IObjEntity *e = NULL;                                                                                                     \
  if (pool.entUsed < pool.entPool.size())                                                                                   \
  {                                                                                                                         \
    e = pool.entPool[pool.entUsed];                                                                                         \
    pool.entUsed++;                                                                                                         \
  }                                                                                                                         \
  else                                                                                                                      \
  {                                                                                                                         \
    e = IDaEditor3Engine::get().cloneEntity(ent[entIdx]);                                                                   \
    if (!e)                                                                                                                 \
      ret_word;                                                                                                             \
    pool.entPool.push_back(e);                                                                                              \
    pool.entUsed++;                                                                                                         \
  }                                                                                                                         \
  e->setSubtype(IObjEntity::ST_NOT_COLLIDABLE);                                                                             \
  if (sgeg.setSeedToEntities)                                                                                               \
    if (IRandomSeedHolder *irsh = e->queryInterface<IRandomSeedHolder>())                                                   \
      irsh->setSeed(seed);                                                                                                  \
  e->setTm(tm);                                                                                                             \
  IColor *ecol = e->queryInterface<IColor>();                                                                               \
  if (ecol)                                                                                                                 \
    ecol->setColor(sgeg.colorRangeIdx), _skip_rnd_ivec4(seed);

namespace internal
{
static const int CSZ = landclass::DensMapLeaf::SZ;
static dag::Span<landclass::SingleEntityPool> pools;
static dag::ConstSpan<IObjEntity *> ent;
static BBox2 rect;
static const void *pMask;
static float world2sampler, world0_x, world0_y, entity_ofs_x, entity_ofs_z;
static int subtype, skip_test_rect;
static float init_y0;
static IHeightmap *hmap;
static int skip_asset_type;
} // namespace internal

template <class BitMask>
struct GenObjCB
{
  const landclass::SingleGenEntityGroup &sgeg;
  float density, gstepx, gstepy, x0, y0;

  GenObjCB(const landclass::SingleGenEntityGroup &g) : sgeg(g)
  {
    density = sgeg.density * sgeg.densMapCellSz.x * sgeg.densMapCellSz.y / 100.0;
    gstepx = sgeg.densMapCellSz.x;
    gstepy = sgeg.densMapCellSz.y;
  }

  void onNonEmptyL1Block(const landclass::DensMapLeaf *l, int u0, int v0) const
  {
    using namespace internal;

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

    Point3 pos;
    TMatrix tm;
    tm.identity();

    int seed1 = u0 + 97, seed2 = v0 + 79;
    _rnd(seed1);
    _rnd(seed2);
    int seed = (sgeg.rseed ^ seed1 ^ seed2) + seed1 + seed2;

    int numu = 0;
    float num = floorf(numc * density * 1024.0f + 0.5f) / 1024.0f;
    float mx = gstepx * u0 + x0, my = gstepy * v0 + y0;

    for (; num >= 1; num -= 1)
    {
      int ou, ov;
      for (;;)
      {
        _rnd_ivec2_mbit(seed, ou, ov);
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
        _rnd_ivec2_mbit(seed, ou, ov);
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
inline void generatePlantedEntitiesInMaskedRect(const landclass::PlantedEntities &planted, int subtype, int editLayerIdx,
  IHeightmap *hmap, dag::Span<landclass::SingleEntityPool> pools, const BitMask &mask, float world2sampler, float world0_x,
  float world0_y, float box_x, float box_y, float entity_ofs_x = 0, float entity_ofs_z = 0, float init_y0 = 0,
  int skip_asset_type = -1)
{
  Point3 pos;
  TMatrix tm;

  tm.identity();
  internal::pools.set(pools.data(), pools.size());
  internal::ent.set(planted.ent.data(), planted.ent.size());
  internal::pMask = &mask;
  internal::world2sampler = world2sampler;
  internal::world0_x = world0_x;
  internal::world0_y = world0_y;
  internal::entity_ofs_x = entity_ofs_x;
  internal::entity_ofs_z = entity_ofs_z;
  internal::init_y0 = init_y0;
  internal::subtype = subtype;
  internal::rect[0] = Point2(world0_x, world0_y);
  internal::rect[1] = Point2(world0_x + box_x, world0_y + box_y);
  internal::hmap = hmap;
  internal::skip_asset_type = skip_asset_type;

#define SET_SUBTYPES(x)                        \
  for (int i = 0; i < pools.size(); i++)       \
  {                                            \
    for (int j = 0; j < pools[i].entUsed; j++) \
      pools[i].entPool[j]->setSubtype(x);      \
  }
#define SET_EDITLAYERIDX(x)                    \
  for (int i = 0; i < pools.size(); i++)       \
  {                                            \
    for (int j = 0; j < pools[i].entUsed; j++) \
      pools[i].entPool[j]->setEditLayerIdx(x); \
  }

  SET_SUBTYPES(IObjEntity::ST_NOT_COLLIDABLE);

  for (int i = 0; i < planted.data.size(); i++)
  {
    const landclass::SingleGenEntityGroup &sgeg = planted.data[i];
    if (sgeg.colliders.size())
      EDITORCORE->setColliders(sgeg.colliders, sgeg.collFilter);

    if (!sgeg.densityMap)
    {
      using internal::CSZ;
      float dx = box_x;
      float dy = box_y;
      float gstepx = box_x / CSZ;
      float gstepy = box_y / CSZ;
      float mx = world0_x, my = world0_y;
      unsigned short cu2[CSZ];
      G_STATIC_ASSERT(CSZ <= 16);

      memset(cu2, 0, sizeof(cu2));

      int seed1 = int(mx / dx) + 97, seed2 = int(my / dy) + 79;
      _rnd(seed1);
      _rnd(seed2);
      int seed = (sgeg.rseed ^ seed1 ^ seed2) + seed1 + seed2;

      int numu = 0;
      float num = sgeg.density * dx * dy / 100.0;

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
        if (_frnd(seed) <= num)
        {
          ADD_ENTITY_BYMASK(1, mask, internal::rect, planted.ent, continue);
        }
      }
      continue;
    }

    GenObjCB<BitMask> cb(sgeg);
    real dx = sgeg.densMapSize.x;
    real dy = sgeg.densMapSize.y;
    real tx0 = floorf((internal::rect[0].x - sgeg.densMapOfs.x) / dx) * dx + sgeg.densMapOfs.x;
    real ty0 = floorf((internal::rect[0].y - sgeg.densMapOfs.y) / dy) * dy + sgeg.densMapOfs.y;
    real tx1 = ceilf((internal::rect[1].x - sgeg.densMapOfs.x) / dx) * dx + sgeg.densMapOfs.x;
    real ty1 = ceilf((internal::rect[1].y - sgeg.densMapOfs.y) / dy) * dy + sgeg.densMapOfs.y;

    IBBox2 densMapClip;
    densMapClip[0].x = densMapClip[0].y = 0;
    densMapClip[1].x = sgeg.densMapSize.x / cb.gstepx;
    densMapClip[1].y = sgeg.densMapSize.y / cb.gstepy;

    for (real mx = tx0; mx < tx1; mx += dx)
      for (real my = ty0; my < ty1; my += dy)
      {
        IBBox2 bb;
        bb[0].x = floor((world0_x - mx) / cb.gstepx);
        bb[0].y = floor((world0_y - my) / cb.gstepy);
        bb[1].x = ceil((internal::rect[1].x - mx) / cb.gstepx);
        bb[1].y = ceil((internal::rect[1].y - my) / cb.gstepy);
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

  SET_SUBTYPES(subtype);
  SET_EDITLAYERIDX(editLayerIdx);
#undef SET_SUBTYPES
#undef SET_EDITLAYERIDX
}
#undef ADD_ENTITY_BYMASK
} // namespace objgenerator
