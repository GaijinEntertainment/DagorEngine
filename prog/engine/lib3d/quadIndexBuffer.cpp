// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_info.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_spinlock.h>


static Ibuffer *box16bit = nullptr;
static Ibuffer *quads16bit = nullptr;
static Ibuffer *quads32bit = nullptr;
static int box16bitCounter = 0;
static int quads16bitCounter = 0;
static int quads32bitCounter = 0;
static uint32_t quads32bitIndicesCount = 0;

static os_spinlock_t box16bitLock, quads16bitLock, quads32bitLock;
enum
{
  NOT_INITIALIZED,
  INITIALIZING,
  INITIALIZED
};
static volatile int atomicForInit = NOT_INITIALIZED;

static void init_and_lock_quads_index_buffer_locks(os_spinlock_t *lock_ptr)
{
  if (interlocked_acquire_load(atomicForInit) != INITIALIZED)
  {
    if (interlocked_compare_exchange(atomicForInit, INITIALIZING, NOT_INITIALIZED) == NOT_INITIALIZED)
    {
      os_spinlock_init(&box16bitLock);
      os_spinlock_init(&quads16bitLock);
      os_spinlock_init(&quads32bitLock);
      interlocked_release_store(atomicForInit, INITIALIZED);
    }
    while (interlocked_acquire_load(atomicForInit) != INITIALIZED)
      ; // VOID
  }
  os_spinlock_lock(lock_ptr);
}

struct QuadsIndexBufferLock
{
  os_spinlock_t *lockPtr;
  QuadsIndexBufferLock(os_spinlock_t *noise_lock_ptr) : lockPtr(noise_lock_ptr) { init_and_lock_quads_index_buffer_locks(lockPtr); }
  ~QuadsIndexBufferLock() { os_spinlock_unlock(lockPtr); }
};

template <typename indices_type>
static void fill_indices_buffer(indices_type *indices, const uint32_t points_count)
{
  for (uint32_t pointIdx = 0; pointIdx < points_count; pointIdx += 4)
  {
    indices[0] = pointIdx;
    indices[1] = pointIdx + 1;
    indices[2] = pointIdx + 2;
    indices[3] = pointIdx;
    indices[4] = pointIdx + 2;
    indices[5] = pointIdx + 3;
    indices += 6;
  }
}

static const uint16_t box_indices[36] = {
  0, 3, 1, 0, 2, 3, 0, 1, 5, 0, 5, 4, 0, 4, 6, 0, 6, 2, 1, 7, 5, 1, 3, 7, 4, 5, 7, 4, 7, 6, 2, 7, 3, 2, 6, 7};

static void try_to_init_box_16bit(const bool modify_counter = true)
{
  box16bit = d3d::create_ib(36 * sizeof(uint16_t), 0, "ib_box_16bit");
  d3d_err(box16bit);

  uint16_t *indices = nullptr;
  if (!(box16bit->lock(0, 0, &indices, VBLOCK_WRITEONLY) && indices))
  {
    del_d3dres(box16bit);
    return;
  }

  memcpy(indices, box_indices, sizeof(box_indices));

  box16bit->unlock();
  if (modify_counter)
  {
    G_ASSERT(box16bitCounter == 0);
    box16bitCounter = 1;
  }
}

static void try_to_init_quads_16bit(const bool modify_counter = true)
{
  const uint32_t POINTS_COUNT = 1 << 16;
  const uint32_t QUADS_COUNT = POINTS_COUNT / 4;
  const uint32_t INDICES_COUNT = QUADS_COUNT * 6;
  quads16bit = d3d::create_ib(INDICES_COUNT * sizeof(uint16_t), 0, "ib_quads_16bit");
  d3d_err(quads16bit);

  uint16_t *indices = nullptr;
  if (!(quads16bit->lock(0, 0, &indices, VBLOCK_WRITEONLY) && indices))
  {
    del_d3dres(quads16bit);
    return;
  }

  fill_indices_buffer(indices, POINTS_COUNT);

  quads16bit->unlock();
  if (modify_counter)
  {
    G_ASSERT(quads16bitCounter == 0);
    quads16bitCounter = 1;
  }
}

