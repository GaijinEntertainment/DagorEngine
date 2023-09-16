//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <gameMath/traceUtils.h>

namespace dacoll
{
bool try_use_trace_cache(const bbox3f &query_box, const TraceMeshFaces *handle);

void validate_trace_cache_oobb(const TMatrix &tm, const bbox3f &oobb, const vec3f &expands, float physmap_expands,
  TraceMeshFaces *handle);

void validate_trace_cache(const bbox3f &query_box, const vec3f &expands, float physmap_expands, TraceMeshFaces *handle);
}; // namespace dacoll
