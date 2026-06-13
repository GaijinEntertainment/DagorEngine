// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_framemem.h>

#include <frontend/internalRegistry.h>
#include <frontend/dependencyData.h>
#include <frontend/validityInfo.h>
#include <frontend/nameResolver.h>
#include <frontend/irGraphBuilder.h>
#include <backend/intermediateRepresentation.h>
#include <id/idRange.h>
#include <render/daFrameGraph/multiplexing.h>
#include <render/daFrameGraph/resourceCreation.h>
#include <render/daFrameGraph/detail/projectors.h>
#include <render/daFrameGraph/detail/resourceType.h>
#include <catch2/catch_test_macros.hpp>


namespace
{

struct IrGraphBuilderFixture
{
  dafg::ResourceProvider resourceProvider;
  dafg::InternalRegistry registry{resourceProvider};
  dafg::DependencyData depData;
  dafg::ValidityInfo validityInfo;
  dafg::NameResolver nameRes{registry};

  dafg::multiplexing::Extents extents{1, 1, 1, 1};
  dafg::intermediate::Graph graph;
  eastl::optional<dafg::IrGraphBuilder> builder;
  dafg::intermediate::Mapping lastMapping;

  void finalize()
  {
    const auto resCount = registry.knownNames.nameCount<dafg::ResNameId>();
    const auto nodeCount = registry.knownNames.nameCount<dafg::NodeNameId>();

    if (resCount > 0)
    {
      registry.resources.expandMapping(static_cast<dafg::ResNameId>(resCount - 1));
      registry.resourceSlots.expandMapping(static_cast<dafg::ResNameId>(resCount - 1));
      depData.resourceLifetimes.expandMapping(static_cast<dafg::ResNameId>(resCount - 1));
    }

    // Identity renaming: each resource maps to itself
    auto iotaRange = IdRange<dafg::ResNameId>(resCount);
    depData.renamingRepresentatives.assign(iotaRange.begin(), iotaRange.end());
    depData.renamingChains.assign(iotaRange.begin(), iotaRange.end());

    // Ensure validity flags are big enough
    validityInfo.resourceValid.resize(resCount, false);
    validityInfo.nodeValid.resize(nodeCount, false);

    // For tests with resources, update the name resolver
    if (resCount > 0)
    {
      FRAMEMEM_REGION;
      dafg::NameResolver::NodesChanged allNodesChanged(nodeCount, true);
      nameRes.update(allNodesChanged);
    }

    builder.emplace(registry, depData, validityInfo, nameRes);
  }

  dafg::IrGraphBuilder::Changes build()
  {
    const auto nodeCount = registry.knownNames.nameCount<dafg::NodeNameId>();
    const auto resCount = registry.knownNames.nameCount<dafg::ResNameId>();
    dafg::IrGraphBuilder::NodesChanged nodesChanged(nodeCount, true);
    dafg::IrGraphBuilder::ResourcesChanged resourcesChanged(resCount, true);
    auto changes = builder->build(graph, extents, extents, dafg::intermediate::Mapping{}, resourcesChanged, nodesChanged);
    lastMapping = graph.calculateMapping();
    return changes;
  }

  dafg::IrGraphBuilder::Changes rebuildNoChanges()
  {
    const auto nodeCount = registry.knownNames.nameCount<dafg::NodeNameId>();
    const auto resCount = registry.knownNames.nameCount<dafg::ResNameId>();
    dafg::IrGraphBuilder::NodesChanged nodesChanged(nodeCount, false);
    dafg::IrGraphBuilder::ResourcesChanged resourcesChanged(resCount, false);
    auto changes = builder->build(graph, extents, extents, lastMapping, resourcesChanged, nodesChanged);
    lastMapping = graph.calculateMapping();
    return changes;
  }

  dafg::IrGraphBuilder::Changes rebuildWithNodeRemoved(dafg::NodeNameId node_id)
  {
    const auto nodeCount = registry.knownNames.nameCount<dafg::NodeNameId>();
    const auto resCount = registry.knownNames.nameCount<dafg::ResNameId>();
    validityInfo.nodeValid.set(node_id, false);
    dafg::IrGraphBuilder::NodesChanged nodesChanged(nodeCount, false);
    nodesChanged.set(node_id, true);
    dafg::IrGraphBuilder::ResourcesChanged resourcesChanged(resCount, false);
    auto changes = builder->build(graph, extents, extents, lastMapping, resourcesChanged, nodesChanged);
    lastMapping = graph.calculateMapping();
    return changes;
  }


