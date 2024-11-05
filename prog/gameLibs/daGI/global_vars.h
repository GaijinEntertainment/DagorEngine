// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shaderVariableInfo.h>

#define GLOBAL_VARS_LIST               \
  VAR(scene_voxels_invalid_start)      \
  VAR(scene_voxels_invalid_width)      \
  VAR(ssgi_copy_y_indices0)            \
  VAR(ssgi_copy_y_indices1)            \
  VAR(ssgi_copy_indices)               \
  VAR(ssgi_start_copy_slice)           \
  VAR(ambient_voxels_move_ofs)         \
  VAR(ssgi_current_world_origin)       \
  VAR(scene_voxels_size)               \
  VAR(ssgi_total_scene_mark_dipatch)   \
  VAR(ambient_voxels_visible_start)    \
  VAR(ambient_voxels_visible_width)    \
  VAR(ssgi_current_frame)              \
  VAR(globtm_psf_0)                    \
  VAR(globtm_psf_1)                    \
  VAR(globtm_psf_2)                    \
  VAR(globtm_psf_3)                    \
  VAR(cascade_id)                      \
  VAR(has_physobj_in_cascade)          \
  VAR(ssgi_restricted_update_bbox_min) \
  VAR(ssgi_restricted_update_bbox_max)

#define GLOBAL_VARS_OPTIONAL_LIST            \
  VAR(wallsGridLT)                           \
  VAR(wallsGridCellSize)                     \
  VAR(gi_quality)                            \
  VAR(ssgi_temporal_weight_limit)            \
  VAR(current_windows_count)                 \
  VAR(current_walls_count)                   \
  VAR(debug_volmap_type)                     \
  VAR(ssgi_debug_rasterize_scene)            \
  VAR(scene_voxels_unwrapped_invalid_start)  \
  VAR(voxelize_world_to_rasterize_space_mul) \
  VAR(voxelize_world_to_rasterize_space_add) \
  VAR(ssgi_debug_rasterize_scene_cascade)    \
  VAR(debug_cascade)

namespace dagi
{
#define VAR(a) extern ShaderVariableInfo a##VarId;
GLOBAL_VARS_LIST
GLOBAL_VARS_OPTIONAL_LIST
#undef VAR
void init_global_vars();
}; // namespace dagi
using namespace dagi;
