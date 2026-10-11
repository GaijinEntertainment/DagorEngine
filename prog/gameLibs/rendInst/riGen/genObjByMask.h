// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "landClassData.h"
#include "genObjUtil.h"

#include <math/dag_bounds2.h>
#include <math/dag_TMatrix.h>


namespace rendinst::gen
{
template <class BitMask>
inline void generateTiledEntitiesInMaskedRect(rendinst::gen::RiGenCellCtx &ctx, const rendinst::gen::land::TiledEntities &lcd,
  dag::Span<rendinst::gen::SingleEntityPool> pools, const BitMask &mask, float world2sampler, float world0_x, float world0_y,
  float box, float mask_ofs_x, float mask_ofs_z, int16_t *ent_remap, float entity_ofs_x, float entity_ofs_z, int layer_idx)
{
#define ADD_ENTITY_BYMASK                                                                         \
  pos.x += mx + entity_ofs_x;                                                                     \
  pos.z += my + entity_ofs_z;                                                                     \
  const Point2 worldXZ = walk.toWorld(pos);                                                       \
  if (!walk.owns(rect, worldXZ - eofs))                                                           \
  {                                                                                               \
    rnd(seed); /* de-tiled: another cell's rect owns this spot */                                 \
    continue;                                                                                     \
  }                                                                                               \
  align_place_xz(pos);                                                                            \
  int mtx = int(floorf(pos.x * world2sampler) - floorf(world2sampler * mask_ofs_x));              \
  int mtz = int(floorf(pos.z * world2sampler) - floorf(world2sampler * mask_ofs_z));              \
  if (!mask.getClamped(mtx, mtz) || !is_place_allowed(pos.x, pos.z) || !walk.shows(worldXZ))      \
  {                                                                                               \
    rnd(seed);                                                                                    \
    continue;                                                                                     \
  }                                                                                               \
  int paletteId = 0;                                                                              \
  if (pool.tryAdd(ctx, pos.x, pos.z))                                                             \
  {                                                                                               \
    TMatrix tm = sep.tm[tmi];                                                                     \
    walk.rotateTm(tm);                                                                            \
                                                                                                  \
    MpPlacementRec mppRec;                                                                        \
    bool is_multi_place = sep.mpRec.mpOrientType != MpPlacementRec::MP_ORIENT_NONE;               \
    TMatrix rot_tm = tm;                                                                          \
    rot_tm.setcol(3, Point3(0, 0, 0));                                                            \
                                                                                                  \
    if (orientType == sep.ORIENT_WORLD)                                                           \
    {                                                                                             \
      custom_get_height(pos, nullptr);                                                            \
      if (is_multi_place)                                                                         \
      {                                                                                           \
        mppRec = sep.mpRec;                                                                       \
        place_multipoint(mppRec, pos, rot_tm, lcd.aboveHt);                                       \
      }                                                                                           \
      else                                                                                        \
        place_on_ground(pos, lcd.aboveHt);                                                        \
    }                                                                                             \
    else                                                                                          \
    {                                                                                             \
      TMatrix rot;                                                                                \
      Point3 norm(0, 1, 0);                                                                       \
      custom_get_height(pos, &norm);                                                              \
      if (is_multi_place)                                                                         \
      {                                                                                           \
        mppRec = sep.mpRec;                                                                       \
        place_multipoint(mppRec, pos, rot_tm, lcd.aboveHt);                                       \
      }                                                                                           \
      else                                                                                        \
        place_on_ground(pos, norm, lcd.aboveHt);                                                  \
      if (orientType == sep.ORIENT_NORMAL_XZ)                                                     \
        rot.setcol(0, norm % Point3(-norm.z, 0, norm.x));                                         \
      else if (orientType == sep.ORIENT_WORLD_XZ)                                                 \
      {                                                                                           \
        if (fabsf(norm.x) + fabsf(norm.z) > 1e-5f)                                                \
          rot.setcol(0, tm.getcol(1) % Point3(-norm.z, 0, norm.x));                               \
        else                                                                                      \
          rot.setcol(0, tm.getcol(1) % Point3(0, 0, 1));                                          \
        norm = tm.getcol(1);                                                                      \
      }                                                                                           \
      else                                                                                        \
        rot.setcol(0, norm % Point3(0, 0, 1));                                                    \
      float xAxisLength = length(rot.getcol(0));                                                  \
      if (xAxisLength <= 1.192092896e-06F)                                                        \
        rot.setcol(0, normalize(norm % Point3(1, 0, 0)));                                         \
      else                                                                                        \
        rot.setcol(0, rot.getcol(0) / xAxisLength);                                               \
      rot.setcol(1, norm);                                                                        \
      rot.setcol(2, normalize(rot.getcol(0) % norm));                                             \
      tm = rot % tm;                                                                              \
    }                                                                                             \
    if (is_multi_place)                                                                           \
      rotate_multipoint(tm, mppRec);                                                              \
    if (walk.on() && paletteRotation) /* the turned axes need their palette id re-picked */       \
      RotationPaletteManager::clamp_euler_angles(palette, tm, &paletteId);                        \
    pos.y += getRandom(seed, yOffset);                                                            \
    tm.setcol(3, pos);                                                                            \
    if (destrExcl.isMarked(pos.x, pos.z))                                                         \
    {                                                                                             \
      tm.setcol(0, Point3(0, 0, 0));                                                              \
      tm.setcol(1, Point3(0, 0, 0));                                                              \
      tm.setcol(2, Point3(0, 0, 0));                                                              \
    }                                                                                             \
    pool.addEntity(ctx, tm, posInst, ent_remap[sep.entityIdx], paletteRotation ? paletteId : -1); \
  }                                                                                               \
  else                                                                                            \
    rnd(seed);

  real dx = lcd.tile.x;
  real dy = lcd.tile.y;
  if (!(dx > 0 && dy > 0)) // a zero tile places nothing, and its tile coordinate would be NaN
    return;
  BBox2 rect(Point2(world0_x, world0_y), Point2(world0_x + box, world0_y + box));
  rect[0].x += mask_ofs_x - entity_ofs_x;
  rect[0].y += mask_ofs_z - entity_ofs_z;
  rect[1].x += mask_ofs_x - entity_ofs_x;
  rect[1].y += mask_ofs_z - entity_ofs_z;
  const Point2 eofs(entity_ofs_x, entity_ofs_z); // rect is in this frame, pos in world metres
  land_detile::ClusterWalk walk(lcd.detile, eofs, lcd.detileClusterTable);
  int kLo, kHi;
  land_detile::NoiseCache nc;
  walk.clusterRange(rect, kLo, kHi, nc);
  // the pattern rect and its tiles depend on the cluster only; tx0 is a whole tile, so the counted index equals the divided one
  struct ClusterTiles
  {
    BBox2 wr;
    real tx0, ty0, tx1, ty1;
    int tileX0, tileY0;
  } tiles[LAND_DETILE_MAX_CLUSTERS];
  for (int k = kLo; k <= kHi; k++)
  {
    walk.select(k);
    ClusterTiles &t = tiles[k];
    t.wr = walk.walkRect(rect);
    t.tx0 = floor(t.wr[0].x / dx) * dx, t.ty0 = floor(t.wr[0].y / dy) * dy;
    t.tx1 = ceil(t.wr[1].x / dx) * dx, t.ty1 = ceil(t.wr[1].y / dy) * dy;
    t.tileX0 = int(floorf(t.tx0 / dx + 0.5f)), t.tileY0 = int(floorf(t.ty0 / dy + 0.5f));
  }

  for (int i = 0; i < lcd.data.size(); i++)
  {
    const rendinst::gen::land::SingleEntityPlaces &sep = lcd.data[i];
    if (sep.entityIdx < 0)
      continue;
    rendinst::gen::SingleEntityPool &pool = pools[sep.entityIdx];
    bool posInst = sep.posInst;
    bool paletteRotation = sep.paletteRotation;
    RotationPaletteManager::Palette palette; // every cluster of the entity shares it
    if (walk.on() && paletteRotation)
      palette = get_rotation_palette_manager()->getPalette({layer_idx, int(ent_remap[sep.entityIdx])});
    const Point2 &yOffset = sep.yOffset;
    unsigned orientType = sep.orientType;

    for (int k = kLo; k <= kHi; k++)
    {
      walk.select(k);
      const ClusterTiles &t = tiles[k];
      const BBox2 &wr = t.wr;
      int seed = sep.rseed;

      int tileX = t.tileX0, tileY = t.tileY0;
      for (real mx = t.tx0; mx < t.tx1; mx += dx, tileX++, tileY = t.tileY0)
        for (real my = t.ty0; my < t.ty1; my += dy, tileY++)
        {
          if (walk.on()) // de-tiled: only the spots in the rotated rect, each with its own seed
          {
            for (int j = sep.byX.lowerBound(wr[0].x - mx); j < sep.byX.x.size() && sep.byX.x[j] <= wr[1].x - mx; j++)
            {
              const int tmi = sep.byX.idx[j];
              Point3 pos = sep.tm[tmi].getcol(3);
              if (pos.z < wr[0].y - my || pos.z > wr[1].y - my)
                continue;
              seed = land_detile::tiled_spot_seed(sep.rseed, tileX, tileY, k, tmi);
              ADD_ENTITY_BYMASK;
            }
            continue;
          }
          const bool tileInside = mx >= rect[0].x && my >= rect[0].y && mx + dx <= rect[1].x && my + dy <= rect[1].y;
          for (int tmi = sep.tm.size() - 1; tmi >= 0; tmi--)
          {
            Point3 pos = sep.tm[tmi].getcol(3);
            if (!tileInside && !(rect & Point2(pos.x + mx, pos.z + my)))
              continue;
            ADD_ENTITY_BYMASK;
          }
        }
    }
  }
#undef ADD_ENTITY_BYMASK
}
} // namespace rendinst::gen
