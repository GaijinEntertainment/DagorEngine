// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <projectiveDecals/projectiveDecalsBuffer.h>

DecalsBuffer::DecalsBuffer(uint32_t size, const char *decal_buffer_name, const char *culled_buffer_name) :
  decalBufferName(decal_buffer_name), culledBufferName(culled_buffer_name)
{
  resize_buffers(size);
}

void DecalsBuffer::resize_buffers(unsigned int size)
{
  auto tmpBuf = dag::buffers::create_ua_sr_structured(sizeof(ProjectiveDecalInstance), size, decalBufferName.c_str());

  if (decalInstances)
  {
    decalInstances->copyTo(tmpBuf.get(), 0, 0, sizeof(ProjectiveDecalInstance) * bufferCapacity);
  }
  eastl::swap(decalInstances, tmpBuf);
  culledInstances = dag::buffers::create_ua_sr_structured(sizeof(ProjectiveDecalInstance), size, culledBufferName.c_str());
  bufferCapacity = size;
}