  dafg::NodeNameId addNode(const char *name, dafg::SideEffects side_effects = dafg::SideEffects::External)
  {
    auto root = registry.knownNames.root();
    auto nodeId = registry.knownNames.addNameId<dafg::NodeNameId>(root, name);

    registry.nodes.expandMapping(nodeId);
    registry.nodes[nodeId].sideEffect = side_effects;
    validityInfo.nodeValid.set(nodeId, true);

    return nodeId;
  }

  dafg::ResNameId addResource(const char *res_name)
  {
    auto root = registry.knownNames.root();
    auto resId = registry.knownNames.addNameId<dafg::ResNameId>(root, res_name);

    dafg::CreatedResourceData resData{
      .creationInfo = dafg::Texture2dCreateInfo{TEXFMT_R8G8B8A8, IPoint2{4, 4}},
      .type = dafg::ResourceType::Texture,
    };

    registry.resources.set(resId, {.createdResData = eastl::move(resData)});
    validityInfo.resourceValid.set(resId, true);

    depData.resourceLifetimes.expandMapping(resId);

    return resId;
  }

  dafg::ResNameId addSinkResource(const char *res_name)
  {
    auto resId = addResource(res_name);
    registry.sinkExternalResources.insert(resId);
    return resId;
  }

  dafg::NodeNameId addNodeCreatingResource(const char *node_name, const char *res_name,
    dafg::SideEffects side_effects = dafg::SideEffects::External)
  {
    auto nodeId = addNode(node_name, side_effects);
    auto resId = addResource(res_name);

    dafg::ResourceRequest req;
    req.usage = {dafg::Usage::COLOR_ATTACHMENT, dafg::Access::READ_WRITE, dafg::Stage::PS};

    registry.nodes[nodeId].createdResources.insert(resId);
    registry.nodes[nodeId].resourceRequests[resId] = req;
    depData.resourceLifetimes[resId].introducedBy = nodeId;

    return nodeId;
  }

  dafg::NodeNameId addNodeCreatingSinkResource(const char *node_name, const char *res_name,
    dafg::SideEffects side_effects = dafg::SideEffects::External)
  {
    auto nodeId = addNodeCreatingResource(node_name, res_name, side_effects);
    addSinkResource(res_name);

    return nodeId;
  }

  dafg::NodeNameId addNodeModifyingResource(const char *node_name, const char *res_name,
    dafg::SideEffects side_effects = dafg::SideEffects::External)
  {
    auto nodeId = addNode(node_name, side_effects);
    auto resId = addResource(res_name);

    dafg::ResourceRequest req;
    req.usage = {dafg::Usage::SHADER_RESOURCE, dafg::Access::READ_WRITE, dafg::Stage::PS};

    registry.nodes[nodeId].modifiedResources.insert(resId);
    registry.nodes[nodeId].resourceRequests[resId] = req;
    depData.resourceLifetimes[resId].modificationChain.push_back(nodeId);

    return nodeId;
  }

  dafg::NodeNameId addNodeReadingResource(const char *node_name, const char *res_name,
    dafg::SideEffects side_effects = dafg::SideEffects::External)
  {
    auto nodeId = addNode(node_name, side_effects);
    auto resId = addResource(res_name);

    dafg::ResourceRequest req;
    req.usage = {dafg::Usage::SHADER_RESOURCE, dafg::Access::READ_ONLY, dafg::Stage::PS};

    registry.nodes[nodeId].readResources.insert(resId);
    registry.nodes[nodeId].resourceRequests[resId] = req;
    depData.resourceLifetimes[resId].readers.push_back(nodeId);

    return nodeId;
  }

  dafg::NodeNameId addNodeRenamingResource(const char *node_name, const char *old_res_name, const char *new_res_name,
    dafg::SideEffects side_effects = dafg::SideEffects::External)
  {
    auto nodeId = addNode(node_name, side_effects);
    auto oldResId = addResource(old_res_name);
    auto newResId = addResource(new_res_name);

    registry.nodes[nodeId].renamedResources.emplace(newResId, oldResId);
    depData.resourceLifetimes[oldResId].consumedBy = nodeId;
    depData.resourceLifetimes[newResId].introducedBy = nodeId;

    return nodeId;
  }

