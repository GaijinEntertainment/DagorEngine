#pragma once

#include <EASTL/bitvector.h>
#include <dag/dag_vector.h>
#include <dag/dag_vectorSet.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/array.h>
#include <EASTL/span.h>

#include <util/dag_oaHashNameMap.h>
#include <memory/dag_framemem.h>

#include <frontend/resourceProvider.h>
#include <frontend/internalRegistry.h>
#include <frontend/nameResolver.h>
#include <backend/intermediateRepresentation.h>
#include <common/graphDumper.h>
#include <id/idIndexedMapping.h>
#include <id/idNameMap.h>
#include <id/idIndexedFlags.h>


namespace dabfg
{

// Responsible for communication with nodes
// * collecting node data
// * validating the graph structure
class NodeTracker final : public IGraphDumper
{
public:
  NodeTracker(InternalRegistry &reg, const NameResolver &res) : registry{reg}, nameResolver{res} {}

  // For node use only
public:
  static NodeTracker &get();

  void registerNode(NodeNameId nodeId);
  void unregisterNode(NodeNameId nodeId, uint32_t gen);

  // Lazily initializes nodes
  void declareNodes();
  // Collects resource and dependency info
  void gatherNodeData();

  // Returns an intermediate representation of the graph that
  // multiplexes nodes and resources (due to history resources,
  // stereo rendering, super/sub sampling, etc), groups nodes and
  // resources (due to subpasses and renaming modify), and culls out
  // unused or broken nodes/resources
  intermediate::Graph emitIR(multiplexing::Extents extents) const;

  bool acquireNodesChanged() { return eastl::exchange(nodesChanged, false); }

  intermediate::RequiredNodeState calcNodeState(NodeNameId node_id, intermediate::MultiplexingIndex multi_index,
    const intermediate::Mapping &mapping) const;

  void dumpRawUserGraph() const override;

  void lock()
  {
    G_ASSERT(nodeChangesLocked == false);
    nodeChangesLocked = true;
  }
  void unlock() { nodeChangesLocked = false; }

  // Temporarily public, to be refactored
  InternalRegistry &registry;

private:
  friend struct StateFieldSetter;
  friend struct DebugVisualizer;

  const NameResolver &nameResolver;

  dag::VectorSet<NodeNameId> deferredDeclarationQueue;

  bool nodesChanged{false};
  bool nodeChangesLocked{false};

  void checkChangesLock() const;


  // NOTE: All NameIds present in this structure are guaranteed to
  // refer to existing nodes by construction. INVALID_NAME_ID
  // means "no node" in this case, as not all resources go through
  // the complete lifetime cycle of produce-modify-read-consume.
  struct VirtualResourceLifetime
  {
    // Either produced or renamed from something else by this node
    NodeNameId introducedBy = NodeNameId::Invalid;

    // Chain of non-renaming modification
    eastl::fixed_vector<NodeNameId, 32> modificationChain;

    // List of nodes that want to read this resource before it gets consumed
    eastl::fixed_vector<NodeNameId, 32> readers;

    // A node that renames this resource (mutually exclusive with nextFrameReaders!!!)
    NodeNameId consumedBy = NodeNameId::Invalid;
  };

  // Note that this mapping is built w.r.t. resolved resource name ids
  IdIndexedMapping<ResNameId, VirtualResourceLifetime> resourceLifetimes;

  // First element in each modifying rename sequence
  IdIndexedMapping<ResNameId, ResNameId> renamingRepresentatives;
  // Mapping of resource -> what it gets renamed into
  IdIndexedMapping<ResNameId, ResNameId> renamingChains;

  void recalculateResourceLifetimes();
  void resolveRenaming();
  void updateRenamedResourceProperties();

  void fixupFalseHistoryFlags(intermediate::Graph &graph) const;

  struct ValidityInfo
  {
    IdIndexedFlags<ResNameId, framemem_allocator> resourceValid;
    IdIndexedFlags<NodeNameId, framemem_allocator> nodeValid;
  };
  bool validateResource(ResNameId resId) const;
  bool validateNode(NodeNameId resId) const;
  void validateLifetimes(ValidityInfo &validity) const;
  ValidityInfo findValidResourcesAndNodes() const;

  // Result contains no edges yet
  eastl::pair<intermediate::Graph, intermediate::Mapping> createDiscreteGraph(const ValidityInfo &validity,
    multiplexing::Extents extents) const;
  void setIrGraphDebugNames(intermediate::Graph &graph) const;
  // Actually adds the edges
  void addEdgesToIrGraph(intermediate::Graph &graph, const ValidityInfo &validity, const intermediate::Mapping &mapping) const;

  using SinkSet = dag::VectorSet<intermediate::NodeIndex, eastl::less<intermediate::NodeIndex>, framemem_allocator>;
  SinkSet findSinkIrNodes(const intermediate::Graph &graph, const intermediate::Mapping &mapping,
    eastl::span<const ResNameId> sinkResources) const;

  // Returns a displacement (old index -> new index) partial mapping
  IdIndexedMapping<intermediate::NodeIndex, intermediate::NodeIndex, framemem_allocator> pruneGraph(const intermediate::Graph &graph,
    const intermediate::Mapping &mapping, eastl::span<const intermediate::NodeIndex> sinkNodes, const ValidityInfo &validity) const;

  void dumpRawUserGraph(const ValidityInfo &info) const;
};

} // namespace dabfg
