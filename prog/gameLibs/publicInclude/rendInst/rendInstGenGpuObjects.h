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
class CollisionResource;
class RenderableInstanceLodsResource;
class TMatrix;
class Point4;
class Point3;
class BBox3;
class BSphere3;
class DataBlock;
struct RiGenVisibility;
struct Frustum;
class DynamicPhysObjectData;
class Occlusion;
class OcclusionMap;
class AcesEffect;
struct RendInstGenData;
typedef bool (*AllowGpuRiCb)();

namespace rendinst
{
extern void add_gpu_object(const String &name, int cell_tile, int grid_size, float cell_size, const gpu_objects::PlacingParameters &);
bool has_pending_gpu_objects();
extern void change_gpu_object_parameters(const String &name, const gpu_objects::PlacingParameters &parameters);
extern void change_gpu_object_grid(const String &name, int cell_tile, int grid_size, float cell_size);
extern void validateDisplacedGPUObjs(float displacement_tex_range);
extern void invalidateGpuObjects();
extern void invalidate_gpu_objects_in_bbox(const BBox2 &bbox);
extern eastl::vector_set<String> get_gpu_object_material_list();
extern void set_allow_gpu_ri_cb(AllowGpuRiCb cb);
} // namespace rendinst
