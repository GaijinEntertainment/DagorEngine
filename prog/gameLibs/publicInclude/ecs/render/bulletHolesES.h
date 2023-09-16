//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <daECS/core/event.h>

namespace decals
{
extern void (*add_bullet_hole)(const Point3 &pos, const Point3 &norm, float rad, uint32_t id, uint32_t matrix_id);
};
