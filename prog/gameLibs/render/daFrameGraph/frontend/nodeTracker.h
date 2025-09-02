// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/optional.h>
#include <dag/dag_vectorSet.h>
#include <dag/dag_vectorMap.h>
#include <generic/dag_fixedVectorSet.h>

#include <render/daFrameGraph/detail/daFG.h>
#include <render/daFrameGraph/detail/nodeNameId.h>
#include <render/daFrameGraph/detail/resNameId.h>
#include <render/daFrameGraph/detail/nameSpaceNameId.h>
#include <common/graphDumper.h>
#include <id/idIndexedMapping.h>


namespace dafg
{

struct InternalRegistry;
struct DependencyData;
struct FrontendRecompilationData;

// Responsible for registering nodes from various APIs
class NodeTracker final : public IGraphDumper
{
public:
  NodeTracker(InternalRegistry &reg, const DependencyData &deps, FrontendRecompilationData &recompData);

public:
  static NameSpaceNameId pre_register_name_space(NameSpaceNameId parent, const char *name);
  static detail::NodeUid pre_register_node(NameSpaceNameId parent, const char *name, const char *node_source,
    detail::DeclarationCallback declare);

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

  void scheduleAllNodeRedeclaration();

  bool acquireNodesChanged() { return eastl::exchange(nodesChanged, false); }

  void dumpRawUserGraph() const override;

  void lock()
  {
    G_ASSERT(nodeChangesLocked == false);
    nodeChangesLocked = true;
  }
  bool isLocked() const { return nodeChangesLocked; }
  void unlock() { nodeChangesLocked = false; }

private:
  InternalRegistry &registry;
  const DependencyData &depData;
  FrontendRecompilationData &frontendRecompilationData;

  dag::VectorSet<NodeNameId> unregisteredNodes;

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
  void updateNodeDeclaration(NodeNameId node_id);

private:
  struct PreRegisteredNameSpace
  {
    NameSpaceNameId parent;
    NameSpaceNameId nameId;
    eastl::string name;
    PreRegisteredNameSpace *next;
  };

  static PreRegisteredNameSpace *preRegisteredNameSpaces;

  struct PreRegisteredNode
  {
    NameSpaceNameId parent;
    NodeNameId nameId;
    eastl::string name;
    eastl::string nodeSource;
    detail::DeclarationCallback declare;

    PreRegisteredNode *next;
  };

  static PreRegisteredNode *preRegisteredNodes;
};

} // namespace dafg
