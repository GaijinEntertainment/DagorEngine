// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "d3d12_debug_names.h"
#include "d3d12_error_handling.h"
#include "d3d12_utils.h"
#include "descriptor_heap.h"
#include "dynamic_array.h"
#include "host_device_shared_memory_region.h"
#include "image_view_state.h"
#include "pipeline_cache.h"
#include "render_state.h"
#include "shader_program_id.h"
#include "tagged_handles.h"

#include <atomic>
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <EASTL/fixed_string.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <perfMon/dag_autoFuncProf.h>
#include <perfMon/dag_statDrv.h>
#include <supp/dag_comPtr.h>


namespace cacheBlk
{
// It is unlikely that we have more than 32 unique signature blobs (eg shader dumps)
using SignatureMask = uint32_t;
struct SignatureHash
{
  eastl::string name;
  eastl::string hash;
  int64_t timestamp;
};
struct SignatureEntry
{
  eastl::string name;
  DynamicArray<SignatureHash> hashes;
};

struct UseCodes
{
  uint32_t staticCode;
  uint32_t dynamicCode;
};

struct ComputeClassUse
{
  SignatureMask validSiagnatures;
  eastl::string name;
  DynamicArray<UseCodes> codes;
};

struct GraphicsVariantUse
{
  eastl::string name;
  DynamicArray<UseCodes> codes;
};

struct MeshVariantProperties
{
  SignatureMask validSiagnatures;
  uint32_t staticRenderState;
  uint32_t frameBufferLayout;
  bool isWireFrame;
};

struct GraphicsVariantProperties : MeshVariantProperties
{
  uint32_t topology;
  uint32_t inputLayout;
};

struct GraphicsVariantGroup : GraphicsVariantProperties
{
  DynamicArray<GraphicsVariantUse> classes;
};
} // namespace cacheBlk


#if !_TARGET_XBOXONE
using CD3DX12_PIPELINE_STATE_STREAM_AS =
  CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS>;
using CD3DX12_PIPELINE_STATE_STREAM_MS =
  CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS>;
#endif

namespace drv3d_dx12
{
// pix has some issues to properly display the object listing when names are too long
// so we have to shorten long ones to a fixed max
inline void shorten_name_for_pix(eastl::string &name)
{
  constexpr size_t max_length = 256;
  if (name.length() > max_length)
  {
    name.resize(max_length - 3);
    name.push_back('.');
    name.push_back('.');
    name.push_back('.');
  }
}
namespace backend
{
class BindlessSetManager;
class ShaderModuleManager;
} // namespace backend
// Wraps the global id of a buffer object, this makes it easier to access
// the index and the buffer property bits its carrying.
class BufferGlobalId
{
  static constexpr uint32_t used_as_vertex_or_const_buffer = 1u << 31;
  static constexpr uint32_t used_as_index_buffer = 1u << 30;
  static constexpr uint32_t used_as_indirect_buffer = 1u << 29;
  static constexpr uint32_t used_as_non_pixel_resource_buffer = 1u << 28;
  static constexpr uint32_t used_as_pixel_resource_buffer = 1u << 27;
  static constexpr uint32_t used_as_copy_source_buffer = 1u << 26;
  static constexpr uint32_t used_as_uav_buffer = 1u << 25;
  static constexpr uint32_t report_state_transitions = 1u << 24;
  static constexpr uint32_t index_mask = report_state_transitions - 1;
  uint32_t value = ~uint32_t(0);

public:
  BufferGlobalId() = default;
  BufferGlobalId(const BufferGlobalId &) = default;
  BufferGlobalId &operator=(const BufferGlobalId &) = default;
  explicit BufferGlobalId(uint32_t index) : value{index} { G_ASSERT(index == (index & index_mask)); }

  void setReportStateTransitions() { value |= report_state_transitions; }

  bool shouldReportStateTransitions() const { return 0 != (value & report_state_transitions); }

  void inerhitStatusBits(BufferGlobalId other) { value |= other.value & ~index_mask; }

  void markUsedAsConstOrVertexBuffer() { value |= used_as_vertex_or_const_buffer; }
  void markUsedAsIndexBuffer() { value |= used_as_index_buffer; }
  void markUsedAsIndirectBuffer() { value |= used_as_indirect_buffer; }
  void markUsedAsCopySourceBuffer() { value |= used_as_copy_source_buffer; }
  void markUsedAsNonPixelShaderResource() { value |= used_as_non_pixel_resource_buffer; }
  void markUsedAsPixelShaderResource() { value |= used_as_pixel_resource_buffer; }
  void markUsedAsUAVBuffer() { value |= used_as_uav_buffer; }
  void removeMarkedAsUAVBuffer() { value &= ~used_as_uav_buffer; }
  void resetStatusBits() { value &= index_mask; }

  bool isUsedAsVertexOrConstBuffer() const { return value & used_as_vertex_or_const_buffer; }
  bool isUsedAsIndexBuffer() const { return value & used_as_index_buffer; }
  bool isUsedAsIndirectBuffer() const { return value & used_as_indirect_buffer; }
  bool isUsedAsNonPixelShaderResourceBuffer() const { return value & used_as_non_pixel_resource_buffer; }
  bool isUseAsPixelShaderResourceBuffer() const { return value & used_as_pixel_resource_buffer; }
  bool isUsedAsCopySourceBuffer() const { return value & used_as_copy_source_buffer; }
  bool isUsedAsUAVBuffer() const { return value & used_as_uav_buffer; }
  uint32_t index() const { return index_mask & value; }

  explicit operator bool() const { return value != ~uint32_t(0); }

  friend bool operator==(const BufferGlobalId l, const BufferGlobalId r) { return ((l.value ^ r.value) & index_mask) == 0; }
  friend bool operator!=(const BufferGlobalId l, const BufferGlobalId r) { return ((l.value ^ r.value) & index_mask) != 0; }
  uint32_t get() const { return value; }
};

struct BufferState
{
  ID3D12Resource *buffer = nullptr;
  uint64_t size = 0;
  uint32_t discardCount = 0;
  uint32_t currentDiscardIndex = 0;
  uint32_t fistDiscardFrame = 0;
  uint64_t offset = 0;
  BufferGlobalId resourceId;
  eastl::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> srvs;
  eastl::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> uavs;
  eastl::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> uavForClear;
#if _TARGET_PC_WIN
  D3D12_GPU_VIRTUAL_ADDRESS gpuPointer = 0;
  uint8_t *cpuPointer = nullptr;
#else
  // on xbox cpu/gpu addresses are identical per object
  union
  {
    D3D12_GPU_VIRTUAL_ADDRESS gpuPointer = 0;
    uint8_t *cpuPointer;
  };
#endif
  uint64_t totalSize() const { return discardCount * size; }

  ValueRange<uint64_t> usageRange() const { return make_value_range<uint64_t>(offset, totalSize()); }

  uint64_t currentOffset() const { return offset + currentDiscardIndex * size; }

  D3D12_GPU_VIRTUAL_ADDRESS currentGPUPointer() const { return gpuPointer + currentDiscardIndex * size; }

  uint8_t *currentCPUPointer() const { return cpuPointer ? cpuPointer + currentDiscardIndex * size : nullptr; }

  D3D12_CPU_DESCRIPTOR_HANDLE currentSRV() const { return srvs ? srvs[currentDiscardIndex] : D3D12_CPU_DESCRIPTOR_HANDLE{}; }

  D3D12_CPU_DESCRIPTOR_HANDLE currentUAV() const { return uavs ? uavs[currentDiscardIndex] : D3D12_CPU_DESCRIPTOR_HANDLE{}; }

  D3D12_CPU_DESCRIPTOR_HANDLE currentClearUAV() const
  {
    return uavForClear ? uavForClear[currentDiscardIndex] : uavs ? uavs[currentDiscardIndex] : D3D12_CPU_DESCRIPTOR_HANDLE{};
  }

  explicit operator bool() const { return nullptr != buffer; }
  bool operator==(const BufferState &other) const { return other.buffer == buffer && other.offset == offset; }
  bool operator!=(const BufferState &other) const { return !(*this == other); }

  void flushMappedMemory(uint32_t r_offset, uint32_t f_size)
  {
    void *ptr = nullptr;
    D3D12_RANGE range{};
    buffer->Map(0, &range, &ptr);
    range.Begin = currentOffset() + r_offset;
    range.End = range.Begin + f_size;
    buffer->Unmap(0, &range);
  }
  void invalidateMappedMemory(uint32_t r_offset, uint32_t f_size)
  {
    void *ptr = nullptr;
    D3D12_RANGE range{};
    range.Begin = currentOffset() + r_offset;
    range.End = range.Begin + f_size;
    buffer->Map(0, &range, &ptr);
    range.Begin = range.End = 0;
    buffer->Unmap(0, &range);
  }
};

struct BufferReference
{
  ID3D12Resource *buffer = nullptr;
  uint64_t offset = 0; // only use this in conjunction with the buffer object, all pointers are
                       // already adjusted!
  uint64_t size = 0;
  BufferGlobalId resourceId;
  D3D12_CPU_DESCRIPTOR_HANDLE srv = {0};
  D3D12_CPU_DESCRIPTOR_HANDLE uav = {0};
  D3D12_CPU_DESCRIPTOR_HANDLE clearView = {0};
#if _TARGET_PC_WIN
  D3D12_GPU_VIRTUAL_ADDRESS gpuPointer = 0;
  uint8_t *cpuPointer = nullptr;
#else
  // on xbox cpu/gpu addresses are identical per object
  union
  {
    D3D12_GPU_VIRTUAL_ADDRESS gpuPointer = 0;
    uint8_t *cpuPointer;
  };
#endif

  BufferReference() = default;

  // clang format produces stupid layout
  // clang-format off
  BufferReference(const BufferState &b)
    : buffer{b.buffer}
    , size{b.size}
    , offset{ b.currentOffset() }
    , resourceId{ b.resourceId }
    , srv{ b.currentSRV() }
    , uav{ b.currentUAV() }
    , clearView { b.currentClearUAV() }
    , gpuPointer { b.currentGPUPointer() }
#if _TARGET_PC_WIN
  , cpuPointer { b.currentCPUPointer() }
#endif
  {
  }

  explicit BufferReference(const HostDeviceSharedMemoryRegion &hdsmr)
    : buffer{hdsmr.buffer}
    , size{uint32_t(hdsmr.range.size())}
    , offset{uint32_t(hdsmr.range.front())}
    , gpuPointer{hdsmr.gpuPointer}
#if _TARGET_PC_WIN
    , cpuPointer{hdsmr.pointer}
#endif
  {
  }
  // clang-format on

  explicit operator bool() const { return nullptr != buffer; }

  bool operator==(const BufferReference &other) const { return buffer == other.buffer && offset == other.offset; }
  bool operator!=(const BufferReference &other) const { return !(*this == other); }
};

struct BufferResourceReference
{
  ID3D12Resource *buffer = nullptr;
  BufferGlobalId resourceId;

  BufferResourceReference() = default;
  BufferResourceReference(const BufferReference &ref) : buffer{ref.buffer}, resourceId{ref.resourceId} {}
  BufferResourceReference(const BufferState &buf) : buffer{buf.buffer}, resourceId{buf.resourceId} {}
  BufferResourceReference(const HostDeviceSharedMemoryRegion &buf) : buffer{buf.buffer}
  // resource id stays as default
  {
    G_ASSERT(!resourceId);
  }

  explicit operator bool() const { return buffer != nullptr; }

  bool operator==(const BufferResourceReference &other) const { return (resourceId == other.resourceId) && (buffer == other.buffer); }
  bool operator!=(const BufferResourceReference &other) const { return !(*this == other); }
};

struct BufferResourceReferenceAndOffset : BufferResourceReference
{
  uint64_t offset = 0;

  BufferResourceReferenceAndOffset() = default;
  BufferResourceReferenceAndOffset(const BufferReference &ref) : BufferResourceReference{ref}, offset{ref.offset} {}
  BufferResourceReferenceAndOffset(const BufferState &buf) : BufferResourceReference{buf}, offset{BufferReference{buf}.offset} {}
  BufferResourceReferenceAndOffset(const BufferReference &ref, uint64_t ofs) : BufferResourceReference{ref}, offset{ref.offset + ofs}
  {}
  BufferResourceReferenceAndOffset(const BufferState &buf, uint64_t ofs) :
    BufferResourceReference{buf}, offset{BufferReference{buf}.offset + ofs}
  {}
  BufferResourceReferenceAndOffset(const HostDeviceSharedMemoryRegion &buf) :
    BufferResourceReference{buf}, offset{uint64_t(buf.range.front())}
  {}
  BufferResourceReferenceAndOffset(const HostDeviceSharedMemoryRegion &buf, uint64_t ofs) :
    BufferResourceReference{buf}, offset{uint64_t(buf.range.front() + ofs)}
  {}

  bool operator==(const BufferResourceReferenceAndOffset &other) const
  {
    return (resourceId == other.resourceId) && (buffer == other.buffer) && (offset == other.offset);
  }

  bool operator!=(const BufferResourceReferenceAndOffset &other) const { return !(*this == other); }
};

struct BufferResourceReferenceAndRange : BufferResourceReferenceAndOffset
{
  uint64_t size = 0;

  BufferResourceReferenceAndRange() = default;
  BufferResourceReferenceAndRange(const BufferReference &ref) : BufferResourceReferenceAndOffset{ref}, size{ref.size} {}
  BufferResourceReferenceAndRange(const BufferState &buf) : BufferResourceReferenceAndOffset{buf}, size{buf.size} {}
  BufferResourceReferenceAndRange(const BufferReference &ref, uint64_t offset) :
    BufferResourceReferenceAndOffset(ref, offset), size{ref.size - offset}
  {}
  BufferResourceReferenceAndRange(const BufferState &buf, uint64_t offset) :
    BufferResourceReferenceAndOffset{buf, offset}, size{buf.size - offset}
  {}
  BufferResourceReferenceAndRange(const BufferReference &ref, uint64_t offset, uint64_t sz) :
    BufferResourceReferenceAndOffset{ref, offset}, size{sz}
  {}
  BufferResourceReferenceAndRange(const BufferState &buf, uint64_t offset, uint64_t sz) :
    BufferResourceReferenceAndOffset{buf, offset}, size{sz}
  {}
  BufferResourceReferenceAndRange(const HostDeviceSharedMemoryRegion &buf) :
    BufferResourceReferenceAndOffset{buf}, size{buf.range.size()}
  {}
  BufferResourceReferenceAndRange(const HostDeviceSharedMemoryRegion &buf, uint64_t ofs) :
    BufferResourceReferenceAndOffset{buf, ofs}, size{buf.range.size() - ofs}
  {}
  BufferResourceReferenceAndRange(const HostDeviceSharedMemoryRegion &buf, uint64_t ofs, uint64_t sz) :
    BufferResourceReferenceAndOffset{buf, ofs}, size{sz}
  {}

  bool operator==(const BufferResourceReferenceAndRange &other) const
  {
    return (resourceId == other.resourceId) && (buffer == other.buffer) && (offset == other.offset) && (size == other.size);
  }

  bool operator!=(const BufferResourceReferenceAndRange &other) const { return !(*this == other); }
};

struct BufferResourceReferenceAndAddress : BufferResourceReference
{
  D3D12_GPU_VIRTUAL_ADDRESS gpuPointer = 0;

  BufferResourceReferenceAndAddress() = default;
  BufferResourceReferenceAndAddress(const BufferReference &ref) : BufferResourceReference{ref}, gpuPointer{ref.gpuPointer} {}
  BufferResourceReferenceAndAddress(const BufferState &buf) : BufferResourceReference{buf}, gpuPointer{BufferReference{buf}.gpuPointer}
  {}
  BufferResourceReferenceAndAddress(const BufferReference &ref, uint64_t offset) :
    BufferResourceReference{ref}, gpuPointer{ref.gpuPointer + offset}
  {}
  BufferResourceReferenceAndAddress(const BufferState &buf, uint64_t offset) :
    BufferResourceReference{buf}, gpuPointer{BufferReference{buf}.gpuPointer + offset}
  {}
  BufferResourceReferenceAndAddress(const HostDeviceSharedMemoryRegion &buf) : BufferResourceReference{buf}, gpuPointer{buf.gpuPointer}
  {}
  BufferResourceReferenceAndAddress(const HostDeviceSharedMemoryRegion &buf, uint64_t offset) :
    BufferResourceReference{buf}, gpuPointer{buf.gpuPointer + offset}
  {}

  bool operator==(const BufferResourceReferenceAndAddress &other) const
  {
    // pointer compare of gpu pointer is sufficient as two different buffer can never have the same gpu address.
    return gpuPointer == other.gpuPointer;
  }

  bool operator!=(const BufferResourceReferenceAndAddress &other) const { return !(*this == other); }

  bool operator==(D3D12_GPU_VIRTUAL_ADDRESS other_adr) const { return gpuPointer == other_adr; }

  bool operator!=(D3D12_GPU_VIRTUAL_ADDRESS other_adr) const { return !(*this == other_adr); }
};

struct BufferResourceReferenceAndAddressRange : BufferResourceReferenceAndAddress
{
  uint64_t size = 0;

