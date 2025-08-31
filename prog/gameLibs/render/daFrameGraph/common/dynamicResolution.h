// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/variant.h>
#include <render/daFrameGraph/detail/autoResTypeNameId.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <memory/dag_framemem.h>
#include <drv/3d/dag_heap.h>
#include <drv/3d/dag_tex3d.h>

#include "id/idIndexedMapping.h"

namespace dafg
{

template <class T>
struct ConcreteDynamicResolutionUpdate
{
  T dynamicRes;
  T staticRes;
};

template <class T>
ConcreteDynamicResolutionUpdate(T, T) -> ConcreteDynamicResolutionUpdate<T>;

struct NoDynamicResolutionUpdate
{};

using DynamicResolutionUpdate =
  eastl::variant<NoDynamicResolutionUpdate, ConcreteDynamicResolutionUpdate<IPoint2>, ConcreteDynamicResolutionUpdate<IPoint3>>;
using DynamicResolutionUpdates = IdIndexedMapping<AutoResTypeNameId, DynamicResolutionUpdate, framemem_allocator>;

using DynamicResolution = eastl::variant<eastl::monostate, IPoint2, IPoint3>;
using DynamicResolutions = IdIndexedMapping<AutoResTypeNameId, DynamicResolution, framemem_allocator>;

inline ResourceDescription set_tex_desc_resolution(const ResourceDescription &desc, int resolution, bool automip)
{
  ResourceDescription copy = desc;
  switch (copy.type)
  {
    case D3DResourceType::CUBETEX:
    case D3DResourceType::CUBEARRTEX:
      copy.asCubeTexRes.extent = resolution;
      if (automip)
        copy.asCubeTexRes.mipLevels = auto_mip_levels_count(resolution, 1);
      break;
    default: G_ASSERT(false); // impossible situation
  }
  return copy;
}

inline ResourceDescription set_tex_desc_resolution(const ResourceDescription &desc, const IPoint2 &resolution, bool automip)
{
  ResourceDescription copy = desc;
  switch (copy.type)
  {
    case D3DResourceType::TEX:
    case D3DResourceType::ARRTEX:
      copy.asTexRes.width = resolution.x;
      copy.asTexRes.height = resolution.y;
      if (automip)
        copy.asCubeTexRes.mipLevels = auto_mip_levels_count(resolution.x, resolution.y, 1);
      break;
    default: G_ASSERT(false); // impossible situation
  }
  return copy;
}

inline ResourceDescription set_tex_desc_resolution(const ResourceDescription &desc, const IPoint3 &resolution, bool automip)
{
  ResourceDescription copy = desc;
  switch (copy.type)
  {
    case D3DResourceType::VOLTEX:
      copy.asVolTexRes.width = resolution.x;
      copy.asVolTexRes.height = resolution.y;
      copy.asVolTexRes.depth = resolution.z;
      if (automip)
        copy.asCubeTexRes.mipLevels = auto_mip_levels_count(resolution.x, resolution.y, resolution.z, 1);
      break;
    default: G_ASSERT(false); // impossible situation
  }
  return copy;
}

} // namespace dafg
