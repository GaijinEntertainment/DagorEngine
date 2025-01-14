// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/optional.h>
#include <dag/dag_vectorSet.h>
#include <dag/dag_vectorMap.h>
#include <generic/dag_fixedVectorSet.h>

#include <render/daBfg/detail/nodeNameId.h>
#include <render/daBfg/detail/resNameId.h>
#include <common/graphDumper.h>
#include <id/idIndexedMapping.h>


namespace dabfg
{

struct InternalRegistry;
struct DependencyData;

// Responsible for registering nodes from various APIs
class NodeTracker final : public IGraphDumper
{
public:
  NodeTracker(InternalRegistry &reg, const DependencyData &deps) : registry{reg}, depData{deps} {}

public:
  // A context is an opaque token that can be used to wipe a group of nodes.
  // The intended use case is to pass a scripting language runtime and
  // wipe the nodes when a runtime is hot-reloaded.
  void registerNode(void *context, NodeNameId nodeId);
  void unregisterNode(NodeNameId nodeId, uint16_t gen);

  using ResourceWipeSet = dag::FixedVectorSet<ResNameId, 16>;
  // Note that when a node is implemented in a scripting language,
  // it can create blobs which contain context-managed data inside of
  // it, and so when the context is destroyed the blobs with history
  // must be carefully wiped too. This function returns the list of
  // history-enabled blobs returned by nodes which have been wiped.
  // Note that nullopt is returned when context was not used by any node.
  eastl::optional<ResourceWipeSet> wipeContextNodes(void *context);

  // Lazily initializes nodes
  void updateNodeDeclarations();

  bool acquireNodesChanged() { return eastl::exchange(nodesChanged, false); }

  void dumpRawUserGraph() const override;

  void lock()
  {
    G_ASSERT(nodeChangesLocked == false);
    nodeChangesLocked = true;
  }
  void unlock() { nodeChangesLocked = false; }

private:
  InternalRegistry &registry;
  const DependencyData &depData;

  dag::VectorSet<NodeNameId> deferredDeclarationQueue;
  IdIndexedMapping<NodeNameId, void *> nodeToContext;
  dag::VectorSet<void *> trackedContexts;

  // When a script unregisters a node right before deciding to destroy
  // it's context, we collect the resources that need to be wiped but
  // defer the wiping to when the context is actually being destroyed.
  dag::VectorMap<void *, ResourceWipeSet> deferredResourceWipeSets;

  bool nodesChanged{false};
  bool nodeChangesLocked{false};

  void checkChangesLock() const;

  void collectCreatedBlobs(NodeNameId node_id, ResourceWipeSet &into) const;

#if DAGOR_DBGLEVEL > 0
  uint32_t getProfileToken(NodeNameId nodeId) const;
#endif
};

} // namespace dabfg