  BufferResourceReferenceAndAddressRange() = default;
  BufferResourceReferenceAndAddressRange(const BufferReference &ref) : BufferResourceReferenceAndAddress{ref}, size{ref.size} {}
  BufferResourceReferenceAndAddressRange(const BufferState &buf) : BufferResourceReferenceAndAddress{buf}, size{buf.size} {}
  BufferResourceReferenceAndAddressRange(const BufferReference &ref, uint64_t offset) :
    BufferResourceReferenceAndAddress(ref, offset), size{ref.size - offset}
  {}
  BufferResourceReferenceAndAddressRange(const BufferState &buf, uint64_t offset) :
    BufferResourceReferenceAndAddress{buf, offset}, size{buf.size - offset}
  {}
  BufferResourceReferenceAndAddressRange(const BufferReference &ref, uint64_t offset, uint64_t sz) :
    BufferResourceReferenceAndAddress{ref, offset}, size{sz ? sz : uint64_t(ref.size - offset)}
  {}
  BufferResourceReferenceAndAddressRange(const BufferState &buf, uint64_t offset, uint64_t sz) :
    BufferResourceReferenceAndAddress{buf, offset}, size{sz ? sz : uint64_t(buf.size - offset)}
  {}
  BufferResourceReferenceAndAddressRange(const HostDeviceSharedMemoryRegion &buf) :
    BufferResourceReferenceAndAddress{buf}, size{uint64_t(buf.range.size())}
  {}
  BufferResourceReferenceAndAddressRange(const HostDeviceSharedMemoryRegion &buf, uint64_t offset) :
    BufferResourceReferenceAndAddress{buf, offset}, size{uint64_t(buf.range.size() - offset)}
  {}
  BufferResourceReferenceAndAddressRange(const HostDeviceSharedMemoryRegion &buf, uint64_t offset, uint64_t sz) :
    BufferResourceReferenceAndAddress{buf, offset}, size{sz ? sz : uint64_t(buf.range.size() - offset)}
  {}

  bool operator==(const BufferResourceReferenceAndAddressRange &other) const
  {
    return gpuPointer == other.gpuPointer && size == other.size;
  }

  bool operator!=(const BufferResourceReferenceAndAddressRange &other) const { return !(*this == other); }

  bool operator==(D3D12_CONSTANT_BUFFER_VIEW_DESC other) const
  {
    return gpuPointer == other.BufferLocation && size == other.SizeInBytes;
  }

  bool operator!=(D3D12_CONSTANT_BUFFER_VIEW_DESC other) const { return !(*this == other); }
};

inline bool operator==(D3D12_GPU_VIRTUAL_ADDRESS l, const BufferResourceReferenceAndAddress &r) { return r == l; }

inline bool operator!=(D3D12_GPU_VIRTUAL_ADDRESS l, const BufferResourceReferenceAndAddress &r) { return r != l; }

inline bool operator==(D3D12_CONSTANT_BUFFER_VIEW_DESC l, const BufferResourceReferenceAndAddressRange &r) { return r == l; }

inline bool operator!=(D3D12_CONSTANT_BUFFER_VIEW_DESC l, const BufferResourceReferenceAndAddressRange &r) { return r != l; }

struct BufferResourceReferenceAndShaderResourceView : BufferResourceReference
{
  D3D12_CPU_DESCRIPTOR_HANDLE srv = {0};

  BufferResourceReferenceAndShaderResourceView() = default;
  BufferResourceReferenceAndShaderResourceView(const BufferReference &ref) : BufferResourceReference{ref}, srv{ref.srv} {}
  BufferResourceReferenceAndShaderResourceView(const BufferState &buf) : BufferResourceReference{buf}, srv{BufferReference{buf}.srv} {}

  bool operator==(const BufferResourceReferenceAndShaderResourceView &other) const { return srv == other.srv; }
  bool operator!=(const BufferResourceReferenceAndShaderResourceView &other) const { return !(*this == other); }

  bool operator==(D3D12_CPU_DESCRIPTOR_HANDLE view) const { return srv == view; }
  bool operator!=(D3D12_CPU_DESCRIPTOR_HANDLE view) const { return srv != view; }
};

inline bool operator==(D3D12_CPU_DESCRIPTOR_HANDLE view, const BufferResourceReferenceAndShaderResourceView &s)
{
  return s.srv == view;
}

inline bool operator!=(D3D12_CPU_DESCRIPTOR_HANDLE view, const BufferResourceReferenceAndShaderResourceView &s)
{
  return s.srv != view;
}

struct BufferResourceReferenceAndUnorderedResourceView : BufferResourceReference
{
  D3D12_CPU_DESCRIPTOR_HANDLE uav = {0};

  BufferResourceReferenceAndUnorderedResourceView() = default;
  BufferResourceReferenceAndUnorderedResourceView(const BufferReference &ref) : BufferResourceReference{ref}, uav{ref.uav} {}
  BufferResourceReferenceAndUnorderedResourceView(const BufferState &buf) : BufferResourceReference{buf}, uav{BufferReference{buf}.uav}
  {}

  bool operator==(const BufferResourceReferenceAndUnorderedResourceView &other) const { return uav == other.uav; }
  bool operator!=(const BufferResourceReferenceAndUnorderedResourceView &other) const { return !(*this == other); }

  bool operator==(D3D12_CPU_DESCRIPTOR_HANDLE view) const { return uav == view; }
  bool operator!=(D3D12_CPU_DESCRIPTOR_HANDLE view) const { return uav != view; }
};

inline bool operator==(D3D12_CPU_DESCRIPTOR_HANDLE view, const BufferResourceReferenceAndUnorderedResourceView &s)
{
  return s.uav == view;
}

inline bool operator!=(D3D12_CPU_DESCRIPTOR_HANDLE view, const BufferResourceReferenceAndUnorderedResourceView &s)
{
  return s.uav != view;
}

struct BufferResourceReferenceAndClearView : BufferResourceReference
{
  D3D12_CPU_DESCRIPTOR_HANDLE clearView = {0};

  BufferResourceReferenceAndClearView() = default;
  BufferResourceReferenceAndClearView(const BufferReference &ref) : BufferResourceReference{ref}, clearView{ref.clearView} {}
  BufferResourceReferenceAndClearView(const BufferState &buf) : BufferResourceReference{buf}, clearView{BufferReference{buf}.clearView}
  {}

  bool operator==(const BufferResourceReferenceAndClearView &other) const { return clearView == other.clearView; }
  bool operator!=(const BufferResourceReferenceAndClearView &other) const { return !(*this == other); }

  bool operator==(D3D12_CPU_DESCRIPTOR_HANDLE view) const { return clearView == view; }
  bool operator!=(D3D12_CPU_DESCRIPTOR_HANDLE view) const { return clearView != view; }
};

inline bool operator==(D3D12_CPU_DESCRIPTOR_HANDLE view, const BufferResourceReferenceAndClearView &s) { return s.clearView == view; }

inline bool operator!=(D3D12_CPU_DESCRIPTOR_HANDLE view, const BufferResourceReferenceAndClearView &s) { return s.clearView != view; }

struct BufferResourceReferenceAndAddressRangeWithClearView : BufferResourceReferenceAndAddressRange
{
  D3D12_CPU_DESCRIPTOR_HANDLE clearView = {0};

  BufferResourceReferenceAndAddressRangeWithClearView() = default;
  BufferResourceReferenceAndAddressRangeWithClearView(const BufferReference &ref) :
    BufferResourceReferenceAndAddressRange{ref}, clearView{ref.clearView}
  {}
  BufferResourceReferenceAndAddressRangeWithClearView(const BufferState &buf) :
    BufferResourceReferenceAndAddressRange{buf}, clearView{BufferReference{buf}.clearView}
  {}

  bool operator==(const BufferResourceReferenceAndAddressRangeWithClearView &other) const { return clearView == other.clearView; }
  bool operator!=(const BufferResourceReferenceAndAddressRangeWithClearView &other) const { return !(*this == other); }

  bool operator==(D3D12_CPU_DESCRIPTOR_HANDLE view) const { return clearView == view; }
  bool operator!=(D3D12_CPU_DESCRIPTOR_HANDLE view) const { return clearView != view; }
};

class PipelineCache;
class BasePipeline;
class Device;
class ShaderProgramDatabase;
class StatefulCommandBuffer;
#if D3D_HAS_RAY_TRACING
struct RaytraceAccelerationStructure;
#endif

struct PipelineCreateInfoSizes
{
  enum
  {
    FLAGS_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_FLAGS),
    NODE_MASK_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK),
    ROOT_SIGNATURE_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE),
    INPUT_LAYOUT_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT),
    INDEX_BUFFER_STRIP_CUT_VALUE_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE),
    PRIMITIVE_TOPOLOGY_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY),
    VERTEX_SHADER_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS),
    PIXEL_SHADER_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS),
    GEOMETRY_SHADER_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_GS),
    STREAM_OUTPUT_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT),
    HULL_SHADER_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_HS),
    DOMAIN_SHADER_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_DS),
    COMPUTE_SHADER_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_CS),
    BLEND_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC),
    DEPTH_STENCIL_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL),
    DEPTH_STENCIL_1_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1),
    DEPTH_STENCIL_FORMAT_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT),
    RASTERIZER_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER),
    RENDER_TARGET_FORMAT_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS),
    MULTI_SAMPLE_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC),
    MULTI_SAMPLE_MASK_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK),
    CACHE_OBJECT_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO),
    VIEW_INSTANCING_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING),
#if !_TARGET_XBOXONE
    AMPLIFICATION_SHADER_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_AS),
    MESH_SHADER_INFO_SIZE = sizeof(CD3DX12_PIPELINE_STATE_STREAM_MS),
#endif

    BASIC_PIPELINE_SIZE = FLAGS_INFO_SIZE + NODE_MASK_INFO_SIZE + ROOT_SIGNATURE_INFO_SIZE + CACHE_OBJECT_INFO_SIZE,

    COMPUTE_STAGE_EVERYTHING_SIZE = BASIC_PIPELINE_SIZE + COMPUTE_SHADER_INFO_SIZE,

    DEPTH_STENCIL_ANY_INFO_SIZE =
      DEPTH_STENCIL_INFO_SIZE > DEPTH_STENCIL_1_INFO_SIZE ? DEPTH_STENCIL_INFO_SIZE : DEPTH_STENCIL_1_INFO_SIZE,

    GRAPHICS_VERTEX_STAGE_SIZE = INPUT_LAYOUT_INFO_SIZE + INDEX_BUFFER_STRIP_CUT_VALUE_INFO_SIZE + PRIMITIVE_TOPOLOGY_INFO_SIZE +
                                 VERTEX_SHADER_INFO_SIZE + GEOMETRY_SHADER_INFO_SIZE + HULL_SHADER_INFO_SIZE + DOMAIN_SHADER_INFO_SIZE,

#if _TARGET_XBOXONE
    GRAPHICS_MESH_STAGE_SIZE = 0,
#else
    GRAPHICS_MESH_STAGE_SIZE = MESH_SHADER_INFO_SIZE + AMPLIFICATION_SHADER_INFO_SIZE,
#endif

    GRAPHICS_PRE_RASTER_STAGE_SIZE =
      GRAPHICS_VERTEX_STAGE_SIZE > GRAPHICS_MESH_STAGE_SIZE ? GRAPHICS_VERTEX_STAGE_SIZE : GRAPHICS_MESH_STAGE_SIZE,

    GRAPHICS_STAGE_EVERYTHING_SIZE = BASIC_PIPELINE_SIZE + GRAPHICS_PRE_RASTER_STAGE_SIZE + PIXEL_SHADER_INFO_SIZE +
                                     STREAM_OUTPUT_INFO_SIZE + BLEND_INFO_SIZE + DEPTH_STENCIL_ANY_INFO_SIZE +
                                     DEPTH_STENCIL_FORMAT_INFO_SIZE + RASTERIZER_INFO_SIZE + RENDER_TARGET_FORMAT_INFO_SIZE +
                                     MULTI_SAMPLE_INFO_SIZE + MULTI_SAMPLE_MASK_INFO_SIZE + VIEW_INSTANCING_INFO_SIZE,

    TOTAL_STORE_SIZE =
      COMPUTE_STAGE_EVERYTHING_SIZE > GRAPHICS_STAGE_EVERYTHING_SIZE ? COMPUTE_STAGE_EVERYTHING_SIZE : GRAPHICS_STAGE_EVERYTHING_SIZE
  };
};

struct PipelineCreateInfoDataBase
{
  template <typename>
  struct ConstructHandler;

  template <bool = false>
  struct ConstructExecuter
  {
    template <typename IST, typename CT, typename... Args>
    static CT &construct(void *at, Args &&...args)
    {
      return *::new (at) CT{IST(eastl::forward<Args>(args)...)};
    }
  };
};

// this just explodes if the inner storage type is a pointer so
// need extra handling of that
template <>
struct PipelineCreateInfoDataBase::ConstructExecuter<true>
{
  template <typename, typename CT, typename... Args>
  static CT &construct(void *at, Args &&...args)
  {
    return *::new (at) CT{eastl::forward<Args>(args)...};
  }
};

template <typename IST, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE T, typename DA>
struct PipelineCreateInfoDataBase::ConstructHandler<CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<IST, T, DA>>
{
  typedef CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<IST, T, DA> ConstructType;
  template <typename... Args>
  static ConstructType &construct(void *at, Args &&...args)
  {
    return ConstructExecuter<eastl::is_pointer<IST>::value>::template construct<IST, ConstructType>(at, eastl::forward<Args>(args)...);
  }
};

// This is a base structure for pipeline create info, it
// can hold elements until its internal storage is filled.
template <size_t MAX_SIZE>
struct PipelineCreateInfoData : PipelineCreateInfoDataBase
{
  uint8_t store[MAX_SIZE]; //-V730_NOINIT
  uint32_t writeSize = 0;
  // use CD3DX12_PIPELINE_STATE_STREAM_ types to append elements
  template <typename T, typename... Args>
  T &append(Args &&...args)
  {
    G_ASSERT(writeSize + sizeof(T) <= MAX_SIZE);
    auto &ref = ConstructHandler<T>::construct(&store[writeSize], eastl::forward<Args>(args)...);
    writeSize += sizeof(T);
    debugTrack(ref);
    return ref;
  }
  template <typename T, typename U, typename... Args>
  T &emplace(Args &&...args)
  {
    return append<T>(U{eastl::forward<Args>(args)...});
  }
  void *root() { return store; }
  uint32_t rootSize() { return writeSize; }
  void reset() { writeSize = 0; }

#if DAGOR_DBGLEVEL > 0 & !_TARGET_XBOX
#define ADD_TRACKED_STATE(Type, Name) \
  const Type *Name = nullptr;         \
  void debugTrack(const Type &v)      \
  {                                   \
    Name = eastl::addressof(v);       \
  }
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT, depthStencilFormat);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS, renderTargetFormats);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC, blendDesc);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT, inputLayout);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER, rasterizer);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC, sampleDesc);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL, depthStencil);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1, depthStencil1);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE, rootSignature);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_VS, vertexShader);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_PS, pixelShader);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_GS, geometryShader);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_HS, hullShader);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_DS, domainShader);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY, primitiveTopology);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_CS, computeShader);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO, cachedPso);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING, viewInstancingDesc);
#if !_TARGET_XBOXONE
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_AS, amplificationShader);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_MS, meshShader);
#endif
#undef ADD_TRACKED_STATE
#else
  template <typename T>
  void debugTrack(const T &)
  {}
#endif
};

struct GraphicsPipelineCreateInfoData : PipelineCreateInfoData<PipelineCreateInfoSizes::GRAPHICS_STAGE_EVERYTHING_SIZE>
{
  // also provide store for extra stuff like input element info array
  D3D12_INPUT_ELEMENT_DESC inputElementStore[MAX_VERTEX_ATTRIBUTES];
};

struct ComputePipelineCreateInfoData : PipelineCreateInfoData<PipelineCreateInfoSizes::COMPUTE_STAGE_EVERYTHING_SIZE>
{
  // nothing extra here
};

enum class RecoverablePipelineCompileBehavior
{
  FATAL_ERROR,
  REPORT_ERROR,
  ASSERT_FAIL
};

inline RecoverablePipelineCompileBehavior get_recover_behavior_from_cfg(bool is_fatal, bool use_assert)
{
  if (is_fatal)
    return RecoverablePipelineCompileBehavior::FATAL_ERROR;
  if (use_assert)
    return RecoverablePipelineCompileBehavior::ASSERT_FAIL;
  return RecoverablePipelineCompileBehavior::REPORT_ERROR;
}

class PipelineVariant
{
  ComPtr<ID3D12PipelineState> pipeline;
  PipelineOptionalDynamicStateMask dynamicMask;

  bool calculateColorWriteMask(const eastl::string &pipeline_name, const BasePipeline &base,
    const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout, uint32_t &color_write_mask) const;
  // Generates description for color target formats, depth stencil format, blend modes, color masks and depth stencil modes.
  bool generateOutputMergerDescriptions(const eastl::string &pipeline_name, const BasePipeline &base,
    const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout,
    GraphicsPipelineCreateInfoData &target) const;
  bool generateInputLayoutDescription(const InputLayout &input_layout, GraphicsPipelineCreateInfoData &target,
    uint32_t &input_count) const;
  // Generates description for raster configuration and view instancing
  bool generateRasterDescription(const RenderStateSystem::StaticState &static_state, bool is_wire_frame,
    GraphicsPipelineCreateInfoData &target) const;

  bool create(ID3D12Device2 *device, backend::ShaderModuleManager &shader_bytecodes, BasePipeline &base, PipelineCache &pipe_cache,
    const InputLayout &input_layout, bool is_wire_frame, const RenderStateSystem::StaticState &static_state,
    const FramebufferLayout &fb_layout, D3D12_PRIMITIVE_TOPOLOGY_TYPE top, RecoverablePipelineCompileBehavior on_error, bool give_name,
    backend::PipelineNameGenerator &name_generator);
#if !_TARGET_XBOXONE
  bool createMesh(ID3D12Device2 *device, backend::ShaderModuleManager &shader_bytecodes, BasePipeline &base, PipelineCache &pipe_cache,
    bool is_wire_frame, const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout,
    RecoverablePipelineCompileBehavior on_error, bool give_name, backend::PipelineNameGenerator &name_generator);
#endif