  dafg::NodeNameId addNodeReadingAndBindingShVar(const char *node_name, const char *res_name, int sv_id, bool is_optional,
    dafg::ResourceSubtypeTag projected_tag, dafg::detail::TypeErasedProjector projector)
  {
    auto root = registry.knownNames.root();
    auto nodeId = registry.knownNames.addNameId<dafg::NodeNameId>(root, node_name);
    auto resId = registry.knownNames.addNameId<dafg::ResNameId>(root, res_name);

    registry.nodes.expandMapping(nodeId);
    auto &nodeData = registry.nodes[nodeId];
    nodeData.sideEffect = dafg::SideEffects::External;
    nodeData.readResources.insert(resId);

    dafg::ResourceRequest req;
    req.usage = {dafg::Usage::SHADER_RESOURCE, dafg::Access::READ_ONLY, dafg::Stage::PS};
    req.optional = is_optional;
    nodeData.resourceRequests[resId] = req;

    // Mirror what bindToShaderVar() actually stores: optional flag plus the
    // projected subtype tag and projector. NodeExecutor::bindShaderVar()
    // dispatches to bindBlob<T>() via projectedTag, and bindBlob<T>() is what
    // performs the "reset to T{}" when an optional resource is missing -- so
    // the IR builder must carry both fields through for the reset to fire.
    dafg::Binding binding;
    binding.type = dafg::BindingType::ShaderVar;
    binding.resource = resId;
    binding.optional = is_optional;
    binding.projectedTag = projected_tag;
    binding.projector = projector;
    nodeData.bindings[sv_id] = binding;

    validityInfo.nodeValid.set(nodeId, true);
    depData.resourceLifetimes.expandMapping(resId);
    depData.resourceLifetimes[resId].readers.push_back(nodeId);
    return nodeId;
  }

  // Source sentinel: no frontendNode, no predecessors
  static bool isSourceSentinel(const dafg::intermediate::Node &node) { return !node.frontendNode && node.predecessors.empty(); }

  // Destination sentinel: no frontendNode, has predecessors
  static bool isDestinationSentinel(const dafg::intermediate::Node &node) { return !node.frontendNode && !node.predecessors.empty(); }
};

} // namespace


TEST_CASE("first build marks all nodes changed", "[irGraphBuilder]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.addNode("node_a");
  f.addNode("node_b");
  f.finalize();
  auto changes = f.build();

  for (auto [idx, node] : f.graph.nodes.enumerate())
    CHECK(changes.irNodesChanged[idx]);
}

TEST_CASE("no-op rebuild marks nothing changed", "[irGraphBuilder]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.addNode("node_a");
  f.addNode("node_b");
  f.finalize();
  f.build();

  auto changes = f.rebuildNoChanges();

  for (auto [idx, node] : f.graph.nodes.enumerate())
    CHECK_FALSE(changes.irNodesChanged[idx]);
}

TEST_CASE("destination sentinel has empty requests", "[irGraphBuilder]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.addNode("node_a");
  f.addNode("node_b");
  f.finalize();
  f.build();

  // Check after first build
  for (auto [idx, node] : f.graph.nodes.enumerate())
  {
    if (IrGraphBuilderFixture::isDestinationSentinel(node))
    {
      CHECK(node.resourceRequests.empty());
      CHECK(node.supressedRequests.empty());
    }
  }

  // Check after rebuild
  f.rebuildNoChanges();
  for (auto [idx, node] : f.graph.nodes.enumerate())
  {
    if (IrGraphBuilderFixture::isDestinationSentinel(node))
    {
      CHECK(node.resourceRequests.empty());
      CHECK(node.supressedRequests.empty());
    }
  }
}

TEST_CASE("graph structure after first build", "[irGraphBuilder]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.addNode("node_a");
  f.addNode("node_b");
  f.finalize();
  f.build();

  // src sentinel + 2 frontend + dst sentinel = 4
  CHECK(f.graph.nodes.used() == 4);

  dafg::intermediate::NodeIndex srcIdx{};
  bool foundSrc = false;

  for (auto [idx, node] : f.graph.nodes.enumerate())
  {
    if (IrGraphBuilderFixture::isSourceSentinel(node))
    {
      srcIdx = idx;
      foundSrc = true;
      break;
    }
  }
  REQUIRE(foundSrc);

  // Frontend nodes should have the source sentinel in predecessors.
  // Destination sentinel should have both frontend nodes as predecessors.
  uint32_t frontendCount = 0;
  uint32_t dstPredecessorCount = 0;
  for (auto [idx, node] : f.graph.nodes.enumerate())
  {
    if (node.frontendNode)
    {
      CHECK(node.predecessors.count(srcIdx) == 1);
      ++frontendCount;
      continue;
    }

    if (IrGraphBuilderFixture::isDestinationSentinel(node))
      dstPredecessorCount = static_cast<uint32_t>(node.predecessors.size());
  }
  CHECK(frontendCount == 2);
  // Destination sentinel has all nodes without successors as predecessors.
  // Both frontend nodes + source sentinel have no incoming edges from other nodes,
  // so the destination sentinel gets the frontend nodes as predecessors.
  // (Source sentinel also has no incoming edges, but it has outgoing edges to frontend nodes,
  // so it DOES have successors and is NOT connected to dst sentinel.)
  CHECK(dstPredecessorCount == 2);
}

