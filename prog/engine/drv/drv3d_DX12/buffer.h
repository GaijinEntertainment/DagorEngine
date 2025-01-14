// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "constants.h"
#include "device_memory_class.h"
#include "format_store.h"
#include "pipeline.h"

#include <genericSbufferImplementation.h>


namespace drv3d_dx12
{
struct PlatformBufferInterfaceConfig;
using GenericBufferInterface = GenericSbufferImplementation<PlatformBufferInterfaceConfig>;

struct BufferInterfaceConfigCommon
{
  constexpr static const char *driverName() { return "DX12"; }

  constexpr static bool REPORT_ERRORS = true;
  constexpr static bool REPORT_WARNINGS = true;
  constexpr static bool TRACK_DISCARD_FRAME = DX12_VALIDATE_STREAM_CB_USAGE_WITHOUT_INITIALIZATION;
  // the buffer heap manager reports memory sizes to the manager
  constexpr static bool REPORT_MEMORY_SIZE_TO_MANAGER = false;
  // memory manager will notify the texture manager at the right time for resource shuffle kick off.
  constexpr static bool KICK_OFF_MEMORY_RELEASE = false;

  using MemoryClass = DeviceMemoryClass;
  constexpr static MemoryClass SHARED_CACHED_MEMORY_CLASS = DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER;
  constexpr static MemoryClass SHARED_UNCACHED_MEMORY_CLASS = DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER;
  constexpr static MemoryClass SHARED_UNCACHED_INDIRECT_MEMORY_CLASS = SHARED_UNCACHED_MEMORY_CLASS;
  constexpr static MemoryClass FAST_SHARED_UNCACHED_MEMORY_CLASS = DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER;
  constexpr static MemoryClass PRIVATE_MEMORY_CLASS = DeviceMemoryClass::DEVICE_RESIDENT_BUFFER;

  using BufferType = BufferState;
  using BufferReferenceType = BufferState &;
  using BufferConstReferenceType = const BufferState &;

  /*constexpr*/ static BufferType getNullBuffer() { return {}; }

  using StagingMemoryType = HostDeviceSharedMemoryRegion;
  using TemporaryMemoryType = HostDeviceSharedMemoryRegion;

  static constexpr TemporaryMemoryType getNullTemporaryMemory() { return {}; }

  static constexpr StagingMemoryType getNullStagingMemory() { return {}; }

  constexpr static uint32_t INITIAL_DISCARD_COUNT_DYNAMIC_CONST_BUFFER = DYNAMIC_CONST_BUFFER_DISCARD_BASE_COUNT;
  constexpr static uint32_t INITIAL_DISCARD_COUNT_DYNAMIC_BUFFER = DYNAMIC_BUFFER_DISCARD_BASE_COUNT;
  constexpr static uint32_t INITIAL_DISCARD_COUNT_DEFAULT = DEFAULT_BUFFER_DISCARD_BASE_COUNT;
  // staging buffers are written to mostly only once per frame, but set discard count to two
  // to allow the buffer allocator to expand to a bigger discard count should the extra memory,
  // due to alignment, allow it.
  constexpr static uint32_t INITIAL_DISCARD_COUNT_STAGING_BUFFER = 2;

  using ViewFormatType = FormatStore;

  static ViewFormatType getRawViewFormat() { return FormatStore::fromCreateFlags(TEXFMT_R32UI); }

  static ViewFormatType viewFormatFromFormatFlags(uint32_t flags) { return FormatStore::fromCreateFlags(flags); }