  static bool validate_blend_desc(const D3D12_BLEND_DESC &blend_desc, const FramebufferLayout &fb_layout, uint32_t color_write_mask);

public:
  // Returns false if any error was discovered, in some cases the pipeline may be created anyway
  bool load(ID3D12Device2 *device, backend::ShaderModuleManager &shader_bytecodes, BasePipeline &base, PipelineCache &pipe_cache,
    const InputLayout &input_layout, bool is_wire_frame, const RenderStateSystem::StaticState &static_state,
    const FramebufferLayout &fb_layout, D3D12_PRIMITIVE_TOPOLOGY_TYPE top, RecoverablePipelineCompileBehavior on_error, bool give_name,
    backend::PipelineNameGenerator &name_generator);
  bool loadMesh(ID3D12Device2 *device, backend::ShaderModuleManager &shader_bytecodes, BasePipeline &base, PipelineCache &pipe_cache,
    bool is_wire_frame, const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout,
    RecoverablePipelineCompileBehavior on_error, bool give_name, backend::PipelineNameGenerator &name_generator);

  bool isReady() const { return nullptr != pipeline.Get(); }

  ID3D12PipelineState *get() const { return pipeline.Get(); }

  PipelineOptionalDynamicStateMask getUsedOptionalDynamicStateMask() const { return dynamicMask; }

  void preRecovery() { pipeline.Reset(); }
};

class FramebufferLayoutManager
{
  dag::Vector<FramebufferLayout> table;

public:
  FramebufferLayoutID getLayoutID(const FramebufferLayout &layout)
  {
    auto ref = eastl::find(begin(table), end(table), layout);
    if (end(table) == ref)
    {
      ref = table.insert(ref, layout);
    }
    return FramebufferLayoutID{static_cast<int>(ref - begin(table))};
  }
  const FramebufferLayout &getLayout(FramebufferLayoutID id) const { return table[id.get()]; }
};

namespace backend
{
class StaticRenderStateManager
{
protected:
  dag::Vector<RenderStateSystem::StaticState> staticRenderStateTable;

public:
  const RenderStateSystem::StaticState &getStaticRenderState(StaticRenderStateID ident) const
  {
    return staticRenderStateTable[ident.get()];
  }
  StaticRenderStateID findOrAddStaticRenderState(const RenderStateSystem::StaticState &state)
  {
    auto ref = eastl::find(begin(staticRenderStateTable), end(staticRenderStateTable), state);
    if (end(staticRenderStateTable) == ref)
    {
      ref = staticRenderStateTable.insert(ref, state);
    }
    return StaticRenderStateID::make(ref - begin(staticRenderStateTable));
  }
};
} // namespace backend

class BasePipeline
{
  friend class PipelineVariant;
  struct BaseVariantKey
  {
    union
    {
      uint64_t key;
      struct
      {
        // more than 500k as max should be sufficient
        uint64_t inputLayout : 19;
        uint64_t staticRenderState : 19;
        uint64_t framebufferLayout : 19;
        // only needs 6 bits (39 is the largest value on xbox)
        uint64_t topology : 6;
        uint64_t isWireFrame : 1;
      };
    };

    void setInputLayout(InternalInputLayoutID il)
    {
      G_ASSERT(il.get() < 0x80000);
      inputLayout = il.get();
    }

    InternalInputLayoutID getInputLayout() const { return InternalInputLayoutID::make(inputLayout); }

    void setStaticRenderState(StaticRenderStateID srs)
    {
      G_ASSERT(srs.get() < 0x80000);
      staticRenderState = srs.get();
    }

    StaticRenderStateID getStaticRenderState() const { return StaticRenderStateID::make(staticRenderState); }

    void moveStaticRenderStateID(StaticRenderStateID from, StaticRenderStateID to)
    {
      if (from.get() == staticRenderState)
      {
        setStaticRenderState(to);
      }
    }

    void swapStaticRenderStateID(StaticRenderStateID a, StaticRenderStateID b)
    {
      if (a.get() == staticRenderState)
      {
        setStaticRenderState(b);
      }
      else if (b.get() == staticRenderState)
      {
        setStaticRenderState(a);
      }
    }

    void setFrambufferLayout(FramebufferLayoutID fbl)
    {
      G_ASSERT(fbl.get() < 0x80000);
      framebufferLayout = fbl.get();
    }

    FramebufferLayoutID getFrambufferLayout() const { return FramebufferLayoutID::make(framebufferLayout); }

    void setTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE ptt)
    {
      G_ASSERT(static_cast<uint32_t>(ptt) < 0x40); // -V547
      topology = ptt;
    }

    D3D12_PRIMITIVE_TOPOLOGY_TYPE getTopology() const { return static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(topology); }

    void setWireFrame(bool wf) { isWireFrame = wf; }

    bool getWireFrame() const { return 0 != isWireFrame; }

    BaseVariantKey() : key{~uint64_t(0)} {}
    BaseVariantKey(const BaseVariantKey &) = default;
    BaseVariantKey &operator=(const BaseVariantKey &) = default;

    friend bool operator<(const BaseVariantKey &l, const BaseVariantKey &r) { return l.key < r.key; }
    friend bool operator==(const BaseVariantKey &l, const BaseVariantKey &r) { return l.key == r.key; }
    friend bool operator!=(const BaseVariantKey &l, const BaseVariantKey &r) { return !(l == r); }

    static BaseVariantKey forVertex(InternalInputLayoutID il, StaticRenderStateID srs, FramebufferLayoutID fbl,
      D3D12_PRIMITIVE_TOPOLOGY_TYPE t, bool wf)
    {
      BaseVariantKey result;
      result.setInputLayout(il);
      result.setStaticRenderState(srs);
      result.setFrambufferLayout(fbl);
      result.setTopology(t);
      result.setWireFrame(wf);
      return result;
    }

    static BaseVariantKey forMesh(StaticRenderStateID srs, FramebufferLayoutID fbl, bool wf)
    {
      BaseVariantKey result;
      result.setInputLayout(InternalInputLayoutID{0});
      result.setTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED);
      result.setStaticRenderState(srs);
      result.setFrambufferLayout(fbl);
      result.setWireFrame(wf);
      return result;
    }
  };
  struct BaseVariantInfo : BaseVariantKey
  {
    eastl::unique_ptr<PipelineVariant> pipeline;

    BaseVariantInfo() = default;
    BaseVariantInfo(const BaseVariantInfo &) = delete;
    BaseVariantInfo(BaseVariantInfo &&) = default;
    BaseVariantInfo(const BaseVariantKey &key) : BaseVariantKey{key} {}
    BaseVariantInfo &operator=(const BaseVariantInfo &) = delete;
    BaseVariantInfo &operator=(BaseVariantInfo &&) = default;
  };
  using VairantTable = dag::Vector<BaseVariantInfo>;
  VairantTable variants;
  GraphicsPipelineSignature &signature;
  uint32_t vsCombinedSRegisterCompareMask = 0;
  uint8_t vsCombinedTRegisterTypes[dxil::MAX_T_REGISTERS] = {};
  uint8_t vsCombinedURegisterTypes[dxil::MAX_U_REGISTERS] = {};
  const backend::VertexShaderModuleRefStore vsModule;
  const backend::PixelShaderModuleRefStore psModule;
  GraphicsProgramID program;
  GraphicsPipelineBaseCacheId cacheId;

public:
  BasePipeline(ID3D12Device2 *device, GraphicsPipelineSignature &s, PipelineCache &cache,
    backend::ShaderModuleManager &shader_bytecodes, backend::InputLayoutManager &input_layouts,
    backend::StaticRenderStateManager &static_states, FramebufferLayoutManager &framebuffer_layouts,
    backend::VertexShaderModuleRefStore vsm, backend::PixelShaderModuleRefStore psm, RecoverablePipelineCompileBehavior on_error,
    bool give_name, backend::PipelineNameGenerator &name_generator) :
    program(GraphicsProgramID::Null()), signature(s), vsModule(vsm), psModule(psm)
  {
    PipelineCache::BasePipelineIdentifier cacheIdent = {};
    cacheIdent.vs = vsModule.header.hash;
    vsCombinedSRegisterCompareMask = vsModule.header.header.sRegisterCompareUseMask;
    eastl::copy(eastl::begin(vsModule.header.header.tRegisterTypes), eastl::end(vsModule.header.header.tRegisterTypes),
      eastl::begin(vsCombinedTRegisterTypes));
    eastl::copy(eastl::begin(vsModule.header.header.uRegisterTypes), eastl::end(vsModule.header.header.uRegisterTypes),
      eastl::begin(vsCombinedURegisterTypes));
    cacheIdent.ps = psModule.header.hash;
    if (auto gsModule = get_gs(vsModule))
    {
      vsCombinedSRegisterCompareMask |= gsModule->sRegisterCompareUseMask;
      eastl::transform(eastl::begin(vsCombinedTRegisterTypes), eastl::end(vsCombinedTRegisterTypes),
        eastl::begin(gsModule->tRegisterTypes), eastl::begin(vsCombinedTRegisterTypes), [](auto a, auto b) { return a | b; });
      eastl::transform(eastl::begin(vsCombinedURegisterTypes), eastl::end(vsCombinedURegisterTypes),
        eastl::begin(gsModule->uRegisterTypes), eastl::begin(vsCombinedURegisterTypes), [](auto a, auto b) { return a | b; });
    }
    if (auto hsModule = get_hs(vsModule))
    {
      vsCombinedSRegisterCompareMask |= hsModule->sRegisterCompareUseMask;
      eastl::transform(eastl::begin(vsCombinedTRegisterTypes), eastl::end(vsCombinedTRegisterTypes),
        eastl::begin(hsModule->tRegisterTypes), eastl::begin(vsCombinedTRegisterTypes), [](auto a, auto b) { return a | b; });
      eastl::transform(eastl::begin(vsCombinedURegisterTypes), eastl::end(vsCombinedURegisterTypes),
        eastl::begin(hsModule->uRegisterTypes), eastl::begin(vsCombinedURegisterTypes), [](auto a, auto b) { return a | b; });
    }
    if (auto dsModule = get_ds(vsModule))
    {
      vsCombinedSRegisterCompareMask |= dsModule->sRegisterCompareUseMask;
      eastl::transform(eastl::begin(vsCombinedTRegisterTypes), eastl::end(vsCombinedTRegisterTypes),
        eastl::begin(dsModule->tRegisterTypes), eastl::begin(vsCombinedTRegisterTypes), [](auto a, auto b) { return a | b; });
      eastl::transform(eastl::begin(vsCombinedURegisterTypes), eastl::end(vsCombinedURegisterTypes),
        eastl::begin(dsModule->uRegisterTypes), eastl::begin(vsCombinedURegisterTypes), [](auto a, auto b) { return a | b; });
    }

    cacheId = cache.getGraphicsPipeline(cacheIdent);
    auto cnt = cache.getGraphicsPipelineVariantCount(cacheId);
    if (cnt > 0)
    {
      // Only report this when *something* can be loaded from cache
      logdbg("DX12: Graphics pipeline create found %u variants in cache, loading...", cnt);

      const bool weAreMesh = isMesh();
      size_t i = 0;
      while (i < cnt)
      {
        InputLayout inputLayout = {};
        bool isWireFrame = false;
        RenderStateSystem::StaticState staticState = {};
        FramebufferLayout framebufferLayout = {};
        auto top = cache.getGraphicsPipelineVariantDesc(cacheId, i, inputLayout, isWireFrame, staticState, framebufferLayout);
        auto staticRenderStateID = static_states.findOrAddStaticRenderState(staticState);

        if (!weAreMesh)
        {
          auto layoutID = input_layouts.addInternalLayout(inputLayout);

          auto &variant = getVariantFromConfiguration(layoutID, staticRenderStateID,
            framebuffer_layouts.getLayoutID(framebufferLayout), top, isWireFrame);
          if (!variant.load(device, shader_bytecodes, *this, cache, inputLayout, isWireFrame, staticState, framebufferLayout, top,
                on_error, give_name, name_generator))
          {
            cnt = cache.removeGraphicsPipelineVariant(cacheId, i);
            continue;
          }
        }
        else
        {
          auto &variant =
            getMeshVariantFromConfiguration(staticRenderStateID, framebuffer_layouts.getLayoutID(framebufferLayout), isWireFrame);
          if (!variant.loadMesh(device, shader_bytecodes, *this, cache, isWireFrame, staticState, framebufferLayout, on_error,
                give_name, name_generator))
          {
            cnt = cache.removeGraphicsPipelineVariant(cacheId, i);
            continue;
          }
        }

        ++i;
      }
    }
  }

  void setProgramId(GraphicsProgramID programId)
  {
    G_ASSERTF(!program, "Program ID must be set only once!");
    program = programId;
  }

  PipelineVariant &getVariantFromConfiguration(InternalInputLayoutID input_layout_id, StaticRenderStateID static_state_id,
    FramebufferLayoutID framebuffer_layout_id, D3D12_PRIMITIVE_TOPOLOGY_TYPE top, bool is_wire_frame)
  {
    auto key = BaseVariantKey::forVertex(input_layout_id, static_state_id, framebuffer_layout_id, top, is_wire_frame);
#if DX12_USE_BINARY_SEARCH_FOR_GRAPHICS_PIPELINE_VARIANTS
    auto ref = eastl::lower_bound(begin(variants), end(variants), key,
      [](const auto &l, const auto &r) //
      { return l < r; });
    if (ref == end(variants) || key != *ref)
#else
    auto ref = eastl::find(begin(variants), end(variants), key);
    if (ref == end(variants))
#endif
    {
      ref = variants.emplace(ref, key);
      ref->pipeline.reset(new PipelineVariant);
    }

    return *ref->pipeline;
  }
  PipelineVariant &getMeshVariantFromConfiguration(StaticRenderStateID static_state_id, FramebufferLayoutID framebuffer_layout_id,
    bool is_wire_frame)
  {
    auto key = BaseVariantKey::forMesh(static_state_id, framebuffer_layout_id, is_wire_frame);
#if DX12_USE_BINARY_SEARCH_FOR_GRAPHICS_PIPELINE_VARIANTS
    auto ref = eastl::lower_bound(begin(variants), end(variants), key,
      [](const auto &l, const auto &r) //
      { return l < r; });
    if (ref == end(variants) || key != *ref)
#else
    auto ref = eastl::find(begin(variants), end(variants), key);
    if (ref == end(variants))
#endif
    {
      ref = variants.emplace(ref, key);
      ref->pipeline.reset(new PipelineVariant);
    }

    return *ref->pipeline;
  }
  void unloadAll() { variants.clear(); }
  GraphicsPipelineSignature &getSignature() const { return signature; }
  bool hasTessellationStage() const { return vsModule.header.hasDsAndHs; }
  bool hasGeometryStage() const { return vsModule.header.hasGsOrAs; }
  bool isMesh() const { return is_mesh(vsModule); }
  D3D_PRIMITIVE getHullInputPrimitiveType() const { return static_cast<D3D_PRIMITIVE>(get_hs(vsModule)->inputPrimitive); }
  const dxil::ShaderHeader &getPSHeader() const { return psModule.header.header; }
  uint32_t getVertexShaderSamplerCompareMask() const { return vsCombinedSRegisterCompareMask; }
  const uint8_t *getVertexShaderCombinedTRegisterTypes() const { return vsCombinedTRegisterTypes; }
  const uint8_t *getVertexShaderCombinedURegisterTypes() const { return vsCombinedURegisterTypes; }
  void preRecovery()
  {
    for (auto &&bucket : variants)
      if (bucket.pipeline)
        bucket.pipeline->preRecovery();
  }
  uint32_t getVertexShaderInputMask() const
  {
    G_ASSERT(!isMesh());
    return vsModule.header.header.inOutSemanticMask;
  }
  bool matches(const backend::VertexShaderModuleRefStore &vs, const backend::PixelShaderModuleRefStore &ps) const
  {
    return vsModule.header.hash == vs.header.hash && psModule.header.hash == ps.header.hash;
  }

  void moveStaticRenderStateID(StaticRenderStateID from, StaticRenderStateID to)
  {
    for (auto &variant : variants)
    {
      variant.moveStaticRenderStateID(from, to);
    }
  }

  void swapStaticRenderStateID(StaticRenderStateID a, StaticRenderStateID b)
  {
    for (auto &variant : variants)
    {
      variant.swapStaticRenderStateID(a, b);
    }
  }

  BasePipelineIdentifier getIdentifier() const
  {
    BasePipelineIdentifier ident;
    ident.vs = vsModule.header.hash;
    ident.ps = psModule.header.hash;
    return ident;
  }

  const backend::VertexShaderModuleRefStore &vertexShaderModule() const { return vsModule; }

  const backend::PixelShaderModuleRefStore &pixelShaderModule() const { return psModule; }

  template <typename T>
  void visitVariants(T &&clb)
  {
    for (auto &v : variants)
    {
      clb(v.getInputLayout(), v.getStaticRenderState(), v.getFrambufferLayout(), v.getTopology(), v.getWireFrame());
    }
  }

