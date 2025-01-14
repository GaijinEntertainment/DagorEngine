// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/vector.h>
#include <drv/3d/dag_consts.h>
#include <util/dag_stdint.h>
#include <util/dag_hash.h>
#include <drv/3d/rayTrace/dag_drvRayTrace.h> // for D3D_HAS_RAY_TRACING

#include "vulkan_api.h"

#if !_TARGET_PC_WIN
#include <stdlib.h>
#include <pthread.h>
#include <util/dag_stdint.h>
inline intptr_t GetCurrentThreadId() { return (intptr_t)pthread_self(); }
#endif

#include "shader_stage_traits.h"
#include "vk_wrapped_handles.h"
#include "format_store.h"
#include "sampler_state.h"
#include "vk_error.h"


#ifndef UINT64_MAX
#define UINT64_MAX 0xFFFFFFFFFFFFFFFFull
#endif
#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFFul
#endif

#if _TARGET_64BIT
#define PTR_LIKE_HEX_FMT "%016X"
#else
#define PTR_LIKE_HEX_FMT "%08X"
#endif

namespace drv3d_vulkan
{
struct InputLayout;
struct ShaderInfo;

enum class ActiveExecutionStage
{
  FRAME_BEGIN,
  GRAPHICS,
  COMPUTE,
  CUSTOM,
#if D3D_HAS_RAY_TRACING
  RAYTRACE,
#endif
};

enum class DeviceQueueType
{
  GRAPHICS,
  COMPUTE,
  TRANSFER,
  ASYNC_GRAPHICS,

  COUNT,
  INVALID = COUNT,
  ZERO = GRAPHICS
};

#define STAGE_MAX_ACTIVE STAGE_MAX

template <typename H, H NullValue, typename T>
class TaggedHandle
{
  H h = H(NullValue);

public:
  inline H get() const { return h; }
  inline explicit TaggedHandle(H a) : h(a) {}
  inline TaggedHandle() {}
  inline bool operator!() const { return h == NullValue; }
  friend inline bool operator==(TaggedHandle l, TaggedHandle r) { return l.get() == r.get(); }
  friend inline bool operator!=(TaggedHandle l, TaggedHandle r) { return l.get() != r.get(); }
  friend inline H operator-(TaggedHandle l, TaggedHandle r) { return l.get() - r.get(); }
  static inline TaggedHandle Null() { return TaggedHandle(NullValue); }
};

struct ProgramIDTag
{};
struct InputLayoutIDTag
{};
struct ShaderIDTag
{};
struct BufferHandleTag
{};

typedef uint32_t ResourceMemoryId;
typedef int LinearStorageIndex;
typedef TaggedHandle<int, -1, ProgramIDTag> ProgramID;
typedef TaggedHandle<int, -1, InputLayoutIDTag> InputLayoutID;
typedef TaggedHandle<int, -1, ShaderIDTag> ShaderID;

struct StringIndexRefTag
{};
typedef TaggedHandle<size_t, ~size_t(0), StringIndexRefTag> StringIndexRef;

struct SamplerInfo
{
  // 0 normal sampler, 1 sampler with depth compare enabled
  VulkanSamplerHandle sampler[2];
  SamplerState state;

  inline VulkanSamplerHandle compareSampler() const { return sampler[1]; }
  inline VulkanSamplerHandle colorSampler() const { return sampler[0]; }
};

class Image;

struct DriverRenderState
{
  LinearStorageIndex dynamicIdx = 0;
  LinearStorageIndex staticIdx = 0;

  bool equal(const DriverRenderState &v) const
  {
    if (v.dynamicIdx != dynamicIdx)
      return false;

    if (v.staticIdx != staticIdx)
      return false;

    return true;
  }
};
} // namespace drv3d_vulkan

inline const VkOffset3D &toOffset(const VkExtent3D &ext)
{
  // sanity checks
  G_STATIC_ASSERT(offsetof(VkExtent3D, width) == offsetof(VkOffset3D, x));
  G_STATIC_ASSERT(offsetof(VkExtent3D, height) == offsetof(VkOffset3D, y));
  G_STATIC_ASSERT(offsetof(VkExtent3D, depth) == offsetof(VkOffset3D, z));
  return reinterpret_cast<const VkOffset3D &>(ext);
}

inline bool operator==(VkExtent2D l, VkExtent2D r) { return l.height == r.height && l.width == r.width; }

inline bool operator!=(VkExtent2D l, VkExtent2D r) { return !(r == l); }

inline bool operator==(VkOffset2D l, VkOffset2D r) { return l.x == r.x && l.y == r.y; }

inline bool operator!=(VkOffset2D l, VkOffset2D r) { return !(r == l); }

