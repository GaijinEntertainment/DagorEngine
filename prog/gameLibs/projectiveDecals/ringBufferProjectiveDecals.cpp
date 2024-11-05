// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <projectiveDecals/projectiveDecals.h>


void RingBufferDecals::init_buffer(uint32_t maximum_size)
{
  ringBufferCapacity = maximum_size;
  ProjectiveDecals::init_buffer(maximum_size);
}

int RingBufferDecals::allocate_decal_id()
{
  G_ASSERT(ringBufferCapacity > 0);
  int decalId = ringBufferIndex;

  decalsToRender = min(decalsToRender + 1, ringBufferCapacity);
  ringBufferIndex = (ringBufferIndex + 1) % ringBufferCapacity;
  return decalId;
}

void RingBufferDecals::clear()
{
  decalsToRender = 0;
  ringBufferIndex = 0;
}