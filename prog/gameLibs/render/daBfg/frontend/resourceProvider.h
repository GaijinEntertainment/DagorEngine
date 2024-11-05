// Copyright (C) Gaijin Games KFT.  All rights reserved.
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

struct MissingOptionalResource
{};
using ProvidedResource = eastl::variant<MissingOptionalResource, ManagedTexView, ManagedBufView, BlobView>;
using ProvidedResolution = eastl::variant<IPoint2, IPoint3>;

struct ResourceProvider
{
  dag::VectorMap<ResNameId, ProvidedResource> providedResources;
  dag::VectorMap<ResNameId, ProvidedResource> providedHistoryResources;

  IdIndexedMapping<AutoResTypeNameId, ProvidedResolution> resolutions;

  void clear()
  {
    providedResources.clear();
    providedHistoryResources.clear();
  }
};

} // namespace dabfg
