// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gpuObjects/gpuObjects.h>
#include <rendInst/constants.h>


struct RiGenVisibility;

namespace rendinst::gpuobjects
{

struct GpuObjectsEntry
{
  eastl::string name;
  int grid_tile, grid_size;
  float cell_size;
  gpu_objects::PlacingParameters parameters;
};

void startup();
void init_r();
void shutdown();
void after_device_reset();

void clear_all();

void rebuild_gpu_instancing_relem_params();

void render_optimization_depth(RenderPass render_pass, const RiGenVisibility *visibility,
  IgnoreOptimizationLimits ignore_optimization_instances_limits, uint32_t instance_count_mul);
void render_layer(RenderPass render_pass, const RiGenVisibility *visibility, LayerFlags layer_flags, LayerFlag layer);

void before_draw(RenderPass render_pass, const RiGenVisibility *visibility, const Frustum &frustum, const class Occlusion *occlusion,
  const char *mission_name, const char *map_name, bool gpu_instancing);

} // namespace rendinst::gpuobjects
