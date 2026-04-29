//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_fixedMoveOnlyFunction.h>
#include <EASTL/unique_ptr.h>
#include <render/daFrameGraph/detail/resourceType.h>


namespace dafg
{

struct BlobDescription
{
  ResourceSubtypeTag typeTag;
  using CtorT = dag::FixedMoveOnlyFunction<sizeof(void *) * 4, void(void *) const>;
  eastl::unique_ptr<CtorT> ctorOverride;

  bool operator==(const BlobDescription &other) const
  {
    if (typeTag != other.typeTag)
      return false;

    if (ctorOverride || other.ctorOverride)
      return false;

    return true;
  }
};

struct BlobView
{
  void *data = nullptr;
  ResourceSubtypeTag typeTag = ResourceSubtypeTag::Invalid;

  friend bool operator==(const BlobView &, const BlobView &) = default;
  explicit operator bool() const { return data != nullptr; }
};

} // namespace dafg
