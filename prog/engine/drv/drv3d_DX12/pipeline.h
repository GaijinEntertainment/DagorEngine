#pragma once

#include <EASTL/vector.h>
#include <perfMon/dag_autoFuncProf.h>
#include <perfMon/dag_statDrv.h>

#if !_TARGET_XBOXONE
using CD3DX12_PIPELINE_STATE_STREAM_AS =
  CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS>;
using CD3DX12_PIPELINE_STATE_STREAM_MS =
  CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS>;
#endif

namespace drv3d_dx12
{
namespace backend
{
class BindlessSetManager;
}
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

  friend bool operator==(const BufferGlobalId l, const BufferGlobalId r) { return (l.value & index_mask) == (r.value & index_mask); }
  friend bool operator!=(const BufferGlobalId l, const BufferGlobalId r) { return (l.value & index_mask) != (r.value & index_mask); }
  uint32_t get() const { return value; }
};

struct BufferState
{
  ID3D12Resource *buffer = nullptr;
  uint32_t size = 0;
  uint32_t discardCount = 0;
  uint32_t currentDiscardIndex = 0;
  uint32_t fistDiscardFrame = 0;
  uint32_t offset = 0;
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
  uint32_t totalSize() const { return discardCount * size; }

  ValueRange<uint32_t> usageRange() const { return make_value_range<uint32_t>(offset, totalSize()); }

  uint32_t currentOffset() const { return offset + currentDiscardIndex * size; }

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
  uint32_t offset = 0; // only use this in conjunction with the buffer object, all pointers are
                       // already adjusted!
  uint32_t size = 0;
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
  uint32_t offset = 0;

  BufferResourceReferenceAndOffset() = default;
  BufferResourceReferenceAndOffset(const BufferReference &ref) : BufferResourceReference{ref}, offset{ref.offset} {}
  BufferResourceReferenceAndOffset(const BufferState &buf) : BufferResourceReference{buf}, offset{BufferReference{buf}.offset} {}
  BufferResourceReferenceAndOffset(const BufferReference &ref, uint32_t ofs) : BufferResourceReference{ref}, offset{ref.offset + ofs}
  {}
  BufferResourceReferenceAndOffset(const BufferState &buf, uint32_t ofs) :
    BufferResourceReference{buf}, offset{BufferReference{buf}.offset + ofs}
  {}
  BufferResourceReferenceAndOffset(const HostDeviceSharedMemoryRegion &buf) :
    BufferResourceReference{buf}, offset{uint32_t(buf.range.front())}
  {}
  BufferResourceReferenceAndOffset(const HostDeviceSharedMemoryRegion &buf, uint32_t ofs) :
    BufferResourceReference{buf}, offset{uint32_t(buf.range.front() + ofs)}
  {}

  bool operator==(const BufferResourceReferenceAndOffset &other) const
  {
    return (resourceId == other.resourceId) && (buffer == other.buffer) && (offset == other.offset);
  }

  bool operator!=(const BufferResourceReferenceAndOffset &other) const { return !(*this == other); }
};

struct BufferResourceReferenceAndRange : BufferResourceReferenceAndOffset
{
  uint32_t size = 0;

