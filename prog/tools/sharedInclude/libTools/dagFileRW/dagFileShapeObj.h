// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/splineShape.h>


class ShapeHolderObj : public Object
{
public:
  DAG_DECLARE_NEW(nodemem)

  SplineShape *shp;

  ShapeHolderObj(SplineShape *s = NULL);
  ~ShapeHolderObj();
  void setshape(SplineShape *);
  int classid();
  bool isSubOf(unsigned);
};
