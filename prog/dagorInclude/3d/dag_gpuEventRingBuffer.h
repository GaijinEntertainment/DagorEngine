//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_fencedRingBuffer.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dReset.h>

class GPUEventRingBuffer
{
  uint32_t resetCounter;

public:
  struct EventQuery
  {
    D3dEventQuery *q = nullptr;
    EventQuery(D3dEventQuery *a = nullptr) : q(a) {}
    // rule of 5
    ~EventQuery() { d3d::release_event_query(q); }
    EventQuery(EventQuery &&a) : q(a.q) { a.q = nullptr; }
    EventQuery &operator=(EventQuery &&a)
    {
      eastl::swap(q, a.q);
      return *this;
    }
    EventQuery(const EventQuery &a) = delete;
    EventQuery &operator=(const EventQuery &a) = delete;
  };
  using offset_type_t = FencedRingBuffer<EventQuery>::offset_type_t;
  static constexpr offset_type_t invalid_offset = offset_type_t(~offset_type_t(0));

  FencedRingBuffer<EventQuery> buffer;
  GPUEventRingBuffer(size_t sz) : buffer(sz), resetCounter(get_d3d_reset_counter()) {}
  void finishCurrentFrame()
  {
    D3dEventQuery *q = d3d::create_event_query();
    d3d::issue_event_query(q);
    buffer.finishCurrentFrame(EventQuery{q});
  }
  offset_type_t allocate(offset_type_t size, offset_type_t alignment)
  {
    uint32_t cReset = get_d3d_reset_counter();
    if (resetCounter != cReset)
    {
      buffer.forceReset();
      resetCounter = cReset;
    }
    return buffer.allocate(size, alignment);
  }
  void forceReset() { buffer.forceReset(); }
  void reset(offset_type_t max_sz) { buffer.reset(max_sz); }
  void releaseCompletedFrames()
  {
    buffer.releaseCompletedFrames([&](auto &f) { return d3d::get_event_query_status(f.q, false); });
  }
  void updateLast(intptr_t allocated_minus_actual_used) { buffer.updateLast(allocated_minus_actual_used); }
};