TEST_CASE("removing node leaves clean sentinel", "[irGraphBuilder]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  auto nodeA = f.addNode("node_a");
  f.addNode("node_b");
  f.finalize();

  // First build
  auto changes1 = f.build();
  for (auto [idx, node] : f.graph.nodes.enumerate())
    CHECK(changes1.irNodesChanged[idx]);

  // Remove node A
  auto changes2 = f.rebuildWithNodeRemoved(nodeA);

  // Destination sentinel should still be clean
  for (auto [idx, node] : f.graph.nodes.enumerate())
  {
    if (IrGraphBuilderFixture::isDestinationSentinel(node))
    {
      CHECK(node.resourceRequests.empty());
      CHECK(node.supressedRequests.empty());
    }
  }

  // Destination sentinel's predecessors should have changed (node A was removed)
  bool dstFound = false;
  for (auto [idx, node] : f.graph.nodes.enumerate())
  {
    if (IrGraphBuilderFixture::isDestinationSentinel(node))
    {
      dstFound = true;
      CHECK(changes2.irNodesChanged[idx]);
    }
  }
  CHECK(dstFound);
}

TEST_CASE("no-op rebuild with pruned nodes marks nothing changed", "[irGraphBuilder]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // Active chain: producer -> consumer (both have side effects, resource is used)
  f.addNodeCreatingResource("producer", "used_tex", dafg::SideEffects::External);
  f.addNodeReadingResource("consumer", "used_tex");

  // Pruned node: no side effects, resource has no consumers -- gets pruned from IR graph
  f.addNodeCreatingResource("pruned_node", "unused_tex", dafg::SideEffects::None);

  f.finalize();

  // First build: everything is new, all marked changed
  auto changes1 = f.build();

  // No-op rebuild: nothing changed in frontend
  auto changes2 = f.rebuildNoChanges();

  for (auto idx : changes2.irNodesChanged.trueKeys())
    CHECK_FALSE(changes2.irNodesChanged[idx]);
  for (auto idx : changes2.irResourcesChanged.trueKeys())
    CHECK_FALSE(changes2.irResourcesChanged[idx]);
}

namespace
{
// Standalone projector function so tests can identity-compare the pointer
// stored in the IR Binding against the one fed into the frontend Binding.
const void *testShVarProjector(const void *p) { return p; }
} // namespace

TEST_CASE("missing optional shader-var binding survives into IR with optional flag", "[irGraphBuilder][bindings]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // Mirrors the user-side scenario: a node reads a blob optionally and binds it
  // to a shader var, but no producer ever creates that resource. The IR builder
  // must still emit the binding (so the runtime can act on it) with no resource
  // index, the optional flag preserved, and crucially the projectedTag carried
  // through -- NodeExecutor::bindShaderVar() dispatches the "reset to T{}"
  // branch via that tag, so dropping it would silently kill the regression fix.
  constexpr int kFakeShVarId = 7;
  const auto kProjectedTag = dafg::tag_for<int>();
  auto consumerId = f.addNodeReadingAndBindingShVar("consumer", "missing_blob", kFakeShVarId, /*is_optional*/ true, kProjectedTag,
    &testShVarProjector);

  f.finalize();
  f.build();

  bool foundBinding = false;
  for (auto [idx, irNode] : f.graph.nodes.enumerate())
  {
    if (irNode.frontendNode != consumerId)
      continue;

    const auto &bindings = f.graph.nodeStates[idx].bindings;
    auto it = bindings.find(kFakeShVarId);
    REQUIRE(it != bindings.end());
    CHECK_FALSE(it->second.resource.has_value());
    CHECK(it->second.optional);
    CHECK_FALSE(it->second.reset);
    // Tag must survive even when the resource is missing, otherwise the
    // runtime can't pick the right bindBlob<T> to reset the shader var.
    CHECK(it->second.projectedTag == kProjectedTag);
    // IR builder intentionally nulls the projector when there is no resource
    // (it would never be invoked); see irGraphBuilder.cpp around line 1504.
    CHECK(it->second.projector == nullptr);
    foundBinding = true;
  }
  CHECK(foundBinding);
}