  BufferResourceReferenceAndRange() = default;
  BufferResourceReferenceAndRange(const BufferReference &ref) : BufferResourceReferenceAndOffset{ref}, size{ref.size} {}
  BufferResourceReferenceAndRange(const BufferState &buf) : BufferResourceReferenceAndOffset{buf}, size{buf.size} {}
  BufferResourceReferenceAndRange(const BufferReference &ref, uint32_t offset) :
    BufferResourceReferenceAndOffset(ref, offset), size{ref.size - offset}
  {}
  BufferResourceReferenceAndRange(const BufferState &buf, uint32_t offset) :
    BufferResourceReferenceAndOffset{buf, offset}, size{buf.size - offset}
  {}
  BufferResourceReferenceAndRange(const BufferReference &ref, uint32_t offset, uint32_t sz) :
    BufferResourceReferenceAndOffset{ref, offset}, size{sz}
  {}
  BufferResourceReferenceAndRange(const BufferState &buf, uint32_t offset, uint32_t sz) :
    BufferResourceReferenceAndOffset{buf, offset}, size{sz}
  {}
  BufferResourceReferenceAndRange(const HostDeviceSharedMemoryRegion &buf) :
    BufferResourceReferenceAndOffset{buf}, size{uint32_t(buf.range.size())}
  {}
  BufferResourceReferenceAndRange(const HostDeviceSharedMemoryRegion &buf, uint32_t ofs) :
    BufferResourceReferenceAndOffset{buf, ofs}, size{uint32_t(buf.range.size() - ofs)}
  {}
  BufferResourceReferenceAndRange(const HostDeviceSharedMemoryRegion &buf, uint32_t ofs, uint32_t sz) :
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
  BufferResourceReferenceAndAddress(const BufferReference &ref, uint32_t offset) :
    BufferResourceReference{ref}, gpuPointer{ref.gpuPointer + offset}
  {}
  BufferResourceReferenceAndAddress(const BufferState &buf, uint32_t offset) :
    BufferResourceReference{buf}, gpuPointer{BufferReference{buf}.gpuPointer + offset}
  {}
  BufferResourceReferenceAndAddress(const HostDeviceSharedMemoryRegion &buf) : BufferResourceReference{buf}, gpuPointer{buf.gpuPointer}
  {}
  BufferResourceReferenceAndAddress(const HostDeviceSharedMemoryRegion &buf, uint32_t offset) :
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
  uint32_t size = 0;

  BufferResourceReferenceAndAddressRange() = default;
  BufferResourceReferenceAndAddressRange(const BufferReference &ref) : BufferResourceReferenceAndAddress{ref}, size{ref.size} {}
  BufferResourceReferenceAndAddressRange(const BufferState &buf) : BufferResourceReferenceAndAddress{buf}, size{buf.size} {}
  BufferResourceReferenceAndAddressRange(const BufferReference &ref, uint32_t offset) :
    BufferResourceReferenceAndAddress(ref, offset), size{ref.size - offset}
  {}
  BufferResourceReferenceAndAddressRange(const BufferState &buf, uint32_t offset) :
    BufferResourceReferenceAndAddress{buf, offset}, size{buf.size - offset}
  {}
  BufferResourceReferenceAndAddressRange(const BufferReference &ref, uint32_t offset, uint32_t sz) :
    BufferResourceReferenceAndAddress{ref, offset}, size{sz ? sz : uint32_t(ref.size - offset)}
  {}
  BufferResourceReferenceAndAddressRange(const BufferState &buf, uint32_t offset, uint32_t sz) :
    BufferResourceReferenceAndAddress{buf, offset}, size{sz ? sz : uint32_t(buf.size - offset)}
  {}
  BufferResourceReferenceAndAddressRange(const HostDeviceSharedMemoryRegion &buf) :
    BufferResourceReferenceAndAddress{buf}, size{uint32_t(buf.range.size())}
  {}
  BufferResourceReferenceAndAddressRange(const HostDeviceSharedMemoryRegion &buf, uint32_t offset) :
    BufferResourceReferenceAndAddress{buf, offset}, size{uint32_t(buf.range.size() - offset)}
  {}
  BufferResourceReferenceAndAddressRange(const HostDeviceSharedMemoryRegion &buf, uint32_t offset, uint32_t sz) :
    BufferResourceReferenceAndAddress{buf, offset}, size{sz ? sz : uint32_t(buf.range.size() - offset)}
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

  // this just explodes if the inner storage type is a pointer so
  // need extra handling of that
  template <>
  struct ConstructExecuter<true>
  {
    template <typename, typename CT, typename... Args>
    static CT &construct(void *at, Args &&...args)
    {
      return *::new (at) CT{eastl::forward<Args>(args)...};
    }
  };

