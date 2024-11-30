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
void set_group_pos(group_id_t group_id, const Point3 &pos);
void set_event_group(EventHandle event_handle, group_id_t group_id);
} // namespace sndsys::occlusion
