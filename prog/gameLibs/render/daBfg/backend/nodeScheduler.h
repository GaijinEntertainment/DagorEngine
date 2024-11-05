// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/span.h>

#include <memory/dag_framemem.h>

#include <common/graphDumper.h>
#include <backend/intermediateRepresentation.h>
#include <backend/passColoring.h>
#include <id/idIndexedFlags.h>


namespace dabfg
{

class NodeScheduler
{
public:
  // old index -> new index mapping
  using NodePermutation = IdIndexedMapping<intermediate::NodeIndex, intermediate::NodeIndex, framemem_allocator>;
  NodePermutation schedule(const intermediate::Graph &graph, const PassColoring &pass_coloring);
};

} // namespace dabfg
