//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <generic/dag_tabFwd.h>


class IOccluderGeomProvider
{
public:
  static constexpr unsigned HUID = 0x96169B26u; // IOccluderGeomProvider

  struct Quad
  {
    Point3 v[4];
  };

  //! adds box occluders (as scaled basis matrix) and quad occluders (as 4 x Point3)
  virtual void gatherOccluders(Tab<TMatrix> &occl_boxes, Tab<Quad> &occl_quads) = 0;

  //! renders list of occluders (derived from entities), if applicable
  virtual void renderOccluders(const Point3 &camPos, float max_dist) = 0;
};
