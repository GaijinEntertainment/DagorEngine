// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_DAGOR_FX_STDTRAILPATH_H
#define _GAIJIN_DAGOR_FX_STDTRAILPATH_H
#pragma once


#include <math/dag_math3d.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <util/dag_globDef.h>
#include <util/dag_stdint.h>
#include <fx/dag_fxInterface.h>


#define FOR_EACH_TRAIL_POINT(trail, p)                                                            \
  for (int pointIndex = (trail)->firstPoint, pointNum = (trail)->numActive; pointNum; --pointNum) \
  {                                                                                               \
    StdTrailPath::TrailPoint &p = (trail)->points[pointIndex];

#define END_FOR_EACH_TRAIL_POINT(trail)       \
  if (++pointIndex >= (trail)->points.size()) \
    pointIndex = 0;                           \
  }


class StdTrailPath
{
public:
  class TrailPoint
  {
  public:
    Point3 pos, vel, dir;
    real time;
  };


  SmallTab<TrailPoint, MidmemAlloc> points;
  int firstPoint, numActive;
  real curTime;
  real segTimeLen, totalTimeLen;

  int keyPoint[4];

  real damping;


  // static Tab<EffectsInterface::StdParticleVertex> renderVerts;
  // static Tab<uint16_t> renderIndices;


  StdTrailPath() : firstPoint(0), numActive(0), segTimeLen(1), totalTimeLen(1), curTime(0), damping(0) {}


  bool isAlive() const { return numActive > 0; }


  /*
  static void resizeRenderBuffers(int num_verts, int num_indices)
  {
    if (renderVerts.size()<num_verts)
      renderVerts.resize(num_verts);

    if (renderIndices.size()<num_indices)
    {
      int i=renderIndices.size();
      renderIndices.resize(num_indices);

      int vi=i/3;
      for ( ; i<renderIndices.size(); i+=6, vi+=2)
      {
        renderIndices[i+0]=vi+0;
        renderIndices[i+1]=vi+1;
        renderIndices[i+2]=vi+2;

        renderIndices[i+3]=vi+1;
        renderIndices[i+4]=vi+3;
        renderIndices[i+5]=vi+2;
      }
    }
  }
  */


  void resize(int num_segs, real time_len)
  {
    if (num_segs < 2)
      num_segs = 2;

    clear_and_resize(points, num_segs + 1);
    firstPoint = 0;
    numActive = 0;
    curTime = 0;

    totalTimeLen = time_len;
    segTimeLen = time_len / (num_segs - 1);

    // resizeRenderBuffers((num_segs+1)*2, num_segs*2*3);
  }


  int addNewPoint(real time)
  {
    if (--firstPoint < 0)
      firstPoint = points.size() - 1;

    TrailPoint &p = points[firstPoint];
    p.time = time;

    if (numActive < points.size())
      ++numActive;
    else
    {
      if (keyPoint[2] == firstPoint)
        keyPoint[0] = keyPoint[1] = keyPoint[2] = keyPoint[3];
      else if (keyPoint[1] == firstPoint)
        keyPoint[0] = keyPoint[1] = keyPoint[2];
      else if (keyPoint[0] == firstPoint)
        keyPoint[0] = keyPoint[1];
    }

    return firstPoint;
  }


  class SplineInt
  {
  public:
    Point3 sk[4];

    SplineInt(const Point3 &p0, const Point3 &v0, const Point3 &v3, const Point3 &p3)
    {
      const float smooth = 0.25f; // 0.333f;
      Point3 v1 = v0 + (v3 - p0) * smooth;
      Point3 v2 = v3 + (v0 - p3) * smooth;

      sk[0] = v0;
      sk[1] = 3.0 * (v1 - v0);
      sk[2] = 3.0 * (v0 - 2.0 * v1 + v2);
      sk[3] = v3 - v0 + 3.0 * (v1 - v2);
    }

    Point3 point(real t) const { return ((sk[3] * t + sk[2]) * t + sk[1]) * t + sk[0]; }
  };


  void adjustPoints(int i0, int i1, int i2, int i3)
  {
    if (i1 == i2)
      return;

    SplineInt pos(points[i0].pos, points[i1].pos, points[i2].pos, points[i3].pos);
    SplineInt vel(points[i0].vel, points[i1].vel, points[i2].vel, points[i3].vel);

    Point3 dir1 = points[i1].dir;
    Point3 dirDelta = points[i2].dir - dir1;

    real t1 = points[i1].time;
    real tk = points[i2].time - t1;
    if (float_nonzero(tk))
      tk = 1 / tk;

    for (int i = i1;;)
    {
      if (--i < 0)
        i = points.size() - 1;

      if (i == i2)
        break;

      real t = (points[i].time - t1) * tk;
      points[i].pos = pos.point(t);
      points[i].vel = vel.point(t);
      points[i].dir = dirDelta * t + dir1;
    }
  }


