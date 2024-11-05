// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_math3d.h>
#include <generic/dag_tab.h>
#include <memory/dag_mem.h>


class DagSpline
{
public:
  DAG_DECLARE_NEW(midmem)

  struct SplineKnot
  {
    Point3 i, p, o;

    void transform(TMatrix &tm)
    {
      i = tm * i;
      p = tm * p;
      o = tm * o;
    }
  };
  Tab<SplineKnot> knot;
  char closed;

  DagSpline(IMemAlloc *m = midmem) : knot(m), closed(0) {}
  int segcount() { return closed ? knot.size() : knot.size() - 1; }
  void transform(TMatrix &); // transform knots
};

class SplineShape
{
public:
  DAG_DECLARE_NEW(midmem)

  Tab<DagSpline *> spl;

  SplineShape(IMemAlloc *m = midmem) : spl(m) {}
  ~SplineShape();
  void transform(TMatrix &);
};
