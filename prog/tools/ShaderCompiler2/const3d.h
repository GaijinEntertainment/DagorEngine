// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_consts.h>

#define DRV3DC

inline constexpr uint32_t MAX_CBUFFER_VECTORS = 4096;

inline constexpr const char *SHADER_STAGE_SHORT_NAMES[STAGE_MAX] = {"cs", "ps", "vs"};
inline constexpr const char *SHADER_STAGE_NAMES[STAGE_MAX] = {"compute", "pixel", "vertex"};

inline constexpr int MATERIAL_PARAMS_CONST_BUF_REGISTER = 1;
inline constexpr int GLOBAL_CONST_BUF_REGISTER = 2;

#if _CROSS_TARGET_DX12
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>

inline constexpr int MAX_T_REGISTERS = dxil::MAX_T_REGISTERS;
inline constexpr int MAX_S_REGISTERS = dxil::MAX_S_REGISTERS;
inline constexpr int MAX_U_REGISTERS = dxil::MAX_U_REGISTERS;
inline constexpr int MAX_B_REGISTERS = dxil::MAX_B_REGISTERS;

static constexpr bool UAVS_CONTEND_WITH_RTVS = false;

#elif _CROSS_TARGET_SPIRV
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>

inline constexpr int MAX_T_REGISTERS = spirv::T_REGISTER_INDEX_MAX;
inline constexpr int MAX_S_REGISTERS = spirv::S_REGISTER_INDEX_MAX;
inline constexpr int MAX_U_REGISTERS = spirv::U_REGISTER_INDEX_MAX;
inline constexpr int MAX_B_REGISTERS = spirv::B_REGISTER_INDEX_MAX;

static constexpr bool UAVS_CONTEND_WITH_RTVS = false;

#else

#if _CROSS_TARGET_DX11
inline constexpr int MAX_U_REGISTERS = 8;
#elif _CROSS_TARGET_METAL
inline constexpr int MAX_U_REGISTERS = 10;
#else
inline constexpr int MAX_U_REGISTERS = 13;
#endif

#if _CROSS_TARGET_DX11
static constexpr bool UAVS_CONTEND_WITH_RTVS = true;
#else
static constexpr bool UAVS_CONTEND_WITH_RTVS = false;
#endif

// @TODO: make per-platform limits for all platforms & drivers

inline constexpr int MAX_T_REGISTERS = 32;
inline constexpr int MAX_S_REGISTERS = 16;
inline constexpr int MAX_B_REGISTERS = 14;

#endif