  void addPoint(const Point3 &pos, const Point3 &vel, const Point3 &dir)
  {
    G_ASSERT(points.size() > 0);

    if (numActive > 0)
    {
      if (numActive >= 2 && curTime - points[keyPoint[2]].time <= segTimeLen)
      {
        G_ASSERT(keyPoint[3] == firstPoint);

        TrailPoint &p = points[firstPoint];
        p.pos = pos;
        p.vel = vel;
        p.dir = dir;
        p.time = curTime;
        return;
      }

      keyPoint[0] = keyPoint[1];
      keyPoint[1] = keyPoint[2];
      keyPoint[2] = keyPoint[3];

      for (real t = points[keyPoint[2]].time + segTimeLen; t < curTime; t += segTimeLen) //-V1034
        addNewPoint(t);

      keyPoint[3] = addNewPoint(curTime);

      TrailPoint &p = points[keyPoint[3]];
      p.pos = pos;
      p.vel = vel;
      p.dir = dir;

      adjustPoints(keyPoint[0], keyPoint[1], keyPoint[2], keyPoint[3]);
      adjustPoints(keyPoint[1], keyPoint[2], keyPoint[3], keyPoint[3]);
    }
    else
    {
      TrailPoint &p = points[firstPoint];
      p.pos = pos;
      p.vel = vel;
      p.dir = dir;
      p.time = curTime;

      keyPoint[0] = firstPoint;
      keyPoint[1] = firstPoint;
      keyPoint[2] = firstPoint;
      keyPoint[3] = firstPoint;

      numActive = 1;
    }
  }


  void updatePoints(real dt)
  {
    real velK = 1 - damping * dt;
    if (velK < 0)
      velK = 0;

    FOR_EACH_TRAIL_POINT(this, p)
    p.pos += p.vel * dt;
    p.vel *= velK;
    END_FOR_EACH_TRAIL_POINT(this)
  }


  void updateTrail(real dt)
  {
    if (numActive <= 0)
      return;

    curTime += dt;

    real endTime = curTime - totalTimeLen;

    if (points[firstPoint].time <= endTime)
    {
      numActive = 0;
      firstPoint = 0;
      curTime = 0;
      return;
    }

    if (numActive > 2)
    {
      // remove old points from the end
      int i = firstPoint + numActive - 2;
      if (i >= (int)points.size())
        i -= points.size();

      int n;
      for (n = numActive - 2; n > 0; --n)
      {
        if (points[i].time > endTime)
          break;

        if (--i < 0)
          i = points.size() - 1;
      }

      numActive = n + 2;
    }

    // avoid FP precision loss
    const real MAX_TIME = 1000;
    if (curTime > MAX_TIME)
    {
      curTime -= MAX_TIME;

      FOR_EACH_TRAIL_POINT(this, p)
      p.time -= MAX_TIME;
      END_FOR_EACH_TRAIL_POINT(this)
    }
  }


  int getQuadCount() const { return (numActive < 2) ? 0 : numActive - 1; }
};


#define START_STD_TRAIL_RENDER(trail, buf_ptr, num_quads, total_quads)                        \
  if ((trail)->numActive >= 2)                                                                \
  {                                                                                           \
    EffectsInterface::StdParticleVertex *vptr = (buf_ptr);                                    \
    int numq = (num_quads), totq = (total_quads);                                             \
    int lastPointIndex = (trail)->firstPoint + (trail)->numActive - 1;                        \
    if (lastPointIndex >= (trail)->points.size())                                             \
      lastPointIndex -= (trail)->points.size();                                               \
    real timeLen = (trail)->curTime - (trail)->points[lastPointIndex].time;                   \
    if (timeLen > (trail)->totalTimeLen || timeLen <= 0)                                      \
      timeLen = (trail)->totalTimeLen;                                                        \
    EffectsInterface::StdParticleVertex renderVerts[4];                                       \
    EffectsInterface::StdParticleVertex *vert = &renderVerts[0], *prevVert = &renderVerts[2]; \
    int nextPointIndex = (trail)->firstPoint + 1;                                             \
    if (nextPointIndex >= (trail)->points.size())                                             \
      nextPointIndex = 0;                                                                     \
    Point3 prevPointPos = (trail)->points[nextPointIndex].pos;                                \
    prevPointPos += ((trail)->points[(trail)->firstPoint].pos - prevPointPos) * 2;            \
    for (int i = (trail)->firstPoint, n = (trail)->numActive; n; --n)                         \
    {                                                                                         \
      StdTrailPath::TrailPoint &p = (trail)->points[i];                                       \
      real timeFactor = ((trail)->curTime - p.time) / timeLen;                                \
      if (timeFactor > 1)                                                                     \
        timeFactor = 1;

#define SET_STD_TRAIL_VERT_POS(position, dir) \
  vert[0].pos = (position) - (dir);           \
  vert[1].pos = (position) + (dir);

#define SET_STD_TRAIL_VERT_TC         \
  vert[0].tc = Point2(timeFactor, 0); \
  vert[1].tc = Point2(timeFactor, 1);


#define END_STD_TRAIL_RENDER(trail, buf_ptr, num_quads, total_quads) \
  prevPointPos = p.pos;                                              \
  if (i != (trail)->firstPoint)                                      \
  {                                                                  \
    vptr[0] = prevVert[1];                                           \
    vptr[1] = prevVert[0];                                           \
    vptr[2] = vert[0];                                               \
    vptr[3] = vert[1];                                               \
    vptr += 4;                                                       \
    --totq;                                                          \
    if (--numq == 0)                                                 \
    {                                                                \
      EffectsInterface::endQuads();                                  \
      numq = EffectsInterface::getMaxQuadCount();                    \
      if (numq > totq)                                               \
        numq = totq;                                                 \
      if (numq)                                                      \
        EffectsInterface::beginQuads(&vptr, numq);                   \
    }                                                                \
  }                                                                  \
  EffectsInterface::StdParticleVertex *tmpv = vert;                  \
  vert = prevVert;                                                   \
  prevVert = tmpv;                                                   \
  if (++i >= (trail)->points.size())                                 \
    i = 0;                                                           \
  }                                                                  \
  (buf_ptr) = vptr;                                                  \
  (num_quads) = numq;                                                \
  (total_quads) = totq;                                              \
  }


#endif
