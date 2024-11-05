// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <memory/dag_framemem.h>
#include <dag/dag_vectorSet.h>

#include <backend/intermediateRepresentation.h>


namespace dabfg
{

struct InternalRegistry;
struct DependencyData;
struct ValidityInfo;
class NameResolver;

class IrGraphBuilder
{
public:
  IrGraphBuilder(const InternalRegistry &reg, const DependencyData &depDataCalc, const ValidityInfo &valInf,
    const NameResolver &nameRes) :
    registry{reg}, depData{depDataCalc}, validityInfo{valInf}, nameResolver{nameRes}
  {}

  // Returns an intermediate representation of the graph that
  // multiplexes nodes and resources (due to history resources,
  // stereo rendering, super/sub sampling, etc), groups nodes and
  // resources (due to subpasses and renaming modify), and culls out
  // unused or broken nodes/resources
  intermediate::Graph build(multiplexing::Extents extents) const;

private:
  const InternalRegistry &registry;
  const DependencyData &depData;
  const ValidityInfo &validityInfo;
  const NameResolver &nameResolver;

  const char *frontendNodeName(const intermediate::Node &node) const;

  template <class F, class G>
  void scanPhysicalResourceUsages(ResNameId res_id, const F &process_reader, const G &process_modifier) const;
  auto findFirstUsageAndUpdatedCreationFlags(ResNameId res_id, uint32_t initial_flags) const;

  void addResourcesToGraph(intermediate::Graph &graph, intermediate::Mapping &mapping, multiplexing::Extents extents) const;
  void addNodesToGraph(intermediate::Graph &graph, intermediate::Mapping &mapping, multiplexing::Extents extents) const;
  void fixupFalseHistoryFlags(intermediate::Graph &graph) const;
  void setIrGraphDebugNames(intermediate::Graph &graph) const;
  // Actually adds the edges
  void addEdgesToIrGraph(intermediate::Graph &graph, const intermediate::Mapping &mapping) const;

  using SinkSet = dag::VectorSet<intermediate::NodeIndex, eastl::less<intermediate::NodeIndex>, framemem_allocator>;
  SinkSet findSinkIrNodes(const intermediate::Graph &graph, const intermediate::Mapping &mapping,
    eastl::span<const ResNameId> sinkResources) const;

  // Returns a displacement (old index -> new index) partial mapping
  using DisplacementFmem = IdIndexedMapping<intermediate::NodeIndex, intermediate::NodeIndex, framemem_allocator>;
  using EdgesToBreakFmem = dag::Vector<eastl::pair<intermediate::NodeIndex, intermediate::NodeIndex>, framemem_allocator>;
  eastl::pair<DisplacementFmem, EdgesToBreakFmem> pruneGraph(const intermediate::Graph &graph, const intermediate::Mapping &mapping,
    eastl::span<const intermediate::NodeIndex> sinksNodes) const;

  intermediate::RequiredNodeState calcNodeState(NodeNameId node_id, intermediate::MultiplexingIndex multi_index,
    const intermediate::Mapping &mapping) const;

  void dumpRawUserGraph() const;
};

} // namespace dabfg
