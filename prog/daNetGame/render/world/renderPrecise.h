// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_TMatrix.h>

struct RenderPrecise
{
  RenderPrecise(const TMatrix &view_tm, const Point3 &view_pos);
  ~RenderPrecise();

private:
  TMatrix savedViewTm;
};