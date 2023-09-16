//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_geomTree.h>
#include <fx/dag_baseFxClasses.h>


static constexpr unsigned HUID_UPDATE_VEL = 0x6AFD92CFu; // UPDATE_VEL
static constexpr unsigned HUID_VERTICAL_NORMAL = 0x6A0792CAu;


class LineParticleFxEmitter : public BaseParticleFxEmitter
{
public:
  struct Line
  {
    const mat44f *node1, *node2;
    Point3 lastPos1, lastPos2;
    Point3 vel1, vel2;
    real sumLen;
  };

  Tab<Line> lines;

  Point3 emitterVel;
  bool verticalNormal;


  LineParticleFxEmitter() : lines(midmem), emitterVel(0, 0, 0), verticalNormal(true) {}


  virtual BaseParamScript *clone() { return new LineParticleFxEmitter(*this); }

  virtual void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb) {}

  virtual void setParam(unsigned id, void *value)
  {
    if (id == HUID_UPDATE_VEL)
    {
      real dt = *(real *)value;
      if (dt <= 0)
        return;

      for (int i = 0; i < lines.size(); ++i)
      {
        Line &l = lines[i];

        Point3 pos = as_point3(&l.node1->col3);
        l.vel1 = (pos - l.lastPos1) / dt;
        l.lastPos1 = pos;

        pos = as_point3(&l.node2->col3);
        l.vel2 = (pos - l.lastPos2) / dt;
        l.lastPos2 = pos;
      }
    }
    else if (id == HUID_VERTICAL_NORMAL)
    {
      verticalNormal = *(bool *)value;
    }
    else if (id == HUID_VELOCITY)
    {
      emitterVel = *(Point3 *)value;
    }
  }

  virtual void *getParam(unsigned id, void *value) { return NULL; }


  void addLine(const mat44f *n1, const mat44f *n2)
  {
    if (!n1 || !n2)
      return;

    Line &l = lines.push_back();
    l.node1 = n1;
    l.node2 = n2;
    l.lastPos1 = as_point3(&n1->col3);
    l.lastPos2 = as_point3(&n2->col3);
    l.vel1 = Point3(0, 0, 0);
    l.vel2 = Point3(0, 0, 0);
    l.sumLen = 0;
  }


  void rebuild()
  {
    real totalLen = 0;

    for (int i = 0; i < lines.size(); ++i)
    {
      Line &l = lines[i];
      l.sumLen = length(as_point3(&l.node1->col3) - as_point3(&l.node2->col3));
      totalLen += l.sumLen;
    }

    real sum = 0;

    for (int i = 0; i < lines.size(); ++i)
    {
      Line &l = lines[i];

      sum += l.sumLen;
      l.sumLen = sum / totalLen;
    }
  }


  virtual void emitPoint(Point3 &pos, Point3 &normal, Point3 &vel, real u, real v, real w)
  {
    for (int i = 0; i < lines.size(); ++i)
    {
      Line &l = lines[i];

      if (u > l.sumLen)
        continue;

      pos = as_point3(&l.node1->col3) * (1 - v) + as_point3(&l.node2->col3) * v;
      vel = l.vel1 * (1 - v) + l.vel2 * v + emitterVel;

      if (verticalNormal)
        normal = Point3(0, 1, 0);
      else
        normal = normalize(as_point3(&l.node2->col3) - as_point3(&l.node1->col3));

      return;
    }

    pos = Point3(0, 0, 0);
    vel = emitterVel;
    normal = Point3(0, 1, 0);
  }


  virtual void getEmitterBoundingSphere(Point3 &center, real &radius)
  {
    center = Point3(0, 0, 0);
    radius = 0;
    //==
  }
};
