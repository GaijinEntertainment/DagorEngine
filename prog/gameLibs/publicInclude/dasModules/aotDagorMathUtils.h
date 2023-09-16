//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorMath.h>
#include <math/dag_math3d.h>
#include <math/dag_math2d.h>
#include <math/dag_mathAng.h>
#include <math/dag_mathUtils.h>
#include <math/dag_bits.h>

namespace bind_dascript
{
inline int point3_get_positional_seed(const Point3 &p, float step) { return get_positional_seed(p, step); }
} // namespace bind_dascript
