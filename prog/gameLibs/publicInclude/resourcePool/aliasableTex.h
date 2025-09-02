//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "3d/dag_resPtr.h"
#include <EASTL/vector_map.h>
#include <cstdint>

// dynamically aliasable 2D texture baked by single memory area (heap)
// aliasing works only when heaps & resource aliasing are supported

namespace resptr_detail
{

using key_t = uint64_t;

namespace aliasable_detail
{
struct D3dDeleter
{
  void operator()(BaseTexture *tex) const { del_d3dres(tex); }
};
using UniqueBaseTex = eastl::unique_ptr<BaseTexture, D3dDeleter>;
} // namespace aliasable_detail

class AliasableManagedTex2D : public ManagedTex
{
public:
  using AliasMap = eastl::vector_map<key_t, aliasable_detail::UniqueBaseTex>;

protected:
  AliasMap aliasesMap;
  key_t currentKey = 0;
  resid_t lastResId = Helper<ManagedTex>::BAD_ID;
  BaseTexture *originalTexture = nullptr;

  void swap(AliasableManagedTex2D &other)
  {
    ManagedTex::swap(other);
    eastl::swap(aliasesMap, other.aliasesMap);
    eastl::swap(currentKey, other.currentKey);
    eastl::swap(lastResId, other.lastResId);
    eastl::swap(originalTexture, other.originalTexture);
  }
  AliasableManagedTex2D() = default;
  void calcKey();
  void recreate(int width, int height, int flags, int levels, key_t new_key);

public:
  void alias(int width, int height, int flags, int levels);
  void resize(int width, int height);
  uint32_t getResSizeBytes() const;
};

class AliasableManagedTex2DHolder : public ManagedResHolder<AliasableManagedTex2D>
{
protected:
  AliasableManagedTex2DHolder() = default;

public:
  void alias(int width, int height, int flags, int levels)
  {
    AliasableManagedTex2D::alias(width, height, flags, levels);
    setVar();
  }
};

using AliasableTex2D = UniqueRes<AliasableManagedTex2D>;
using AliasableTex2DHolder = ConcreteResHolder<UniqueRes<AliasableManagedTex2DHolder>>;

} // namespace resptr_detail

using AliasableTex2D = resptr_detail::AliasableTex2D;
using AliasableTex2DHolder = resptr_detail::AliasableTex2DHolder;
