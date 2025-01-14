// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "drvCommonConsts.h"
#include <cstdint>

namespace drv3d_dx12
{
#if _TARGET_PC_WIN
inline constexpr uint32_t FRAME_FRAME_BACKLOG_LENGTH = 4;
#else
inline constexpr uint32_t FRAME_FRAME_BACKLOG_LENGTH = 2;
#endif
// has to be two to avoid constantly triggering waits on gpu and cpu
inline constexpr uint32_t FRAME_LATENCY = 2;
inline constexpr uint32_t DYNAMIC_BUFFER_DISCARD_BASE_COUNT = FRAME_FRAME_BACKLOG_LENGTH;
inline constexpr uint32_t DYNAMIC_CONST_BUFFER_DISCARD_BASE_COUNT = DYNAMIC_BUFFER_DISCARD_BASE_COUNT * 3;
inline constexpr uint32_t DEFAULT_BUFFER_DISCARD_BASE_COUNT = 1;

inline constexpr uint32_t MAX_VERTEX_ATTRIBUTES = 16;
inline constexpr uint32_t MAX_VERTEX_INPUT_STREAMS = 4;

inline constexpr uint32_t MAX_VIEW_INSTANCES = 4;

inline constexpr uint32_t SHADER_REGISTER_ELEMENTS = 4;
typedef float ShaderRegisterElementType;
inline constexpr uint32_t SHADER_REGISTER_ELEMENT_SIZE = sizeof(ShaderRegisterElementType);
inline constexpr uint32_t SHADER_REGISTER_SIZE = SHADER_REGISTER_ELEMENTS * SHADER_REGISTER_ELEMENT_SIZE;

inline constexpr uint32_t DEFAULT_WAIT_SPINS = 4000;

inline constexpr uint32_t UNIFORM_BUFFER_BLOCK_SIZE = 1024 * 1024 * 12;
inline constexpr uint32_t USER_POINTER_VERTEX_BLOCK_SIZE = 1024 * 1024 * 8;
inline constexpr uint32_t USER_POINTER_INDEX_BLOCK_SIZE = 1024 * 1024 * 2;
inline constexpr uint32_t INITIAL_UPDATE_BUFFER_BLOCK_SIZE = 1024 * 1024 * 2;
// can be adjusted as needed, but be careful, too many may degrade performance because of spilling
inline constexpr uint32_t MAX_ROOT_CONSTANTS = 4;
inline constexpr uint32_t ROOT_CONSTANT_BUFFER_INDEX = 8;

inline constexpr uint32_t MAX_MIPMAPS = 16;
inline constexpr uint32_t TEXTURE_TILE_SIZE = 64 * 1024;

inline constexpr uint32_t MAX_OBJECT_NAME_LENGTH = 512;

// After half a second we give up and assume lockup because of an error
inline constexpr uint32_t MAX_WAIT_OBJECT_TIMEOUT_MS = 500;

inline constexpr uint32_t MAX_COMPUTE_CONST_REGISTERS = 4096;
inline constexpr uint32_t MIN_COMPUTE_CONST_REGISTERS = DEF_CS_CONSTS;
inline constexpr uint32_t VERTEX_SHADER_MAX_REGISTERS = 4096;
inline constexpr uint32_t VERTEX_SHADER_MIN_REGISTERS = DEF_VS_CONSTS;
inline constexpr uint32_t PIXEL_SHADER_REGISTERS = MAX_PS_CONSTS;
} // namespace drv3d_dx12

namespace gpu
{
enum
{
  VENDOR_ID_AMD = 0x1002,
  VENDOR_ID_INTEL = 0x8086,
  VENDOR_ID_NVIDIA = 0x10DE
};
}
