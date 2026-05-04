// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <projectiveDecals/projectiveDecalsBuffer.h>

DecalsBuffer::DecalsBuffer(uint32_t size, const char *decal_buffer_name, const char *culled_buffer_name, size_t instance_struct_size) :
  decalBufferName(decal_buffer_name), culledBufferName(culled_buffer_name), instanceStructSize(instance_struct_size)
{
  resize_buffers(size);
}

void DecalsBuffer::resize_buffers(unsigned int size)
{
  BufPtr tmpBuf =
    dag::buffers::create_ua_sr_structured(instanceStructSize, size, decalBufferName.c_str(), d3d::buffers::Init::No, RESTAG_DECALS);

  if (decalInstances)
  {
    decalInstances->copyTo(tmpBuf.get(), 0, 0, instanceStructSize * bufferCapacity);
  }
  eastl::swap(decalInstances, tmpBuf);
  culledInstances =
    dag::buffers::create_ua_sr_structured(instanceStructSize, size, culledBufferName.c_str(), d3d::buffers::Init::No, RESTAG_DECALS);
  bufferCapacity = size;
}