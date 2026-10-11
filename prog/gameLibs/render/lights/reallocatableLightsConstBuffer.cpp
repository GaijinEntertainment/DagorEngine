// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/reallocatableLightsConstBuffer.h>
#include <generic/dag_align.h>

BaseReallocatableLightsConstBuffer::BaseReallocatableLightsConstBuffer() = default;
BaseReallocatableLightsConstBuffer::~BaseReallocatableLightsConstBuffer() { close(); }

bool BaseReallocatableLightsConstBuffer::reallocateInternal(int target_size, const char *stat_name, Flags new_mode)
{
  wasWritten = false;
  if (size >= target_size && mode == new_mode)
    return true;
  BufPtr cb2;
  if ((new_mode & Flags::CpuWrite) == Flags::None)
    cb2 = dag::create_sbuffer(d3d::buffers::CBUFFER_REGISTER_SIZE, target_size, SBCF_BIND_CONSTANT, 0, stat_name, RESTAG_LIGHTS);
  else if ((new_mode & Flags::Persistent) != Flags::None)
    cb2 = dag::buffers::create_persistent_cb(target_size, stat_name, RESTAG_LIGHTS);
  else
    cb2 = dag::buffers::create_one_frame_cb(target_size, stat_name, RESTAG_LIGHTS);
  if (!cb2)
  {
    logerr("can't re-create buffer <%s> for size %d from %d", stat_name, target_size, size);
    return false;
  }
  size = target_size;
  mode = new_mode;
  buf = eastl::move(cb2);
  return true;
}

void BaseReallocatableLightsConstBuffer::close()
{
  buf.close();
  size = 0;
  mode = Flags::None;
  wasWritten = false;
}

bool BaseReallocatableLightsConstBuffer::updateConsts(void *data, int data_size, int elems_count)
{
  // Lock only the written bytes: a lock of size 0 means the whole buffer, which
  // is allocated at the maximum light count and charged to framemem each frame.
  // An empty update still locks one constant, zeroed below: a framemem buffer
  // needs its discard every frame, and one constant is the minimum.
  const int constantSizeInBytes = sizeof(uint32_t) * 4;
  const int headerSizeInBytes = elems_count >= 0 ? constantSizeInBytes : 0;
  const int writtenSizeInBytes = headerSizeInBytes + data_size;
  const int lockSizeInBytes = max(writtenSizeInBytes, constantSizeInBytes);

  G_ASSERT_RETURN((mode & Flags::CpuWrite) != Flags::None, false);

  uint32_t *destData = 0;
  bool ret = buf.getBuf()->lock(0, lockSizeInBytes, (void **)&destData, VBLOCK_WRITEONLY | VBLOCK_DISCARD);

  d3d_err(ret);

  if (!ret)
    return false;
  if (!destData)
  {
    buf.getBuf()->unlock();
    return false;
  }

  if (writtenSizeInBytes == 0)
    memset(destData, 0, lockSizeInBytes);
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
  G_ASSERT_RETURN((mode & Flags::CpuWrite) == Flags::None || wasWritten, false);
  bool copied = buf.getBuf()->copyTo(indirect_args, record_byte_offset + instance_count_byte_offset, 0, sizeof(uint32_t));
  return copied;
}
