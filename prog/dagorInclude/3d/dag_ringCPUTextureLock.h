//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_ringCPUQueryLock.h>

class BaseTexture;
typedef BaseTexture Texture;

class RingCPUTextureLock : public RingCPUBufferLock
{
public:
  template <typename T>
  Texture *getNewTarget(T &frame)
  {
    return (Texture *)RingCPUBufferLock::getNewTarget(frame);
  }
  template <typename T>
  Texture *getNewTargetAndId(T &frame, D3DRESID &id)
  {
    return (Texture *)RingCPUBufferLock::getNewTargetAndId(frame, id);
  }
  void init(uint32_t w, uint32_t h, int max_buffers_count, const char *name, uint32_t fmt)
  {
    RingCPUBufferLock::init(w, h, max_buffers_count, name, 0, fmt, true);
  }

  // if lock is successfull, it will return stride, raw data, and frame locked
  // max_frames_to_flush is max frame to wait before it will try to flushevent/face
  // if lock is not successful, than there is no newer frame available on CPU (then previous)
  // if object is not inited - lock will always fail
  uint8_t *lock(int &stride, uint32_t &frame, int max_frames_to_flush = 8)
  {
    return RingCPUBufferLock::lock(stride, frame, true, max_frames_to_flush);
  }
};
