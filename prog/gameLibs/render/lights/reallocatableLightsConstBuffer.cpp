// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/reallocatableLightsConstBuffer.h>
#include <generic/dag_align.h>

BaseReallocatableLightsConstBuffer::BaseReallocatableLightsConstBuffer() = default;
BaseReallocatableLightsConstBuffer::~BaseReallocatableLightsConstBuffer() { close(); }

bool BaseReallocatableLightsConstBuffer::reallocateInternal(int target_size, const char *stat_name, bool persistent)
{
  wasWritten = false;
  if (size >= target_size)
    return true;
  BufPtr cb2 = persistent ? dag::buffers::create_persistent_cb(target_size, stat_name, RESTAG_LIGHTS)
                          : dag::buffers::create_one_frame_cb(target_size, stat_name, RESTAG_LIGHTS);
  if (!cb2)
  {
    logerr("can't re-create buffer <%s> for size %d from %d", stat_name, target_size, size);
    return false;
  }
  size = target_size;
  buf = eastl::move(cb2);
  return true;
}

void BaseReallocatableLightsConstBuffer::close()
{
  buf.close();
  size = 0;
  wasWritten = false;
}

bool BaseReallocatableLightsConstBuffer::updateConsts(void *data, int data_size, int elems_count)
{
  uint32_t *destData = 0;
  bool ret = buf.getBuf()->lock(0, 0, (void **)&destData, VBLOCK_WRITEONLY | VBLOCK_DISCARD);

  d3d_err(ret);

  if (!ret)
    return false;
  if (!destData)
  {
    buf.getBuf()->unlock();
    return false;
  }

  if (elems_count >= 0)
  {
    destData[0] = elems_count;
    destData[1] = dag::divide_align_up(elems_count, 32);
    destData[2] = elems_count ? elems_count - 1 : 0;
    destData[3] = 0;
    destData += 4;
  }
  if (data_size)
    memcpy(destData, data, data_size);
  buf.getBuf()->unlock();

  wasWritten = true;
  return true;
}

bool BaseReallocatableLightsConstBuffer::copyIndirectInstanceCount(Sbuffer *indirect_args, uint32_t record_byte_offset,
  uint32_t instance_count_byte_offset) const
{
  G_ASSERT_RETURN(wasWritten, false);
  bool copied = buf.getBuf()->copyTo(indirect_args, record_byte_offset + instance_count_byte_offset, 0, sizeof(uint32_t));
  return copied;
}
