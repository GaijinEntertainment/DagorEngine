// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_consts.h>

#define DRV3DC

#if _CROSS_TARGET_DX12
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>

inline constexpr int MAX_T_REGISTERS = dxil::MAX_T_REGISTERS;

#elif _CROSS_TARGET_SPIRV
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>

inline constexpr int MAX_T_REGISTERS = spirv::T_REGISTER_INDEX_MAX;

#else

// @TODO: make per-platform limits for all platforms & drivers

inline constexpr int MAX_T_REGISTERS = 32;

#endif
