//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <vecmath/dag_vecMath_common.h>


struct OcclusionEllipse
{
  vec4f axes = v_zero();        // {xAxis.x, xAxis.y, yAxis.x, yAxis.y}
  vec4f centerScale = v_zero(); // {center.x, center.y, yScale, 0}
  vec4f cellBox = v_zero();     // ellipse AABB in occlusion-buffer cell coords {x0, y0, x1, y1};

  operator bool() const { return !v_test_all_bits_zeros(v_or(axes, centerScale)); }
};