inline bool operator==(const VkRect2D &l, const VkRect2D &r) { return l.extent == r.extent && l.offset == r.offset; }

inline bool operator!=(const VkRect2D &l, const VkRect2D &r) { return !(r == l); }

inline bool operator==(const VkViewport &l, const VkViewport &r)
{
  return l.height == r.height && l.maxDepth == r.maxDepth && l.minDepth == r.minDepth && l.width == r.width && l.x == r.x &&
         l.y == r.y;
}

inline bool operator!=(const VkViewport &l, const VkViewport &r) { return !(r == l); }

inline VkImageSubresourceRange makeImageSubresourceRange(VkImageAspectFlags aspect, uint32_t base_mip, uint32_t mip_count,
  uint32_t array_index, uint32_t array_range)
{
  VkImageSubresourceRange r = {aspect, base_mip, mip_count, array_index, array_range};
  return r;
}

inline uint32_t nextPowerOfTwo(uint32_t u)
{
  --u;
  u |= u >> 1;
  u |= u >> 2;
  u |= u >> 4;
  u |= u >> 8;
  u |= u >> 16;
  return ++u;
}

inline uint64_t nextPowerOfTwo(uint64_t u)
{
  --u;
  u |= u >> 1;
  u |= u >> 2;
  u |= u >> 4;
  u |= u >> 8;
  u |= u >> 16;
  u |= u >> 32;
  return ++u;
}

inline constexpr VkStencilFaceFlags VK_STENCIL_FACE_BOTH_BIT = VK_STENCIL_FACE_FRONT_BIT | VK_STENCIL_FACE_BACK_BIT;

inline constexpr VkColorComponentFlags VK_COLOR_COMPONENT_RGBA_BIT =
  VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

class DataBlock;
namespace drv3d_vulkan
{
template <typename T, typename U>
void chain_structs(T &t, U &u)
{
  G_ASSERT(u.pNext == nullptr);
  u.pNext = (void *)t.pNext;
  t.pNext = &u;
}

VkApplicationInfo create_app_info(const DataBlock *cfg);
void set_app_info(const char *name, uint32_t ver);
uint32_t get_app_ver();
VkPresentModeKHR get_requested_present_mode(bool use_vsync, bool use_adaptive_vsync);

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
struct ShaderDebugInfo
{
  String name;
  String spirvDisAsm;
  String dxbcDisAsm;
  String sourceGLSL;
  String reconstructedHLSL;
  String reconstructedHLSLAndSourceHLSLXDif;
  String sourceHLSL;
  // name from engine with additional usefull info
  String debugName;
};
#endif

struct ShaderModuleBlob
{
  eastl::vector<uint32_t> blob;
  spirv::HashValue hash;
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  String name;
#endif
  size_t getBlobSize() const { return blob.size() * sizeof(uint32_t); }

  uint32_t getHash32() const { return mem_hash_fnv1<32>((const char *)&hash, sizeof(hash)); }
};

struct ShaderModuleHeader
{
  spirv::ShaderHeader header;
  spirv::HashValue hash;
  VkShaderStageFlags stage;

  uint32_t getHash32() const { return mem_hash_fnv1<32>((const char *)&hash, sizeof(hash)); }
};

struct ShaderModuleUse
{
  ShaderModuleHeader header;
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  ShaderDebugInfo dbg;
#endif
  uint32_t blobId;
};

VkBufferImageCopy make_copy_info(FormatStore format, uint32_t mip, uint32_t base_layer, uint32_t layers, VkExtent3D original_size,
  VkDeviceSize src_offset);

const char *formatActiveExecutionStage(ActiveExecutionStage stage);
const char *formatShaderStage(ShaderStage stage);

template <typename T>
static uint32_t get_binary_size_unit_prefix_index(T size)
{
  uint32_t power = 0;
  const uint32_t shift = 10;
  for (;; ++power)
    if (0 == (size / (1ull << ((power + 1) * shift))))
      break;
  return power;
}

template <typename T>
static String byte_size_unit(T size)
{
  static const char *suffix[] = {"", "Ki", "Mi", "Gi"};
  uint32_t power = get_binary_size_unit_prefix_index(size);
  return String(128, "%f %sBytes", double(size) / (1ull << (power * 10)), suffix[power]);
}

enum RegisterType
{
  T,
  U,
  B,
  COUNT
};

enum class ExtendedShaderStage
{
  CS,
  PS,
  VS,
  RT,
  TC,
  TE,
  GS,
  MAX
};

const char *formatExtendedShaderStage(ExtendedShaderStage stage);

} // namespace drv3d_vulkan
