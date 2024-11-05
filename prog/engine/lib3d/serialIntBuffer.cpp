// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_info.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_miscApi.h>
#include <3d/dag_serialIntBuffer.h>


static Vbuffer *serial_ints = nullptr;
static volatile int serial_ints_counter = 0;
static uint32_t max_count = 0;

static os_spinlock_t serial_ints_lock;
enum
{
  NOT_INITIALIZED,
  INITIALIZING,
  INITIALIZED
};
static volatile int atomicForInit = NOT_INITIALIZED;

static void init_and_lock_buffer_locks()
{
  if (interlocked_acquire_load(atomicForInit) != INITIALIZED)
  {
    if (interlocked_compare_exchange(atomicForInit, INITIALIZING, NOT_INITIALIZED) == NOT_INITIALIZED)
    {
      os_spinlock_init(&serial_ints_lock);
      interlocked_release_store(atomicForInit, INITIALIZED);
    }
    // avoid sheduling problems with spin_wait
    spin_wait([&] { return interlocked_acquire_load(atomicForInit) != INITIALIZED; });
  }
  os_spinlock_lock(&serial_ints_lock);
}

struct SerialBufferLock
{
  SerialBufferLock() { init_and_lock_buffer_locks(); }
  ~SerialBufferLock() { os_spinlock_unlock(&serial_ints_lock); }
};

static void fill_buffer(uint32_t *ints, const uint32_t ints_count)
{
  for (uint32_t idx = 0; idx < ints_count; ++ints, ++idx)
    *ints = idx;
}

static bool fill(Vbuffer *vb, uint32_t count)
{
  uint32_t *data = nullptr;
  if (!vb->lock(0, 0, (void **)&data, VBLOCK_WRITEONLY) || !data)
    return false;
  fill_buffer(data, count);
  vb->unlock();
  return true;
}

static Vbuffer *try_to_init(uint32_t count)
{
  if (count == 0)
    return nullptr;
  Vbuffer *ints = d3d::create_vb(count * sizeof(uint32_t), 0, "serial_ints_vb");
  d3d_err(ints);
  if (fill(ints, count))
    return ints;
  del_d3dres(ints);
  return nullptr;
}

static void reset()
{
  if (!serial_ints_counter)
    return;
  SerialBufferLock lock;
  if (!serial_ints) // When buffer wasn't created because of device reset.
    serial_ints = try_to_init(max_count);
  if (serial_ints) // Skip fill if buffer wasn't created again. It means we will call reset again and will fill it later.
    fill(serial_ints, max_count);
}

namespace serial_buffer
{
static volatile int usage_count = 0;
void term_serial()
{
  SerialBufferLock lock;
  if (interlocked_acquire_load(usage_count))
    logerr("can't term serial buffer while using it");
  --serial_ints_counter;
  if (serial_ints_counter == 0)
    del_d3dres(serial_ints);
  if (serial_ints_counter < 0)
  {
    DAG_FATAL("non paired termination of serial buffer");
    serial_ints_counter = 0;
  }
}
void init_serial(const uint32_t count)
{
  {
    SerialBufferLock lock;
    if (interlocked_acquire_load(usage_count))
      logerr("can't init serial buffer while using it");
    if (serial_ints && count <= max_count)
    {
      ++serial_ints_counter;
      return;
    }
  }
  Vbuffer *bigger = try_to_init(count); // do that outside of lock
  SerialBufferLock lock;
  ++serial_ints_counter;
  max_count = count;
  del_d3dres(serial_ints);
  serial_ints = bigger;
}

static Sbuffer *start_use_serial_internal(uint32_t count)
{
  SerialBufferLock lock; // likely path
  if (serial_ints && count <= max_count)
  {
    interlocked_increment(usage_count);
    return serial_ints;
  }
  return nullptr;
}
Sbuffer *start_use_serial(uint32_t count)
{
  if (auto s = start_use_serial_internal(count))
    return s;
  // unhappy path
  init_serial(count);
  return start_use_serial_internal(count);
}
void stop_use_serial(Sbuffer *s)
{
  if (!s)
    return;
  SerialBufferLock lock; // likely path
  G_ASSERT(s == serial_ints);
  interlocked_decrement(usage_count);
}
void reset_buffers(bool) { reset(); }
} // namespace serial_buffer

#include <drv/3d/dag_resetDevice.h>
REGISTER_D3D_AFTER_RESET_FUNC(serial_buffer::reset_buffers);
