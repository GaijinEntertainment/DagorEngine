//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "dag_resPtr.h"
#include <EASTL/vector_map.h>
#include <cstdint>

// dynamically resizable texture baked by single memory area (heap)
// resize to smaller size works only when heaps & resource aliasing are supported

namespace resptr_detail
{

using key_t = uint64_t;

class ResizableManagedTex : public ManagedTex
{
public:
  using AliasMap = eastl::vector_map<key_t, UniqueTex>;

protected:
  AliasMap mAliases;
  key_t currentKey = 0;
  resid_t lastMResId = Helper<ManagedTex>::BAD_ID;

  void swap(ResizableManagedTex &other)
  {
    ManagedTex::swap(other);
    eastl::swap(mAliases, other.mAliases);
    eastl::swap(currentKey, other.currentKey);
    eastl::swap(lastMResId, other.lastMResId);
  }
  ResizableManagedTex() = default;

public:
  void calcKey();
  void resize(int width, int height, int d = 1);
};

class ResizableManagedTexHolder : public ManagedResHolder<ResizableManagedTex>
{
protected:
  ResizableManagedTexHolder() = default;

public:
  void resize(int width, int height, int d = 1)
  {
    ResizableManagedTex::resize(width, height, d);
    setVar();
  }
};

using ResizableTex = UniqueRes<ResizableManagedTex>;
using ResizableTexHolder = ConcreteResHolder<UniqueRes<ResizableManagedTexHolder>>;

} // namespace resptr_detail

using ResizableTex = resptr_detail::ResizableTex;
using ResizableTexHolder = resptr_detail::ResizableTexHolder;
