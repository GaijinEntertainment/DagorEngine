// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <projectiveDecals/projectiveDecals.h>


void RingBufferDecalsBase::init_buffer(uint32_t maximum_size, size_t instance_struct_size)
{
  ringBufferCapacity = maximum_size;
  ProjectiveDecalsBase::init_buffer(maximum_size, instance_struct_size);
}

int RingBufferDecalsBase::allocate_decal_id()
{
  G_ASSERT(ringBufferCapacity > 0);
  int decalId = ringBufferIndex;

  decalsToRender = min(decalsToRender + 1, ringBufferCapacity);
  ringBufferIndex = (ringBufferIndex + 1) % ringBufferCapacity;
  return decalId;
}

void RingBufferDecalsBase::clear()
{
  decalsToRender = 0;
  ringBufferIndex = 0;
}

int RingBufferDecalsBase::addDecal(dag::Span<Point4> decal_data)
{
  int decal_id = allocate_decal_id();
  setDecalId(decal_data, decal_id);
  updateDecal(decal_data);
  return decal_id;
}