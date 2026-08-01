// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/algorithm.h>
#include <EASTL/unique_ptr.h>
#include <generic/dag_span.h>
#include <daECS/core/componentType.h>


namespace net
{
struct TemplateCachedCompLists
{
  eastl::unique_ptr<ecs::component_index_t[]> allComps;
  uint16_t ignoredCnt = 0, replicatedLocalCnt = 0, replicatedFilteredLocalCnt = 0;
  bool valid = false;

  dag::ConstSpan<ecs::component_index_t> getIgnored() const { return make_span_const(allComps.get(), ignoredCnt); }
  dag::ConstSpan<ecs::component_index_t> getReplicatedLocalIdx() const
  {
    return make_span_const(allComps.get() + ignoredCnt, replicatedLocalCnt);
  }
  dag::ConstSpan<ecs::component_index_t> getReplicatedFilteredLocalIdx() const
  {
    return make_span_const((allComps.get() + ignoredCnt) + replicatedLocalCnt, replicatedFilteredLocalCnt);
  }
  bool isIgnored(const ecs::component_index_t cidx) const
  {
    auto ignored = getIgnored();
    return eastl::binary_search(ignored.begin(), ignored.end(), cidx);
  }
};
} // namespace net
