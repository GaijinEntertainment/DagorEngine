//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
  const Point2 worldXZ = walk.toWorldLocal(pos);                                                  \
  const bool owned = walk.owns(rect, Point2::xz(pos));                                            \
  if (!owned || sep.entity && sep.entity->getAssetTypeId() == skip_asset_type ||                  \
      !mask.getClamped((pos.x - world0_x) * world2sampler, (pos.z - world0_y) * world2sampler) || \
      !is_place_allowed(pos.x + entity_ofs_x, pos.z + entity_ofs_z) || !walk.shows(worldXZ))      \
  {                                                                                               \
    rnd(seed);                                                                                    \
    continue;                                                                                     \
  }                                                                                               \
  pos.x += entity_ofs_x;                                                                          \
  pos.z += entity_ofs_z;                                                                          \
  TMatrix tm = sep.tm[tmi];                                                                       \
  walk.rotateTm(tm);                                                                              \
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
    ecol->setColor(sep.colorRangeIdx), skip_rnd_ivec4(seed);                                      \
  obj_idx++;

  BBox2 rect(Point2(world0_x, world0_y), Point2(world0_x + box_x, world0_y + box_y));
  real dx = lcd.tile.x;
  real dy = lcd.tile.y;
  if (lcd.colliders.size())
    EDITORCORE->setColliders(lcd.colliders, lcd.collFilter);

  land_detile::ClusterWalk walk(lcd.detile, Point2(entity_ofs_x, entity_ofs_z));
  int kLo, kHi;
  land_detile::NoiseCache nc;
  walk.clusterRange(rect, kLo, kHi, nc);
  for (int k = kLo; k <= kHi; k++)
  {
    walk.select(k);
    const BBox2 wr = walk.walkRect(rect);
    real tx0 = floor(wr[0].x / dx) * dx, ty0 = floor(wr[0].y / dy) * dy;
    real tx1 = ceil(wr[1].x / dx) * dx, ty1 = ceil(wr[1].y / dy) * dy;

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
        {
          if (walk.on()) // de-tiled: the runtime's spot range and seeds, without its x index
          {
            const int tileX = int(floorf(mx / dx + 0.5f)), tileY = int(floorf(my / dy + 0.5f));
            for (int tmi = sep.tm.size() - 1; tmi >= 0; tmi--)
            {
              const Point3 sp = sep.tm[tmi].getcol(3);
              if (sp.x < wr[0].x - mx || sp.x > wr[1].x - mx || sp.z < wr[0].y - my || sp.z > wr[1].y - my)
                continue;
              seed = land_detile::tiled_spot_seed(sep.rseed, tileX, tileY, k, tmi);
              Point3 pos = sp + Point3(mx, 0, my);
              ADD_ENTITY_BYMASK;
            }
            continue;
          }
          const bool tileInside = mx >= rect[0].x && my >= rect[0].y && mx + dx <= rect[1].x && my + dy <= rect[1].y;
          for (int tmi = sep.tm.size() - 1; tmi >= 0; tmi--)
          {
            Point3 pos = sep.tm[tmi].getcol(3) + Point3(mx, 0, my);
            if (!tileInside && !(rect & Point2(pos.x, pos.z)))
              continue;
            ADD_ENTITY_BYMASK;
          }
        }

      pool.entUsed = obj_idx;
    }
  }
#undef ADD_ENTITY_BYMASK
}
} // namespace objgenerator
