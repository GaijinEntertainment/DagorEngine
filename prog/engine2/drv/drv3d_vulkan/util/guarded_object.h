#pragma once
#include <osApiWrappers/dag_critSec.h>

namespace drv3d_vulkan
{

class SharedGuardedObjectAutoLock
{
  WinAutoLock lock;

public:
  template <typename TargetClass>
  SharedGuardedObjectAutoLock(TargetClass &instance) : lock(*instance.sharedGuard)
  {}
};

class GuardedObjectAutoLock
{
  WinAutoLock lock;

public:
  template <typename TargetClass>
  GuardedObjectAutoLock(TargetClass &instance) : lock(instance.getGuard())
  {}
};

template <typename ObjectType>
class GuardedObject
{
  ObjectType instance;
  WinCritSec guard;

public:
  GuardedObject() = default;
  ~GuardedObject() = default;
  GuardedObject(const GuardedObject &) = delete;

  // special case for data that is not affected by multithreading
  ObjectType &unlocked() { return instance; }
  const ObjectType &unlocked() const { return instance; }

  // this methods should be called with lock acquired
  ObjectType *operator->() { return &instance; }
  ObjectType &get() { return instance; }

  WinCritSec &getGuard() { return guard; }
};

} // namespace drv3d_vulkan