TEST_CASE("mandatory shader-var binding does not set IR optional flag", "[irGraphBuilder][bindings]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // Counter-test: a non-optional read with a producer present must yield a
  // binding with the resource mapped and optional == false, so the runtime
  // does not enter the reset branch. The projectedTag and projector must
  // both survive to the IR so bindBlob<T> can fetch and project the blob.
  constexpr int kFakeShVarId = 7;
  const auto kProjectedTag = dafg::tag_for<int>();
  f.addNodeCreatingResource("producer", "the_blob", dafg::SideEffects::External);
  auto consumerId =
    f.addNodeReadingAndBindingShVar("consumer", "the_blob", kFakeShVarId, /*is_optional*/ false, kProjectedTag, &testShVarProjector);

  f.finalize();
  f.build();

  bool foundBinding = false;
  for (auto [idx, irNode] : f.graph.nodes.enumerate())
  {
    if (irNode.frontendNode != consumerId)
      continue;

    const auto &bindings = f.graph.nodeStates[idx].bindings;
    auto it = bindings.find(kFakeShVarId);
    REQUIRE(it != bindings.end());
    CHECK(it->second.resource.has_value());
    CHECK_FALSE(it->second.optional);
    CHECK(it->second.projectedTag == kProjectedTag);
    CHECK(it->second.projector == &testShVarProjector);
    foundBinding = true;
  }
  CHECK(foundBinding);
}


TEST_CASE("no-effect reader is culled away", "[irGraphBuilder][pruning]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // create and modify
  f.addNodeCreatingResource("A", "texture");
  f.addNodeModifyingResource("B", "texture");

  // then read with no effect
  f.addNodeReadingResource("C", "texture", dafg::SideEffects::None);

  f.finalize();
  f.build();

  // C has no effect and should be culled away
  // 2 sentinels + A + B = 4
  CHECK(f.graph.nodes.used() == 4);
}

TEST_CASE("no-effect node, renaming resource for external node, survives pruning", "[irGraphBuilder][pruning]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // create and modify
  f.addNodeCreatingResource("A", "texture");
  f.addNodeModifyingResource("B", "texture");

  // no effect rename ...
  f.addNodeRenamingResource("C", "texture", "texture_2", dafg::SideEffects::None);

  // ..., that used by external node
  f.addNodeReadingResource("D", "texture_2", dafg::SideEffects::External);

  f.finalize();
  f.build();

  // C has no effect, but renames resource for ext-effect D
  // so everyone should survive
  // 2 sentinels + A + B + C + D = 6
  CHECK(f.graph.nodes.used() == 6);
}

TEST_CASE("no-effect renaming chain after last usage is culled away", "[irGraphBuilder][pruning]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // create and modify
  f.addNodeCreatingResource("A", "texture");
  f.addNodeModifyingResource("B", "texture");

  // a couple of renames with no effects
  f.addNodeRenamingResource("C", "texture", "texture_2", dafg::SideEffects::None);
  f.addNodeRenamingResource("D", "texture_2", "texture_3", dafg::SideEffects::None);

  f.finalize();
  f.build();

  // C and D rename resource, but it is not used
  // so they should be culled away
  // 2 sentinels + A + B = 4
  CHECK(f.graph.nodes.used() == 4);
}


TEST_CASE("internal nodes, operating with resource for external node, survive pruning", "[irGraphBuilder][pruning]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // create and modify resource with internal nodes
  f.addNodeCreatingResource("A", "texture", dafg::SideEffects::Internal);
  f.addNodeModifyingResource("B", "texture", dafg::SideEffects::Internal);

  // then read it with external node
  f.addNodeReadingResource("C", "texture", dafg::SideEffects::External);

  f.finalize();
  f.build();

  // A and B are int-effect, but operate resource, used by ext-effect node C,
  // so everyone should survive
  // 2 sentinels + A + B + C = 5
  CHECK(f.graph.nodes.used() == 5);
}

