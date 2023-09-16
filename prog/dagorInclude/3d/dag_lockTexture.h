//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_drv3d.h>
#include <EASTL/unique_ptr.h>

template <typename T>
class UnlockDeleter
{
  T *lockedObject = nullptr;

public:
  UnlockDeleter() = default;
  UnlockDeleter(T *locked_object) : lockedObject{locked_object} {}
  void operator()(void *data)
  {
    G_ASSERTF((data != nullptr) == (lockedObject != nullptr),
      "Pointer to data and a stored locked object should be both nullptr or both valid pointers.");
    G_UNUSED(data);
    if (lockedObject)
      lockedObject->unlock();
  }
};


template <typename T>
using LockedTexture = eastl::unique_ptr<T[], UnlockDeleter<BaseTexture>>;

template <typename T>
LockedTexture<T> lock_texture(BaseTexture *tex, int &stride_bytes, int level, unsigned flags)
{
  T *ptr = nullptr;
  if (tex->lockimg((void **)&ptr, stride_bytes, level, flags) && ptr)
    return {ptr, UnlockDeleter<BaseTexture>(tex)};
  return {};
}

template <typename T>
LockedTexture<T> lock_texture(BaseTexture *tex, int &stride_bytes, int layer, int level, unsigned flags)
{
  T *ptr = nullptr;
  if (tex->lockimg((void **)&ptr, stride_bytes, layer, level, flags) && ptr)
    return {ptr, UnlockDeleter<BaseTexture>(tex)};
  return {};
}
