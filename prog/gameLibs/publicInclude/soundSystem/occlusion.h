//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <soundSystem/handle.h>

namespace sndsys::occlusion
{
using group_id_t = uint32_t;

bool is_inited();
bool is_enabled();
bool is_valid();
void enable(bool enabled);
void set_group_pos(group_id_t group_id, const Point3 &pos);
void set_event_group(EventHandle event_handle, group_id_t group_id);

typedef float (*trace_proc_t)(const Point3 &from, const Point3 &to, float near, float far);
typedef void (*before_trace_proc_t)(const Point3 &trace_from, float far);
void set_trace_proc(trace_proc_t trace_proc);
void set_before_trace_proc(before_trace_proc_t before_trace_proc);
} // namespace sndsys::occlusion
