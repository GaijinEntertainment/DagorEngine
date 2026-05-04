// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if _TARGET_XBOX
#include "driver_xbox.h"
#elif _TARGET_PC_WIN
#include "driver_win.h"
#else
#error "Dx12 driver is not supported on targets other than PC and Xbox"
#endif

#include "versioned_com_ptr.h"

#include <value_range.h>

#if _TARGET_PC_WIN
typedef VersionedComPtr<D3DDevice, ID3D12Device7> AnyDeviceComPtr;
using AnyDevicePtr = VersionedPtr<D3DDevice, ID3D12Device7>;
#else
typedef VersionedComPtr<D3DDevice> AnyDeviceComPtr;
using AnyDevicePtr = VersionedPtr<D3DDevice>;
#endif

// Workaround for https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0806r2.html
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#define DX12_CAPTURE_DEF_EQ =, this
#else
#define DX12_CAPTURE_DEF_EQ =
#endif

inline constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_COPY_QUEUE_TARGET = D3D12_RESOURCE_STATE_COMMON;
inline constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_COPY_QUEUE_SOURCE = D3D12_RESOURCE_STATE_COMMON;
inline constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE = D3D12_RESOURCE_STATE_COMMON;

#define DX12_FOLD_BATCHED_SPLIT_BARRIERS                  1
#define DX12_REUSE_UNORDERD_ACCESS_VIEW_DESCRIPTOR_RANGES 1
#define DX12_REUSE_SHADER_RESOURCE_VIEW_DESCRIPTOR_RANGES 1
#define DX12_VALIDATE_DRAW_CALL_USEFULNESS                0
#define DX12_DROP_NOT_USEFUL_DRAW_CALLS                   1
#define DX12_WARN_EMPTY_CLEARS                            1

#define COMMAND_BUFFER_DEBUG_INFO_DEFINED                     _TARGET_PC_WIN
// Binary search on pipeline variants is substantially slower than linear search (up to 8% worse bench results).
// The probable reason for this is the low average number of variants (majority has less than 3 variants),
// the binary search working against prefetching and increased complexity of the search.
#define DX12_USE_BINARY_SEARCH_FOR_GRAPHICS_PIPELINE_VARIANTS 0

#if DX12_AUTOMATIC_BARRIERS && DX12_PROCESS_USER_BARRIERS
#define DX12_CONFIGUREABLE_BARRIER_MODE 1
#else
#define DX12_CONFIGUREABLE_BARRIER_MODE 0
#endif

#define DX12_REPORT_BUFFER_PADDING 0

#define DX12_PRINT_USER_BUFFER_BARRIERS  0
#define DX12_PRINT_USER_TEXTURE_BARRIERS 0

#if DAGOR_DBGLEVEL > 0 || _TARGET_PC_WIN
#define DX12_DOES_SET_DEBUG_NAMES 1
#else
#define DX12_DOES_SET_DEBUG_NAMES 0
#endif

#if !DX12_AUTOMATIC_BARRIERS && !DX12_PROCESS_USER_BARRIERS
#error "DX12 Driver configured to _not_ generate required barriers and to _ignore_ user barriers, this will crash on execution"
#endif

#define DX12_ENABLE_MT_VALIDATION                  DAGOR_DBGLEVEL > 0
#define DX12_ENABLE_ALL_COMMANDS_PROFILING_MARKERS 0

#define DX12_SUPPORT_RESOURCE_MEMORY_METRICS  DAGOR_DBGLEVEL > 0
#define DX12_RESOURCE_USAGE_TRACKER           DAGOR_DBGLEVEL > 0
#define DX12_DEBUG_RESOURCE_ALLOCATOR_ENABLED (DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN)

#define DX12_SET_HEAP_RESIDENCY_PRIORITY (_TARGET_PC_WIN)

namespace drv3d_dx12
{
/// Describes in which way a optional feature is implemented, when there are more than one possibility
enum class FeatureImplementation
{
  /// Feature is not implemented at all and can not be used
  None,
  /// Feature is implemented with DX12 core API
  Core,
  /// Feature is implemented with Nvidia proprietary API (NVAPI)
  Nvidia,
  /// Feature is implemented with AMD proprietary API
  Amd,
  /// Feature is implemented with Intel proprietary API
  Intel,
};

inline const char *as_string(FeatureImplementation imp)
{
  switch (imp)
  {
    case FeatureImplementation::None: return "None";
    case FeatureImplementation::Core: return "Direct3D12 API";
    case FeatureImplementation::Nvidia: return "Nvidia driver extensions";
    case FeatureImplementation::Amd: return "AMD driver extensions";
    case FeatureImplementation::Intel: return "Intel driver extension";
  }
  return nullptr;
}

struct HLSLVendorExtensions
{
  uint16_t vendor = 0;
  uint16_t vendorExtensionMask = 0;
};

#if _TARGET_XBOX
union GPUAndCPUAddress
{
  uint8_t *cpuAddress = nullptr;
  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
};

// Represents a location of resource memory
struct ResourceMemoryLocation
{
  uint8_t *location = nullptr;

  ResourceMemoryLocation operator+(uint64_t offset) const { return {.location = location + offset}; }
};
// Represents a range of resource memory
struct ResourceMemoryRange
{
  ValueRange<uint8_t *> range;

  bool intersectsWith(const ResourceMemoryRange &mem) const { return range.overlaps(mem.range); }