TEST_CASE("internal nodes, operating with resource for internal node, are culled away", "[irGraphBuilder][pruning]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // create and modify resource
  f.addNodeCreatingResource("A", "texture", dafg::SideEffects::External);
  f.addNodeModifyingResource("B", "texture", dafg::SideEffects::External);

  // create and modify another resource, but with internal nodes
  f.addNodeCreatingResource("C", "buffer", dafg::SideEffects::Internal);
  f.addNodeModifyingResource("D", "buffer", dafg::SideEffects::Internal);
  f.addNodeReadingResource("E", "buffer", dafg::SideEffects::Internal);

  f.finalize();
  f.build();

  // C, D, E - int-effect nodes, operating resource with no external uses
  // so they should be culled away
  // 2 sentinels + A + B = 4
  CHECK(f.graph.nodes.used() == 4);
}

TEST_CASE("internal reader after last external usage is culled away", "[irGraphBuilder][pruning]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // create and modify resource
  f.addNodeCreatingResource("A", "texture", dafg::SideEffects::External);
  f.addNodeModifyingResource("B", "texture", dafg::SideEffects::External);

  // then read, but with internal node
  f.addNodeReadingResource("C", "texture", dafg::SideEffects::Internal);

  f.finalize();
  f.build();

  // C is int-effect node, that actually has no effect
  // so it should be culled away
  // 2 sentinels + A + B = 4
  CHECK(f.graph.nodes.used() == 4);
}

TEST_CASE("internal rename/modify chain after last external usage is culled away", "[irGraphBuilder][pruning]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // create and modify resource
  f.addNodeCreatingResource("A", "texture", dafg::SideEffects::External);
  f.addNodeModifyingResource("B", "texture", dafg::SideEffects::External);

  // then rename and modify it twice, but with internal nodes
  f.addNodeRenamingResource("D", "texture", "texture_2", dafg::SideEffects::Internal);
  f.addNodeModifyingResource("E", "texture_2", dafg::SideEffects::Internal);
  f.addNodeRenamingResource("F", "texture_2", "texture_3", dafg::SideEffects::Internal);
  f.addNodeModifyingResource("G", "texture_3", dafg::SideEffects::Internal);

  f.finalize();
  f.build();

  // D, E, F, G - int-effect nodes, that operate resources with no external uses
  // so, they should be culled away
  // 2 sentinels + A + B = 4
  CHECK(f.graph.nodes.used() == 4);
}

TEST_CASE("sink resource make introducer survive pruning", "[irGraphBuilder][pruning]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // create and modify resource
  f.addNodeCreatingResource("A", "texture", dafg::SideEffects::External);
  f.addNodeModifyingResource("B", "texture", dafg::SideEffects::External);

  // create and read sink resource
  f.addNodeReadingResource("C", "texture", dafg::SideEffects::Internal);
  f.addNodeCreatingSinkResource("C", "buffer", dafg::SideEffects::Internal);

  f.finalize();
  f.build();

  // C is int-effect node, but introduces sink resource,
  // so it should survive
  // 2 sentinels + A + B + C = 5
  CHECK(f.graph.nodes.used() == 5);
}

TEST_CASE("sink resource make modifiers survive pruning", "[irGraphBuilder][pruning]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // create sink resource
  f.addNodeCreatingSinkResource("A", "texture", dafg::SideEffects::External);

  // modify it
  f.addNodeModifyingResource("B", "texture", dafg::SideEffects::Internal);
  f.addNodeModifyingResource("C", "texture", dafg::SideEffects::Internal);

  // and read
  f.addNodeReadingResource("D", "texture", dafg::SideEffects::Internal);

  f.finalize();
  f.build();

  // B and C are int-effect nodes, but modifiy sink resource,
  // so they should survive, unlike D, that dies
  // 2 sentinels + A + B + C = 5
  CHECK(f.graph.nodes.used() == 5);
}


TEST_CASE("no-effect creation and renaming chain requests are cleared", "[irGraphBuilder]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // no effect create and rename
  const auto creatorNode = f.addNodeCreatingResource("A", "texture", dafg::SideEffects::None);
  const auto renamerNode = f.addNodeRenamingResource("B", "texture", "texture_2", dafg::SideEffects::None);

  // and only then modify and read
  f.addNodeModifyingResource("C", "texture_2");
  f.addNodeReadingResource("D", "texture_2");

  f.finalize();
  f.build();

  // A and B have no effects, so resource requsts should be cleared for them
  bool hasRequests = false;
  for (auto [idx, irNode] : f.graph.nodes.enumerate())
    if (irNode.frontendNode && (*irNode.frontendNode == creatorNode || *irNode.frontendNode == renamerNode))
      hasRequests |= !irNode.resourceRequests.empty();
  CHECK(!hasRequests);
}
