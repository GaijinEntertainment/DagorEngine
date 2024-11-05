// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shaderVariableInfo.h>

#define GLOBAL_VARS_LIST                    \
  VAR(scene_25d_voxels_invalid_start_width) \
  VAR(scene_25d_voxels_invalid_coord_box)   \
  VAR(scene_25d_voxels_origin)              \
  VAR(scene_25d_voxels_size)                \
  VAR(gi_25d_volmap_tc)                     \
  VAR(gi_25d_volmap_invalid_start_width)    \
  VAR(gi_25d_volmap_origin)                 \
  VAR(gi_25d_volmap_size)

#define GLOBAL_VARS_OPTIONAL_LIST \
  VAR(ssgi_debug_rasterize_scene) \
  VAR(debug_volmap_type)

namespace dagi25d
{
#define VAR(a) extern ShaderVariableInfo a##VarId;
GLOBAL_VARS_LIST
GLOBAL_VARS_OPTIONAL_LIST
#undef VAR
void init_global_vars();
}; // namespace dagi25d
using namespace dagi25d;
