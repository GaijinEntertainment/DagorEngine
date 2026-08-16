// Copyright (C) Gaijin Games KFT.  All rights reserved.
// No #pragma once: dsc2 does not accept it in an included shader file, and this file has only defines.

// Record layout shared by omm_debug_extract_uv, omm_debug_visualize and the C++ viewer.
#define OMM_DEBUG_UV_UV0_OFFSET 0u             // float2
#define OMM_DEBUG_UV_UV1_OFFSET 8u             // float2
#define OMM_DEBUG_UV_UV2_OFFSET 16u            // float2
#define OMM_DEBUG_UV_PREDICTED_LEVEL_OFFSET 24u // uint, the level that the bake would select
#define OMM_DEBUG_UV_TEXEL_AREA_OFFSET 28u     // float, the UV area in texels; recorded, but not read yet
#define OMM_DEBUG_UV_RECORD_SIZE 32u
