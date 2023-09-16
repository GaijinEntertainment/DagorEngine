#pragma once

#include <EASTL/variant.h>
#include <dag/dag_vectorMap.h>
#include <3d/dag_resPtr.h>
#include <render/daBfg/detail/resNameId.h>
#include <render/daBfg/detail/blob.h>


namespace dabfg
{

using ProvidedResource = eastl::variant<ManagedTexView, ManagedBufView, BlobView>;

struct ResourceProvider
{
  dag::VectorMap<ResNameId, ProvidedResource> providedResources;
  dag::VectorMap<ResNameId, ProvidedResource> providedHistoryResources;

  void clear()
  {
    providedResources.clear();
    providedHistoryResources.clear();
  }
};

} // namespace dabfg