  operator ResourceMemoryLocation() const { return {.location = range.front()}; }
};

inline ResourceMemoryRange make_range(const ResourceMemoryLocation &location, uint64_t size)
{
  return {.range = make_value_range(location.location, size)};
}

// generic location and GPU location is the same
union ResourceMemoryLocationWithGPUAddress
{
  uint8_t *location = nullptr;
  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;

  operator ResourceMemoryLocation() const { return {.location = location}; }

  ResourceMemoryLocationWithGPUAddress operator+(uint64_t offset) const { return {.location = location + offset}; }
};

// generic range and GPU range is the same
union ResourceMemoryRangeWithGPUAddress
{
  ValueRange<uint8_t *> range;
  ValueRange<D3D12_GPU_VIRTUAL_ADDRESS> gpuRange;

  operator ResourceMemoryRange() const
  {
    return {
      .range = range,
    };
  }
};

// generic location, GPU location and CPU location is the same
union ResourceMemoryLocationWithGPUAndCPUAddress
{
  uint8_t *location = nullptr;
  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
  uint8_t *cpuAddress;

  operator ResourceMemoryLocationWithGPUAddress() const { return {.location = location}; }
  operator ResourceMemoryLocation() const { return {.location = location}; }
  operator GPUAndCPUAddress() const { return {.cpuAddress = cpuAddress}; }

  ResourceMemoryLocationWithGPUAndCPUAddress operator+(uint64_t offset) const { return {.location = location + offset}; }
};

// generic range, GPU range and CPU range is the same
union ResourceMemoryRangeWithGPUAndCPUAddress
{
  ValueRange<uint8_t *> range;
  ValueRange<D3D12_GPU_VIRTUAL_ADDRESS> gpuRange;
  ValueRange<uint8_t *> cpuRange;

  operator ResourceMemoryRangeWithGPUAddress() const
  {
    return {
      .range = range,
    };
  }

  operator ResourceMemoryRange() const
  {
    return {
      .range = range,
    };
  }

  operator GPUAndCPUAddress() const
  {
    return {
      .cpuAddress = cpuRange.front(),
    };
  }
};
#else
struct GPUAndCPUAddress
{
  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress{};
  uint8_t *cpuAddress = nullptr;
};
// Represents a location of resource memory
struct ResourceMemoryLocation
{
  ID3D12Heap *heap = nullptr;
  uint64_t offset = 0;

  ResourceMemoryLocation operator+(uint64_t ofs) const
  {
    return {
      .heap = heap,
      .offset = offset + ofs,
    };
  }
};
// Represents a range of resource memory
struct ResourceMemoryRange
{
  ID3D12Heap *heap = nullptr;
  ValueRange<uint64_t> range;

  bool intersectsWith(const ResourceMemoryRange &mem) const { return (heap == mem.heap) && range.overlaps(mem.range); }

  operator ResourceMemoryLocation() const { return {.heap = heap, .offset = range.front()}; }
};

inline ResourceMemoryRange make_range(const ResourceMemoryLocation &location, uint64_t size)
{
  return {
    .heap = location.heap,
    .range = make_value_range(location.offset, size),
  };
}

struct ResourceMemoryLocationWithGPUAddress
{
  ID3D12Heap *heap = nullptr;
  uint64_t offset = 0;
  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress{};

  operator ResourceMemoryLocation() const
  {
    return {
      .heap = heap,
      .offset = offset,
    };
  }

  ResourceMemoryLocationWithGPUAddress operator+(uint64_t ofs) const
  {
    return {
      .heap = heap,
      .offset = offset + ofs,
      .gpuAddress = gpuAddress ? gpuAddress + ofs : 0,
    };
  }
};

struct ResourceMemoryRangeWithGPUAddress
{
  ID3D12Heap *heap = nullptr;
  ValueRange<uint64_t> range;
  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress{};

  operator ResourceMemoryRange() const
  {
    return {
      .heap = heap,
      .range = range,
    };
  }
};

struct ResourceMemoryLocationWithGPUAndCPUAddress
{
  ID3D12Heap *heap = nullptr;
  uint64_t offset = 0;
  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress{};
  uint8_t *cpuAddress = nullptr;

  operator ResourceMemoryLocationWithGPUAddress() const
  {
    return {
      .heap = heap,
      .offset = offset,
      .gpuAddress = gpuAddress,
    };
  }

  operator ResourceMemoryLocation() const
  {
    return {
      .heap = heap,
      .offset = offset,
    };
  }

  operator GPUAndCPUAddress() const
  {
    return {
      .gpuAddress = gpuAddress,
      .cpuAddress = cpuAddress,
    };
  }

  ResourceMemoryLocationWithGPUAndCPUAddress operator+(uint64_t ofs) const
  {
    return {
      .heap = heap,
      .offset = offset + ofs,
      .gpuAddress = gpuAddress ? gpuAddress + ofs : 0,
      .cpuAddress = cpuAddress ? cpuAddress + ofs : 0,
    };
  }
};

struct ResourceMemoryRangeWithGPUAndCPUAddress
{
  ID3D12Heap *heap = nullptr;
  ValueRange<uint64_t> range;
  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress{};
  uint8_t *cpuAddress = nullptr;

  operator ResourceMemoryRangeWithGPUAddress() const
  {
    return {
      .heap = heap,
      .range = range,
      .gpuAddress = gpuAddress,
    };
  }

  operator ResourceMemoryRange() const
  {
    return {
      .heap = heap,
      .range = range,
    };
  }

  operator GPUAndCPUAddress() const
  {
    return {
      .gpuAddress = gpuAddress,
      .cpuAddress = cpuAddress,
    };
  }
};
#endif

class Device;
Device &get_device();

} // namespace drv3d_dx12
