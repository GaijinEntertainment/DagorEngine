//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <math/dag_bezierPrec.h>
#include <math/dag_TMatrix.h>
#include <math/dag_mathUtils.h>
#include <generic/dag_tab.h>
#include <de3_objEntity.h>
#include <de3_interface.h>
#include <de3_splineClassData.h>
#include <de3_genObjUtil.h>
#include <util/dag_string.h>
#include <util/dag_globDef.h>
#include <de3_baseInterfaces.h>
#include <de3_randomSeed.h>
#include <de3_cableSrv.h>

namespace objgenerator
{
static int seed = 1;
static int gen_inst_seed = 0;

static Tab<Point3> prevCablePoints;
static ICableService *cableSrv;
inline real getRandom(const Point2 &r) { return (*(int *)&r.y) ? r.x + _srnd(seed) * r.y : r.x; }

inline const splineclass::SingleGenEntityGroup::GenEntityRec &selectGeom(const splineclass::SingleGenEntityGroup &gItem,
  int &inout_ord, bool last_obj)
{
  int tag = -1;
  if (inout_ord == -1)
  {
    // first
    if (gItem.genTagFirst.size())
      tag = gItem.genTagFirst[_rnd(seed) % gItem.genTagFirst.size()];
    else
    {
      inout_ord++;
      if (gItem.genTagSeq.size())
        tag = gItem.genTagSeq[inout_ord % gItem.genTagSeq.size()];
    }
  }
  else if (last_obj && gItem.genTagLast.size())
  {
    // last
    tag = gItem.genTagLast[_rnd(seed) % gItem.genTagLast.size()];
  }
  else if (gItem.genTagSeq.size())
    tag = gItem.genTagSeq[inout_ord % gItem.genTagSeq.size()];
  inout_ord++;

  if (gItem.genEntRecs.size() - gItem.integralLastEntCount == 1)
  {
    _rnd(seed);
    return gItem.genEntRecs[0];
  }

  float w = _frnd(seed) * (tag < 0 ? gItem.sumWeight : gItem.genTagSumWt[tag]);

  for (int i = 0; i < gItem.genEntRecs.size() - gItem.integralLastEntCount; ++i)
  {
    if (tag >= 0 && gItem.genEntRecs[i].genTag != tag)
      continue;
    w -= gItem.genEntRecs[i].weight;
    if (w <= 0)
      return gItem.genEntRecs[i];
  }

  return gItem.genEntRecs[0];
}

inline void getTm(TMatrix &tm, const splineclass::SingleGenEntityGroup &groop,
  const splineclass::SingleGenEntityGroup::GenEntityRec &gen, const Point3 &pos, const Point3 &tang, float xofs, float zofs,
  float force_x_scale = 0)
{
  Point3 dx = normalizeDef(tang, Point3(1, 0, 0));
  Point3 dz = normalizeDef(Point3(-dx.z, 0, dx.x), Point3(0, 0, 1));
  TMatrix spline_tm = TMatrix::IDENT;
  MpPlacementRec mppRec;
  bool is_multi_place = gen.mpRec.mpOrientType != MpPlacementRec::MP_ORIENT_NONE;

  if (is_multi_place || groop.orientType == splineclass::SingleGenEntityGroup::ORIENT_SPLINE)
  {
    spline_tm.setcol(2, dz);
    spline_tm.setcol(1, normalize(spline_tm.getcol(2) % dx));
    spline_tm.setcol(0, dx);
  }
  else if (groop.orientType == splineclass::SingleGenEntityGroup::ORIENT_SPLINE_UP)
  {
    spline_tm.setcol(2, normalizeDef(dx % Point3(0, 1, 0), Point3(0, 0, 1)));
    spline_tm.setcol(1, normalize(spline_tm.getcol(2) % dx));
    spline_tm.setcol(0, dx);
  }

  Point3 worldPos = pos + dx * xofs + dz * zofs;

  Point3 norm(0, 1, 0);
  if (groop.colliders.size())
  {
    if (is_multi_place)
    {
      TMatrix rot_tm = TMatrix::IDENT;
      calc_matrix_33(gen, rot_tm, seed, force_x_scale);
      rot_tm = spline_tm * rot_tm;
      mppRec = gen.mpRec;
      place_multipoint(mppRec, worldPos, rot_tm, groop.aboveHt);
    }
    else
      place_on_ground(worldPos, norm, groop.aboveHt);
  }

  if (groop.orientType == splineclass::SingleGenEntityGroup::ORIENT_FENCE ||
      groop.orientType == splineclass::SingleGenEntityGroup::ORIENT_SPLINE ||
      groop.orientType == splineclass::SingleGenEntityGroup::ORIENT_SPLINE_UP)
  {
    if (groop.orientType == splineclass::SingleGenEntityGroup::ORIENT_FENCE)
    {
      dx = normalizeDef(Point3::x0z(dx), Point3(1, 0, 0));
      tm.setcol(0, dx);
      tm.setcol(1, Point3(0, 1, 0));
      tm.setcol(2, normalize(Point3(-dx.z, 0, dx.x)));
    }
    else
      tm = spline_tm;
  }
  else if (groop.orientType == groop.ORIENT_NORMAL || groop.orientType == groop.ORIENT_NORMAL_XZ)
  {
    if (groop.orientType == groop.ORIENT_NORMAL_XZ)
      tm.setcol(0, norm % Point3(-norm.z, 0, norm.x));
    else if (groop.orientType == groop.ORIENT_WORLD_XZ)
    {
      if (fabsf(norm.x) + fabsf(norm.z) > 1e-5f)
        tm.setcol(0, Point3(0, 1, 0) % Point3(-norm.z, 0, norm.x));
      else
        tm.setcol(0, Point3(1, 0, 0));
      norm.set(0, 1, 0);
    }
    else
      tm.setcol(0, norm % Point3(0, 0, 1));
    float xAxisLength = length(tm.getcol(0));
    if (xAxisLength <= 1.192092896e-06F)
      tm.setcol(0, normalize(norm % Point3(1, 0, 0)));
    else
      tm.setcol(0, tm.getcol(0) / xAxisLength);
    tm.setcol(1, norm);
    tm.setcol(2, normalize(tm.getcol(0) % norm));
  }
  else if (groop.orientType == splineclass::SingleGenEntityGroup::ORIENT_FENCE_NORM)
  {
    dx = normalizeDef(Point3::x0z(dx), Point3(1, 0, 0));
    Point3 pn = normalize(dx % Point3(0, 1, 0));
    norm -= pn * (pn * norm);
    tm.setcol(1, norm);
    tm.setcol(2, normalize(dx % norm));
    tm.setcol(0, normalize(norm % tm.getcol(2)));
  }
  else
    tm = TMatrix::IDENT;

  if (is_multi_place)
    rotate_multipoint(tm, mppRec);
  calc_matrix_33(gen, tm, seed, force_x_scale);
  tm.setcol(3, worldPos + normalize(tm.getcol(1)) * getRandom(gen.yOffset));
}
inline Point3 calcOffsettedSplinePoint(BezierSplinePrec3d &bezierSpline, float pos, float xofs, float zofs)
{
  Point3 p0 = bezierSpline.get_pt(pos);
  if (xofs || zofs)
  {
    Point3 dx = normalize(bezierSpline.get_tang(pos));
    Point3 dz = normalize(Point3(-dx.z, 0, dx.x));
    return p0 + dx * xofs + dz * zofs;
  }
  return p0;
}
inline void placeCables(Tab<cable_handle_t> &cablesPool, const TMatrix &tm,
  const splineclass::SingleGenEntityGroup::GenEntityRec::CablePoints &cablePoints, const splineclass::SingleGenEntityGroup &gItem,
  bool last_obj)
{
  if (!cableSrv)
    cableSrv = EDITORCORE->queryEditorInterface<ICableService>();
  if (!cableSrv)
    return;

  if (prevCablePoints.empty())
  {
    // Add ragged cables on the first pole
    if (gItem.cableRaggedDistribution.lengthSq() > REAL_EPS) // ragged cable length is set
    {
      int savedSeed = seed; // to save random calcuted things when artists add ragged cables on existing spline
      for (int i = 0; i < cablePoints.in.size(); ++i)
      {
        cablesPool.push_back(
          cableSrv->addCable(cablePoints.in[i] * tm, cablePoints.in[i] * tm + Point3(0, -getRandom(gItem.cableRaggedDistribution), 0),
            -getRandom(gItem.cableRadius), // ragged cable bit is a sign bit of cable radius
            0));
      }
      seed = savedSeed;
    }
  }

  size_t n = min(prevCablePoints.size(), cablePoints.in.size());
  for (int i = 0; i < n; ++i)
  {
    cable_handle_t id =
      cableSrv->addCable(prevCablePoints[i], cablePoints.in[i] * tm, getRandom(gItem.cableRadius), getRandom(gItem.cableSag));
    cablesPool.push_back(id);
  }
  prevCablePoints.clear();
  for (int i = 0; i < cablePoints.out.size(); ++i)
    prevCablePoints.push_back(cablePoints.out[i] * tm);

  // Add ragged cables on the last pole
  if (last_obj)
  {
    if (gItem.cableRaggedDistribution.lengthSq() > REAL_EPS) // ragged cable length is set
    {
      int savedSeed = seed; // to save random calcuted things when artists add ragged cables on existing spline
      for (int i = 0; i < n; ++i)
      {
        cablesPool.push_back(cableSrv->addCable(cablePoints.out[i] * tm,
          cablePoints.out[i] * tm + Point3(0, -getRandom(gItem.cableRaggedDistribution), 0),
          -getRandom(gItem.cableRadius), // ragged cable bit is a sign bit of cable radius
          0));
      }
      seed = savedSeed;
    }
  }
}
inline float placeObject1(const splineclass::SingleGenEntityGroup &gItem, int &inout_ord, real pos, real pos_max,
  Tab<splineclass::SingleEntityPool> &pools, Tab<cable_handle_t> *cablesPool, splineclass::GenEntities &ent,
  BezierSplinePrec3d &bezierSpline, bool last_obj = false)
{
  const splineclass::SingleGenEntityGroup::GenEntityRec *objGen = &selectGeom(gItem, inout_ord, last_obj);
  float ret_step = pos_max - pos;

  TMatrix tm;
  float xofs = getRandom(objGen->xOffset), zofs = getRandom(objGen->zOffset) + gItem.offset.y;
  if (gItem.tightFenceOrient)
  {
    bool xyz = (gItem.orientType == splineclass::SingleGenEntityGroup::ORIENT_SPLINE ||
                gItem.orientType == splineclass::SingleGenEntityGroup::ORIENT_SPLINE_UP);
    Point3 p0 = calcOffsettedSplinePoint(bezierSpline, pos, xofs, zofs), p1 = p0;
    float exact_width = 0, force_x_scale = 0;
    if (last_obj && gItem.tightFenceIntegral)
    {
      p1 = calcOffsettedSplinePoint(bezierSpline, pos_max, xofs, zofs);
      exact_width = xyz ? length(p1 - p0) : Point2::xz(p1 - p0).length();
    }
    if (exact_width > 0 && exact_width < objGen->width)
    {
      int sel_idx = -1;
      float sel_scale = 10;
      for (int i = gItem.genEntRecs.size() - gItem.integralLastEntCount; i < gItem.genEntRecs.size(); ++i)
      {
        const splineclass::SingleGenEntityGroup::GenEntityRec &er = gItem.genEntRecs[i];
        float wd_min = er.width * (er.scale.x - er.scale.y);
        float wd_max = er.width * (er.scale.x + er.scale.y);
        if (wd_min <= exact_width && exact_width <= wd_max)
          if (sel_idx < 0 || fabsf(exact_width / (er.width * er.scale.x) - 1) < fabsf(sel_scale - 1))
            sel_idx = i, sel_scale = exact_width / (er.width * er.scale.x);
      }
      if (sel_idx < 0)
        return ret_step;
      objGen = &gItem.genEntRecs[sel_idx];
      force_x_scale = sel_scale * objGen->scale.x;
    }
    else
    {
      float len2 = objGen->width * objGen->width, endp = pos;
      p1 = p0;
      while (endp <= pos_max && (xyz ? lengthSq(p1 - p0) : Point2::xz(p1 - p0).lengthSq()) < len2)
        p1 = calcOffsettedSplinePoint(bezierSpline, endp += 0.02, xofs, zofs);
      ret_step = endp - pos;

      float l = (xyz ? length(p1 - p0) : Point2::xz(p1 - p0).length());
      if (l < objGen->width && l > 0)
        p1 = p0 + (p1 - p0) * (objGen->width / l);
    }
    getTm(tm, gItem, *objGen, (p0 + p1) * .5f, normalize(p1 - p0), 0, 0, force_x_scale);
  }
  else
    getTm(tm, gItem, *objGen, bezierSpline.get_pt(pos), bezierSpline.get_tang(pos), xofs, zofs);
  if (tm.getcol(3).y < gItem.minGenHt || tm.getcol(3).y > gItem.maxGenHt)
    return ret_step;
  if (gItem.checkSweep2 && !is_place_allowed_g(tm.getcol(3).x, tm.getcol(3).z))
    return ret_step;
  if (gItem.checkSweep1 && !is_place_allowed(tm.getcol(3).x, tm.getcol(3).z))
    return ret_step;

  splineclass::SingleEntityPool &pool = pools[objGen->entIdx];

  IObjEntity *e = NULL;
  if (pool.entUsed < pool.entPool.size())
  {
    e = pool.entPool[pool.entUsed];
    pool.entUsed++;
  }
  else
  {
    e = IDaEditor3Engine::get().cloneEntity(ent.ent[objGen->entIdx]);
    if (!e)
      return ret_step;
    pool.entPool.push_back(e);
    pool.entUsed++;
  }
  e->setSubtype(IObjEntity::ST_NOT_COLLIDABLE);
  if (IRandomSeedHolder *irsh = e->queryInterface<IRandomSeedHolder>())
  {
    if (gItem.setSeedToEntities)
      irsh->setSeed(seed);
    irsh->setPerInstanceSeed(gen_inst_seed);
  }
  e->setTm(tm);
  if (cablesPool)
  {
    placeCables(*cablesPool, tm, objGen->cablePoints, gItem, last_obj);
  }

  IColor *ecol = e->queryInterface<IColor>();
  if (ecol)
    ecol->setColor(gItem.colorRangeIdx), _skip_rnd_ivec4(seed);
  return ret_step;
}
inline void placeObject(const splineclass::SingleGenEntityGroup &gItem, int &inout_ord, real &pos,
  Tab<splineclass::SingleEntityPool> &pools, Tab<cable_handle_t> *cablesPool, splineclass::GenEntities &ent,
  BezierSplinePrec3d &bezierSpline, float eT)
{
  // object must be placed between pos and pos + step
  float step = getRandom(gItem.step);
  float pos_max = min(pos + step, eT);
  // set flag: is it last object in spline?
  // if placeAtEnd is true, last object will be placed in separate placeObject1 call
  bool last_obj = (pos + step >= eT) & !gItem.placeAtEnd;
  // update pos with real step: it may be (pos_max - pos) or lesser [if tightFenceOrient is true]
  step = placeObject1(gItem, inout_ord, pos, pos_max, pools, cablesPool, ent, bezierSpline, last_obj);
  pos += step;
}

inline void generateBySplinePart(BezierSplinePrec3d &bezierFullSpline, BezierSplinePrec3d *bezierLoftSpline, bool start_seg,
  bool end_seg, int start_s, int end_s, splineclass::GenEntities &ent, const splineclass::SingleGenEntityGroup &gItem,
  Tab<splineclass::SingleEntityPool> &pools, Tab<cable_handle_t> *cablesPool, bool close, int subtype, int editLayerIdx, int rseed,
  int &ord)
{
  TMatrix tm;
  real startT = 0, endT = 0;

  for (int i = 0; i < start_s; i++)
    startT += bezierFullSpline.segs[i].len;

  endT = startT;
  G_ASSERT(end_s <= bezierFullSpline.segs.size());
  for (int i = start_s; i < end_s; i++)
    endT += bezierFullSpline.segs[i].len;

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
  {
    BezierSplinePrec3d &bezierSpline = gItem.useLoftSegs ? *bezierLoftSpline : bezierFullSpline;
    real sT = startT, eT = endT;

    if (gItem.useLoftSegs)
    {
      // bezierLoftSpline represents only relevant part of spline, so remove startgenereal start offset
      sT = 0;
      eT = bezierSpline.getLength();
    }

    bool place_at_start = gItem.placeAtStart || (gItem.placeAtVeryStart && start_s == 0);
    bool place_at_end = gItem.placeAtEnd || (gItem.placeAtVeryEnd && end_s == bezierFullSpline.segs.size());

    if (gItem.colliders.size() && (gItem.step.x > 0 || place_at_start || place_at_end))
      EDITORCORE->setColliders(gItem.colliders, gItem.collFilter);

    seed = rseed + gItem.rseed;

    if (gItem.step.x <= 0)
    {
      startT += gItem.offset.x;
      endT += gItem.offset.x;
      if (gItem.placeAtPoint)
      {
        if (start_seg && gItem.placeAtStart)
          placeObject1(gItem, ord, startT, startT, pools, cablesPool, ent, bezierSpline);
        if (!end_seg || gItem.placeAtEnd)
          placeObject1(gItem, ord, endT, endT, pools, cablesPool, ent, bezierSpline);
      }
      else
      {
        if (place_at_start)
          placeObject1(gItem, ord, startT, startT, pools, cablesPool, ent, bezierSpline);
        if (place_at_end)
          placeObject1(gItem, ord, endT, endT, pools, cablesPool, ent, bezierSpline);
      }
      SET_SUBTYPES(subtype);
      SET_EDITLAYERIDX(editLayerIdx);
      return;
    }

    real startStep = getRandom(gItem.step);
    if (!gItem.placeAtPoint)
    {
      real k = (startT + gItem.offset.x) / startStep;
      real startOffs = (k - floor(k) - 1) * fabs(startStep);
      startStep += startOffs; // to save step when switch to next segment
    }

    if (!gItem.tightFenceOrient)
      sT += startStep;

    if (start_seg && place_at_start && gItem.tightFenceOrient)
      sT += placeObject1(gItem, ord, startT, startT + gItem.step.x, pools, cablesPool, ent, bezierSpline);
    else if (start_seg && place_at_start)
    {
      placeObject1(gItem, ord, startT, startT, pools, cablesPool, ent, bezierSpline);
      while (sT - startT < gItem.step.x * 0.5)
        sT += gItem.step.x;
    }
    if ((end_seg && place_at_end) || (!end_seg && gItem.placeAtPoint))
      eT -= gItem.step.x * 0.5;

    if (sT < gItem.startPadding)
      sT = gItem.startPadding;
    if (eT > bezierSpline.getLength() - gItem.endPadding)
      eT = bezierSpline.getLength() - gItem.endPadding;

    real pos = sT;
    while (pos < eT - 1e-5f) // pos updates in placeObject, check for loss of precision
      placeObject(gItem, ord, pos, pools, cablesPool, ent, bezierSpline, eT);

    if ((end_seg && place_at_end) || (!end_seg && gItem.placeAtPoint))
      placeObject1(gItem, ord, endT, endT, pools, cablesPool, ent, bezierSpline, (end_seg && place_at_end));

    if (close)
    {
      Point3 p1 = bezierSpline.get_pt(0);
      Point3 p2 = bezierSpline.get_pt(bezierSpline.getLength());

      Point3 diff = p1 - p2;
      real ln = diff.length();

      if (fabs(ln) > 1e-3)
      {
        pos = 0;
        while (pos < ln)
        {
          real t = pos / ln;
          Point3 pp = p1 * t + p2 * (1 - t);
          placeObject(gItem, ord, pos, pools, cablesPool, ent, bezierSpline, ln);
        }
      }
    }
  }

  SET_SUBTYPES(subtype);
  SET_EDITLAYERIDX(editLayerIdx);
#undef SET_SUBTYPES
#undef SET_EDITLAYERIDX
}
inline void generateBySpline(BezierSplinePrec3d &bezierFullSpline, BezierSplinePrec3d *bezierLoftSpline, int start_s, int end_s,
  const splineclass::AssetData *asset, Tab<splineclass::SingleEntityPool> &pools, Tab<cable_handle_t> *cablesPool, bool close,
  int subtype, int editLayerIdx, int rseed, int inst_seed)
{
  if (!asset || !asset->gen)
    return;

  splineclass::GenEntities &ent = *asset->gen;
  objgenerator::gen_inst_seed = inst_seed;
  pools.resize(ent.ent.size());

  for (int gi = 0; gi < ent.data.size(); ++gi)
  {
    const splineclass::SingleGenEntityGroup &gItem = ent.data[gi];
    if (gItem.useLoftSegs && !bezierLoftSpline)
      continue;

    prevCablePoints.clear();
    bool single_seg = gItem.tightFenceIntegralPerSegment;
    bool corner_seg = gItem.tightFenceIntegralPerCorner;
    bool whole_spline = (gItem.useLoftSegs || !gItem.tightFenceOrient || (!single_seg && !corner_seg)) && !gItem.placeAtPoint;
    for (int s0 = start_s, s1 = s0, ord = -1; s0 < end_s; s0 = s1)
    {
      if (whole_spline)
        s1 = end_s;
      else if (corner_seg)
      {
        for (s1 = s0 + 1; s1 < end_s; s1++) // stop on point where tangent differs > ~1 degree
          if (fabsf(normalize(bezierFullSpline.segs[s1 - 1].tang(1)) * normalize(bezierFullSpline.segs[s1].tang(0))) < 0.9999f)
            break;
      }
      else // single_seg
        s1 = s0 + 1;

      generateBySplinePart(bezierFullSpline, bezierLoftSpline, s0 == start_s, s1 == end_s, s0, s1, ent, gItem, pools, cablesPool,
        s1 == end_s ? close : false, subtype, editLayerIdx, rseed++, ord);
    }
  }
}
} // namespace objgenerator
