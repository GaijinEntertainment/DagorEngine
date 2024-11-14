// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"

#include <debug/dag_assert.h>
#include <value_range.h>


namespace drv3d_dx12
{

template <typename T>
inline D3D12_RANGE asDx12Range(ValueRange<T> range)
{
  return {static_cast<SIZE_T>(range.front()), static_cast<SIZE_T>(range.back() + 1)};
}

struct HostDeviceSharedMemoryRegion
{
  enum class Source
  {
    TEMPORARY, // TODO: may split into temporary upload and readback (bidirectional makes no sense)
    PERSISTENT_UPLOAD,
    PERSISTENT_READ_BACK,
    PERSISTENT_BIDIRECTIONAL,
    PUSH_RING, // illegal to free
  };
  // buffer object that supplies the memory
  ID3D12Resource *buffer = nullptr;
#if _TARGET_XBOX
  // on xbox gpu and cpu pointer are the same
  union
  {
    D3D12_GPU_VIRTUAL_ADDRESS gpuPointer = 0;
    uint8_t *pointer;
  };
#else
  // offset into gpu virtual memory, including offset!
  D3D12_GPU_VIRTUAL_ADDRESS gpuPointer = 0;
  // pointer into cpu visible memory, offset is already applied (pointer - offset yields address
  // base of the buffer)
  uint8_t *pointer = nullptr;
#endif
  // offset range of this allocation
  // gpuPointer and pointer already have the start of the range added to it
  ValueRange<uint64_t> range;
  Source source = Source::TEMPORARY;

  explicit constexpr operator bool() const { return nullptr != buffer; }
  constexpr bool isTemporary() const { return Source::TEMPORARY == source; }
  void flushRegion(ValueRange<uint64_t> sub_range) const
  {
    D3D12_RANGE r = {};
    uint8_t *ptr = nullptr;
    buffer->Map(0, &r, reinterpret_cast<void **>(&ptr));
    G_ASSERT(ptr + range.front() == pointer);
    r = asDx12Range(sub_range.shiftBy(range.front()));
    buffer->Unmap(0, &r);
  }
  void invalidateRegion(ValueRange<uint64_t> sub_range) const
  {
    D3D12_RANGE r = asDx12Range(sub_range.shiftBy(range.front()));
    uint8_t *ptr = nullptr;
    buffer->Map(0, &r, reinterpret_cast<void **>(&ptr));
    G_ASSERT(pointer + sub_range.front() == ptr + range.front());
    r.Begin = r.End = 0;
    buffer->Unmap(0, &r);
  }
  void flush() const
  {
    D3D12_RANGE r = {};
    uint8_t *ptr = nullptr;
    buffer->Map(0, &r, reinterpret_cast<void **>(&ptr));
    G_ASSERT(ptr + range.front() == pointer);
    r = asDx12Range(range);
    buffer->Unmap(0, &r);
  }
  void invalidate() const
  {
    D3D12_RANGE r = asDx12Range(range);
    uint8_t *ptr = nullptr;
    buffer->Map(0, &r, reinterpret_cast<void **>(&ptr));
    G_ASSERT(ptr + range.front() == pointer);
    r.Begin = r.End = 0;
    buffer->Unmap(0, &r);
  }
  template <typename T>
  T *as() const
  {
    return reinterpret_cast<T *>(pointer);
  }
};

} // namespace drv3d_dx12
