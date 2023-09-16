//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class Point3;
struct TraceMeshFaces;

namespace gamephys
{
void draw_trace_handle(const TraceMeshFaces *trace_handle);

void draw_trace_handle_debug_stats(const TraceMeshFaces *trace_handle, const Point3 &wpos);

void draw_trace_handle_debug_invalidate(const TraceMeshFaces *trace_handle, bool is_for_ri);
}; // namespace gamephys
