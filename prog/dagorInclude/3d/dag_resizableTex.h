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

namespace resizeable_detail
{
struct D3dDeleter
{
  void operator()(BaseTexture *tex) const { del_d3dres(tex); }
};
using UniqueBaseTex = eastl::unique_ptr<BaseTexture, D3dDeleter>;
} // namespace resizeable_detail

class ResizableManagedTex : public ManagedTex
{
public:
  using AliasMap = eastl::vector_map<key_t, resizeable_detail::UniqueBaseTex>;

protected:
  AliasMap mAliases;
  key_t currentKey = 0;
  resid_t lastMResId = Helper<ManagedTex>::BAD_ID;
  BaseTexture *originalTexture = nullptr;

  void swap(ResizableManagedTex &other)
  {
    ManagedTex::swap(other);
    eastl::swap(mAliases, other.mAliases);
    eastl::swap(currentKey, other.currentKey);
    eastl::swap(lastMResId, other.lastMResId);
    eastl::swap(originalTexture, other.originalTexture);
  }
  ResizableManagedTex() = default;

  void alias(int width, int height, int depth, int flags, int levels);

public:
  void calcKey();
  void resize(int width, int height, int d = 1);
  void dropAliases()
  {
#if !_TARGET_C1 && !_TARGET_C2
    mAliases.clear();
    currentKey = 0;
    lastMResId = Helper<ManagedTex>::BAD_ID;
    originalTexture = nullptr;
#endif
  }
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