#if _TARGET_PC_WIN
  void onCacheInvalidated(PipelineCache &cache, backend::InputLayoutManager &input_layouts,
    backend::StaticRenderStateManager &static_states, FramebufferLayoutManager &framebuffer_layouts)
  {
    PipelineCache::BasePipelineIdentifier cacheIdent = {};
    cacheIdent.vs = vsModule.header.hash;
    cacheIdent.ps = psModule.header.hash;
    cacheId = cache.getGraphicsPipeline(cacheIdent);
    // Also re-populate the cache with loaded pipelines
    if (isMesh())
    {
      for (auto &v : variants)
      {
        if (!v.pipeline || !v.pipeline->get())
        {
          continue;
        }
        auto &staticState = static_states.getStaticRenderState(v.getStaticRenderState());
        auto &frameBufferLayout = framebuffer_layouts.getLayout(v.getFrambufferLayout());
        cache.addGraphicsMeshPipelineVariant(cacheId, v.getWireFrame(), staticState, frameBufferLayout, v.pipeline->get());
      }
    }
    else
    {
      for (auto &v : variants)
      {
        if (!v.pipeline || !v.pipeline->get())
        {
          continue;
        }
        auto &inputLayout = input_layouts.getInputLayout(v.getInputLayout());
        auto &staticState = static_states.getStaticRenderState(v.getStaticRenderState());
        auto &frameBufferLayout = framebuffer_layouts.getLayout(v.getFrambufferLayout());
        cache.addGraphicsPipelineVariant(cacheId, v.getTopology(), inputLayout, v.getWireFrame(), staticState, frameBufferLayout,
          v.pipeline->get());
      }
    }
  }
#endif
};

class ComputePipeline
{
  ComputeShaderModule shaderModule;
  ComputePipelineSignature &signature;
  ComPtr<ID3D12PipelineState> pipeline;

  bool load(ID3D12Device2 *device, PipelineCache &cache, RecoverablePipelineCompileBehavior on_error, bool from_cache_only,
    bool give_name, backend::PipelineNameGenerator &name_generator)
  {
    G_UNUSED(give_name);
    if (pipeline)
      return true;
    auto name = name_generator.generateComputePipelineName(shaderModule);
#if DX12_REPORT_PIPELINE_CREATE_TIMING
    eastl::string profilerMsg;
    profilerMsg.sprintf("ComputePipeline(\"%s\")::load: took %%dus", name.c_str());
    AutoFuncProf funcProfiler{profilerMsg.c_str()};
#endif
    TIME_PROFILE_DEV(ComputePipeline_load);
    TIME_PROFILE_UNIQUE_EVENT_NAMED_DEV(name.c_str());

#if _TARGET_PC_WIN
    if (!signature.signature)
    {
#if DX12_REPORT_PIPELINE_CREATE_TIMING
      // turns off reporting of the profile, we don't want to know how long it took to _not_
      // load a pipeline
      funcProfiler.fmt = nullptr;
#endif
      logdbg("DX12: Rootsignature for %s was null, skipping creating compute pipeline", name);
      return true;
    }

    ComputePipelineCreateInfoData cpci;

    cpci.append<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(signature.signature.Get());
    cpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_CS, CD3DX12_SHADER_BYTECODE>(shaderModule.byteCode.get(), shaderModule.byteCodeSize);
    D3D12_CACHED_PIPELINE_STATE &cacheTarget = cpci.append<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO>();

    D3D12_PIPELINE_STATE_STREAM_DESC cpciDesc = {cpci.rootSize(), cpci.root()};
    pipeline = cache.loadCompute(shaderModule.ident.shaderHash, cpciDesc, cacheTarget);
    if (pipeline)
    {
      if (give_name)
      {
        shorten_name_for_pix(name);
        dag::Vector<wchar_t> nameBuf;
        nameBuf.resize(name.size() + 1);
        DX12_SET_DEBUG_OBJ_NAME(pipeline, lazyToWchar(name.data(), nameBuf.data(), nameBuf.size()));
      }
      return true;
    }

    // if we only want to load from cache and loadCompute on the cache did not load it
    // and we don't have a pso blob for creation, we don't have a cache entry for this
    // pipeline yet.
    if (from_cache_only && !cacheTarget.CachedBlobSizeInBytes)
    {
#if DX12_REPORT_PIPELINE_CREATE_TIMING
      // turns off reporting of the profile, we don't want to know how long it took to _not_
      // load a pipeline
      funcProfiler.fmt = nullptr;
#endif
      return true;
    }
#else
    // console has no cache so we load only when the shader is needed the first time
    if (from_cache_only)
      return true;

    G_UNUSED(cache);
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {0};
    desc.pRootSignature = signature.signature.Get();
    desc.CS.pShaderBytecode = shaderModule.byteCode.get();
    desc.CS.BytecodeLength = shaderModule.byteCodeSize;
    desc.NodeMask = 0;
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
#endif
    if (!pipeline)
    {
#if _TARGET_PC_WIN
      auto result = DX12_CHECK_RESULT(device->CreatePipelineState(&cpciDesc, COM_ARGS(&pipeline)));

      // if we use cache blob and it fails, try again without blob, it might not be compatible
      if (FAILED(result) && cacheTarget.CachedBlobSizeInBytes > 0)
      {
        // with cache only, we stop here
        if (from_cache_only)
        {
#if DX12_REPORT_PIPELINE_CREATE_TIMING
          // turns off reporting of the profile, we don't want to know how long it took to _not_
          // load a pipeline
          funcProfiler.fmt = nullptr;
#endif
          return true;
        }

        logdbg("DX12: Retrying to create compute pipeline without cache");
        cacheTarget.pCachedBlob = nullptr;
        cacheTarget.CachedBlobSizeInBytes = 0;
        result = DX12_CHECK_RESULT(device->CreatePipelineState(&cpciDesc, COM_ARGS(&pipeline)));
      }
#else
      auto result = DX12_CHECK_RESULT(device->CreateComputePipelineState(&desc, COM_ARGS(&pipeline)));
#endif
      // if it still fails then something else is not ok
      if (FAILED(result))
      {
        if (is_recoverable_error(result))
        {
          switch (on_error)
          {
            case RecoverablePipelineCompileBehavior::FATAL_ERROR: return false; break;
            case RecoverablePipelineCompileBehavior::REPORT_ERROR:
              // DX12_CHECK_RESULT reports the error already
              break;
            case RecoverablePipelineCompileBehavior::ASSERT_FAIL:
              G_ASSERT(!"CreatePipelineState failed, "
                        "see error log for details");
          }
        }
        else
        {
          return false;
        }
      }
#if _TARGET_PC_WIN
      // only write back cache if none exists yet (or is invalid)
      else if (0 == cacheTarget.CachedBlobSizeInBytes)
      {
        cache.addCompute(shaderModule.ident.shaderHash, pipeline.Get());
      }
#endif
    }

    if (give_name && pipeline && !name.empty())
    {
      shorten_name_for_pix(name);
      dag::Vector<wchar_t> nameBuf;
      nameBuf.resize(name.size() + 1);
      DX12_SET_DEBUG_OBJ_NAME(pipeline, lazyToWchar(name.data(), nameBuf.data(), nameBuf.size()));
    }
    return true;
  }

public:
  ComputePipelineSignature &getSignature() const { return signature; };
  ID3D12PipelineState *loadAndGetHandle(ID3D12Device2 *device, PipelineCache &cache, RecoverablePipelineCompileBehavior on_error,
    bool give_name, backend::PipelineNameGenerator &name_generator)
  {
    load(device, cache, on_error, false, give_name, name_generator);
    return pipeline.Get();
  }

  ComputePipeline(ComputePipelineSignature &sign, ComputeShaderModule module, ID3D12Device2 *device, PipelineCache &cache,
    RecoverablePipelineCompileBehavior on_error, bool give_name, backend::PipelineNameGenerator &name_generator) :
    shaderModule{eastl::move(module)}, signature{sign}
  {
    load(device, cache, on_error, true, give_name, name_generator);
  }
  const dxil::ShaderHeader &getHeader() const { return shaderModule.header; }
  const ComputeShaderModule &getModule() const { return shaderModule; }

  void preRecovery() { pipeline.Reset(); }
  bool matches(const dxil::HashValue &hash) const { return shaderModule.ident.shaderHash == hash; }

#if _TARGET_PC_WIN
  void onCacheInvalidated(PipelineCache &cache)
  {
    if (!pipeline)
    {
      return;
    }
    cache.addCompute(shaderModule.ident.shaderHash, pipeline.Get());
  }
#endif
};

#if D3D_HAS_RAY_TRACING
class RaytracePipeline
{
  RaytracePipelineSignature &signature;
  ComPtr<ID3D12StateObject> pipeline;
  ProgramID progID;

public:
  RaytracePipelineSignature &getSignature() const { return signature; }
  ID3D12StateObject *getHandle() { return pipeline.Get(); }
  // TODO placeholder....
  RaytracePipeline(RaytracePipelineSignature &sig) : signature{sig} {}
  void preRecovery() { pipeline.Reset(); }
};
#endif

struct GraphicsPipelinePreloadInfo
{
  BasePipelineIdentifier base;
  DynamicArray<GraphicsPipelineVariantState> variants;
};

struct MeshPipelinePreloadInfo
{
  BasePipelineIdentifier base;
  DynamicArray<MeshPipelineVariantState> variants;
};

struct ComputePipelinePreloadInfo
{
  ComputePipelineIdentifier base;
};

void to_debuglog(const dxil::ShaderDeviceRequirement &req);

#if _TARGET_PC_WIN
class ShaderDeviceRequirementChecker
{
  dxil::ShaderDeviceRequirement supportedRequirement;

public:
  void init(ID3D12Device *device);
  bool isCompatibleTo(const dxil::ShaderDeviceRequirement &requirement) const
  {
    if (!is_compatible(supportedRequirement, requirement))
    {
      logdbg("DX12: ShaderDeviceRequirementChecker::isCompatibleTo failed, requirement:");
      to_debuglog(requirement);
      return false;
    }
    return true;
  }
};
#else
class ShaderDeviceRequirementChecker
{
public:
  void init(ID3D12Device *) {}
  // On consoles shaders are always compatible, otherwise they would fail to compile
  bool isCompatibleTo(const dxil::ShaderDeviceRequirement &) const { return true; }
};
#endif