  static bool isValidBuffer(BufferConstReferenceType buffer) { return buffer.buffer != nullptr; }
  static void deleteBuffer(BufferReferenceType buffer);
  static void addRawResourceView(BufferReferenceType buffer);
  static void addStructuredResourceView(BufferReferenceType buffer, uint32_t struct_size);
  static void addShaderResourceView(BufferReferenceType buffer, FormatStore format);
  static void addRawUnorderedView(BufferReferenceType buffer);
  static void addStructuredUnorderedView(BufferReferenceType buffer, uint32_t struct_size);
  static void addUnorderedAccessView(BufferReferenceType buffer, FormatStore format);
  static void onDestroyRequest(GenericBufferInterface *self);
  static HostDeviceSharedMemoryRegion allocateTemporaryUploadMemory(uint32_t size);
  static bool isValidMemory(HostDeviceSharedMemoryRegion mem) { return static_cast<bool>(mem); }
  static uint8_t *getMemoryPointer(HostDeviceSharedMemoryRegion mem, uint32_t offset) { return mem.pointer + offset; }
  static void updateBuffer(HostDeviceSharedMemoryRegion mem, GenericBufferInterface *self, uint32_t buf_flags,
    BufferReferenceType buffer, uint32_t dst_offset);
  static void copyBuffer(GenericBufferInterface *src_buf, uint32_t src_flags, BufferReferenceType src, TemporaryMemoryType src_stream,
    bool src_is_stream, GenericBufferInterface *dst_buf, uint32_t dst_flags, BufferReferenceType dst, TemporaryMemoryType dst_stream,
    bool dst_is_stream, uint32_t src_offset, uint32_t dst_offset, uint32_t size);
  static void invalidateMappedRange(BufferReferenceType buffer, uint32_t offset, uint32_t size);
  static void flushMappedRange(BufferReferenceType buffer, uint32_t offset, uint32_t size);
  static uint8_t *getMappedPointer(BufferReferenceType buffer, uint32_t offset);
  static void invalidateMemory(HostDeviceSharedMemoryRegion mem, uint64_t offset, uint32_t size);
  static void readBackBuffer(BufferReferenceType src, HostDeviceSharedMemoryRegion dst, uint32_t src_offset, uint32_t dst_offset,
    uint32_t size);
  static void blockingReadBackBuffer(BufferReferenceType src, HostDeviceSharedMemoryRegion dst, uint32_t src_offset,
    uint32_t dst_offset, uint32_t size);
  static void flushBuffer(BufferReferenceType src, uint32_t offset, uint32_t size);
  static void blockingFlushBuffer(BufferReferenceType src, uint32_t offset, uint32_t size);
  static void uploadBuffer(HostDeviceSharedMemoryRegion src, BufferReferenceType dst, uint32_t src_offset, uint32_t dst_offset,
    uint32_t size);
  static void freeMemory(HostDeviceSharedMemoryRegion mem);
  static StagingMemoryType allocateReadWriteStagingMemory(uint32_t size);
  static StagingMemoryType allocateReadOnlyStagingMemory(uint32_t size);
  static StagingMemoryType allocateWriteOnlyStagingMemory(uint32_t size);
  static const char *getBufferName(BufferReferenceType) { return "<buffer name not stored>"; }
  static StagingMemoryType discardReadWriteStagingMemory(StagingMemoryType mem, uint32_t size)
  {
    freeMemory(mem);
    return allocateReadWriteStagingMemory(size);
  }
  static StagingMemoryType discardReadOnlyStagingMemory(StagingMemoryType mem, uint32_t size)
  {
    freeMemory(mem);
    return allocateReadOnlyStagingMemory(size);
  }
  static StagingMemoryType discardWriteOnlyStagingMemory(StagingMemoryType mem, uint32_t size)
  {
    freeMemory(mem);
    return allocateWriteOnlyStagingMemory(size);
  }
  static void addGenericView(BufferReferenceType, uint32_t, ViewFormatType) {}
  static uint32_t getBufferSize(BufferConstReferenceType buffer);
  static uint32_t minBufferSize(uint32_t cflags);
  static void pushBufferUpdate(GenericBufferInterface *self, uint32_t buf_flags, BufferReferenceType buffer, uint32_t offset,
    const void *src, uint32_t size);
  static TemporaryMemoryType discardStreamMememory(GenericBufferInterface *self, uint32_t size, uint32_t struct_size, uint32_t flags,
    TemporaryMemoryType prev_memory, const char *name);
  static bool allowsStreamBuffer(uint32_t flags);
};

#if _TARGET_PC_WIN
struct PlatformBufferInterfaceConfig : BufferInterfaceConfigCommon
{
  constexpr static bool IS_UMA = false;
  constexpr static bool HAS_RELOAD_SUPPORT = true;
  constexpr static bool INDIRECT_BUFFER_IS_SPECIAL = false;
  constexpr static bool HAS_STRICT_LOCKING_RULES = true;
  constexpr static bool TRACK_GPU_WRITES = false;

  static bool isMapable(BufferConstReferenceType buf);

  static D3D12_RESOURCE_FLAGS getResourceFlags(uint32_t flags)
  {
    D3D12_RESOURCE_FLAGS result = D3D12_RESOURCE_FLAG_NONE;
    if ((SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE | SBCF_BIND_UNORDERED) & flags)
    {
      result |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    return result;
  }
  static BufferType createBuffer(uint32_t size, uint32_t structure_size, uint32_t discard_count, MemoryClass memory_class,
    uint32_t buf_flags, const char *name);
  static BufferType discardBuffer(GenericBufferInterface *self, BufferReferenceType current_buffer, uint32_t size,
    uint32_t structure_size, MemoryClass memory_class, FormatStore view_format, uint32_t buf_flags, const char *name);
};
#else
struct PlatformBufferInterfaceConfig : BufferInterfaceConfigCommon
{
  constexpr static bool IS_UMA = true;
  constexpr static bool HAS_RELOAD_SUPPORT = false;
  constexpr static bool INDIRECT_BUFFER_IS_SPECIAL = true;
  constexpr static bool HAS_STRICT_LOCKING_RULES = false;
  constexpr static bool TRACK_GPU_WRITES = false;

  static bool isMapable(BufferConstReferenceType buf);

  static D3D12_RESOURCE_FLAGS getResourceFlags(uint32_t flags)
  {
    D3D12_RESOURCE_FLAGS result = D3D12_RESOURCE_FLAG_NONE;
    if ((SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE | SBCF_BIND_UNORDERED) & flags)
    {
      result |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if (SBCF_MISC_DRAWINDIRECT & flags)
    {
      result |= RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER;
    }
    if (SBCF_BIND_INDEX == (flags & SBCF_BIND_MASK))
    {
      result |= RESOURCE_FLAG_PREFER_INDEX_BUFFER;
    }
    return result;
  }

  static BufferType createBuffer(uint32_t size, uint32_t structure_size, uint32_t discard_count, MemoryClass memory_class,
    uint32_t buf_flags, const char *name);
  static BufferType discardBuffer(GenericBufferInterface *self, BufferReferenceType current_buffer, uint32_t size,
    uint32_t structure_size, MemoryClass memory_class, FormatStore view_format, uint32_t buf_flags, const char *name);
};
#endif

inline DXGI_FORMAT get_index_format(GenericBufferInterface *buffer)
{
  return buffer->has32BitIndexType() ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
}

inline BufferReference get_any_buffer_ref(GenericBufferInterface *buf)
{
  if (!buf->isStreamBuffer())
  {
    return buf->getDeviceBuffer();
  }
  else
  {
    return BufferReference{buf->getStreamBuffer()};
  }
}
} // namespace drv3d_dx12
