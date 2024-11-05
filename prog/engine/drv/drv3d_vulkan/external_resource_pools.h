// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// resource pool not managed by resource manager, for d3d wrapper level resources

#include <generic/dag_objectPool.h>
#include <osApiWrappers/dag_spinlock.h>
#include "texture.h"
#include "buffer.h"

namespace drv3d_vulkan
{

template <typename ObjectType>
struct ExternalResourcePool
{
  OSSpinlock guard;
  ObjectPool<ObjectType> data;

  template <typename CbType>
  void freeWithLeakCheck(CbType cb)
  {
    OSSpinlockScopedLock lock{guard};

    data.iterateAllocated(cb);
    data.freeAll();
  }

  void free(ObjectType *obj)
  {
    OSSpinlockScopedLock lock{guard};

    data.free(obj);
  }

  void reserve(int size)
  {
    OSSpinlockScopedLock lock{guard};

    data.reserve(size);
  }

  int capacity()
  {
    OSSpinlockScopedLock lock{guard};

    return data.capacity();
  }

  int size()
  {
    OSSpinlockScopedLock lock{guard};

    return data.size();
  }

  template <typename CbType>
  void iterateAllocated(CbType cb)
  {
    OSSpinlockScopedLock lock{guard};

    data.iterateAllocated(cb);
  }
};

struct TexturePool : public ExternalResourcePool<TextureInterfaceBase>
{
  TextureInterfaceBase *allocate(int res_type, uint32_t cflg)
  {
    OSSpinlockScopedLock lock{guard};

    return data.allocate(res_type, cflg);
  }
};

struct BufferPool : public ExternalResourcePool<GenericBufferInterface>
{
  GenericBufferInterface *allocate(uint32_t struct_size, uint32_t element_count, uint32_t flags, FormatStore format, bool managed,
    const char *stat_name)
  {
    OSSpinlockScopedLock lock{guard};

    return data.allocate(struct_size, element_count, flags, format, managed, stat_name);
  }
};

} // namespace drv3d_vulkan
