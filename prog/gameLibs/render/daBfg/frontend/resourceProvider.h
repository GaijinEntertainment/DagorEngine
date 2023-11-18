#pragma once

#include <EASTL/variant.h>
#include <dag/dag_vectorMap.h>
#include <3d/dag_resPtr.h>
#include <math/integer/dag_IPoint2.h>

#include <render/daBfg/detail/resNameId.h>
#include <render/daBfg/detail/autoResTypeNameId.h>
#include <render/daBfg/detail/blob.h>
#include <id/idIndexedMapping.h>


namespace dabfg
{

using ProvidedResource = eastl::variant<ManagedTexView, ManagedBufView, BlobView>;

struct ResourceProvider
{
  dag::VectorMap<ResNameId, ProvidedResource> providedResources;
  dag::VectorMap<ResNameId, ProvidedResource> providedHistoryResources;

  IdIndexedMapping<AutoResTypeNameId, IPoint2> resolutions;

  void clear()
  {
    providedResources.clear();
    providedHistoryResources.clear();
  }
};

} // namespace dabfg
