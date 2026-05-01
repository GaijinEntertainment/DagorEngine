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

  dafg::NodeNameId addSideEffectNode(const char *name)
  {
    auto root = registry.knownNames.root();
    auto nodeId = registry.knownNames.addNameId<dafg::NodeNameId>(root, name);
    registry.nodes.expandMapping(nodeId);
    registry.nodes[nodeId].sideEffect = dafg::SideEffects::External;
    validityInfo.nodeValid.set(nodeId, true);
    return nodeId;
  }

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

  dafg::ResNameId addResource(const char *res_name)
  {
    auto root = registry.knownNames.root();
    auto resId = registry.knownNames.addNameId<dafg::ResNameId>(root, res_name);
    registry.resources.expandMapping(resId);
    dafg::CreatedResourceData resData;
    resData.creationInfo = dafg::Texture2dCreateInfo{TEXFMT_R8G8B8A8, IPoint2{4, 4}};
    resData.type = dafg::ResourceType::Texture;
    registry.resources[resId].createdResData = eastl::move(resData);
    validityInfo.resourceValid.set(resId, true);
    depData.resourceLifetimes.expandMapping(resId);
    return resId;
  }

  dafg::NodeNameId addNodeCreatingResource(const char *node_name, const char *res_name, dafg::SideEffects side_effects)
  {
    auto resId = addResource(res_name);

    auto root = registry.knownNames.root();
    auto nodeId = registry.knownNames.addNameId<dafg::NodeNameId>(root, node_name);
    registry.nodes.expandMapping(nodeId);
    registry.nodes[nodeId].sideEffect = side_effects;
    registry.nodes[nodeId].createdResources.insert(resId);

    dafg::ResourceRequest req;
    req.usage = {dafg::Usage::COLOR_ATTACHMENT, dafg::Access::READ_WRITE, dafg::Stage::PS};
    registry.nodes[nodeId].resourceRequests[resId] = req;

    validityInfo.nodeValid.set(nodeId, true);
    depData.resourceLifetimes.expandMapping(resId);
    depData.resourceLifetimes[resId].introducedBy = nodeId;
    return nodeId;
  }

  void addNodeReadingResource(const char *node_name, const char *res_name)
  {
    auto root = registry.knownNames.root();
    auto nodeId = registry.knownNames.addNameId<dafg::NodeNameId>(root, node_name);
    auto resId = registry.knownNames.addNameId<dafg::ResNameId>(root, res_name);

    registry.nodes.expandMapping(nodeId);
    registry.nodes[nodeId].sideEffect = dafg::SideEffects::External;
    registry.nodes[nodeId].readResources.insert(resId);

    dafg::ResourceRequest req;
    req.usage = {dafg::Usage::SHADER_RESOURCE, dafg::Access::READ_ONLY, dafg::Stage::PS};
    registry.nodes[nodeId].resourceRequests[resId] = req;

    validityInfo.nodeValid.set(nodeId, true);
    depData.resourceLifetimes[resId].readers.push_back(nodeId);
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
  f.addSideEffectNode("node_a");
  f.addSideEffectNode("node_b");
  f.finalize();
  auto changes = f.build();

  for (auto [idx, node] : f.graph.nodes.enumerate())
    CHECK(changes.irNodesChanged[idx]);
}

TEST_CASE("no-op rebuild marks nothing changed", "[irGraphBuilder]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.addSideEffectNode("node_a");
  f.addSideEffectNode("node_b");
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
  f.addSideEffectNode("node_a");
  f.addSideEffectNode("node_b");
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
  f.addSideEffectNode("node_a");
  f.addSideEffectNode("node_b");
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
  auto nodeA = f.addSideEffectNode("node_a");
  f.addSideEffectNode("node_b");
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