class PipelineManager : public backend::ShaderModuleManager,
                        protected backend::InputLayoutManager,
                        public backend::StaticRenderStateManager,
                        protected ShaderDeviceRequirementChecker
{
  dag::Vector<eastl::unique_ptr<BasePipeline>> graphicsPipelines[max_scripted_shaders_bin_groups];
  dag::Vector<eastl::unique_ptr<ComputePipeline>> computePipelines[max_scripted_shaders_bin_groups];
  dag::Vector<eastl::unique_ptr<GraphicsPipelineSignature>> graphicsSignatures;
#if !_TARGET_XBOXONE
  dag::Vector<eastl::unique_ptr<GraphicsPipelineSignature>> graphicsMeshSignatures;
#endif
  dag::Vector<eastl::unique_ptr<ComputePipelineSignature>> computeSignatures;
#if D3D_HAS_RAY_TRACING
  dag::Vector<eastl::unique_ptr<RaytracePipeline>> raytracePipelines;
  dag::Vector<eastl::unique_ptr<RaytracePipelineSignature>> raytraceSignatures;
#endif
  struct PrepedForDelete
  {
    ProgramID program;
    GraphicsProgramID graphicsProgram;
    eastl::unique_ptr<BasePipeline> graphics;
    eastl::unique_ptr<ComputePipeline> compute;
#if D3D_HAS_RAY_TRACING
    eastl::unique_ptr<RaytracePipeline> raytrace;
#endif
  };
  struct FormatPipeline
  {
    ComPtr<ID3D12PipelineState> pipeline;
    DXGI_FORMAT outFormat;
  };
  dag::Vector<PrepedForDelete> prepedForDeletion;
  // blit signature is special
  ComPtr<ID3D12RootSignature> blitSignature;
  ComPtr<ID3D12RootSignature> clearSignature;
  dag::Vector<FormatPipeline> blitPipelines;
  dag::Vector<FormatPipeline> clearPipelines;
  PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature = nullptr;
  D3D_ROOT_SIGNATURE_VERSION signatureVersion = D3D_ROOT_SIGNATURE_VERSION_1;
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  bool rootSignaturesUsesCBVDescriptorRanges = false;
#endif
  std::atomic<uint32_t> compilePipelineSetQueueLength{0};

  void setCompilePipelineSetQueueLength();
  void createBlitSignature(ID3D12Device *device);
  void createClearSignature(ID3D12Device *device);
  ID3D12PipelineState *createBlitPipeline(ID3D12Device2 *device, DXGI_FORMAT out_fmt, bool give_name);
  ID3D12PipelineState *createClearPipeline(ID3D12Device2 *device, DXGI_FORMAT out_fmt, bool give_name);
  GraphicsPipelineSignature *getGraphicsPipelineSignature(ID3D12Device *device, PipelineCache &cache, bool has_vertex_input,
    bool has_acceleration_structure, const dxil::ShaderResourceUsageTable &vs_header, const dxil::ShaderResourceUsageTable &ps_header,
    const dxil::ShaderResourceUsageTable *gs_header, const dxil::ShaderResourceUsageTable *hs_header,
    const dxil::ShaderResourceUsageTable *ds_header);
#if !_TARGET_XBOXONE
  GraphicsPipelineSignature *getGraphicsMeshPipelineSignature(ID3D12Device *device, PipelineCache &cache,
    bool has_acceleration_structure, const dxil::ShaderResourceUsageTable &ms_header, const dxil::ShaderResourceUsageTable &ps_header,
    const dxil::ShaderResourceUsageTable *as_header);
#endif
  ComputePipelineSignature *getComputePipelineSignature(ID3D12Device *device, PipelineCache &cache, bool has_acceleration_structure,
    const dxil::ShaderResourceUsageTable &cs_header);

public:
  PipelineManager() = default;
  ~PipelineManager() = default;
  PipelineManager(const PipelineManager &) = delete;
  PipelineManager &operator=(const PipelineManager &) = delete;
  PipelineManager(PipelineManager &&) = delete;
  PipelineManager &operator=(PipelineManager &&) = delete;

  struct SetupParameters
  {
    ID3D12Device *device;
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serializeRootSignature;
    D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion;
    PipelineCache *pipelineCache;
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
    bool rootSignaturesUsesCBVDescriptorRanges;
#endif
  };

  void init(const SetupParameters &params);

  ComputePipeline *getCompute(ProgramID program);
  BasePipeline *getGraphics(GraphicsProgramID program);
  void addCompute(ID3D12Device2 *device, PipelineCache &cache, ProgramID id, ComputeShaderModule shader,
    RecoverablePipelineCompileBehavior on_error, bool give_name, CSPreloaded preloaded);

  void addGraphics(ID3D12Device2 *device, PipelineCache &cache, FramebufferLayoutManager &fbs, GraphicsProgramID program, ShaderID vs,
    ShaderID ps, RecoverablePipelineCompileBehavior on_error, bool give_name);

  eastl::unique_ptr<BasePipeline> createGraphics(ID3D12Device2 *device, PipelineCache &cache, FramebufferLayoutManager &fbs,
    backend::VertexShaderModuleRefStore vertexShader, backend::PixelShaderModuleRefStore pixelShader,
    RecoverablePipelineCompileBehavior on_error, bool give_name);

  void unloadAll();
  // remove of a program is a 2 step process, first prepareForRemove needs to be called, this
  // move the program into a free queue and marks the program slot as free, so that the frontend
  // can reuse the id as expected.
  // When the frame finishes where the deletion was requested then the program is freed, this
  // is needed to ensure that the gpu does not try to execute programs that are dead already.
  void removeProgram(ProgramID program);
  void removeProgram(GraphicsProgramID program);
  void prepareForRemove(ProgramID program);
  void prepareForRemove(GraphicsProgramID program);

#if D3D_HAS_RAY_TRACING
  RaytracePipeline &getRaytraceProgram(ProgramID program);
  void copyRaytraceShaderGroupHandlesToMemory(ProgramID prog, uint32_t first_group, uint32_t group_count, uint32_t size, void *ptr);
  void addRaytrace(ID3D12Device *device, PipelineCache &cache, ProgramID id, uint32_t max_recursion, uint32_t shader_count,
    const RaytraceShaderModule *shaders, uint32_t group_count, const RaytraceShaderGroup *groups);
#endif

  ID3D12RootSignature *getBlitSignature() { return blitSignature.Get(); }
  ID3D12RootSignature *getClearSignature() { return clearSignature.Get(); }
  ID3D12PipelineState *getBlitPipeline(ID3D12Device2 *device, DXGI_FORMAT out_fmt, bool give_name)
  {
    auto ref =
      eastl::find_if(begin(blitPipelines), end(blitPipelines), [=](const FormatPipeline &bp) { return bp.outFormat == out_fmt; });
    if (ref == end(blitPipelines))
    {
      return createBlitPipeline(device, out_fmt, give_name);
    }
    return ref->pipeline.Get();
  }

  ID3D12PipelineState *getClearPipeline(ID3D12Device2 *device, DXGI_FORMAT out_fmt, bool give_name)
  {
    auto ref =
      eastl::find_if(begin(clearPipelines), end(clearPipelines), [=](const FormatPipeline &cp) { return cp.outFormat == out_fmt; });
    if (ref == end(clearPipelines))
      return createClearPipeline(device, out_fmt, give_name);

    return ref->pipeline.Get();
  }

  // tries to recover from a device remove error with a new device instance
  bool recover(ID3D12Device2 *device, PipelineCache &cache);

  void preRecovery();

  // TODO this needs to be here for now to allow access to pipelines to patch the ids.
  void registerStaticRenderState(StaticRenderStateID ident, const RenderStateSystem::StaticState &state)
  {
    if (ident.get() == staticRenderStateTable.size())
    {
      staticRenderStateTable.push_back(state);
    }
    else if (ident.get() > staticRenderStateTable.size())
    {
      DAG_FATAL("DX12: registerStaticRenderState was called with ident of %u but table has %u entries", ident.get(),
        staticRenderStateTable.size());
    }
    else if (staticRenderStateTable[ident.get()] != state)
    {
      logdbg("DX12: need to patch render state %u", ident.get());
      auto ref = eastl::find(begin(staticRenderStateTable) + ident.get() + 1, end(staticRenderStateTable), state);
      if (end(staticRenderStateTable) == ref)
      {
        auto moveID = StaticRenderStateID::make(staticRenderStateTable.size());
        logdbg("DX12: moving existing render state to %u", moveID.get());
        // need to copy first, as push_back frees memory before creating the new element, which results in use after free
        auto copy = staticRenderStateTable[ident.get()];
        staticRenderStateTable.push_back(copy);
        staticRenderStateTable[ident.get()] = state;
        for (auto &group : graphicsPipelines)
        {
          for (auto &pipeline : group)
          {
            if (pipeline)
            {
              pipeline->moveStaticRenderStateID(ident, moveID);
            }
          }
        }
        for (auto &pipeline : preloadedGraphicsPipelines)
        {
          pipeline.pipeline->moveStaticRenderStateID(ident, moveID);
        }
      }
      else
      {
        auto swapID = StaticRenderStateID::make(ref - begin(staticRenderStateTable));
        logdbg("DX12: swapping existing render state with %u", swapID.get());
        staticRenderStateTable[swapID.get()] = staticRenderStateTable[ident.get()];
        staticRenderStateTable[ident.get()] = state;
        for (auto &group : graphicsPipelines)
        {
          for (auto &pipeline : group)
          {
            if (pipeline)
            {
              pipeline->swapStaticRenderStateID(ident, swapID);
            }
          }
        }
        for (auto &pipeline : preloadedGraphicsPipelines)
        {
          pipeline.pipeline->swapStaticRenderStateID(ident, swapID);
        }
      }
    }
  }

#if _TARGET_PC_WIN
  void onCacheInvalidated(PipelineCache &cache, backend::InputLayoutManager &input_layouts,
    backend::StaticRenderStateManager &static_states, FramebufferLayoutManager &framebuffer_layouts)
  {
    // Recreating root signatures would require a mid sized refactor to avoid a chunk of repeated code
    // and its a "minor optimization" (a few 100 ups is saved each) cache and we can live without
    // rebuilding it now. It will on later runs be rebuild. So we don't do it for now.
#if 0
    for (auto &s : computeSignatures)
    {
      // cache.addComputeSignature(s->def, <generate bytecode from s->def>);
    }

    for (auto &s : graphicsMeshSignatures)
    {
      // cache.addGraphicsMeshSignature(s->def, <generate bytecode from s->def>);
    }

    for (auto &s: graphicsSignatures)
    {
      // cache.addGraphicsSignature(s->def, <generate bytecode from s->def>);
    }
#endif
    for (auto &g : graphicsPipelines)
    {
      for (auto &p : g)
      {
        if (!p)
        {
          continue;
        }
        p->onCacheInvalidated(cache, input_layouts, static_states, framebuffer_layouts);
      }
    }

    for (auto &g : computePipelines)
    {
      for (auto &p : g)
      {
        if (!p)
        {
          continue;
        }
        p->onCacheInvalidated(cache);
      }
    }
  }
#else
  void onCacheInvalidated(PipelineCache &, backend::InputLayoutManager &, backend::StaticRenderStateManager &,
    FramebufferLayoutManager &)
  {}
#endif

  using backend::InputLayoutManager::getInputLayout;
  using backend::InputLayoutManager::registerInputLayout;
  using backend::InputLayoutManager::remapInputLayout;

  struct PreloadedGraphicsPipeline
  {
    ShaderID vsID;
    ShaderID psID;
    eastl::unique_ptr<BasePipeline> pipeline;
  };
  dag::Vector<PreloadedGraphicsPipeline> preloadedGraphicsPipelines;

  struct ShaderCompressionIndex
  {
    uint16_t compressionGroup;
    uint16_t compressionIndex;
  };
  dag::Vector<ShaderCompressionIndex> computeProgramIndexToDumpShaderIndex[max_scripted_shaders_bin_groups];
  dag::Vector<eastl::variant<eastl::monostate, ShaderID, ProgramID>> pixelShaderComputeProgramIDMap[max_scripted_shaders_bin_groups];
  struct ScriptedShaderBinDumpInspector
  {
    uint32_t group = 0;
    ShaderID nullPixelShader;
    PipelineManager *target = nullptr;
    ScriptedShadersBinDumpOwner *owner = nullptr;
    uint32_t pixelShaderCount = 0;
    uint32_t computeShaderCount = 0;
    ID3D12Device2 *device = nullptr;
    PipelineCache *pipelineCache = nullptr;
    FramebufferLayoutManager *frameBufferLayoutManager = nullptr;

    void vertexShaderCount(uint32_t count)
    {
      auto dump = owner->getDump();
      target->reserveVertexShaderRange(group, count);
      for (uint32_t i = 0; i < count; ++i)
      {
        auto &map = dump->shGroupsMapping[i];
        auto &hash = dump->shaderHashes[i];
        target->setVertexShaderCompressionGroup(group, i, hash, map.groupId, map.indexInGroup);
      }
    }
    void pixelOrComputeShaderCount(uint32_t count) { target->pixelShaderComputeProgramIDMap[group].resize(count); };

    void preloadGraphicsPipeline(ShaderID vsID, ShaderID psID)
    {
      PipelineCache::BasePipelineIdentifier cacheIdent = {};
      cacheIdent.vs = target->getVertexShaderHash(vsID);
      cacheIdent.ps = target->getPixelShaderHash(psID);

      if (!pipelineCache->containsGraphicsPipeline(cacheIdent))
      {
        return;
      }

      auto preloadedPipelineIt = eastl::find_if(target->preloadedGraphicsPipelines.begin(), target->preloadedGraphicsPipelines.end(),
        [vsID, psID](const auto &pp) { return (pp.vsID == vsID) && (pp.psID == psID); });

      if (preloadedPipelineIt != target->preloadedGraphicsPipelines.end())
      {
        return;
      }

      auto pipeline = target->createGraphics(device, *pipelineCache, *frameBufferLayoutManager, target->getVertexShader(vsID),
        target->getPixelShader(psID), RecoverablePipelineCompileBehavior::REPORT_ERROR, true);

      if (pipeline)
      {
        auto &preloadedPipeline = target->preloadedGraphicsPipelines.emplace_back();

        preloadedPipeline.vsID = vsID;
        preloadedPipeline.psID = psID;
        preloadedPipeline.pipeline = eastl::move(pipeline);
      }
    }

    void addGraphicsProgram(uint16_t vs_index, uint16_t ps_index)
    {
      ShaderID psID;
      if (auto shader = eastl::get_if<ShaderID>(&target->pixelShaderComputeProgramIDMap[group][ps_index]))
      {
        psID = *shader;
      }
      else
      {
        auto dump = owner->getDump();
        auto pixelShaderInDumpIndex = dump->vprCount + ps_index;
        auto &map = dump->shGroupsMapping[pixelShaderInDumpIndex];
        auto &hash = dump->shaderHashes[pixelShaderInDumpIndex];
        auto id = pixelShaderCount++;
        target->setPixelShaderCompressionGroup(group, id, hash, map.groupId, map.indexInGroup);
        psID = ShaderID::make(group, id);
        target->pixelShaderComputeProgramIDMap[group][ps_index] = psID;
      }
      auto vsID = ShaderID::make(group, vs_index);
      preloadGraphicsPipeline(vsID, psID);
    }
    void addGraphicsProgramWithNullPixelShader(uint16_t vs_index)
    {
      auto vsID = ShaderID::make(group, vs_index);
      auto psID = nullPixelShader;
      preloadGraphicsPipeline(vsID, psID);
    }
    void addComputeProgram(uint32_t shader_index)
    {
      if (eastl::holds_alternative<ProgramID>(target->pixelShaderComputeProgramIDMap[group][shader_index]))
      {
        return;
      }
      auto id = ProgramID::asComputeProgram(group, target->computeProgramIndexToDumpShaderIndex[group].size());
      target->pixelShaderComputeProgramIDMap[group][shader_index] = id;
      auto dump = owner->getDump();
      auto &map = dump->shGroupsMapping[dump->vprCount + shader_index];
      ShaderCompressionIndex compressionIndex;
      compressionIndex.compressionGroup = map.groupId;
      compressionIndex.compressionIndex = map.indexInGroup;
      target->computeProgramIndexToDumpShaderIndex[group].push_back(compressionIndex);
      // TODO could be used to create a pipeline object with a not yet loaded pipeline
      //      but this needs more work as the references to shader modules need to support
      //      the handling of not loaded blobs
    }
  };
  void addShaderGroup(ID3D12Device2 *device, PipelineCache *pipelineCache, FramebufferLayoutManager *fbs, uint32_t group,
    ScriptedShadersBinDumpOwner *dump, ShaderID null_pixel_shader, eastl::string_view name)
  {
    auto v2 = dump->getDumpV2();
    bool cacheIsOk = pipelineCache->onBindumpLoad(device, {v2->shaderHashes.begin(), v2->shaderHashes.end()});
    if (!cacheIsOk)
    {
      onCacheInvalidated(*pipelineCache, *this, *this, *fbs);
    }
    ScriptedShaderBinDumpInspector inspector;
    inspector.group = group;
    inspector.nullPixelShader = null_pixel_shader;
    inspector.target = this;
    inspector.owner = dump;
    inspector.device = device;
    inspector.pipelineCache = pipelineCache;
    inspector.frameBufferLayoutManager = fbs;
    setDumpOfGroup(group, dump, name);
    inspect_scripted_shader_bin_dump<true>(dump, inspector);
  }
#if _TARGET_PC_WIN
  void writeBlkCache(FramebufferLayoutManager &fblm, uint32_t group);
#endif
  template <typename ProgramRecorder, typename GraphicsProgramRecorder>
  void removeShaderGroup(FramebufferLayoutManager &fblm, uint32_t group, ProgramRecorder &&prog, GraphicsProgramRecorder &&g_prog)
  {
    if (needToUpdateCache)
    {
#if _TARGET_PC_WIN
      writeBlkCache(fblm, group);
#else
      G_UNUSED(fblm);
#endif
      needToUpdateCache = false;
    }

    for (uint32_t i = 0; i < graphicsPipelines[group].size(); ++i)
    {
      auto program = GraphicsProgramID::make(group, i);
      g_prog(program);
      PrepedForDelete info;
      info.program = ProgramID::Null();
      info.graphicsProgram = program;
      info.graphics = eastl::move(graphicsPipelines[group][i]);
      prepedForDeletion.push_back(eastl::move(info));
    }
    for (uint32_t i = 0; i < computePipelines[group].size(); ++i)
    {
      auto program = ProgramID::asComputeProgram(group, i);
      prog(program);
      PrepedForDelete info;
      info.program = program;
      info.graphicsProgram = GraphicsProgramID::Null();
      info.compute = eastl::move(computePipelines[group][i]);
      prepedForDeletion.push_back(eastl::move(info));
    }
    resetDumpOfGroup(group);
    if (preloadedGraphicsPipelines.size() != 0)
    {
      logwarn("Graphics pipelines are preloaded, but never used");
    }
    preloadedGraphicsPipelines.clear();
    computeProgramIndexToDumpShaderIndex[group].clear();
    pixelShaderComputeProgramIDMap[group].clear();
  }
  void loadComputeShaderFromDump(ID3D12Device2 *device, PipelineCache &cache, ProgramID program,
    RecoverablePipelineCompileBehavior on_error, bool give_name, CSPreloaded preloaded)
  {
    auto shaderCompressionIndex = computeProgramIndexToDumpShaderIndex[program.getGroup()][program.getIndex()];
    auto byteCode =
      getShaderByteCode(program.getGroup(), shaderCompressionIndex.compressionGroup, shaderCompressionIndex.compressionIndex);
    if (byteCode.empty())
      return;

    auto basicModule = decode_shader_layout<ComputeShaderModule>(byteCode.data());
    if (!basicModule)
    {
      basicModule = decode_shader_binary(byteCode.data(), ~uint32_t(0));
      if (!basicModule)
      {
        return;
      }
    }

    addCompute(device, cache, program, eastl::move(basicModule), on_error, give_name, preloaded);
  }

  BasePipeline *findLoadedPipeline(const backend::VertexShaderModuleRefStore &vs, const backend::PixelShaderModuleRefStore &ps)
  {
    for (auto &group : graphicsPipelines)
    {
      for (auto &pipeline : group)
      {
        if (!pipeline)
        {
          continue;
        }
        if (pipeline->matches(vs, ps))
        {
          return pipeline.get();
        }
      }
    }
    return nullptr;
  }

  bool isCompatible(backend::VertexShaderModuleRefStore vs) { return isCompatibleTo(vs.header.header.deviceRequirement); }
  bool isCompatible(backend::PixelShaderModuleRefStore ps) { return isCompatibleTo(ps.header.header.deviceRequirement); }
  bool isCompatible(const ComputeShaderModule &cs) { return isCompatibleTo(cs.header.deviceRequirement); }

  class ShaderClassFilter
  {
    enum class Mode
    {
      AllowAll,
      DenyAll,
      LookUp
    };
    dag::Vector<eastl::string> allowedClasses;
    Mode mode = Mode::DenyAll;

  public:
    void denyAll()
    {
      mode = Mode::DenyAll;
      allowedClasses.clear();
    }
    void allowAll()
    {
      mode = Mode::AllowAll;
      allowedClasses.clear();
    }
    void allowClass(const eastl::string &name)
    {
      mode = Mode::LookUp;
      allowedClasses.push_back(name);
    }
    bool allowsAny() const { return mode != Mode::DenyAll; }
    bool isAllowed(const eastl::string &name) const
    {
      // most common case, blk cache matches the shader dump
      if (Mode::AllowAll == mode)
      {
        return true;
      }
      if (Mode::DenyAll == mode)
      {
        return false;
      }
      return end(allowedClasses) != eastl::find(begin(allowedClasses), end(allowedClasses), name);
    }

    // Assumes that there are no duplicates in 'signature'.
    static ShaderClassFilter make(ScriptedShadersBinDump &dump, const DynamicArray<cacheBlk::SignatureHash> &signature)
    {
      ShaderClassFilter result;
      logdbg("DX12: Generating shader class filter...");
      if (signature.size() == dump.classes.size())
      {
        logdbg("DX12: ...shader class count match, verifying matches...");
        // if all match we can use optimized allowAll mode
        result.allowAll();
        for (auto &s : signature)
        {
          auto shader = dump.findShaderClass(s.name.c_str());
          if (!shader)
          {
            logdbg("DX12: ...a shader class is missing in scripted shader dump, falling back to individual matches...");
            result.denyAll();
            break;
          }
          auto currentHash = calculate_hash(dump, *shader);
          auto cacheHash = dxil::HashValue::fromString(s.hash.c_str(), s.hash.length());
          if (currentHash != cacheHash)
          {
            logdbg("DX12: ...a shader class is not matching, falling back to individual matches...");
            result.denyAll();
            break;
          }
        }
      }

      // allowsAny will return false if either the signature count did not match with shader count or any of them did not match
      if (!result.allowsAny())
      {
        logdbg("DX12: ...scanning for individual shader class matches...");
        for (auto &s : signature)
        {
          auto shader = dump.findShaderClass(s.name.c_str());
          if (!shader)
          {
            logdbg("DX12: ...shader class %s not in scripted shader dump...", s.name);
            continue;
          }
          auto currentHash = calculate_hash(dump, *shader);
          auto cacheHash = dxil::HashValue::fromString(s.hash.c_str(), s.hash.length());
          if (currentHash != cacheHash)
          {
            logdbg("DX12: ...shader class %s not matching hash...", s.name);
            continue;
          }
          logdbg("DX12: ...%s matched...", s.name);
          result.allowClass(s.name);
        }
        if (!result.allowsAny())
        {
          logdbg("DX12: ...no matches found...");
        }
      }

      return result;
    }
  };

  uint32_t getCompilePipelineSetQueueLength() const { return compilePipelineSetQueueLength.load(std::memory_order_acquire); }

  struct PipelineSetCompilation
  {
    DynamicArray<InputLayoutIDWithHash> inputLayouts;
    DynamicArray<StaticRenderStateIDWithHash> staticRenderStates;
    DynamicArray<FramebufferLayoutWithHash> framebufferLayouts;
    DynamicArray<GraphicsPipelinePreloadInfo> graphicsPipelines;
    DynamicArray<MeshPipelinePreloadInfo> meshPipelines;
    DynamicArray<ComputePipelinePreloadInfo> computePipelines;
    uint32_t graphicsPipelineSetCompilationIdx = 0;
    uint32_t meshPipelineSetCompilationIdx = 0;
    uint32_t computePipelineSetCompilationIdx = 0;
  };
  eastl::optional<PipelineSetCompilation> pipelineSetCompilation;

  void storePipelineSetForCompilation(DynamicArray<InputLayoutIDWithHash> &&input_layouts,
    DynamicArray<StaticRenderStateIDWithHash> &&static_render_states, DynamicArray<FramebufferLayoutWithHash> &&framebuffer_layouts,
    DynamicArray<GraphicsPipelinePreloadInfo> &&graphics_pipelines, DynamicArray<MeshPipelinePreloadInfo> &&mesh_pipelines,
    DynamicArray<ComputePipelinePreloadInfo> &&compute_pipelines)
  {
    pipelineSetCompilation.emplace();
    pipelineSetCompilation->inputLayouts = eastl::move(input_layouts);
    pipelineSetCompilation->staticRenderStates = eastl::move(static_render_states);
    pipelineSetCompilation->framebufferLayouts = eastl::move(framebuffer_layouts);
    pipelineSetCompilation->graphicsPipelines = eastl::move(graphics_pipelines);
    pipelineSetCompilation->meshPipelines = eastl::move(mesh_pipelines);
    pipelineSetCompilation->computePipelines = eastl::move(compute_pipelines);
    setCompilePipelineSetQueueLength();
  }

  struct PipelineSetCompilation2
  {
    DynamicArray<InputLayoutIDWithHash> inputLayouts;
    DynamicArray<StaticRenderStateIDWithHash> staticRenderStates;
    DynamicArray<FramebufferLayoutWithHash> framebufferLayouts;
    DynamicArray<cacheBlk::SignatureEntry> scriptedShaderDumpSignature;
    DynamicArray<cacheBlk::ComputeClassUse> computePipelines;
    DynamicArray<cacheBlk::GraphicsVariantGroup> graphicsPipelines;
    DynamicArray<cacheBlk::GraphicsVariantGroup> graphicsPipelinesWithNullOverrides;
    eastl::optional<ShaderClassFilter> filter;
    bool foundAnyMatch = false;
    uint32_t groupIndex = 0;
    ShaderID nullPixelShader;
    bool giveName = false;
    uint32_t computePipelineSetCompilationIdx = 0;
    uint32_t graphicsPipelineSetCompilationIdx = 0;
    uint32_t graphicsPipelinesWithNullOverridesSetCompilationIdx = 0;
  };
  eastl::optional<PipelineSetCompilation2> pipelineSetCompilation2;

  template <typename CancellationPredicate>
  void continuePipelineSetCompilation(ID3D12Device2 *device, PipelineCache &pipeline_cache, FramebufferLayoutManager &fbs,
    const CancellationPredicate &cancellation_predicate)
  {
    if (!pipelineSetCompilation && !pipelineSetCompilation2)
      return;
    logdbg("DX12: Continue pipeline set compilation");
    if (pipelineSetCompilation)
    {
      const bool isGraphicsFinished = compileGraphicsPipelineSet(device, pipeline_cache, fbs, cancellation_predicate);
      const bool isMeshFinished = compileMeshPipelineSet(device, pipeline_cache, fbs, cancellation_predicate);
      const bool isComputeFinished = compileComputePipelineSet(device, pipeline_cache, cancellation_predicate);
      const bool isFinished = isGraphicsFinished && isMeshFinished && isComputeFinished;
      if (isFinished)
        onPipelineSetCompilationFinish();
    }
    if (pipelineSetCompilation2)
    {
      const bool isFinished = compilePipelineSet2(device, pipeline_cache, fbs, cancellation_predicate);
      if (isFinished)
        onPipelineSetCompilationFinish();
    }
    setCompilePipelineSetQueueLength();
    logdbg("DX12: Continue pipeline set compilation finished");
  }

  void storePipelineSetForCompilation2(DynamicArray<InputLayoutIDWithHash> &&input_layouts,
    DynamicArray<StaticRenderStateIDWithHash> &&static_render_states, DynamicArray<FramebufferLayoutWithHash> &&framebuffer_layouts,
    DynamicArray<cacheBlk::SignatureEntry> &&scripted_shader_dump_signature,
    DynamicArray<cacheBlk::ComputeClassUse> &&compute_pipelines, DynamicArray<cacheBlk::GraphicsVariantGroup> &&graphics_pipelines,
    DynamicArray<cacheBlk::GraphicsVariantGroup> &&graphics_with_null_override_pipelines, ShaderID null_pixel_shader, bool give_name)
  {
    pipelineSetCompilation2.emplace();
    pipelineSetCompilation2->inputLayouts = eastl::move(input_layouts);
    pipelineSetCompilation2->staticRenderStates = eastl::move(static_render_states);
    pipelineSetCompilation2->framebufferLayouts = eastl::move(framebuffer_layouts);
    pipelineSetCompilation2->scriptedShaderDumpSignature = eastl::move(scripted_shader_dump_signature);
    pipelineSetCompilation2->computePipelines = eastl::move(compute_pipelines);
    pipelineSetCompilation2->graphicsPipelines = eastl::move(graphics_pipelines);
    pipelineSetCompilation2->graphicsPipelinesWithNullOverrides = eastl::move(graphics_with_null_override_pipelines);
    pipelineSetCompilation2->nullPixelShader = null_pixel_shader;
    pipelineSetCompilation2->giveName = give_name;
    setCompilePipelineSetQueueLength();
  }

  void compileGraphicsPipeline(ID3D12Device2 *device, PipelineCache &pipeline_cache, FramebufferLayoutManager &fbs,
    const GraphicsPipelinePreloadInfo &pipeline)
  {
    logdbg("DX12: precomiling graphics pipeline...");

    char hashString[1 + 2 * sizeof(dxil::HashValue)];
    pipeline.base.vs.convertToString(hashString, countof(hashString));
    logdbg("DX12: Looking for VS %s...", hashString);
    auto vsID = findVertexShader(pipeline.base.vs);
    if (ShaderID::Null() == vsID)
    {
      logdbg("DX12: ...shader not found");
      return;
    }
    pipeline.base.ps.convertToString(hashString, countof(hashString));
    logdbg("DX12: Looking for PS %s...", hashString);
    auto psID = findPixelShader(pipeline.base.ps);
    if (ShaderID::Null() == psID)
    {
      logdbg("DX12: ...shader not found");
      return;
    }
    logdbg("DX12: looking for existing pipeline...");
    BasePipeline *pipelineBase = nullptr;
    for (auto &preloadInfo : preloadedGraphicsPipelines)
    {
      if ((vsID != preloadInfo.vsID) || (psID != preloadInfo.psID))
      {
        continue;
      }
      logdbg("DX12: ...found in preloadedGraphicsPipelines");
      pipelineBase = preloadInfo.pipeline.get();
      break;
    }
    if (!pipelineBase)
    {
      auto vs = getVertexShader(vsID);
      auto ps = getPixelShader(psID);
      if (!isCompatible(vs) || !isCompatible(ps))
      {
        logdbg("DX12: ...a shader (%u) requires features that the device can not provide, ignoring...",
          (isCompatible(vs) ? 1 : 0) | (isCompatible(ps) ? 2 : 0));
        return;
      }
      pipelineBase = findLoadedPipeline(vs, ps);

      if (!pipelineBase)
      {
        auto &preloadedPipeline = preloadedGraphicsPipelines.emplace_back();
        preloadedPipeline.vsID = vsID;
        preloadedPipeline.psID = psID;
        preloadedPipeline.pipeline =
          createGraphics(device, pipeline_cache, fbs, vs, ps, RecoverablePipelineCompileBehavior::REPORT_ERROR, true);
        pipelineBase = preloadedPipeline.pipeline.get();
        logdbg("DX12: ...none found, creating new...");
      }
      else
      {
        logdbg("DX12: ...found loaded pipeline");
      }
    }
    if (!pipelineBase)
    {
      logdbg("DX12: ...failed");
      return;
    }
    logdbg("DX12: ...loading variants...");
    for (auto &variant : pipeline.variants)
    {
      if ((variant.inputLayoutIndex >= pipelineSetCompilation->inputLayouts.size()) ||
          (variant.framebufferLayoutIndex >= pipelineSetCompilation->framebufferLayouts.size()) ||
          (variant.staticRenderStateIndex >= pipelineSetCompilation->staticRenderStates.size()))
      {
        logdbg("DX12: ...invalid state index, ignoring pipeline variant...");
        continue;
      }
      if (StaticRenderStateID::Null() == pipelineSetCompilation->staticRenderStates[variant.staticRenderStateIndex].id)
      {
        logdbg("DX12: ...unsupported render state, ignoring pipeline variant...");
        continue;
      }
      logdbg("DX12: ...resolving input layout and output layout...");
      auto inputLayout = InputLayoutManager::remapInputLayout(pipelineSetCompilation->inputLayouts[variant.inputLayoutIndex].id,
        pipelineBase->getVertexShaderInputMask());
      logdbg("DX12: ...resolving frame buffer layout...");
      auto fbLayout = fbs.getLayoutID(pipelineSetCompilation->framebufferLayouts[variant.framebufferLayoutIndex].layout);
      logdbg("DX12: ...compiling variant...");
      pipelineBase->getVariantFromConfiguration(inputLayout,
        pipelineSetCompilation->staticRenderStates[variant.staticRenderStateIndex].id, fbLayout, variant.topology,
        0 != variant.isWireFrame);
    }
    logdbg("DX12: ...completed");
  }

  template <typename CancellationPredicate>
  bool compileGraphicsPipelineSet(ID3D12Device2 *device, PipelineCache &pipeline_cache, FramebufferLayoutManager &fbs,
    const CancellationPredicate &cancellation_predicate)
  {
    const auto size = pipelineSetCompilation->graphicsPipelines.size();
    for (auto &idx = pipelineSetCompilation->graphicsPipelineSetCompilationIdx; idx < size; idx++)
    {
      if (cancellation_predicate())
      {
        logdbg("DX12: Graphics pipelines compilation interrupted (idx: %d/%d)", idx, size);
        return false;
      }
      auto &pipeline = pipelineSetCompilation->graphicsPipelines[idx];
      compileGraphicsPipeline(device, pipeline_cache, fbs, pipeline);
    }
    return true;
  }

  void compileMeshPipeline(ID3D12Device2 *device, PipelineCache &pipeline_cache, FramebufferLayoutManager &fbs,
    MeshPipelinePreloadInfo &pipeline)
  {
    logdbg("DX12: precomiling mesh pipeline...");

    char hashString[1 + 2 * sizeof(dxil::HashValue)];
    pipeline.base.vs.convertToString(hashString, countof(hashString));
    logdbg("DX12: Looking for VS %s...", hashString);
    auto vsID = findVertexShader(pipeline.base.vs);
    if (ShaderID::Null() == vsID)
    {
      logdbg("DX12: ...shader not found");
      return;
    }
    pipeline.base.ps.convertToString(hashString, countof(hashString));
    logdbg("DX12: Looking for PS %s...", hashString);
    auto psID = findPixelShader(pipeline.base.ps);
    if (ShaderID::Null() == psID)
    {
      logdbg("DX12: ...shader not found");
      return;
    }
    logdbg("DX12: looking for existing pipeline...");
    BasePipeline *pipelineBase = nullptr;
    for (auto &preloadInfo : preloadedGraphicsPipelines)
    {
      if ((vsID != preloadInfo.vsID) || (psID != preloadInfo.psID))
      {
        continue;
      }
      logdbg("DX12: ...found in preloadedGraphicsPipelines");
      pipelineBase = preloadInfo.pipeline.get();
      break;
    }
    if (!pipelineBase)
    {
      auto vs = getVertexShader(vsID);
      auto ps = getPixelShader(psID);
      if (!isCompatible(vs) || !isCompatible(ps))
      {
        logdbg("DX12: ...a shader (%u) requires features that the device can not provide, ignoring...",
          (isCompatible(vs) ? 1 : 0) | (isCompatible(ps) ? 2 : 0));
        return;
      }
      pipelineBase = findLoadedPipeline(vs, ps);

      if (!pipelineBase)
      {
        auto &preloadedPipeline = preloadedGraphicsPipelines.emplace_back();
        preloadedPipeline.vsID = vsID;
        preloadedPipeline.psID = psID;
        preloadedPipeline.pipeline =
          createGraphics(device, pipeline_cache, fbs, vs, ps, RecoverablePipelineCompileBehavior::REPORT_ERROR, true);
        pipelineBase = preloadedPipeline.pipeline.get();
        logdbg("DX12: ...none found, creating new...");
      }
      else
      {
        logdbg("DX12: ...found loaded pipeline");
      }
    }
    if (!pipelineBase)
    {
      logdbg("DX12: ...failed");
      return;
    }
    logdbg("DX12: ...loading variants...");
    for (auto &variant : pipeline.variants)
    {
      if ((variant.framebufferLayoutIndex >= pipelineSetCompilation->framebufferLayouts.size()) ||
          (variant.staticRenderStateIndex >= pipelineSetCompilation->staticRenderStates.size()))
      {
        logdbg("DX12: ...invalid state index, ignoring pipeline variant...");
        continue;
      }
      if (StaticRenderStateID::Null() == pipelineSetCompilation->staticRenderStates[variant.staticRenderStateIndex].id)
      {
        logdbg("DX12: ...unsupported render state, ignoring pipeline variant...");
        continue;
      }
      logdbg("DX12: ...resolving frame buffer layout...");
      auto fbLayout = fbs.getLayoutID(pipelineSetCompilation->framebufferLayouts[variant.framebufferLayoutIndex].layout);
      logdbg("DX12: ...compiling variant...");
      pipelineBase->getMeshVariantFromConfiguration(pipelineSetCompilation->staticRenderStates[variant.staticRenderStateIndex].id,
        fbLayout, 0 != variant.isWireFrame);
    }
    logdbg("DX12: ...completed");
  }

  template <typename CancellationPredicate>
  bool compileMeshPipelineSet(ID3D12Device2 *device, PipelineCache &pipeline_cache, FramebufferLayoutManager &fbs,
    const CancellationPredicate &cancellation_predicate)
  {
    const auto size = pipelineSetCompilation->meshPipelines.size();
    for (auto &idx = pipelineSetCompilation->meshPipelineSetCompilationIdx; idx < size; idx++)
    {
      if (cancellation_predicate())
      {
        logdbg("DX12: Mesh pipelines compilation interrupted (idx: %d/%d)", idx, size);
        return false;
      }
      auto &pipeline = pipelineSetCompilation->meshPipelines[idx];
      compileMeshPipeline(device, pipeline_cache, fbs, pipeline);
    }
    return true;
  }

  void compileComputePipeline(ID3D12Device2 *device, PipelineCache &pipeline_cache, ComputePipelinePreloadInfo &pipeline)
  {
    logdbg("DX12: precomiling compute pipeline...");
    char hashString[1 + 2 * sizeof(dxil::HashValue)];
    pipeline.base.hash.convertToString(hashString, countof(hashString));
    logdbg("DX12: Looking for CS %s...", hashString);
    bool found = false;
    enumerateShaderFromHash(pipeline.base.hash, [device, &pipeline_cache, &found, this](auto gi, auto si, auto vs_count) {
      // has collision with a vs?!
      if (si < vs_count)
      {
        return true;
      }
      auto &mapping = pixelShaderComputeProgramIDMap[gi];
      auto shaderIndex = si - vs_count;
      // mapping table incomplete?!
      if (shaderIndex >= mapping.size())
      {
        return true;
      }
      // has collision with a ps?!
      auto program = eastl::get_if<ProgramID>(&mapping[shaderIndex]);
      if (!program)
      {
        return true;
      }
      auto progId = *program;
      auto &progGroup = computePipelines[progId.getGroup()];
      if (progId.getIndex() < progGroup.size() && progGroup[progId.getIndex()])
      {
        logdbg("DX12: ...already loaded...");
        // already loaded, so we are done
        found = true;
        return false;
      }
      logdbg("DX12: ...loading...");
      // CSPreloaded::No as we doing the preload right now
      loadComputeShaderFromDump(device, pipeline_cache, progId, RecoverablePipelineCompileBehavior::REPORT_ERROR, true,
        CSPreloaded::No);
      found = true;
      return false;
    });
    if (found)
    {
      logdbg("DX12: ...completed");
    }
    else
    {
      logdbg("DX12: ...not found");
    }
  }

  template <typename CancellationPredicate>
  bool compileComputePipelineSet(ID3D12Device2 *device, PipelineCache &pipeline_cache,
    const CancellationPredicate &cancellation_predicate)
  {
    const auto size = pipelineSetCompilation->computePipelines.size();
    for (auto &idx = pipelineSetCompilation->computePipelineSetCompilationIdx; idx < size; idx++)
    {
      if (cancellation_predicate())
      {
        logdbg("DX12: Compute pipelines compilation interrupted (idx: %d/%d)", idx, size);
        return false;
      }
      auto &pipeline = pipelineSetCompilation->computePipelines[idx];
      compileComputePipeline(device, pipeline_cache, pipeline);
    }
    return true;
  }

  static shaderbindump::ShaderCode::ShRef evaluate_use(const ScriptedShadersBinDump &dump, const char *name, cacheBlk::UseCodes code)
  {
    auto shaderClass = dump.findShaderClass(name);
    if (!shaderClass)
    {
      logdbg("DX12: evaluate_use %p %s %u|%u, missing shader class", &dump, name, code.staticCode, code.dynamicCode);
      return {0xFFFF, 0xFFFF, 0xFFFF};
    }

    auto staticIndex = shaderClass->stVariants.findVariant(code.staticCode);
    if (staticIndex >= shaderClass->code.size())
    {
      logdbg("DX12: evaluate_use %p %s %u|%u, missing static code (%04X)", &dump, name, code.staticCode, code.dynamicCode,
        staticIndex);
      return {0xFFFF, 0xFFFF, 0xFFFF};
    }
    auto &staticVariant = shaderClass->code[staticIndex];
    auto dynamicIndex = staticVariant.dynVariants.findVariant(code.dynamicCode);
    if (dynamicIndex >= staticVariant.passes.size())
    {
      logdbg("DX12: evaluate_use %p %s %u|%u, missing dynamic code (%04X)", &dump, name, code.staticCode, code.dynamicCode,
        dynamicIndex);
      return {0xFFFF, 0xFFFF, 0xFFFF};
    }
    return *staticVariant.passes[dynamicIndex].rpass;
  }

  BasePipeline *preloadGrahpicsBasePipeline(ID3D12Device2 *device, PipelineCache &pipeline_cache, FramebufferLayoutManager &fbs,
    ShaderID vs_id, ShaderID ps_id)
  {
    BasePipeline *pipelineBase = nullptr;
    for (auto &preloadInfo : preloadedGraphicsPipelines)
    {
      if ((vs_id != preloadInfo.vsID) || (ps_id != preloadInfo.psID))
      {
        continue;
      }
      pipelineBase = preloadInfo.pipeline.get();
      break;
    }

    if (!pipelineBase)
    {
      auto vs = getVertexShader(vs_id);
      auto ps = getPixelShader(ps_id);
      if (!isCompatible(vs) || !isCompatible(ps))
      {
        logdbg("DX12: ...a shader (%u) requires features that the device can not provide, ignoring...",
          (isCompatible(vs) ? 1 : 0) | (isCompatible(ps) ? 2 : 0));
        return pipelineBase;
      }
      pipelineBase = findLoadedPipeline(vs, ps);

      if (!pipelineBase)
      {
        auto &preloadedPipeline = preloadedGraphicsPipelines.emplace_back();
        preloadedPipeline.vsID = vs_id;
        preloadedPipeline.psID = ps_id;
        preloadedPipeline.pipeline =
          createGraphics(device, pipeline_cache, fbs, vs, ps, RecoverablePipelineCompileBehavior::REPORT_ERROR, true);
        pipelineBase = preloadedPipeline.pipeline.get();
      }
    }
    return pipelineBase;
  }

  void compileGraphicsPipeline2(ID3D12Device2 *device, PipelineCache &pipeline_cache, FramebufferLayoutManager &fbs, uint32_t group,
    const ShaderClassFilter &filter, cacheBlk::SignatureMask signature_mask, ScriptedShadersBinDump *v1,
    cacheBlk::GraphicsVariantGroup &variant)
  {
    // a mask of 0 means that this variant is for any signature valid
    if (variant.validSiagnatures && 0 == (signature_mask & variant.validSiagnatures))
    {
      return;
    }
    if ((variant.staticRenderState < pipelineSetCompilation2->staticRenderStates.size()) &&
        StaticRenderStateID::Null() == pipelineSetCompilation2->staticRenderStates[variant.staticRenderState].id)
    {
      return;
    }
    if ((variant.inputLayout >= pipelineSetCompilation2->inputLayouts.size()) ||
        (variant.frameBufferLayout >= pipelineSetCompilation2->framebufferLayouts.size()))
    {
      D3D_ERROR("DX12: ...invalid state index, ignoring pipeline variant...");
      return;
    }
    for (auto &shaderClass : variant.classes)
    {
      if (!filter.isAllowed(shaderClass.name))
      {
        logdbg("DX12: ...ignoring shader class <%s>...", shaderClass.name);
        continue;
      }
      for (auto code : shaderClass.codes)
      {
        auto useRef = evaluate_use(*v1, shaderClass.name.c_str(), code);
        if (useRef.vprId == 0xFFFF)
        {
          D3D_ERROR("DX12: ...failed to evaluate use <%s> %u | %u...", shaderClass.name, code.dynamicCode, code.staticCode);
          continue;
        }

        auto vsID = ShaderID::make(group, useRef.vprId);
        ShaderID psID = pipelineSetCompilation2->nullPixelShader;
        if (useRef.fshId != 0xFFFF)
        {
          auto &mapping = pixelShaderComputeProgramIDMap[group];
          auto psIdent = eastl::get_if<ShaderID>(&mapping[useRef.fshId]);
          if (!psIdent)
          {
            continue;
          }
          psID = *psIdent;
        }
        // TODO validate

        auto base = preloadGrahpicsBasePipeline(device, pipeline_cache, fbs, vsID, psID);

        if (!base)
        {
          D3D_ERROR("DX12: ...failed to create basic pipeline for <%s> %u | %u...", shaderClass.name, code.dynamicCode,
            code.staticCode);
          continue;
        }
        auto fbLayout = fbs.getLayoutID(pipelineSetCompilation2->framebufferLayouts[variant.frameBufferLayout].layout);
        // TODO restructure to early quit on mismatch
        StaticRenderStateID staticRenderStateID = StaticRenderStateID::Null();
        if (variant.staticRenderState < pipelineSetCompilation2->staticRenderStates.size())
        {
          staticRenderStateID = pipelineSetCompilation2->staticRenderStates[variant.staticRenderState].id;
        }
        else
        {
          staticRenderStateID =
            findOrAddStaticRenderState(RenderStateSystem::StaticState::fromRenderState(v1->renderStates[useRef.renderStateNo]));
        }
        if (base->isMesh())
        {
          if (variant.topology)
          {
            D3D_ERROR("DX12: ...loaded pipeline is a mesh pipeline, but use <%s> %u | %u was not...", shaderClass.name,
              code.dynamicCode, code.staticCode);
            continue;
          }
          PipelineVariant &pipelineVariant = base->getMeshVariantFromConfiguration(staticRenderStateID, fbLayout, variant.isWireFrame);

          if (!pipelineVariant.isReady())
          {
            pipelineVariant.loadMesh(device, *this, *base, pipeline_cache, variant.isWireFrame,
              getStaticRenderState(staticRenderStateID), fbs.getLayout(fbLayout), RecoverablePipelineCompileBehavior::REPORT_ERROR,
              pipelineSetCompilation2->giveName, *this);
          }
        }
        else
        {
          if (!variant.topology)
          {
            D3D_ERROR("DX12: ...loaded pipeline was a graphics pipeline, but use <%s> %u | %u was not...", shaderClass.name,
              code.dynamicCode, code.staticCode);
            continue;
          }
          auto inputLayout = InputLayoutManager::remapInputLayout(pipelineSetCompilation2->inputLayouts[variant.inputLayout].id,
            base->getVertexShaderInputMask());
          PipelineVariant &pipelineVariant = base->getVariantFromConfiguration(inputLayout, staticRenderStateID, fbLayout,
            static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(variant.topology), variant.isWireFrame);

          if (!pipelineVariant.isReady())
          {
            pipelineVariant.load(device, *this, *base, pipeline_cache, InputLayoutManager::getInputLayout(inputLayout),
              variant.isWireFrame, getStaticRenderState(staticRenderStateID), fbs.getLayout(fbLayout),
              static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(variant.topology), RecoverablePipelineCompileBehavior::REPORT_ERROR,
              pipelineSetCompilation2->giveName, *this);
          }
        }
      }
    }
  }

  void compileGraphicsPipelineNullOverride(ID3D12Device2 *device, PipelineCache &pipeline_cache, FramebufferLayoutManager &fbs,
    uint32_t group, const ShaderClassFilter &filter, cacheBlk::SignatureMask signature_mask, ScriptedShadersBinDump *v1,
    cacheBlk::GraphicsVariantGroup &variant)
  {
    // a mask of 0 means that this variant is for any signature valid
    if (variant.validSiagnatures && 0 == (signature_mask & variant.validSiagnatures))
    {
      return;
    }
    if ((variant.staticRenderState < pipelineSetCompilation2->staticRenderStates.size()) &&
        StaticRenderStateID::Null() == pipelineSetCompilation2->staticRenderStates[variant.staticRenderState].id)
    {
      return;
    }
    if ((variant.inputLayout >= pipelineSetCompilation2->inputLayouts.size()) ||
        (variant.frameBufferLayout >= pipelineSetCompilation2->framebufferLayouts.size()))
    {
      D3D_ERROR("DX12: ...invalid state index, ignoring pipeline variant...");
      return;
    }
    for (auto &shaderClass : variant.classes)
    {
      if (!filter.isAllowed(shaderClass.name))
      {
        logdbg("DX12: ...ignoring shader class <%s>...", shaderClass.name);
        continue;
      }
      for (auto code : shaderClass.codes)
      {
        auto useRef = evaluate_use(*v1, shaderClass.name.c_str(), code);
        if (useRef.vprId == 0xFFFF)
        {
          D3D_ERROR("DX12: ...failed to evaluate use <%s> %u | %u...", shaderClass.name, code.dynamicCode, code.staticCode);
          continue;
        }

        auto vsID = ShaderID::make(group, useRef.vprId);
        ShaderID psID = pipelineSetCompilation2->nullPixelShader;
        // TODO validate

        auto base = preloadGrahpicsBasePipeline(device, pipeline_cache, fbs, vsID, psID);

        if (!base)
        {
          D3D_ERROR("DX12: ...failed to create basic pipeline for <%s> %u | %u...", shaderClass.name, code.dynamicCode,
            code.staticCode);
          continue;
        }
        auto fbLayout = fbs.getLayoutID(pipelineSetCompilation2->framebufferLayouts[variant.frameBufferLayout].layout);
        // TODO restructure to early quit on mismatch
        StaticRenderStateID staticRenderStateID = StaticRenderStateID::Null();
        if (variant.staticRenderState < pipelineSetCompilation2->staticRenderStates.size())
        {
          staticRenderStateID = pipelineSetCompilation2->staticRenderStates[variant.staticRenderState].id;
        }
        else
        {
          staticRenderStateID =
            findOrAddStaticRenderState(RenderStateSystem::StaticState::fromRenderState(v1->renderStates[useRef.renderStateNo]));
        }
        if (base->isMesh())
        {
          if (variant.topology)
          {
            D3D_ERROR("DX12: ...loaded pipeline is a mesh pipeline, but use <%s> %u | %u was not...", shaderClass.name,
              code.dynamicCode, code.staticCode);
            continue;
          }
          PipelineVariant &pipelineVariant = base->getMeshVariantFromConfiguration(staticRenderStateID, fbLayout, variant.isWireFrame);

          if (!pipelineVariant.isReady())
          {
            pipelineVariant.loadMesh(device, *this, *base, pipeline_cache, variant.isWireFrame,
              getStaticRenderState(staticRenderStateID), fbs.getLayout(fbLayout), RecoverablePipelineCompileBehavior::REPORT_ERROR,
              pipelineSetCompilation2->giveName, *this);
          }
        }
        else
        {
          if (!variant.topology)
          {
            D3D_ERROR("DX12: ...loaded pipeline was a graphics pipeline, but use <%s> %u | %u was not...", shaderClass.name,
              code.dynamicCode, code.staticCode);
            continue;
          }
          auto inputLayout = InputLayoutManager::remapInputLayout(pipelineSetCompilation2->inputLayouts[variant.inputLayout].id,
            base->getVertexShaderInputMask());
          PipelineVariant &pipelineVariant = base->getVariantFromConfiguration(inputLayout, staticRenderStateID, fbLayout,
            static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(variant.topology), variant.isWireFrame);

          if (!pipelineVariant.isReady())
          {
            pipelineVariant.load(device, *this, *base, pipeline_cache, InputLayoutManager::getInputLayout(inputLayout),
              variant.isWireFrame, getStaticRenderState(staticRenderStateID), fbs.getLayout(fbLayout),
              static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(variant.topology), RecoverablePipelineCompileBehavior::REPORT_ERROR,
              pipelineSetCompilation2->giveName, *this);
          }
        }
      }
    }
  }

  template <typename CancellationPredicate>
  bool compileGraphicsPipelineSet2(ID3D12Device2 *device, PipelineCache &pipeline_cache, FramebufferLayoutManager &fbs, uint32_t group,
    const ShaderClassFilter &filter, cacheBlk::SignatureMask signature_mask, const CancellationPredicate &cancellation_predicate)
  {
    auto dump = getDump(group);
    if (!dump)
    {
      return true;
    }
    auto v1 = dump->getDump();
    if (!v1)
    {
      return true;
    }
    logdbg("DX12: Loading graphics pipelines from pipeline set...");
    const auto graphicsSize = pipelineSetCompilation2->graphicsPipelines.size();
    for (auto &graphicsIdx = pipelineSetCompilation2->graphicsPipelineSetCompilationIdx; graphicsIdx < graphicsSize; graphicsIdx++)
    {
      if (cancellation_predicate())
      {
        logdbg("DX12: Graphics pipelines compilation (2) interrupted (group: %d, idx: %d/%d)", group, graphicsIdx, graphicsSize);
        return false;
      }
      auto &variant = pipelineSetCompilation2->graphicsPipelines[graphicsIdx];
      compileGraphicsPipeline2(device, pipeline_cache, fbs, group, filter, signature_mask, v1, variant);
    }

    logdbg("DX12: Loading graphics pipelines has finished. Loading graphics pipelines with null overrides...");

    const auto graphicsNullOverridesSize = pipelineSetCompilation2->graphicsPipelinesWithNullOverrides.size();
    for (auto &graphicsIdx = pipelineSetCompilation2->graphicsPipelinesWithNullOverridesSetCompilationIdx;
         graphicsIdx < graphicsNullOverridesSize; graphicsIdx++)
    {
      if (cancellation_predicate())
      {
        logdbg("DX12: Graphics pipelines (null overrides) compilation (2) interrupted (group: %d, idx: %d/%d)", group, graphicsIdx,
          graphicsNullOverridesSize);
        return false;
      }
      auto &variant = pipelineSetCompilation2->graphicsPipelinesWithNullOverrides[graphicsIdx];
      compileGraphicsPipelineNullOverride(device, pipeline_cache, fbs, group, filter, signature_mask, v1, variant);
    }
    return true;
  }

  template <typename CancellationPredicate>
  bool compileComputePipelineSet2(ID3D12Device2 *device, PipelineCache &pipeline_cache, uint32_t group,
    const ShaderClassFilter &filter, cacheBlk::SignatureMask signature_mask, const CancellationPredicate &cancellation_predicate)
  {
    auto dump = getDump(group);
    if (!dump)
    {
      return true;
    }
    auto v1 = dump->getDump();
    if (!v1)
    {
      return true;
    }
    logdbg("DX12: Loading compute pipelines from pipeline set...");
    auto &mapping = pixelShaderComputeProgramIDMap[group];
    const auto size = pipelineSetCompilation2->computePipelines.size();
    for (auto &computeIdx = pipelineSetCompilation2->computePipelineSetCompilationIdx; computeIdx < size; computeIdx++)
    {
      if (cancellation_predicate())
      {
        logdbg("DX12: Compute pipelines compilation (2) interrupted (group: %d, idx: %d/%d)", group, computeIdx, size);
        return false;
      }
      auto &shaderClass = pipelineSetCompilation2->computePipelines[computeIdx];
      // a mask of 0 means that this variant is for any signature valid
      if (shaderClass.validSiagnatures && 0 == (signature_mask & shaderClass.validSiagnatures))
      {
        continue;
      }
      if (!filter.isAllowed(shaderClass.name))
      {
        continue;
      }
      for (auto code : shaderClass.codes)
      {
        auto useRef = evaluate_use(*v1, shaderClass.name.c_str(), code);
        if (useRef.fshId == 0xFFFF && useRef.vprId == 0xFFFF)
        {
          D3D_ERROR("DX12: ...failed to evaluate use <%s> %u | %u...", shaderClass.name, code.dynamicCode, code.staticCode);
          continue;
        }
        auto asCSIndex = useRef.fshId;
        if (asCSIndex >= mapping.size())
        {
          D3D_ERROR("DX12: ...CS index out of rang of known CS in dump...");
          continue;
        }
        auto program = eastl::get_if<ProgramID>(&mapping[asCSIndex]);
        if (!program)
        {
          D3D_ERROR("DX12: ...CS index did not contain CS program ID...");
          continue;
        }
        auto progId = *program;
        auto &progGroup = computePipelines[progId.getGroup()];
        if (progId.getIndex() < progGroup.size() && progGroup[progId.getIndex()])
        {
          // already loaded, so we are done
          continue;
        }
        // CSPreloaded::No as we doing the preload right now
        loadComputeShaderFromDump(device, pipeline_cache, progId, RecoverablePipelineCompileBehavior::REPORT_ERROR,
          pipelineSetCompilation2->giveName, CSPreloaded::No);
      }
    }
    return true;
  }

  void onPipelineSetCompilationFinish()
  {
    if (pipelineSetCompilation)
      logdbg("DX12: Pipeline set compilation is finished");
    pipelineSetCompilation.reset();
    if (pipelineSetCompilation2)
      logdbg("DX12: Pipeline set compilation (2) is finished");
    pipelineSetCompilation2.reset();
  }

  template <typename CancellationPredicate>
  bool compilePipelineSet2(ID3D12Device2 *device, PipelineCache &pipeline_cache, FramebufferLayoutManager &fbs,
    const CancellationPredicate &cancellation_predicate)
  {
    for (uint32_t &groupIndex = pipelineSetCompilation2->groupIndex; groupIndex < max_scripted_shaders_bin_groups; ++groupIndex)
    {
      auto dump = getDump(groupIndex);
      if (!dump)
      {
        continue;
      }
      auto v1 = dump->getDump();
      if (!v1)
      {
        continue;
      }
      auto &name = getGroupName(groupIndex);
      auto groupSignature = eastl::find_if(eastl::begin(pipelineSetCompilation2->scriptedShaderDumpSignature),
        eastl::end(pipelineSetCompilation2->scriptedShaderDumpSignature), [&name](auto &signature) { return name == signature.name; });
      if (groupSignature == eastl::end(pipelineSetCompilation2->scriptedShaderDumpSignature))
      {
        continue;
      }
      if (!pipelineSetCompilation2->filter)
        pipelineSetCompilation2->filter = ShaderClassFilter::make(*v1, groupSignature->hashes);
      if (!pipelineSetCompilation2->filter->allowsAny())
      {
        continue;
      }
      pipelineSetCompilation2->foundAnyMatch = true;

      cacheBlk::SignatureMask signatureMask =
        1u << (eastl::distance(eastl::begin(pipelineSetCompilation2->scriptedShaderDumpSignature), groupSignature));

      const bool isGraphicsFinished = compileGraphicsPipelineSet2(device, pipeline_cache, fbs, groupIndex,
        *pipelineSetCompilation2->filter, signatureMask, cancellation_predicate);
      const bool isComputeFinished = compileComputePipelineSet2(device, pipeline_cache, groupIndex, *pipelineSetCompilation2->filter,
        signatureMask, cancellation_predicate);
      if (!isGraphicsFinished || !isComputeFinished)
        return false;
      pipelineSetCompilation2->filter.reset();
      pipelineSetCompilation2->computePipelineSetCompilationIdx = 0;
      pipelineSetCompilation2->graphicsPipelineSetCompilationIdx = 0;
      pipelineSetCompilation2->graphicsPipelinesWithNullOverridesSetCompilationIdx = 0;
    }
    if (!pipelineSetCompilation2->foundAnyMatch)
    {
      logdbg(
        "DX12: Scripted shader bin dump signature does not match scripted shader blk cache signature, skipping pipeline preloading");
    }
    return true;
  }

  bool needToUpdateCache = false;
  bool hasFeatureSetInCache = true;
};


