//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <gameMath/traceUtils.h>

namespace dacoll
{
bool try_use_trace_cache(const bbox3f &query_box, const TraceMeshFaces *handle);

void validate_trace_cache_oobb(const TMatrix &tm, const bbox3f &oobb, const vec3f &expands, float physmap_expands,
  TraceMeshFaces *handle);

void validate_trace_cache(const bbox3f &query_box, const vec3f &expands, float physmap_expands, TraceMeshFaces *handle,
  float rel_shift_threshold = 0.15f);
}; // namespace dacoll
