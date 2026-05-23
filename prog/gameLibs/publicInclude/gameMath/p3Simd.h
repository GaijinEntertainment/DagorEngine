//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_vecMathCompatibility.h>

// Accepts a Point3 by const-ref (including temporaries) and loads it into a vec3f.
// Use instead of v_ldu_p3(&p.x) when the source is a temporary or return value.
__forceinline vec3f v_ldu_p3(const Point3 &p) { return v_ldu_p3(&p.x); }
