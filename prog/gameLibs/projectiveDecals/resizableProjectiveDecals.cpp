// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <projectiveDecals/projectiveDecals.h>


void ResizableDecalsBase::init_buffer(uint32_t initial_size, size_t instance_struct_size)
{
  if (initial_size > 0)
    ProjectiveDecalsBase::init_buffer(initial_size, instance_struct_size);
  else
    logerr("size of decals buffer should be greater zero");
}


int ResizableDecalsBase::allocate_decal_id(DecalsBuffer &decals_buffer)
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
void ResizableDecalsBase::free_decal_id(int decal_id)
{
  decalsCount--;
  freeDecalIds.push_back(decal_id);
  G_ASSERTF(decalsCount >= 0, "we removed more decals then creates");
}
void ResizableDecalsBase::clear()
{
  freeDecalIds.clear();
  bufferSize = 0;
  ProjectiveDecalsBase::clear();
}


int ResizableDecalsBase::addDecal(dag::Span<Point4> decal_data)
{
  int decal_id = allocate_decal_id(buffer);
  setDecalId(decal_data, decal_id);
  updateDecal(decal_data);
  return decal_id;
}


void ResizableDecalsBase::removeDecal(dag::ConstSpan<Point4> decal_data)
{
  updateDecal(decal_data);
  free_decal_id(getDecalId(decal_data));
}