struct NullResourceTable
{
  // TODO wrong place?
  enum
  {
    RENDER_TARGET_VIEW,
    SAMPLER,
    SAMPLER_COMPARE,
    // TODO do buffers need two (formatted and structured)?
    UAV_BUFFER,
    UAV_BUFFER_RAW,
    UAV_TEX2D,
    UAV_TEX2D_ARRAY,
    UAV_TEX3D,
    // TODO do buffers need two (formatted and structured)?
    SRV_BUFFER,
    SRV_TEX2D,
    SRV_TEX2D_ARRAY,
    SRV_TEX_CUBE,
    SRV_TEX_CUBE_ARRAY,
    SRV_TEX3D,
#ifdef D3D_HAS_RAY_TRACING
    SRV_TLAS,
#endif
    COUNT,
    INVALID = COUNT
  };
  D3D12_CPU_DESCRIPTOR_HANDLE table[COUNT] = {};

  D3D12_CPU_DESCRIPTOR_HANDLE get(D3D_SHADER_INPUT_TYPE type, D3D_SRV_DIMENSION dim) const
  {
    // PVS warning suppress for "Two or more case-branches perform the same actions" for all switch block in this function
    //-V::1037
    if (D3D_SIT_CBUFFER == type)
    {
      DAG_FATAL("unexpected descriptor lookup");
    }
    else if (D3D_SIT_TBUFFER == type)
    {
      return table[SRV_BUFFER];
    }
    else if (D3D_SIT_TEXTURE == type)
    {
      switch (dim)
      {
        case D3D_SRV_DIMENSION_UNKNOWN: return table[SRV_TEX2D];
        case D3D_SRV_DIMENSION_TEXTURE1D:
        case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        case D3D_SRV_DIMENSION_TEXTURE2DMS:
        case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY: DAG_FATAL("unexpected descriptor lookup"); break;
        case D3D_SRV_DIMENSION_BUFFER: return table[SRV_BUFFER];
        case D3D_SRV_DIMENSION_TEXTURE2D: return table[SRV_TEX2D];
        case D3D_SRV_DIMENSION_TEXTURE2DARRAY: return table[SRV_TEX2D_ARRAY];
        case D3D_SRV_DIMENSION_TEXTURE3D: return table[SRV_TEX3D];
        case D3D_SRV_DIMENSION_TEXTURECUBE: return table[SRV_TEX_CUBE];
        case D3D_SRV_DIMENSION_TEXTURECUBEARRAY: return table[SRV_TEX_CUBE_ARRAY];
        case D3D_SRV_DIMENSION_BUFFEREX: return table[SRV_BUFFER];
      }
    }
    else if (D3D_SIT_SAMPLER == type)
    {
      DAG_FATAL("unexpected descriptor lookup");
    }
    else if (D3D_SIT_UAV_RWTYPED == type)
    {
      switch (dim)
      {
        case D3D_SRV_DIMENSION_UNKNOWN: return table[UAV_TEX2D];
        case D3D_SRV_DIMENSION_TEXTURE1D:
        case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        case D3D_SRV_DIMENSION_TEXTURE2DMS:
        case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
        case D3D_SRV_DIMENSION_TEXTURECUBE:
        case D3D_SRV_DIMENSION_TEXTURECUBEARRAY: DAG_FATAL("unexpected descriptor lookup"); break;
        case D3D_SRV_DIMENSION_BUFFER: return table[UAV_BUFFER];
        case D3D_SRV_DIMENSION_TEXTURE2D: return table[UAV_TEX2D];
        case D3D_SRV_DIMENSION_TEXTURE2DARRAY: return table[UAV_TEX2D_ARRAY];
        case D3D_SRV_DIMENSION_TEXTURE3D: return table[UAV_TEX3D];
        case D3D_SRV_DIMENSION_BUFFEREX: return table[UAV_BUFFER_RAW];
      }
    }
    else if (D3D_SIT_STRUCTURED == type)
    {
      return table[SRV_BUFFER];
    }
    else if (D3D_SIT_UAV_RWSTRUCTURED == type)
    {
      return table[UAV_BUFFER];
    }
    else if (D3D_SIT_BYTEADDRESS == type)
    {
      return table[SRV_BUFFER];
    }
    else if (D3D_SIT_UAV_RWBYTEADDRESS == type)
    {
      return table[UAV_BUFFER];
    }
    else if (D3D_SIT_UAV_APPEND_STRUCTURED == type)
    {
      DAG_FATAL("Use of invalid resource type D3D_SIT_UAV_APPEND_STRUCTURED");
    }
    else if (D3D_SIT_UAV_CONSUME_STRUCTURED == type)
    {
      DAG_FATAL("Use of invalid resource type D3D_SIT_UAV_APPEND_STRUCTURED");
    }
    else if (D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER == type)
    {
      DAG_FATAL("Use of invalid resource type D3D_SIT_UAV_APPEND_STRUCTURED");
    }
#ifdef D3D_HAS_RAY_TRACING
    else if (D3D_SIT_RTACCELERATIONSTRUCTURE == type)
    {
      return table[SRV_TLAS];
    }
#endif
    return {0};
  }
};

