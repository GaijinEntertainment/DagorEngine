//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/fixed_function.h>

#include <render/daBfg/detail/resourceType.h>


namespace dabfg
{

// Intentionally copyable
using TypeErasedCall = eastl::fixed_function<sizeof(void *) * 4, void(void *)>;
using TypeErasedCopyCall = eastl::fixed_function<sizeof(void *) * 3, void(void *, const void *)>;

struct BlobDescription
{
  ResourceSubtypeTag typeTag;
  size_t size;
  size_t alignment;
  TypeErasedCall activate;
  TypeErasedCall deactivate;
  TypeErasedCopyCall copy;
};

struct BlobView
{
  void *data = nullptr;
  ResourceSubtypeTag typeTag = ResourceSubtypeTag::Invalid;

  friend bool operator==(const BlobView &lhs, const BlobView &rhs) { return lhs.data == rhs.data && lhs.typeTag == rhs.typeTag; }
  friend bool operator!=(const BlobView &lhs, const BlobView &rhs) { return !(lhs == rhs); }
  friend bool operator!(const BlobView &arg) { return !arg.data; }
};

} // namespace dabfg