  template <typename IST, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE T, typename DA>
  struct ConstructHandler<CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<IST, T, DA>>
  {
    typedef CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<IST, T, DA> ConstructType;
    template <typename... Args>
    static ConstructType &construct(void *at, Args &&...args)
    {
      return ConstructExecuter<eastl::is_pointer<IST>::value>::template construct<IST, ConstructType>(at,
        eastl::forward<Args>(args)...);
    }
  };
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
    Name = &v;                        \
  }
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT, depthStencilFormat);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS, renderTargetFormats);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC, blendDesc);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT, inputLayout);
  ADD_TRACKED_STATE(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER, rasterizer);
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

inline RecoverablePipelineCompileBehavior get_recover_behvior_from_cfg(bool is_fatal, bool use_assert)
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

  eastl::string getVariantName(BasePipeline &base, const InputLayout &input_layout, const RenderStateSystem::StaticState &static_state,
    const FramebufferLayout &fb_layout);
  bool isVariantNameEmpty(BasePipeline &base, const InputLayout &input_layout, const RenderStateSystem::StaticState &static_state,
    const FramebufferLayout &fb_layout);

  eastl::string getVariantName(BasePipeline &base, const RenderStateSystem::StaticState &static_state,
    const FramebufferLayout &fb_layout);
  bool isVariantNameEmpty(BasePipeline &base, const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout);

  bool calculateColorWriteMask(const BasePipeline &base, const RenderStateSystem::StaticState &static_state,
    const FramebufferLayout &fb_layout, uint32_t &color_write_mask) const;
  // Generates description for color target formats, depth stencil format, blend modes, color masks and depth stencil modes.
  bool generateOutputMergerDescriptions(const BasePipeline &base, const RenderStateSystem::StaticState &static_state,
    const FramebufferLayout &fb_layout, GraphicsPipelineCreateInfoData &target) const;
  bool generateInputLayoutDescription(const InputLayout &input_layout, GraphicsPipelineCreateInfoData &target,
    uint32_t &input_count) const;
  // Generates description for raster configuration and view instancing
  bool generateRasterDescription(const RenderStateSystem::StaticState &static_state, bool is_wire_frame,
    GraphicsPipelineCreateInfoData &target) const;

  bool create(ID3D12Device2 *device, backend::ShaderModuleManager &shader_bytecodes, BasePipeline &base, PipelineCache &pipe_cache,
    const InputLayout &input_layout, bool is_wire_frame, const RenderStateSystem::StaticState &static_state,
    const FramebufferLayout &fb_layout, D3D12_PRIMITIVE_TOPOLOGY_TYPE top, RecoverablePipelineCompileBehavior on_error,
    bool give_name);
#if !_TARGET_XBOXONE
  bool createMesh(ID3D12Device2 *device, backend::ShaderModuleManager &shader_bytecodes, BasePipeline &base, PipelineCache &pipe_cache,
    bool is_wire_frame, const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout,
    RecoverablePipelineCompileBehavior on_error, bool give_name);
#endif

public:
  // Returns false if any error was discovered, in some cases the pipeline may be created anyway
  bool load(ID3D12Device2 *device, backend::ShaderModuleManager &shader_bytecodes, BasePipeline &base, PipelineCache &pipe_cache,
    const InputLayout &input_layout, bool is_wire_frame, const RenderStateSystem::StaticState &static_state,
    const FramebufferLayout &fb_layout, D3D12_PRIMITIVE_TOPOLOGY_TYPE top, RecoverablePipelineCompileBehavior on_error,
    bool give_name);
  bool loadMesh(ID3D12Device2 *device, backend::ShaderModuleManager &shader_bytecodes, BasePipeline &base, PipelineCache &pipe_cache,
    bool is_wire_frame, const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout,
    RecoverablePipelineCompileBehavior on_error, bool give_name);

  bool isReady() const { return nullptr != pipeline.Get(); }

  ID3D12PipelineState *get() const { return pipeline.Get(); }

  PipelineOptionalDynamicStateMask getUsedOptionalDynamicStateMask() const { return dynamicMask; }

  void preRecovery() { pipeline.Reset(); }
};

class FramebufferLayoutManager
{
  eastl::vector<FramebufferLayout> table;

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

    void setStaticRenderState(StaticRenderStateID srs)
    {
      G_ASSERT(srs.get() < 0x80000);
      staticRenderState = srs.get();
    }

