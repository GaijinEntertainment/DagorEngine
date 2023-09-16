//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_bounds2.h>
#include <math/dag_TMatrix.h>
#include <de3_objEntity.h>
#include <de3_interface.h>
#include <de3_landClassData.h>
#include <de3_heightmap.h>
#include <de3_genObjUtil.h>
#include <de3_randomSeed.h>


namespace objgenerator
{
template <class BitMask>
inline void generateTiledEntitiesInMaskedRect(landclass::TiledEntities &lcd, int subtype, int editLayerIdx, IHeightmap *hmap,
  dag::Span<landclass::SingleEntityPool> pools, const BitMask &mask, float world2sampler, float world0_x, float world0_y, float box_x,
  float box_y, float entity_ofs_x = 0, float entity_ofs_z = 0, int skip_asset_type = -1)
{
#define ADD_ENTITY_BYMASK                                                                         \
  if (sep.entity && sep.entity->getAssetTypeId() == skip_asset_type ||                            \
      !mask.getClamped((pos.x - world0_x) * world2sampler, (pos.z - world0_y) * world2sampler) || \
      !is_place_allowed(pos.x + entity_ofs_x, pos.z + entity_ofs_z))                              \
  {                                                                                               \
    _rnd(seed);                                                                                   \
    continue;                                                                                     \
  }                                                                                               \
  pos.x += entity_ofs_x;                                                                          \
  pos.z += entity_ofs_z;                                                                          \
  TMatrix tm = sep.tm[tmi];                                                                       \
                                                                                                  \
  MpPlacementRec mppRec;                                                                          \
  bool is_multi_place = sep.mpRec.mpOrientType != MpPlacementRec::MP_ORIENT_NONE;                 \
  TMatrix rot_tm = tm;                                                                            \
  rot_tm.setcol(3, Point3(0, 0, 0));                                                              \
                                                                                                  \
  if (orientType == sep.ORIENT_WORLD)                                                             \
  {                                                                                               \
    hmap ? hmap->getHeightmapPointHt(pos, NULL) : false;                                          \
    if (lcd.colliders.size())                                                                     \
    {                                                                                             \
      if (is_multi_place)                                                                         \
      {                                                                                           \
        mppRec = sep.mpRec;                                                                       \
        place_multipoint(mppRec, pos, rot_tm, lcd.aboveHt);                                       \
      }                                                                                           \
      else                                                                                        \
        place_on_ground(pos, lcd.aboveHt);                                                        \
    }                                                                                             \
  }                                                                                               \
  else                                                                                            \
  {                                                                                               \
    TMatrix rot;                                                                                  \
    Point3 norm(0, 1, 0);                                                                         \
    hmap ? hmap->getHeightmapPointHt(pos, &norm) : false;                                         \
    if (lcd.colliders.size())                                                                     \
    {                                                                                             \
      if (is_multi_place)                                                                         \
      {                                                                                           \
        mppRec = sep.mpRec;                                                                       \
        place_multipoint(mppRec, pos, rot_tm, lcd.aboveHt);                                       \
      }                                                                                           \
      else                                                                                        \
        place_on_ground(pos, norm, lcd.aboveHt);                                                  \
    }                                                                                             \
    if (orientType == sep.ORIENT_NORMAL_XZ)                                                       \
      rot.setcol(0, norm % Point3(-norm.z, 0, norm.x));                                           \
    else if (orientType == sep.ORIENT_WORLD_XZ)                                                   \
    {                                                                                             \
      if (fabsf(norm.x) + fabsf(norm.z) > 1e-5f)                                                  \
        rot.setcol(0, tm.getcol(1) % Point3(-norm.z, 0, norm.x));                                 \
      else                                                                                        \
        rot.setcol(0, tm.getcol(1) % Point3(0, 0, 1));                                            \
      norm = tm.getcol(1);                                                                        \
    }                                                                                             \
    else                                                                                          \
      rot.setcol(0, norm % Point3(0, 0, 1));                                                      \
    float xAxisLength = length(rot.getcol(0));                                                    \
    if (xAxisLength <= 1.192092896e-06F)                                                          \
      rot.setcol(0, normalize(norm % Point3(1, 0, 0)));                                           \
    else                                                                                          \
      rot.setcol(0, rot.getcol(0) / xAxisLength);                                                 \
    rot.setcol(1, norm);                                                                          \
    rot.setcol(2, normalize(rot.getcol(0) % norm));                                               \
    tm = rot % tm;                                                                                \
  }                                                                                               \
  if (is_multi_place)                                                                             \
    rotate_multipoint(tm, mppRec);                                                                \
  pos.y += getRandom(seed, yOffset);                                                              \
  tm.setcol(3, pos);                                                                              \
  IObjEntity *e = NULL;                                                                           \
  if (obj_idx < ent_avail)                                                                        \
    e = pool.entPool[obj_idx];                                                                    \
  else                                                                                            \
  {                                                                                               \
    e = IDaEditor3Engine::get().cloneEntity(sep.entity);                                          \
    if (!e)                                                                                       \
      continue;                                                                                   \
    pool.entPool.push_back(e);                                                                    \
  }                                                                                               \
  e->setSubtype(subtype);                                                                         \
  e->setEditLayerIdx(editLayerIdx);                                                               \
  if (lcd.setSeedToEntities)                                                                      \
    if (IRandomSeedHolder *irsh = e->queryInterface<IRandomSeedHolder>())                         \
      irsh->setSeed(seed);                                                                        \
  e->setTm(tm);                                                                                   \
  IColor *ecol = e->queryInterface<IColor>();                                                     \
  if (ecol)                                                                                       \
    ecol->setColor(sep.colorRangeIdx), _skip_rnd_ivec4(seed);                                     \
  obj_idx++;

  BBox2 rect(Point2(world0_x, world0_y), Point2(world0_x + box_x, world0_y + box_y));
  real dx = lcd.tile.x;
  real dy = lcd.tile.y;
  real tx0 = floor(world0_x / dx) * dx, ty0 = floor(world0_y / dy) * dy;
  real tx1 = ceil(rect[1].x / dx) * dx, ty1 = ceil(rect[1].y / dy) * dy;
  if (lcd.colliders.size())
    EDITORCORE->setColliders(lcd.colliders, lcd.collFilter);

  for (int i = 0; i < lcd.data.size(); i++)
  {
    landclass::SingleEntityPlaces &sep = lcd.data[i];
    landclass::SingleEntityPool &pool = pools[i];
    int obj_idx = pool.entUsed, ent_avail = pool.entPool.size();
    const Point2 &yOffset = sep.yOffset;
    int seed = sep.rseed;
    unsigned orientType = sep.orientType;

    for (real mx = tx0; mx < tx1; mx += dx)
      for (real my = ty0; my < ty1; my += dy)
        if (mx >= rect[0].x && my >= rect[0].y && mx + dx <= rect[1].y && my + dy <= rect[1].y)
        {
          for (int tmi = sep.tm.size() - 1; tmi >= 0; tmi--)
          {
            Point3 pos = sep.tm[tmi].getcol(3) + Point3(mx, 0, my);
            ADD_ENTITY_BYMASK;
          }
        }
        else
        {
          for (int tmi = sep.tm.size() - 1; tmi >= 0; tmi--)
          {
            Point3 pos = sep.tm[tmi].getcol(3) + Point3(mx, 0, my);

            if (!(rect & Point2(pos.x, pos.z)))
              continue;

            ADD_ENTITY_BYMASK;
          }
        }

    pool.entUsed = obj_idx;
  }
#undef ADD_ENTITY_BYMASK
}
} // namespace objgenerator
