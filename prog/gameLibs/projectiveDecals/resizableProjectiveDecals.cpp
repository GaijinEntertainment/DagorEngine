// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <projectiveDecals/projectiveDecals.h>


void ResizableDecals::init_buffer(uint32_t initial_size)
{
  if (initial_size > 0)
    ProjectiveDecals::init_buffer(initial_size);
  else
    logerr("size of decals buffer should be greater zero");
}


int ResizableDecals::allocate_decal_id(DecalsBuffer &decals_buffer)
{
  int decalId = -1;
  if (!freeDecalIds.empty())
  {
    decalId = freeDecalIds.back();
    freeDecalIds.pop_back();
  }
  else
  {
    if (decalsToRender >= decals_buffer.bufferCapacity)
    {
      decals_buffer.resize_buffers(decals_buffer.bufferCapacity * 2);
    }
    decalId = decalsToRender;
    decalsToRender++;
  }
  G_ASSERTF(decalId >= 0, "failed to create new decal");
  decalsCount++;
  return decalId;
}

// unsafe for double removing one decal
void ResizableDecals::free_decal_id(int decal_id)
{
  decalsCount--;
  freeDecalIds.push_back(decal_id);
  G_ASSERTF(decalsCount >= 0, "we removed more decals then creates");
}
void ResizableDecals::clear()
{
  freeDecalIds.clear();
  bufferSize = 0;
  ProjectiveDecals::clear();
}
