#pragma once
#include "drvCommonConsts.h"

namespace drv3d_vulkan
{
const uint32_t FRAME_FRAME_BACKLOG_LENGTH = 2;
const uint32_t MAX_PENDING_WORK_ITEMS = 0;
const uint32_t MAX_RETIREMENT_QUEUE_ITEMS = 6;
const int64_t MEMORY_STATISTICS_PERIOD = 60;
const uint32_t PRESENT_ENGINE_LOCKED_IMAGES = 1;

#define VK_MAX(l, r) (l) > (r) ? (l) : (r)
const uint32_t MIN_LISTED_MODE_WIDTH = 800;
const uint32_t MIN_LISTED_MODE_HEIGHT = 600;

const uint32_t MAX_RENDER_PASS_ATTACHMENTS = 16;

const uint32_t SHADER_REGISTER_ELEMENTS = 4;
typedef float ShaderRegisterElementType;
const uint32_t SHADER_REGISTER_ELEMENT_SIZE = sizeof(ShaderRegisterElementType);
const uint32_t SHADER_REGISTER_SIZE = SHADER_REGISTER_ELEMENTS * SHADER_REGISTER_ELEMENT_SIZE;
const uint32_t VERTEX_SHADER_MAX_REGISTERS = 4096; // needs to be in sync with vulkan specefic defines in shaders
const uint32_t VERTEX_SHADER_MIN_REGISTERS = MAX_VS_CONSTS_BONES;
const uint32_t FRAGMENT_SHADER_REGISTERS = MAX_PS_CONSTS;
const uint32_t MAX_VERTEX_INPUT_STREAMS = 4;
const uint32_t MAX_VERTEX_ATTRIBUTES = 16;
const uint32_t MAX_ACTIVE_DESCRIPTOR_SETS = 2;
const uint8_t MAX_IMMEDIATE_CONST_WORDS = 4;
const uint32_t IMMEDAITE_CB_REGISTER_NO = 7;

// holds driver specific cache for pipeline creations
// const char VULKAN_PIPELINE_CACHE_FILE_NAME[] = "cache/vulkan.bin";
// holds cache for used pipeline variants, this is driver/devie independent
// const char VULKAN_PIPELINE_STATE_CACHE_FILE_NAME[] = "cache/vulkan.state.bin";

const uint32_t MAX_QUEUE_FAMILY_TYPES = 6;

const uint32_t UNIFORM_BUFFER_BLOCK_SIZE = 1024 * 1024 * 12;
const uint32_t USER_POINTER_VERTEX_BLOCK_SIZE = 1024 * 1024 * 8;
const uint32_t USER_POINTER_INDEX_BLOCK_SIZE = 1024 * 1024 * 2;
const uint32_t INITIAL_UPDATE_BUFFER_BLOCK_SIZE = 1024 * 1024 * 2;

const uint32_t MAX_MIPMAPS = 16;

const uint32_t COMMAND_BUFFER_ALLOC_BLOCK_SIZE = 0xA;

const uint32_t MAX_COMPUTE_CONST_REGISTERS = 4096;
const uint32_t MIN_COMPUTE_CONST_REGISTERS = DEF_CS_CONSTS;

const uint32_t MAX_CONST_REGISTERS =
  VK_MAX(MAX_COMPUTE_CONST_REGISTERS, VK_MAX(VERTEX_SHADER_MAX_REGISTERS, FRAGMENT_SHADER_REGISTERS));
#undef VK_MAX
const uint32_t MAX_UNIFORM_BUFFER_RANGE = MAX_CONST_REGISTERS * SHADER_REGISTER_SIZE;

constexpr uint32_t DEFAULT_WAIT_SPINS = 4000;
constexpr uint32_t MAX_DEBUG_MARKER_BUFFER_ENTRIES = 4096 * 2;

constexpr float MINIMUM_REPRESENTABLE_D16 = 2e-5;
constexpr float MINIMUM_REPRESENTABLE_D24 = 33e-8;
constexpr float MINIMUM_REPRESENTABLE_D32 = 3e-10;

const uint32_t STENCIL_REF_OVERRIDE_DISABLE = 0x100;
const uint32_t FRAME_TIMING_HISTORY_LENGTH = 32;

// if pipeline compiles longer than this threshold, it will be logged
constexpr int64_t PIPELINE_COMPILATION_LONG_THRESHOLD = 4000;
// 1s in microseconds, if we have a single draw/frame that takes that time, it is a possible TDR
constexpr int64_t LONG_FRAME_THRESHOLD_US = 1000000;
// 2s in ns as timeout for general GPU waiting calls
// no point to wait more than this limit,
// if something takes more time than this it should be fixed
// as various systems will trigger forced GPU driver shutdown/restart

#if _TARGET_ANDROID
// some android drivers will syslog every api call with timeout (about not supported timeouts)
// probably generating slowdown, so disable them
constexpr uint64_t GENERAL_GPU_TIMEOUT_NS = UINT64_MAX;
// this one is used in non vulkan API wait, so we can keep it
constexpr uint64_t GENERAL_GPU_TIMEOUT_US = 2 * 1000000;
#else
constexpr uint64_t GENERAL_GPU_TIMEOUT_NS = 2 * 1000000000;
constexpr uint64_t GENERAL_GPU_TIMEOUT_US = GENERAL_GPU_TIMEOUT_NS / 1000;
#endif
constexpr size_t MAX_REPLAY_WAIT_CYCLES = 1000000;
// on some devices present includes completion of previously queued work, use threshold to detect that
constexpr int64_t LONG_PRESENT_DURATION_THRESHOLD_US = 3 * 1000;


#define VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT   (DAGOR_DBGLEVEL > 0)
#define VULKAN_DO_SIGN_CHECK                   0
// allow debug names on PC in release, as we can allow memory overhead there
// and keeping names in log is better for fixing user feedback based bugs
#define VULKAN_RESOURCE_DEBUG_NAMES            (DAGOR_DBGLEVEL > 0) || _TARGET_PC
#define VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA (DAGOR_DBGLEVEL > 0)
#define VULKAN_RECORD_TIMING_DATA              (DAGOR_DBGLEVEL > 0)
#define VULKAN_TRACK_DEAD_RESOURCE_USAGE       1

// print page memory map in memory stat printouts
#define VULKAN_LOG_DEVICE_MEMORY_PAGES_MAP (DAGOR_DBGLEVEL > 0)

// switches for deep debug in case of hideous failures
// i.e. target device undocumented behaivour, spec uncoformity,
// other "wide timeline/delayed" bugs/crashes
// 1 for compilation tracking
// 2 for compilation+bind tracking
#define VULKAN_LOG_PIPELINE_ACTIVITY 0

// allow collecting caller data if validation error is occured in execution context
#define VULKAN_VALIDATION_COLLECT_CALLER 0

#define MRT_INDEX_DEPTH_STENCIL Driver3dRenderTarget::MAX_SIMRT

// enable to verify that mapped buffers are not writed outside of their size
#define VULKAN_MAPPED_BUFFER_OVERRUN_WRITE_CHECK 0

// enable to track callers of sync ops
#define EXECUTION_SYNC_TRACK_CALLER         0
// enable to verify that sync ops in single step has no self conflicts
#define EXECUTION_SYNC_CHECK_SELF_CONFLICTS (DAGOR_DBGLEVEL > 0)

} // namespace drv3d_vulkan