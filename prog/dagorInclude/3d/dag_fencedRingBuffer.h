//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/deque.h>
#include <stdint.h>
#include <debug/dag_assert.h>
#include <drv/3d/dag_query.h>

template <class FenceType = D3dEventQuery *>
class FencedRingBuffer
{
public:
  using offset_type_t = size_t;
  static constexpr offset_type_t invalid_offset = offset_type_t(~offset_type_t(0));
  struct FrameHeadAttribs
  {
    FrameHeadAttribs(FenceType &&f, offset_type_t off, offset_type_t sz) : fence(eastl::move(f)), offset(off), size(sz) {}

    FenceType fence;
    offset_type_t offset;
    offset_type_t size;
  };

  FencedRingBuffer(offset_type_t sz) : maxSize(sz) {}

  FencedRingBuffer(FencedRingBuffer &&a) :
    completedFrameHeads(eastl::move(a.completedFrameHeads)),
    tail(a.tail),
    head(a.head),
    maxSize(a.maxSize),
    usedSize(a.usedSize),
    currentFrameSize(a.currentFrameSize)
  {
    a.tail = a.head = a.maxSize = a.usedSize = a.currentFrameSize = 0;
  }

  FencedRingBuffer &operator=(FencedRingBuffer &&a) noexcept
  {
    completedFrameHeads = eastl::move(a.completedFrameHeads);
    tail = a.tail;
    head = a.head;
    maxSize = a.maxSize;
    usedSize = a.usedSize;
    currentFrameSize = a.currentFrameSize;
    a.tail = a.head = a.maxSize = a.usedSize = a.currentFrameSize = 0;

    return *this;
  }

  FencedRingBuffer(const FencedRingBuffer &) = delete;
  FencedRingBuffer &operator=(const FencedRingBuffer &) = delete;

  ~FencedRingBuffer() { G_ASSERTF(usedSize == 0, "Ring buffer must be released"); }

  offset_type_t allocateFromHead(offset_type_t size, offset_type_t alignedHead)
  {
    // Allocate from current head
    auto offset = alignedHead;
    auto adjustedSize = size + (alignedHead - head);
    head += adjustedSize;
    usedSize += adjustedSize;
    currentFrameSize += adjustedSize;
    return offset;
  }

  offset_type_t allocate(offset_type_t size, offset_type_t alignment) // returns allocated offset
  {
    G_ASSERT(size > 0);
    size = ((size + alignment - 1) / alignment) * alignment;

    if (usedSize + size > maxSize)
    {
      return invalid_offset;
    }

    auto alignedHead = ((head + alignment - 1) / alignment) * alignment;
    if (head >= tail)
    {
      if (alignedHead + size <= maxSize)
      {
        return allocateFromHead(size, alignedHead);
      }
      else if (size <= tail)
      {
        // Allocate from the beginning of the buffer
        offset_type_t addSize = (maxSize - head) + size;
        usedSize += addSize;
        currentFrameSize += addSize;
        head = size;
        return 0;
      }
    }
    else if (alignedHead + size <= tail)
    {
      return allocateFromHead(size, alignedHead);
    }

    return invalid_offset;
  }
  void updateLast(intptr_t allocated_minus_actual_used)
  {
    G_ASSERT(head >= allocated_minus_actual_used && currentFrameSize >= allocated_minus_actual_used);
    head -= allocated_minus_actual_used;
    currentFrameSize -= allocated_minus_actual_used;
  }

  // fv is the fence value associated with the command list in which the head. Currently we use only one gpu fence
  void finishCurrentFrame(FenceType &&fv)
  {
    // Ignore zero-size frames
    if (currentFrameSize != 0)
    {
      completedFrameHeads.emplace_back(eastl::move(fv), head, currentFrameSize);
      currentFrameSize = 0;
    }
  }

  // GPU progress
  // currently use d3d::get_event_query_status(completedFrameHeads.front().fence, false)
  template <class IsFinishedFence>
  void releaseCompletedFrames(IsFinishedFence finished)
  {
    // We can release all heads whose associated fence value is less than or equal to CompletedFenceValue
    while (!completedFrameHeads.empty() && finished(completedFrameHeads.front().fence))
    {
      const auto &oldestFrameHead = completedFrameHeads.front();
      G_ASSERT(oldestFrameHead.size <= usedSize);
      usedSize -= oldestFrameHead.size;
      tail = oldestFrameHead.offset;
      completedFrameHeads.pop_front();
    }

    if (isEmpty())
    {
      completedFrameHeads.clear();
      tail = head = 0;
    }
  }
  void forceReset() // on reset
  {
    completedFrameHeads.clear();
    tail = head = usedSize = currentFrameSize = 0;
  }
  void reset(offset_type_t sz)
  {
    maxSize = sz;
    forceReset();
  }

  offset_type_t getMaxSize() const { return maxSize; }
  bool isFull() const { return usedSize == maxSize; };
  bool isEmpty() const { return usedSize == 0; };
  offset_type_t getUsedSize() const { return usedSize; }
  offset_type_t getHead() const { return head; }
  offset_type_t getTail() const { return tail; }
  offset_type_t getCurrentFrameSize() const { return currentFrameSize; }

private:
  eastl::deque<FrameHeadAttribs> completedFrameHeads;

  offset_type_t tail = 0;
  offset_type_t head = 0;
  offset_type_t maxSize = 0;
  offset_type_t usedSize = 0;
  offset_type_t currentFrameSize = 0;
};
