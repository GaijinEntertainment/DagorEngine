//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_staticTab.h>
#include <EASTL/string.h>

class D3dResource;
class D3dEventQuery;

class RingCPUBufferLock
{
public:
  D3dResource *getNewTargetAndId(uint32_t &frame, D3DRESID &id);
  D3dResource *getNewTarget(uint32_t &frame)
  {
    D3DRESID id;
    return getNewTargetAndId(frame, id);
  }
  D3dResource *getNewTarget(int &frame)
  {
    D3DRESID id;
    return getNewTargetAndId((uint32_t &)frame, id);
  }
  D3dResource *getNewTargetAndId(int &frame, D3DRESID &id) { return getNewTargetAndId((uint32_t &)frame, id); }
  void startCPUCopy();
  //  Texture *getCurrentTarget();
  RingCPUBufferLock() {}
  ~RingCPUBufferLock() { close(); }

  // max_buffers_count supposed to be 2-3 for single GPU, and 4 for SLI. However it can actually be any number (1..6), as class will
  // gracefully handle insufficient data
  void init(uint32_t elem_size, uint32_t elems_count, int max_buffers_count, const char *name, uint32_t flags, uint32_t fmt,
    bool is_tex);
  void close();

  // if lock is successfull, it will return stride, raw data, and frame locked
  // max_frames_to_flush is max frame to wait before it will try to flushevent/face
  // if lock is not successful, than there is no newer frame available on CPU (then previous)
  // if object is not inited - lock will always fail
  uint8_t *lock(int &stride, uint32_t &frame, bool get_to_topmost, int max_frames_to_flush = 8); // if get_to_topmost - it will return
                                                                                                 // topmost event, otherwise will
                                                                                                 // proceed generously
  // close unlock only if lock was succesfill
  void unlock();
  void reset() { currentBufferIssued = currentBufferToLock = bufferLockCounter = 0; }

protected:
  struct FencedGPUResource
  {
    D3dResource *gpu = 0;
    D3DRESID id = BAD_D3DRESID;
    D3dEventQuery *event = 0;
  };
  StaticTab<FencedGPUResource, 6> buffers; //

  void unlockData(int buffer);
  bool lockData(int buffer, int &stride, uint8_t **pdata, bool wait);
  uint32_t currentBufferIssued = 0, currentBufferToLock = 0, bufferLockCounter = 0;
  enum CurrentState
  {
    NORMAL,
    LOCKED,
    NEWTARGET
  } state = NORMAL;
  eastl::string resourceName = "Not inited buffer";
};
