//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_capsule.h>
// #include <debug/dag_debug.h>

struct TriangleFaceCached
{
  void setnorm(const Point3 &norm) { n = norm; }

  void init(const Point3 &v0, const Point3 &v1, const Point3 &v2)
  {
    vp[0] = v0;
    vp[1] = v1;
    vp[2] = v2;
    fd = -(vp[0] * n);
  }

  void precalc()
  {
    en[0] = normalize(n % (vp[1] - vp[0]));
    en[1] = normalize(n % (vp[2] - vp[1]));
    en[2] = normalize(n % (vp[0] - vp[2]));
    ed[0] = -(en[0] * vp[0]);
    ed[1] = -(en[1] * vp[1]);
    ed[2] = -(en[2] * vp[2]);
  }

public:
  // data available after init
  Point3 n;
  Point3 vp[3];
  real fd;

  // data available after precalc
  real ed[3];
  Point3 en[3];
};

bool clipCapsuleTriangleCached(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, TriangleFaceCached &f, int &ready_stage);
