#pragma once

#include "landClassData.h"
#include "genObjUtil.h"

#include <math/dag_bounds2.h>
#include <math/dag_TMatrix.h>


namespace rendinstgen
{
template <class BitMask>
inline void generateTiledEntitiesInMaskedRect(const rendinstgenland::TiledEntities &lcd,
  dag::Span<rendinstgen::SingleEntityPool> pools, const BitMask &mask, float world2sampler, float world0_x, float world0_y, float box,
  float mask_ofs_x, float mask_ofs_z, int16_t *ent_remap, float entity_ofs_x, float entity_ofs_z)
{
#define ADD_ENTITY_BYMASK                                                                    \
  pos.x += mx + entity_ofs_x;                                                                \
  pos.z += my + entity_ofs_z;                                                                \
  align_place_xz(pos);                                                                       \
  int mtx = int(floorf(pos.x * world2sampler) - floorf(world2sampler * mask_ofs_x));         \
  int mtz = int(floorf(pos.z * world2sampler) - floorf(world2sampler * mask_ofs_z));         \
  if (!mask.getClamped(mtx, mtz) || !is_place_allowed(pos.x, pos.z))                         \
  {                                                                                          \
    _rnd(seed);                                                                              \
    continue;                                                                                \
  }                                                                                          \
  int paletteId = 0;                                                                         \
  if (pool.tryAdd(pos.x, pos.z))                                                             \
  {                                                                                          \
    TMatrix tm = sep.tm[tmi];                                                                \
                                                                                             \
    MpPlacementRec mppRec;                                                                   \
    bool is_multi_place = sep.mpRec.mpOrientType != MpPlacementRec::MP_ORIENT_NONE;          \
    TMatrix rot_tm = tm;                                                                     \
    rot_tm.setcol(3, Point3(0, 0, 0));                                                       \
                                                                                             \
    if (orientType == sep.ORIENT_WORLD)                                                      \
    {                                                                                        \
      custom_get_height(pos, NULL);                                                          \
      if (is_multi_place)                                                                    \
      {                                                                                      \
        mppRec = sep.mpRec;                                                                  \
        place_multipoint(mppRec, pos, rot_tm, lcd.aboveHt);                                  \
      }                                                                                      \
      else                                                                                   \
        place_on_ground(pos, lcd.aboveHt);                                                   \
    }                                                                                        \
    else                                                                                     \
    {                                                                                        \
      TMatrix rot;                                                                           \
      Point3 norm(0, 1, 0);                                                                  \
      custom_get_height(pos, &norm);                                                         \
      if (is_multi_place)                                                                    \
      {                                                                                      \
        mppRec = sep.mpRec;                                                                  \
        place_multipoint(mppRec, pos, rot_tm, lcd.aboveHt);                                  \
      }                                                                                      \
      else                                                                                   \
        place_on_ground(pos, norm, lcd.aboveHt);                                             \
      if (orientType == sep.ORIENT_NORMAL_XZ)                                                \
        rot.setcol(0, norm % Point3(-norm.z, 0, norm.x));                                    \
      else if (orientType == sep.ORIENT_WORLD_XZ)                                            \
      {                                                                                      \
        if (fabsf(norm.x) + fabsf(norm.z) > 1e-5f)                                           \
          rot.setcol(0, tm.getcol(1) % Point3(-norm.z, 0, norm.x));                          \
        else                                                                                 \
          rot.setcol(0, tm.getcol(1) % Point3(0, 0, 1));                                     \
        norm = tm.getcol(1);                                                                 \
      }                                                                                      \
      else                                                                                   \
        rot.setcol(0, norm % Point3(0, 0, 1));                                               \
      float xAxisLength = length(rot.getcol(0));                                             \
      if (xAxisLength <= 1.192092896e-06F)                                                   \
        rot.setcol(0, normalize(norm % Point3(1, 0, 0)));                                    \
      else                                                                                   \
        rot.setcol(0, rot.getcol(0) / xAxisLength);                                          \
      rot.setcol(1, norm);                                                                   \
      rot.setcol(2, normalize(rot.getcol(0) % norm));                                        \
      tm = rot % tm;                                                                         \
    }                                                                                        \
    if (is_multi_place)                                                                      \
      rotate_multipoint(tm, mppRec);                                                         \
    pos.y += getRandom(seed, yOffset);                                                       \
    tm.setcol(3, pos);                                                                       \
    if (destrExcl.isMarked(pos.x, pos.z))                                                    \
    {                                                                                        \
      tm.setcol(0, Point3(0, 0, 0));                                                         \
      tm.setcol(1, Point3(0, 0, 0));                                                         \
      tm.setcol(2, Point3(0, 0, 0));                                                         \
    }                                                                                        \
    pool.addEntity(tm, posInst, ent_remap[sep.entityIdx], paletteRotation ? paletteId : -1); \
  }                                                                                          \
  else                                                                                       \
    _rnd(seed);

  BBox2 rect(Point2(world0_x, world0_y), Point2(world0_x + box, world0_y + box));
  rect[0].x += mask_ofs_x - entity_ofs_x;
  rect[0].y += mask_ofs_z - entity_ofs_z;
  rect[1].x += mask_ofs_x - entity_ofs_x;
  rect[1].y += mask_ofs_z - entity_ofs_z;
  real dx = lcd.tile.x;
  real dy = lcd.tile.y;
  real tx0 = floor(rect[0].x / dx) * dx, ty0 = floor(rect[0].y / dy) * dy;
  real tx1 = ceil(rect[1].x / dx) * dx, ty1 = ceil(rect[1].y / dy) * dy;

  for (int i = 0; i < lcd.data.size(); i++)
  {
    const rendinstgenland::SingleEntityPlaces &sep = lcd.data[i];
    if (sep.entityIdx < 0)
      continue;
    rendinstgen::SingleEntityPool &pool = pools[sep.entityIdx];
    bool posInst = sep.posInst;
    bool paletteRotation = sep.paletteRotation;
    const Point2 &yOffset = sep.yOffset;
    int seed = sep.rseed;
    unsigned orientType = sep.orientType;

    for (real mx = tx0; mx < tx1; mx += dx)
      for (real my = ty0; my < ty1; my += dy)
        if (mx >= rect[0].x && my >= rect[0].y && mx + dx <= rect[1].y && my + dy <= rect[1].y)
        {
          for (int tmi = sep.tm.size() - 1; tmi >= 0; tmi--)
          {
            Point3 pos = sep.tm[tmi].getcol(3);
            ADD_ENTITY_BYMASK;
          }
        }
        else
        {
          for (int tmi = sep.tm.size() - 1; tmi >= 0; tmi--)
          {
            Point3 pos = sep.tm[tmi].getcol(3);

            if (!(rect & Point2(pos.x + mx, pos.z + my)))
              continue;

            ADD_ENTITY_BYMASK;
          }
        }
  }
#undef ADD_ENTITY_BYMASK
}
} // namespace rendinstgen