    void setFrambufferLayout(FramebufferLayoutID fbl)
    {
      G_ASSERT(fbl.get() < 0x80000);
      framebufferLayout = fbl.get();
    }

    void setTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE ptt)
    {
      G_ASSERT(static_cast<uint32_t>(ptt) < 0x40); // -V547
      topology = ptt;
    }

    void setWireFrame(bool wf) { isWireFrame = wf; }

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
  using VairantTable = eastl::vector<BaseVariantInfo>;
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
    const eastl::vector<RenderStateSystem::StaticState> &static_state_list, FramebufferLayoutManager &framebuffer_layouts,
    backend::VertexShaderModuleRefStore vsm, backend::PixelShaderModuleRefStore psm, RecoverablePipelineCompileBehavior on_error,
    bool give_name) :
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
      debug("DX12: Graphics pipeline create found %u variants in cache, loading...", cnt);

      const bool weAreMesh = isMesh();
      size_t i = 0;
      while (i < cnt)
      {
        InputLayout inputLayout = {};
        bool isWireFrame = false;
        RenderStateSystem::StaticState staticState = {};
        FramebufferLayout framebufferLayout = {};
        auto top = cache.getGraphicsPipelineVariantDesc(cacheId, i, inputLayout, isWireFrame, staticState, framebufferLayout);
        auto staticStateListEntry = eastl::find(begin(static_state_list), end(static_state_list), staticState);
        if (staticStateListEntry == end(static_state_list))
        {
          logerr("DX12: Graphics pipeline cache contains entry with static state that is not in "
                 "the runtime static state list, removing from cache and skipping");
          debug("DX12: Was looking for %s", staticState.toString());
          for (auto &&e : static_state_list)
          {
            debug("DX12: %s", e.toString());
          }
          cnt = cache.removeGraphicsPipelineVariant(cacheId, i);
          continue;
        }

        if (!weAreMesh)
        {
          auto layoutID = input_layouts.addInternalLayout(inputLayout);

          auto &variant = getVariantFromConfiguration(layoutID,
            StaticRenderStateID{static_cast<int>(staticStateListEntry - begin(static_state_list))},
            framebuffer_layouts.getLayoutID(framebufferLayout), top, isWireFrame);
          if (!variant.load(device, shader_bytecodes, *this, cache, inputLayout, isWireFrame, staticState, framebufferLayout, top,
                on_error, give_name))
          {
            cnt = cache.removeGraphicsPipelineVariant(cacheId, i);
            continue;
          }
        }
        else
        {
          auto &variant =
            getMeshVariantFromConfiguration(StaticRenderStateID{static_cast<int>(staticStateListEntry - begin(static_state_list))},
              framebuffer_layouts.getLayoutID(framebufferLayout), isWireFrame);
          if (!variant.loadMesh(device, shader_bytecodes, *this, cache, isWireFrame, staticState, framebufferLayout, on_error,
                give_name))
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
};

class ComputePipeline
{
  ComputeShaderModule shaderModule;
  ComputePipelineSignature &signature;
  ComPtr<ID3D12PipelineState> pipeline;

  bool load(ID3D12Device2 *device, PipelineCache &cache, RecoverablePipelineCompileBehavior on_error, bool from_cache_only,
    bool give_name)
  {
    if (pipeline)
      return true;
#if DX12_REPORT_PIPELINE_CREATE_TIMING
    eastl::string profilerMsg;
    AutoFuncProf funcProfiler(
      !shaderModule.debugName.empty()
        ? (profilerMsg.sprintf("ComputePipeline(\"%s\")::load: took %%dus", shaderModule.debugName.c_str()), profilerMsg.c_str())
        : "ComputePipeline::load: took %dus");
#endif
    TIME_PROFILE_DEV(ComputePipeline_load);

#if _TARGET_PC_WIN
    if (!signature.signature)
    {
#if DX12_REPORT_PIPELINE_CREATE_TIMING
      // turns off reporting of the profile, we don't want to know how long it took to _not_
      // load a pipeline
      funcProfiler.fmt = nullptr;
#endif
      debug("DX12: Rootsignature was null, skipping creating compute pipeline");
      return true;
    }
    ComputePipelineCreateInfoData cpci;

    cpci.append<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(signature.signature.Get());
    cpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_CS, CD3DX12_SHADER_BYTECODE>(shaderModule.byteCode.get(), shaderModule.byteCodeSize);
    D3D12_CACHED_PIPELINE_STATE &cacheTarget = cpci.append<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO>();
    D3D12_PIPELINE_STATE_STREAM_DESC cpciDesc = {cpci.rootSize(), cpci.root()};
    pipeline = cache.loadCompute(shaderModule.ident.shaderHash, cpciDesc, cacheTarget);
    if (pipeline)
      return true;

    // if we only want to load from cache and loadCompute on the cache did not load it
    // and we don't have a pso blob for creation, we don't have a cache entry for this
    // pipeline yet.
    if (from_cache_only && !cacheTarget.CachedBlobSizeInBytes)
    {
      // turns off reporting of the profile, we don't want to know how long it took to _not_
      // load a pipeline
      funcProfiler.fmt = nullptr;
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
          // turns off reporting of the profile, we don't want to know how long it took to _not_
          // load a pipeline
          funcProfiler.fmt = nullptr;
          return true;
        }

        debug("DX12: Retrying to create compute pipeline without cache");
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

    if (give_name && pipeline && !shaderModule.debugName.empty())
    {
      eastl::vector<wchar_t> nameBuf;
      nameBuf.resize(shaderModule.debugName.length() + 1);
      DX12_SET_DEBUG_OBJ_NAME(pipeline, lazyToWchar(shaderModule.debugName.data(), nameBuf.data(), nameBuf.size()));
    }
    return true;
  }

public:
  ComputePipelineSignature &getSignature() const { return signature; };
  ID3D12PipelineState *loadAndGetHandle(ID3D12Device2 *device, PipelineCache &cache, RecoverablePipelineCompileBehavior on_error,
    bool give_name)
  {
    load(device, cache, on_error, false, give_name);
    return pipeline.Get();
  }

  ComputePipeline(ComputePipelineSignature &sign, ComputeShaderModule module, ID3D12Device2 *device, PipelineCache &cache,
    RecoverablePipelineCompileBehavior on_error, bool give_name) :
    shaderModule{eastl::move(module)}, signature{sign}
  {
    load(device, cache, on_error, true, give_name);
  }
  const dxil::ShaderHeader &getHeader() const { return shaderModule.header; }

  void preRecovery() { pipeline.Reset(); }
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

class PipelineManager : public backend::ShaderModuleManager, protected backend::InputLayoutManager
{
  eastl::vector<eastl::unique_ptr<BasePipeline>> graphicsPipelines[max_scripted_shaders_bin_groups];
  eastl::vector<RenderStateSystem::StaticState> staticRenderStateTable;
  eastl::vector<eastl::unique_ptr<ComputePipeline>> computePipelines[max_scripted_shaders_bin_groups];
  eastl::vector<eastl::unique_ptr<GraphicsPipelineSignature>> graphicsSignatures;
#if !_TARGET_XBOXONE
  eastl::vector<eastl::unique_ptr<GraphicsPipelineSignature>> graphicsMeshSignatures;
#endif
  eastl::vector<eastl::unique_ptr<ComputePipelineSignature>> computeSignatures;
#if D3D_HAS_RAY_TRACING
  eastl::vector<eastl::unique_ptr<RaytracePipeline>> raytracePipelines;
  eastl::vector<eastl::unique_ptr<RaytracePipelineSignature>> raytraceSignatures;
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
  struct BlitPipeline
  {
    ComPtr<ID3D12PipelineState> pipeline;
    DXGI_FORMAT outFormat;
  };
  eastl::vector<PrepedForDelete> prepedForDeletion;
  // blit signature is special
  ComPtr<ID3D12RootSignature> blitSignature;
  eastl::vector<BlitPipeline> blitPipelines;
  PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature = nullptr;
  D3D_ROOT_SIGNATURE_VERSION signatureVersion = D3D_ROOT_SIGNATURE_VERSION_1;
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  bool rootSignaturesUsesCBVDescriptorRanges = false;
#endif

  void createBlitSignature(ID3D12Device *device);
  ID3D12PipelineState *createBlitPipeline(ID3D12Device2 *device, DXGI_FORMAT out_fmt, bool give_name);
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
  PipelineManager(PipelineManager &&) = default;
  PipelineManager &operator=(PipelineManager &&) = default;

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
    RecoverablePipelineCompileBehavior on_error, bool give_name);

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
  ID3D12PipelineState *getBlitPipeline(ID3D12Device2 *device, DXGI_FORMAT out_fmt, bool give_name)
  {
    auto ref =
      eastl::find_if(begin(blitPipelines), end(blitPipelines), [=](const BlitPipeline &bp) { return bp.outFormat == out_fmt; });
    if (ref == end(blitPipelines))
    {
      return createBlitPipeline(device, out_fmt, give_name);
    }
    return ref->pipeline.Get();
  }

  // tries to recover from a device remove error with a new device instance
  bool recover(ID3D12Device2 *device, PipelineCache &cache);

  void preRecovery();

  void registerStaticRenderState(StaticRenderStateID ident, const RenderStateSystem::StaticState &state)
  {
    if (ident.get() == staticRenderStateTable.size())
    {
      staticRenderStateTable.push_back(state);
    }
    else
    {
      fatal("DX12: registerStaticRenderState was called with ident of %u but table has %u entries", ident.get(),
        staticRenderStateTable.size());
    }
  }

  const RenderStateSystem::StaticState &getStaticRenderState(StaticRenderStateID ident) { return staticRenderStateTable[ident.get()]; }

  using backend::InputLayoutManager::getInputLayout;
  using backend::InputLayoutManager::registerInputLayout;
  using backend::InputLayoutManager::remapInputLayout;

  struct PreloadedGraphicsPipeline
  {
    ShaderID vsID;
    ShaderID psID;
    eastl::unique_ptr<BasePipeline> pipeline;
  };
  eastl::vector<PreloadedGraphicsPipeline> preloadedGraphicsPipelines;

  struct ShaderCompressionIndex
  {
    uint16_t compressionGroup;
    uint16_t compressionIndex;
  };
  eastl::vector<ShaderCompressionIndex> computeProgramIndexToDumpShaderIndex[max_scripted_shaders_bin_groups];
  struct ScriptedShaderBinDumpInspector
  {
    uint32_t group = 0;
    ShaderID nullPixelShader;
    PipelineManager *target = nullptr;
    ScriptedShadersBinDumpOwner *owner = nullptr;
    eastl::vector<eastl::variant<eastl::monostate, ShaderID, ProgramID>> pixelShaderComputeProgramIDMap;
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
    void pixelOrComputeShaderCount(uint32_t count) { pixelShaderComputeProgramIDMap.resize(count); };

    void preloadGraphicsPipeline(ShaderID vsID, ShaderID psID)
    {
      PipelineCache::BasePipelineIdentifier cacheIdent = {};
      cacheIdent.vs = target->getVertexShaderHash(vsID);
      cacheIdent.ps = target->getPixelShaderHash(psID);

      if (!pipelineCache->containsGraphicsPipeline(cacheIdent))
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
      if (auto shader = eastl::get_if<ShaderID>(&pixelShaderComputeProgramIDMap[ps_index]))
      {
        psID = *shader;
      }
      else
      {
        auto dump = owner->getDump();
        auto pixelShaderInDumpIndex = dump->vprId.size() + ps_index;
        auto &map = dump->shGroupsMapping[pixelShaderInDumpIndex];
        auto &hash = dump->shaderHashes[pixelShaderInDumpIndex];
        auto id = pixelShaderCount++;
        target->setPixelShaderCompressionGroup(group, id, hash, map.groupId, map.indexInGroup);
        psID = ShaderID::make(group, id);
        pixelShaderComputeProgramIDMap[ps_index] = psID;
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
      if (eastl::holds_alternative<ProgramID>(pixelShaderComputeProgramIDMap[shader_index]))
      {
        return;
      }
      auto id = ProgramID::asComputeProgram(group, target->computeProgramIndexToDumpShaderIndex[group].size());
      pixelShaderComputeProgramIDMap[shader_index] = id;
      auto dump = owner->getDump();
      auto &map = dump->shGroupsMapping[dump->vprId.size() + shader_index];
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
    ScriptedShadersBinDumpOwner *dump, ShaderID null_pixel_shader)
  {
    ScriptedShaderBinDumpInspector inspector;
    inspector.group = group;
    inspector.nullPixelShader = null_pixel_shader;
    inspector.target = this;
    inspector.owner = dump;
    inspector.device = device;
    inspector.pipelineCache = pipelineCache;
    inspector.frameBufferLayoutManager = fbs;
    setDumpOfGroup(group, dump);
    inspect_scripted_shader_bin_dump(dump, inspector);
  }
  template <typename ProgramRecorder, typename GraphicsProgramRecorder>
  void removeShaderGroup(uint32_t group, ProgramRecorder &&prog, GraphicsProgramRecorder &&g_prog)
  {
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
  }
  void loadComputeShaderFromDump(ID3D12Device2 *device, PipelineCache &cache, ProgramID program,
    RecoverablePipelineCompileBehavior on_error, bool give_name)
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

    addCompute(device, cache, program, eastl::move(basicModule), on_error, give_name);
  }
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
      fatal("unexpected descriptor lookup");
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
        case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY: fatal("unexpected descriptor lookup"); break;
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
      fatal("unexpected descriptor lookup");
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
        case D3D_SRV_DIMENSION_TEXTURECUBEARRAY: fatal("unexpected descriptor lookup"); break;
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
      fatal("Use of invalid resource type D3D_SIT_UAV_APPEND_STRUCTURED");
    }
    else if (D3D_SIT_UAV_CONSUME_STRUCTURED == type)
    {
      fatal("Use of invalid resource type D3D_SIT_UAV_APPEND_STRUCTURED");
    }
    else if (D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER == type)
    {
      fatal("Use of invalid resource type D3D_SIT_UAV_APPEND_STRUCTURED");
    }
    else if (12 == type) // rtx structure TODO replace with correct symbol
    {
      fatal("ray trace acceleration structure was not set!");
    }
    return {0};
  }
};

