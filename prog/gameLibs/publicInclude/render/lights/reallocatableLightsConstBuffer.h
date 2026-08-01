//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <EASTL/type_traits.h>
#include <stddef.h>

struct BaseReallocatableLightsConstBuffer
{
public:
  void close();
  Sbuffer *getBuf() const { return buf.getBuf(); }

protected:
  BaseReallocatableLightsConstBuffer();
  ~BaseReallocatableLightsConstBuffer();
  bool reallocateInternal(int target_size, const char *stat_name, bool persistent);
  bool updateConsts(void *data, int data_size, int elems_count);
  bool copyIndirectInstanceCount(Sbuffer *indirect_args, uint32_t record_byte_offset, uint32_t instance_count_byte_offset) const;

  UniqueBuf buf;
  uint16_t size = 0; // in constants, i.e. 16 bytes*size is size in bytes
  bool wasWritten = false;
};

template <int elem_size_in_constants, bool store_elems_count>
struct ReallocatableLightsConstBuffer : BaseReallocatableLightsConstBuffer
{
  bool update(void *data, int data_size)
  {
    G_ASSERT(data_size % ELEM_SIZE_IN_BYTES == 0);
    int elems_count = data_size / ELEM_SIZE_IN_BYTES;
    return updateConsts(data, data_size, store_elems_count ? elems_count : -1);
  }

  bool reallocate(int target_size_in_elems, int max_size_in_elems, const char *stat_name, bool persistent = false)
  {
    wasWritten = false;
    int targetSizeInElems = min(target_size_in_elems, max_size_in_elems);
    // on both metal and vulkan OOB access is validation complain/device lost/crash
    // so allocate max possible
    if (d3d::get_driver_code().is(d3d::metal || d3d::vulkan))
      targetSizeInElems = max_size_in_elems;
    int targetSizeInConstants = targetSizeInElems * ELEM_SIZE + (store_elems_count ? 1 : 0);
    if (!targetSizeInConstants || size >= targetSizeInConstants)
      return true;
    return reallocateInternal(targetSizeInConstants, stat_name, persistent);
  }
  D3DRESID getId() const
  {
    G_ASSERT(wasWritten || !store_elems_count);
    return buf.getBufId();
  }

  template <typename IndirectArgsT>
    requires(
      store_elems_count &&
      (eastl::is_same_v<IndirectArgsT, DrawIndirectArgs> || eastl::is_same_v<IndirectArgsT, DrawIndexedIndirectArgs> ||
        eastl::is_same_v<IndirectArgsT, DrawIndirectArgsWithId> || eastl::is_same_v<IndirectArgsT, DrawIndexedIndirectArgsWithId>))
  bool copyIndirectInstanceCount(Sbuffer *indirect_args, uint32_t record_index = 0) const
  {
    uint32_t instanceCountByteOffset;
    if constexpr (eastl::is_same_v<IndirectArgsT, DrawIndirectArgsWithId>)
      instanceCountByteOffset = offsetof(DrawIndirectArgsWithId, args) + offsetof(DrawIndirectArgs, instanceCount);
    else if constexpr (eastl::is_same_v<IndirectArgsT, DrawIndexedIndirectArgsWithId>)
      instanceCountByteOffset = offsetof(DrawIndexedIndirectArgsWithId, args) + offsetof(DrawIndexedIndirectArgs, instanceCount);
    else
      instanceCountByteOffset = offsetof(IndirectArgsT, instanceCount);

    return BaseReallocatableLightsConstBuffer::copyIndirectInstanceCount(indirect_args, record_index * sizeof(IndirectArgsT),
      instanceCountByteOffset);
  }

private:
  enum
  {
    ELEM_SIZE = elem_size_in_constants,
    ELEM_SIZE_IN_BYTES = ELEM_SIZE * sizeof(Point4)
  };
};
