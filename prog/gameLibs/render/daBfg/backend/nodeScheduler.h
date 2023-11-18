#pragma once

#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/span.h>

#include <memory/dag_framemem.h>

#include <common/graphDumper.h>
#include <backend/intermediateRepresentation.h>
#include <id/idIndexedFlags.h>


namespace dabfg
{

class NodeScheduler
{
public:
  NodeScheduler(IGraphDumper &dumper) : graphDumper{dumper} {}

  // old index -> new index mapping
  using NodePermutation = IdIndexedMapping<intermediate::NodeIndex, intermediate::NodeIndex, framemem_allocator>;
  NodePermutation schedule(const intermediate::Graph &graph);

private:
  IGraphDumper &graphDumper;
};

} // namespace dabfg