using ConstBufferStreamDescriptorHeap = StreamDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>;

class ResourceUsageManagerWithHistory;
class BarrierBatcher;
class SplitTransitionTracker;

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
  BufferResourceReference bRegisterBuffers[dxil::MAX_B_REGISTERS] = {};
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
  void resetDescriptorRanges()
  {
    bRegisterDescribtorRange = {};
    tRegisterDescriptorRange = {};
    sRegisterDescriptorRange = {};
    uRegisterDescriptorRange = {};
  }
  void invalidateState()
  {
    bRegisterValidMask = tRegisterValidMask = uRegisterValidMask = 0;
    sRegisterValidMask = 0;
    bRegisterStateDirtyMask = tRegisterStateDirtyMask = uRegisterStateDirtyMask = 0;
  }
  void onFlush()
  {
    bRegisterValidMask = tRegisterValidMask = uRegisterValidMask = 0;
    sRegisterValidMask = 0;
    bRegisterStateDirtyMask = tRegisterStateDirtyMask = uRegisterStateDirtyMask = ~0u;
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
  void setConstBuffer(uint32_t unit, BufferResourceReferenceAndAddressRange buffer);
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
    StatefulCommandBuffer &cmd, uint32_t stage, ConstantBufferPushMode mode);
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
  // If the descriptor heap has changed we have to push everything again
  void onResourceDescriptorHeapChanged() { bRegisterValidMask = tRegisterValidMask = uRegisterValidMask = 0; }
};
} // namespace drv3d_dx12

template <>
struct DebugConverter<drv3d_dx12::BufferGlobalId>
{
  static uint32_t getDebugType(const drv3d_dx12::BufferGlobalId &v) { return v.get(); }
};