static void try_to_init_quads_32bit(const bool modify_counter = true)
{
  if (quads32bitIndicesCount == 0)
    return;
  Ibuffer *biggerBuffer = d3d::create_ib(quads32bitIndicesCount * sizeof(uint32_t), SBCF_INDEX32, "ib_quads_32bit");
  d3d_err(biggerBuffer);

  uint32_t *indices = nullptr;
  if (!(biggerBuffer->lock32(0, 0, &indices, VBLOCK_WRITEONLY) && indices))
  {
    del_d3dres(biggerBuffer);
    return;
  }
  const uint32_t pointsCount = quads32bitIndicesCount / 6 * 4;
  fill_indices_buffer(indices, pointsCount);
  biggerBuffer->unlock();
  if (modify_counter)
    ++quads32bitCounter;
  del_d3dres(quads32bit);
  quads32bit = biggerBuffer;
}

static void reset_box_16bit()
{
  if (!box16bitCounter)
    return;
  QuadsIndexBufferLock lock(&box16bitLock);
  del_d3dres(box16bit);
  try_to_init_box_16bit(false);
}

static void reset_quads_16bit()
{
  if (!quads16bitCounter)
    return;
  QuadsIndexBufferLock lock(&quads16bitLock);
  del_d3dres(quads16bit);
  try_to_init_quads_16bit(false);
}

static void reset_quads_32bit()
{
  if (!quads32bitCounter)
    return;
  QuadsIndexBufferLock lock(&quads32bitLock);
  del_d3dres(quads32bit);
  try_to_init_quads_32bit(false);
}

namespace index_buffer
{
template <typename Fn>
static void init_ib(os_spinlock_t &lock, int &counter, Ibuffer *&ib, Fn &&f)
{
  QuadsIndexBufferLock lock_(&lock);
  if (ib)
  {
    ++counter;
    return;
  }
  f();
}
static void release_ib(os_spinlock_t &lock, int &counter, Ibuffer *&ib)
{
  QuadsIndexBufferLock lock_(&lock);
  if (!counter)
    return;
  --counter;
  G_ASSERT(counter >= 0);
  if (counter == 0)
    del_d3dres(ib);
}

template <typename Fn>
static void use_ib(os_spinlock_t &lock, int &counter, Ibuffer *&ib, Fn &&f)
{
  QuadsIndexBufferLock lock_(&lock);
  if (counter == 0)
    f();
  d3d_err(d3d::setind(ib));
}

void init_quads_16bit()
{
  init_ib(quads16bitLock, quads16bitCounter, quads16bit, []() { try_to_init_quads_16bit(); });
}
void release_quads_16bit() { release_ib(quads16bitLock, quads16bitCounter, quads16bit); }
void use_quads_16bit()
{
  use_ib(quads16bitLock, quads16bitCounter, quads16bit, []() { try_to_init_quads_16bit(); });
}

void init_box()
{
  init_ib(box16bitLock, box16bitCounter, box16bit, []() { try_to_init_box_16bit(); });
}
void release_box() { release_ib(box16bitLock, box16bitCounter, box16bit); }
void use_box()
{
  use_ib(box16bitLock, box16bitCounter, box16bit, []() { try_to_init_box_16bit(); });
}

void init_quads_32bit(const int quads_count)
{
  QuadsIndexBufferLock lock(&quads32bitLock);
  const uint32_t indicesCount = quads_count * 6;
  if (quads32bit && indicesCount <= quads32bit->getNumElements())
  {
    ++quads32bitCounter;
    return;
  }
  quads32bitIndicesCount = max(quads32bitIndicesCount, indicesCount);
  try_to_init_quads_32bit();
}

void start_use_quads_32bit()
{
  init_and_lock_quads_index_buffer_locks(&quads32bitLock);
  if (!quads32bit && quads32bitIndicesCount)
    try_to_init_quads_32bit();
  if (quads32bit && (quads32bitCounter == 0 || quads32bitIndicesCount * sizeof(uint32_t) > quads32bit->ressize()))
    try_to_init_quads_32bit();
  d3d_err(d3d::setind(quads32bit));
}

void end_use_quads_32bit() { os_spinlock_unlock(&quads32bitLock); }

void release_quads_32bit()
{
  QuadsIndexBufferLock lock(&quads32bitLock);
  if (!quads32bitCounter)
    return;
  --quads32bitCounter;
  G_ASSERT(quads32bitCounter >= 0);
  if (quads32bitCounter == 0)
  {
    del_d3dres(quads32bit);
    quads32bitIndicesCount = 0;
  }
}

void reset_buffers(bool)
{
  reset_box_16bit();
  reset_quads_16bit();
  reset_quads_32bit();
}
} // namespace index_buffer

#include <drv/3d/dag_resetDevice.h>
REGISTER_D3D_AFTER_RESET_FUNC(index_buffer::reset_buffers);
