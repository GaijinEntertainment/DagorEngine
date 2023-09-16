//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <vecmath/dag_vecMathDecl.h>

namespace gamemath
{
void calc_convex_segments(dag::ConstSpan<plane3f> convex, Tab<Point3> &segments);
};
