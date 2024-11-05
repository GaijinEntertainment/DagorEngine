// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>

#include "vk_wrapped_handles.h"
#include "buffer_resource.h"


namespace drv3d_vulkan
{

class Buffer;

// references a buffer with a fixed discard index
// provides a buffer like interface but returns values for the fixed discard index
struct BufferRef
{
  Buffer *buffer = nullptr;
  uint32_t offset = 0;
  VkDeviceSize visibleDataSize = 0;
  BufferRef() = default;
  ~BufferRef() = default;
  BufferRef(const BufferRef &) = default;
  BufferRef &operator=(const BufferRef &) = default;
  // make this explicit to make it clear that we grab and hold on to the current
  // discard index
  explicit BufferRef(Buffer *bfr, uint32_t visible_data_size = 0);
  explicit BufferRef(Buffer *bfr, uint32_t visible_data_size, uint32_t in_offset);
  explicit operator bool() const;
  VulkanBufferHandle getHandle() const;
  VulkanBufferViewHandle getView() const;
  VkDeviceSize bufOffset(VkDeviceSize ofs) const;
  VkDeviceSize memOffset(VkDeviceSize ofs) const;
  uint8_t *ptrOffset(VkDeviceSize ofs) const;
  void markNonCoherentRange(uint32_t ofs, uint32_t size, bool flush);
  void addOffset(uint32_t in_offset);

#if VK_KHR_buffer_device_address
  VkDeviceAddress devOffset(VkDeviceSize ofs) const;
#endif
  void clear()
  {
    buffer = nullptr;
    offset = 0;
    visibleDataSize = 0;
  }

  friend bool operator==(const BufferRef &l, const BufferRef &r) { return (l.buffer == r.buffer) && (l.offset == r.offset); }
  friend bool operator!=(const BufferRef &l, const BufferRef &r) { return !(l == r); }
};

inline BufferRef::BufferRef(Buffer *bfr, uint32_t visible_data_size) :
  buffer(bfr),
  offset(bfr ? bfr->getCurrentDiscardOffset() : 0),
  visibleDataSize(bfr && !visible_data_size ? bfr->getCurrentDiscardVisibleSize() : visible_data_size)
{}

inline BufferRef::BufferRef(Buffer *bfr, uint32_t visible_data_size, uint32_t in_offset) :
  buffer(bfr),
  offset((bfr ? bfr->getCurrentDiscardOffset() : 0) + in_offset),
  visibleDataSize(bfr && !visible_data_size ? bfr->getBlockSize() : visible_data_size)
{}

inline BufferRef::operator bool() const { return nullptr != buffer; }

inline VulkanBufferHandle BufferRef::getHandle() const { return buffer->getHandle(); }

inline VulkanBufferViewHandle BufferRef::getView() const
{
  return buffer->hasView() ? buffer->getViewOfDiscardIndex(offset / buffer->getBlockSize()) : VulkanBufferViewHandle{};
}

inline void BufferRef::addOffset(uint32_t in_offset)
{
  G_ASSERTF(visibleDataSize > in_offset, "vulkan: trying to add offset %u that is over visible range %u in buffer %p:%s reference",
    in_offset, visibleDataSize, buffer, buffer->getDebugName());
  offset += in_offset;
  visibleDataSize -= in_offset;
}

inline VkDeviceSize BufferRef::bufOffset(VkDeviceSize ofs) const { return offset + buffer->bufOffsetAbs(ofs); }
inline VkDeviceSize BufferRef::memOffset(VkDeviceSize ofs) const { return offset + buffer->memOffsetAbs(ofs); }
inline uint8_t *BufferRef::ptrOffset(VkDeviceSize ofs) const { return offset + buffer->ptrOffsetAbs(ofs); }
inline void BufferRef::markNonCoherentRange(uint32_t ofs, uint32_t size, bool flush)
{
  return buffer->markNonCoherentRangeAbs(ofs + offset, size, flush);
}

#if VK_KHR_buffer_device_address
inline VkDeviceSize BufferRef::devOffset(VkDeviceSize ofs) const { return offset + buffer->devOffsetAbs(ofs); }
#endif

} // namespace drv3d_vulkan
