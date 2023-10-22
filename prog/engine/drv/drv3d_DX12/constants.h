#pragma once

namespace drv3d_dx12
{
#if _TARGET_PC_WIN
constexpr uint32_t FRAME_FRAME_BACKLOG_LENGTH = 4;
#else
constexpr uint32_t FRAME_FRAME_BACKLOG_LENGTH = 2;
#endif
// has to be two to avoid constantly triggering waits on gpu and cpu
constexpr uint32_t FRAME_LATENCY = 2;
constexpr uint32_t DYNAMIC_BUFFER_DISCARD_BASE_COUNT = FRAME_FRAME_BACKLOG_LENGTH;
constexpr uint32_t DYNAMIC_CONST_BUFFER_DISCARD_BASE_COUNT = DYNAMIC_BUFFER_DISCARD_BASE_COUNT * 3;
constexpr uint32_t DEFAULT_BUFFER_DISCARD_BASE_COUNT = 1;

constexpr uint32_t MAX_VERTEX_ATTRIBUTES = 16;
constexpr uint32_t MAX_VERTEX_INPUT_STREAMS = 4;

constexpr uint32_t MAX_VIEW_INSTANCES = 4;

constexpr uint32_t SHADER_REGISTER_ELEMENTS = 4;
typedef float ShaderRegisterElementType;
constexpr uint32_t SHADER_REGISTER_ELEMENT_SIZE = sizeof(ShaderRegisterElementType);
constexpr uint32_t SHADER_REGISTER_SIZE = SHADER_REGISTER_ELEMENTS * SHADER_REGISTER_ELEMENT_SIZE;

constexpr uint32_t DEFAULT_WAIT_SPINS = 4000;

constexpr uint32_t UNIFORM_BUFFER_BLOCK_SIZE = 1024 * 1024 * 12;
constexpr uint32_t USER_POINTER_VERTEX_BLOCK_SIZE = 1024 * 1024 * 8;
constexpr uint32_t USER_POINTER_INDEX_BLOCK_SIZE = 1024 * 1024 * 2;
constexpr uint32_t INITIAL_UPDATE_BUFFER_BLOCK_SIZE = 1024 * 1024 * 2;
// can be adjusted as needed, but be careful, too many may degrade performance because of spilling
constexpr uint32_t MAX_ROOT_CONSTANTS = 4;
constexpr uint32_t ROOT_CONSTANT_BUFFER_INDEX = 8;

constexpr uint32_t MAX_OBJECT_NAME_LENGTH = 512;

// After half a second we give up and assume lockup because of an error
constexpr uint32_t MAX_WAIT_OBJECT_TIMEOUT_MS = 500;

constexpr uint32_t MAX_COMPUTE_CONST_REGISTERS = 4096;
constexpr uint32_t MIN_COMPUTE_CONST_REGISTERS = DEF_CS_CONSTS;
constexpr uint32_t VERTEX_SHADER_MAX_REGISTERS = 4096;
constexpr uint32_t VERTEX_SHADER_MIN_REGISTERS = MAX_VS_CONSTS_BONES;
constexpr uint32_t PIXEL_SHADER_REGISTERS = MAX_PS_CONSTS;
} // namespace drv3d_dx12