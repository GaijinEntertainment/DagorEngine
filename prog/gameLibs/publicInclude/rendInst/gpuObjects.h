//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/vector_set.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>

namespace gpu_objects
{
struct PlacingParameters;
class GpuObjects;
} // namespace gpu_objects

class Point3;
struct RiGenVisibility;

namespace rendinst::gpuobjects
{

using AllowGpuRiCb = bool (*)();

void add(const String &name, int cell_tile, int grid_size, float cell_size, const gpu_objects::PlacingParameters &);
void erase_inside_sphere(const Point3 &center, const float radius);
void update(const Point3 &origin);

bool has_pending();
void change_parameters(const String &name, const gpu_objects::PlacingParameters &parameters);
void change_grid(const String &name, int cell_tile, int grid_size, float cell_size);
void validate_displaced(float displacement_tex_range);
void invalidate();
void invalidate_inside_bbox(const BBox2 &bbox);
eastl::vector_set<String> get_gpu_object_material_list();
void set_allow_gpu_ri_cb(AllowGpuRiCb cb);

void enable_for_visibility(RiGenVisibility *visibility);
void disable_for_visibility(RiGenVisibility *visibility);
void clear_from_visibility(RiGenVisibility *visibility);


} // namespace rendinst::gpuobjects
