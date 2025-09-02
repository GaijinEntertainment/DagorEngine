//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>
#include <EASTL/vector_set.h>
#include <generic/dag_tab.h>

using BaseTextureIdSet =
  eastl::vector_set<TEXTUREID, eastl::less<TEXTUREID>, dag::MemPtrAllocator, dag::Vector<TEXTUREID, dag::MemPtrAllocator>>;

class TextureIdSet : public BaseTextureIdSet
{
public:
  TextureIdSet() = default;
  TextureIdSet(IMemAlloc *m) : BaseTextureIdSet(dag::MemPtrAllocator(m)) {}

  void reset() { clear(); }

  bool add(TEXTUREID tid) { return insert(tid).second; }
  bool del(TEXTUREID tid) { return erase(tid); }
  bool has(TEXTUREID tid) const { return find(tid) != end(); }
};
