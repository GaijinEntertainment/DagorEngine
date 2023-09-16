//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "dag_resPtr.h"
#include <EASTL/vector_map.h>
#include <cstdint>

// dynamically resizable texture baked by single memory area (heap)
// resize to smaller size works only when heaps & resource aliasing are supported

namespace resptr_detail
{

using key_t = uint32_t;

class ResizableManagedTex : public ManagedTex
{
protected:
  eastl::vector_map<key_t, UniqueTex> mAliases;

  void swap(ResizableManagedTex &other)
  {
    ManagedTex::swap(other);
    eastl::swap(mAliases, other.mAliases);
  }

  ResizableManagedTex() = default;

public:
  void resize(int width, int height);
};

class ResizableManagedTexHolder : public ManagedResHolder<ResizableManagedTex>
{
protected:
  ResizableManagedTexHolder() = default;

public:
  void resize(int width, int height)
  {
    this->ResizableManagedTex::resize(width, height);
    this->setVar();
  }
};

using ResizableTex = UniqueRes<ResizableManagedTex>;
using ResizableTexHolder = ConcreteResHolder<UniqueRes<ResizableManagedTexHolder>>;

} // namespace resptr_detail

using ResizableTex = resptr_detail::ResizableTex;
using ResizableTexHolder = resptr_detail::ResizableTexHolder;