using ConstBufferStreamDescriptorHeap = StreamDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>;

class ResourceUsageManagerWithHistory;
class BarrierBatcher;
class SplitTransitionTracker;

struct ConstBufferSetupInformation
{
  BufferResourceReferenceAndAddressRange buffer;
#if DX12_VALIDATE_STREAM_CB_USAGE_WITHOUT_INITIALIZATION
  bool isStreamBuffer = false;
  uint32_t lastDiscardFrameIdx = 0;
  eastl::string name;
#endif
};

struct ConstBufferSetupInformationStream
{
  BufferResourceReferenceAndAddressRange buffer;
#if DX12_VALIDATE_STREAM_CB_USAGE_WITHOUT_INITIALIZATION
  bool isStreamBuffer = false;
  uint32_t lastDiscardFrameIdx = 0;
#endif
};

struct PipelineStageStateBase
{
  struct TRegister
  {
    Image *image = nullptr;
    ImageViewState imageView = {};
    BufferResourceReference buffer = {};
#if D3D_HAS_RAY_TRACING
    RaytraceAccelerationStructure *as = nullptr;
#endif
    D3D12_CPU_DESCRIPTOR_HANDLE view = {};

    bool is(D3D_SHADER_INPUT_TYPE type, D3D_SRV_DIMENSION dim);
    void reset()
    {

      image = nullptr;
      imageView = ImageViewState{};
      buffer = BufferResourceReference{};
#if D3D_HAS_RAY_TRACING
      as = nullptr;
#endif
      view = D3D12_CPU_DESCRIPTOR_HANDLE{0};
    }
  };
  struct URegister
  {
    Image *image = nullptr;
    ImageViewState imageView = {};
    BufferResourceReference buffer = {};
    D3D12_CPU_DESCRIPTOR_HANDLE view = {};

