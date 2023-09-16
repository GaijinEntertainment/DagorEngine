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
#include <de3_genObjUtil.h>
#include <de3_randomSeed.h>


namespace objgenerator
{
template <class Point>
inline bool point_inside_poly(const Point2 &p, dag::ConstSpan<Point *> points)
{
  int pj, pk = 0;
  double wrkx, yu, yl;

  for (pj = 0; pj < points.size(); pj++)
  {
    Point3 ppj = points[pj]->getPt();
    Point3 ppj1 = points[(pj + 1) % points.size()]->getPt();

    yu = ppj.z > ppj1.z ? ppj.z : ppj1.z;
    yl = ppj.z < ppj1.z ? ppj.z : ppj1.z;

    if (ppj1.z - ppj.z)
      wrkx = ppj.x + (ppj1.x - ppj.x) * (p.y - ppj.z) / (ppj1.z - ppj.z);
    else
      wrkx = ppj.x;

    if (yu >= p.y)
      if (yl < p.y)
      {
        if (p.x > wrkx)
          pk++;
        if (fabs(p.x - wrkx) < 0.001)
          return true;
      }

    if ((fabs(p.y - yl) < 0.001) && (fabs(yu - yl) < 0.001) &&
        (fabs(fabs(wrkx - ppj.x) + fabs(wrkx - ppj1.x) - fabs(ppj.x - ppj1.x)) < 0.001))
      return true;
  }

  if (pk % 2)
    return true;
  else
    return false;

  return false;
}

template <class Point>
inline void generateTiledEntitiesInsidePoly(const landclass::TiledEntities &lcd, int subtype, int editLayerIdx, IHeightmap *hmap,
  dag::Span<landclass::SingleEntityPool> pools, dag::ConstSpan<Point *> points, float ang, const Point2 &polyObjOffs)
{
  bool needRotate = (fabs(ang) > 1e-5);
  BBox2 polyBBox;
  polyBBox.setempty();
  TMatrix roty;
  float mid_y = 0;

  for (int i = 0; i < points.size(); i++)
  {
    Point3 p3 = points[i]->getPt();
    polyBBox += Point2(p3.x, p3.z);
    mid_y += p3.y / points.size();
  }

  Point3 center(polyBBox.center().x, 0, polyBBox.center().y);
  if (needRotate)
  {
    roty = rotyTM(ang);
    for (int i = 0; i < points.size(); i++)
    {
      Point3 pos = points[i]->getPt();
      Point3 diff = pos - center;
      diff = roty * diff;
      pos = diff + center;
      points[i]->setPos(pos);
    }
    roty = rotyTM(-ang);
  }

  Point2 offs;
  real dx = lcd.tile.x;
  real dy = lcd.tile.y;

  real k = polyBBox.lim[0].x / dx;
  offs.x = (k - floor(k) - 1) * fabs(dx);

  k = polyBBox.lim[0].y / dy;
  offs.y = (k - floor(k) - 1) * fabs(dy);

  IPoint2 mapped = IPoint2(0, 0);

  real limX = polyBBox.lim[1].x + dx;
  real limY = polyBBox.lim[1].y + dy;

  real startX = polyBBox.lim[0].x - offs.x - dx;
  real startY = polyBBox.lim[0].y - offs.y - dy;

#define SET_SUBTYPES(x)                        \
  for (int i = 0; i < lcd.data.size(); i++)    \
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
  if (lcd.colliders.size())
    EDITORCORE->setColliders(lcd.colliders, lcd.collFilter);

  SET_SUBTYPES(IObjEntity::ST_NOT_COLLIDABLE);

  for (int i = 0; i < lcd.data.size(); i++)
  {
    const landclass::SingleEntityPlaces &sep = lcd.data[i];
    unsigned orientType = sep.orientType;

    landclass::SingleEntityPool &pool = pools[i];

    int obj_idx = pool.entUsed, ent_avail = pool.entPool.size();
    int seed = sep.rseed;

    for (real mx = startX; mx <= limX; mx += dx)
      for (real my = startY; my <= limY; my += dy)
        for (int tmi = sep.tm.size() - 1; tmi >= 0; tmi--)
        {
          Point3 pos3 = sep.tm[tmi].getcol(3);
          Point3 newPos(pos3.x + polyObjOffs.x, mid_y, pos3.z + polyObjOffs.y);

          newPos.x += mx - floor(newPos.x / dx) * dx;
          newPos.z += my - floor(newPos.z / dy) * dy;

          if (point_inside_poly(Point2(newPos.x, newPos.z), points))
          {
            TMatrix tm = sep.tm[tmi];

            MpPlacementRec mppRec;
            bool is_multi_place = sep.mpRec.mpOrientType != MpPlacementRec::MP_ORIENT_NONE;
            TMatrix rot_tm = tm; // TMatrix::IDENT;
            rot_tm.setcol(3, Point3(0, 0, 0));
            if (lcd.colliders.size() && needRotate && is_multi_place)
            {
              rot_tm.setcol(3, rot_tm.getcol(3) - center);
              rot_tm = roty * rot_tm;
              rot_tm.setcol(3, rot_tm.getcol(3) + center);
            }

            if (orientType == sep.ORIENT_WORLD)
            {
              hmap ? hmap->getHeightmapPointHt(newPos, NULL) : false;
              if (lcd.colliders.size())
              {
                if (is_multi_place)
                {
                  mppRec = sep.mpRec;
                  place_multipoint(mppRec, newPos, rot_tm, lcd.aboveHt);
                }
                else
                  place_on_ground(newPos, lcd.aboveHt);
              }
            }
            else
            {
              TMatrix rot;
              Point3 norm(0, 1, 0);
              hmap ? hmap->getHeightmapPointHt(newPos, &norm) : false;
              if (lcd.colliders.size())
              {
                if (is_multi_place)
                {
                  mppRec = sep.mpRec;
                  place_multipoint(mppRec, newPos, rot_tm, lcd.aboveHt);
                }
                else
                  place_on_ground(newPos, norm, lcd.aboveHt);
              }
              if (orientType == sep.ORIENT_NORMAL_XZ)
                rot.setcol(0, norm % Point3(-norm.z, 0, norm.x));
              else if (orientType == sep.ORIENT_WORLD_XZ)
              {
                if (fabsf(norm.x) + fabsf(norm.z) > 1e-5f)
                  rot.setcol(0, tm.getcol(1) % Point3(-norm.z, 0, norm.x));
                else
                  rot.setcol(0, tm.getcol(1) % Point3(0, 0, 1));
                norm = tm.getcol(1);
              }
              else
                rot.setcol(0, norm % Point3(0, 0, 1));
              float xAxisLength = length(rot.getcol(0));
              if (xAxisLength <= 1.192092896e-06F)
                rot.setcol(0, normalize(norm % Point3(1, 0, 0)));
              else
                rot.setcol(0, rot.getcol(0) / xAxisLength);
              rot.setcol(1, norm);
              rot.setcol(2, normalize(rot.getcol(0) % norm));
              tm = rot % tm;
            }

            if (is_multi_place)
              rotate_multipoint(tm, mppRec);
            newPos.y += getRandom(seed, sep.yOffset);
            if (lcd.useYpos)
              newPos.y += pos3.y;
            tm.setcol(3, newPos);

            if (needRotate)
            {
              tm.setcol(3, tm.getcol(3) - center);
              tm = roty * tm;
              tm.setcol(3, tm.getcol(3) + center);
            }

            IObjEntity *e = NULL;
            if (obj_idx < ent_avail)
              e = pool.entPool[obj_idx];
            else
            {
              e = IDaEditor3Engine::get().cloneEntity(sep.entity);
              if (!e)
                continue;
              pool.entPool.push_back(e);
            }
            e->setSubtype(IObjEntity::ST_NOT_COLLIDABLE);
            if (lcd.setSeedToEntities)
              if (IRandomSeedHolder *irsh = e->queryInterface<IRandomSeedHolder>())
                irsh->setSeed(seed);
            e->setTm(tm);
            IColor *ecol = e->queryInterface<IColor>();
            if (ecol)
              ecol->setColor(sep.colorRangeIdx), _skip_rnd_ivec4(seed);
            obj_idx++;
          }
          else
            _rnd(seed);
        }
    pool.entUsed = obj_idx;
  }

  SET_SUBTYPES(subtype);
  SET_EDITLAYERIDX(editLayerIdx);
#undef SET_SUBTYPES
#undef SET_EDITLAYERIDX

  if (needRotate)
    for (int i = 0; i < points.size(); i++)
    {
      Point3 pos = points[i]->getPt();
      Point3 diff = pos - center;
      diff = roty * diff;
      pos = diff + center;
      points[i]->setPos(pos);
    }
}
} // namespace objgenerator