    bool is(D3D_SHADER_INPUT_TYPE type, D3D_SRV_DIMENSION dim);
    void reset()
    {
      image = nullptr;
      imageView = ImageViewState{};
      buffer = BufferResourceReference{};
      view = D3D12_CPU_DESCRIPTOR_HANDLE{0};
    }
  };
  // deliberately store address and buffer reference info in different array as they are never accessed together.
  D3D12_CONSTANT_BUFFER_VIEW_DESC bRegisters[dxil::MAX_B_REGISTERS] = {};
  ConstBufferSetupInformation bRegisterBuffers[dxil::MAX_B_REGISTERS] = {};
  TRegister tRegisters[dxil::MAX_T_REGISTERS] = {};
  D3D12_CPU_DESCRIPTOR_HANDLE sRegisters[dxil::MAX_T_REGISTERS] = {};
  URegister uRegisters[dxil::MAX_U_REGISTERS] = {};
  uint32_t bRegisterValidMask = 0;
  uint32_t tRegisterValidMask = 0;
  uint32_t uRegisterValidMask = 0;
  uint32_t sRegisterValidMask = 0;
  uint32_t bRegisterStateDirtyMask = 0;
  uint32_t tRegisterStateDirtyMask = 0;
  uint32_t uRegisterStateDirtyMask = 0;
  eastl::bitset<dxil::MAX_T_REGISTERS> isConstDepthStencil = {};
  D3D12_CONSTANT_BUFFER_VIEW_DESC constRegisterLastBuffer = {};
  DescriptorHeapRange bRegisterDescribtorRange;
  DescriptorHeapRange tRegisterDescriptorRange;
  DescriptorHeapRange sRegisterDescriptorRange;
  DescriptorHeapRange uRegisterDescriptorRange;

  PipelineStageStateBase() = default;

  // If the descriptor heap has changed we have to push everything again
  void onResourceDescriptorHeapChanged()
  {
    bRegisterValidMask = tRegisterValidMask = uRegisterValidMask = 0; //
  }
  void onFlush()
  {
    bRegisterValidMask = tRegisterValidMask = uRegisterValidMask = sRegisterValidMask = 0; //
  }
  void invalidateState() { bRegisterValidMask = tRegisterValidMask = uRegisterValidMask = sRegisterValidMask = 0; }
  void invalidateResourceStates()
  {
    bRegisterStateDirtyMask = tRegisterStateDirtyMask = uRegisterStateDirtyMask = ~0u; //
  }
  void resetDescriptorRanges()
  {
    bRegisterDescribtorRange = {};
    tRegisterDescriptorRange = {};
    sRegisterDescriptorRange = {};
    uRegisterDescriptorRange = {};
  }
  void resetAllState()
  {
    resetDescriptorRanges();
    invalidateState();
    for (auto &reg : bRegisterBuffers)
      reg = {};
    for (auto &reg : tRegisters)
      reg.reset();
    for (auto &reg : uRegisters)
      reg.reset();
    eastl::fill(eastl::begin(sRegisters), eastl::end(sRegisters), D3D12_CPU_DESCRIPTOR_HANDLE{0});
  }
  void setSRVTexture(uint32_t unit, Image *image, ImageViewState view_state, bool as_const_ds, D3D12_CPU_DESCRIPTOR_HANDLE view);
  void setSampler(uint32_t unit, D3D12_CPU_DESCRIPTOR_HANDLE sampler);
  void setUAVTexture(uint32_t unit, Image *image, ImageViewState view_state, D3D12_CPU_DESCRIPTOR_HANDLE view);
  bool setSRVBuffer(uint32_t unit, BufferResourceReferenceAndShaderResourceView buffer);
  bool setUAVBuffer(uint32_t unit, BufferResourceReferenceAndUnorderedResourceView buffer);
  void setConstBuffer(uint32_t unit, const ConstBufferSetupInformation &info);
#if D3D_HAS_RAY_TRACING
  void setRaytraceAccelerationStructureAtT(uint32_t unit, RaytraceAccelerationStructure *as);
#endif
  void setSRVNull(uint32_t unit);
  void setUAVNull(uint32_t unit);
  template <typename T>
  void enumerateUAVResources(uint32_t uav_mask, T clb);
  // Pushes
  // Has to be the last just before migrateAllViews to make sure they are pushed to the right heap
  enum class ConstantBufferPushMode
  {
    ROOT_DESCRIPTOR,
    DESCRIPTOR_HEAP,
    // Split mode is possible, where slot 0 is a root descriptor and all others are descriptor heap
  };
  void pushConstantBuffers(ID3D12Device *device, ShaderResourceViewDescriptorHeapManager &heap,
    ConstBufferStreamDescriptorHeap &stream_heap, D3D12_CONSTANT_BUFFER_VIEW_DESC default_const_buffer, uint32_t cb_mask,
    StatefulCommandBuffer &cmd, uint32_t stage, ConstantBufferPushMode mode, uint32_t frame_idx);
  void pushSamplers(ID3D12Device *device, SamplerDescriptorHeapManager &heap, D3D12_CPU_DESCRIPTOR_HANDLE default_sampler,
    D3D12_CPU_DESCRIPTOR_HANDLE default_cmp_sampler, uint32_t sampler_mask, uint32_t cmp_sampler_mask);
  void pushUnorderedViews(ID3D12Device *device, ShaderResourceViewDescriptorHeapManager &heap, const NullResourceTable &null_table,
    uint32_t uav_mask, const uint8_t *uav_types);
  void pushShaderResourceViews(ID3D12Device *device, ShaderResourceViewDescriptorHeapManager &heap,
    const NullResourceTable &null_table, uint32_t srv_mask, const uint8_t *srv_types);
  void migrateAllSamplers(ID3D12Device *device, SamplerDescriptorHeapManager &heap);
  void dirtyBufferState(BufferGlobalId id);
  void dirtyTextureState(Image *texture);
  void flushResourceStates(uint32_t b_register_mask, uint32_t t_register_mask, uint32_t u_register_mask, uint32_t stage,
    ResourceUsageManagerWithHistory &res_usage, BarrierBatcher &barrier_batcher, SplitTransitionTracker &split_tracker);
  uint32_t cauclateBRegisterDescriptorCount(uint32_t b_register_mask) const { return __popcount(b_register_mask); }
  uint32_t cauclateTRegisterDescriptorCount(uint32_t t_register_mask) const { return __popcount(t_register_mask); }
  uint32_t cauclateURegisterDescriptorCount(uint32_t u_register_mask) const { return __popcount(u_register_mask); }
};

// regroup this to avoid branching in loop:
// constBuffers[constBufferCount]
// readViews[readBufferCount + readTextureCount + readTlasCount]
// writeViews[writeBufferCount + writeTextureCount]
// readBuffers[readBufferCount]
// readTextures[readTextureCount]
// writeBuffers[writeBufferCount]
// writeTextures[writeTextureCount]
// readTlas[readTlasCount]
// samplers[SamplerCount]
struct ResourceBindingTable
{
  struct ReadResourceInfo
  {
    BufferResourceReference bufferRef;
    Image *image = nullptr;
    ImageViewState imageView;
    bool isConstDepthRead = false;
#if D3D_HAS_RAY_TRACING
    RaytraceAccelerationStructure *tlas = nullptr;
#endif
  };
  struct WriteResourceInfo
  {
    BufferResourceReference bufferRef;
    Image *image = nullptr;
    ImageViewState imageView;
  };
  BufferResourceReferenceAndAddressRange constBuffers[8];
  ReadResourceInfo readResources[32];
  D3D12_CPU_DESCRIPTOR_HANDLE readResourceViews[32]{};
  D3D12_CPU_DESCRIPTOR_HANDLE samplers[32]{};
  D3D12_CPU_DESCRIPTOR_HANDLE writeResourceViews[8]{};
  WriteResourceInfo writeResources[8];
  uint32_t contBufferCount = 0;
  uint32_t readCount = 0;
  uint32_t writeCount = 0;
  uint32_t samplerCount = 0;
};
#if D3D_HAS_RAY_TRACING
struct RayDispatchBasicParameters
{
  const RaytracePipelineSignature *rootSignature = nullptr;
  ID3D12StateObject *pipeline = nullptr;
};
struct RayDispatchParameters
{
  BufferResourceReferenceAndAddressRange rayGenTable;
  BufferResourceReferenceAndAddressRange missTable;
  BufferResourceReferenceAndAddressRange hitTable;
  BufferResourceReferenceAndAddressRange callableTable;
  uint32_t missStride = 0;
  uint32_t hitStride = 0;
  uint32_t callableStride = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t depth = 0;
};
struct RayDispatchIndirectParameters
{
  BufferResourceReferenceAndOffset argumentBuffer;
  BufferResourceReferenceAndOffset countBuffer; // optional
  uint32_t argumentStrideInBytes = 0;
  uint32_t maxCount = 0;
};
#endif
} // namespace drv3d_dx12

template <>
struct DebugConverter<drv3d_dx12::BufferGlobalId>
{
  static uint32_t getDebugType(const drv3d_dx12::BufferGlobalId &v) { return v.get(); }
};
