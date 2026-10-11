// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_framemem.h>

#include <frontend/internalRegistry.h>
#include <frontend/dependencyData.h>
#include <frontend/validityInfo.h>
#include <frontend/nameResolver.h>
#include <frontend/irGraphBuilder.h>
#include <frontend/multiplexingInternal.h>
#include <backend/intermediateRepresentation.h>
#include <id/idRange.h>
#include <render/daFrameGraph/multiplexing.h>
#include <render/daFrameGraph/resourceCreation.h>
#include <render/daFrameGraph/detail/projectors.h>
#include <render/daFrameGraph/detail/resourceType.h>
#include <render/daFrameGraph/detail/rtti.h>
#include <drv/3d/dag_samplerHandle.h>
#include <drv/3d/dag_driverDesc.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "testRuntime.h"
#include <util/dag_convar.h>


namespace dafg
{
extern ConVarT<bool, false> randomize_order;
}

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
    auto changes =
      builder->build(graph, extents, extents, dafg::intermediate::Mapping{}, resourcesChanged, resourcesChanged, nodesChanged);
    lastMapping = graph.calculateMapping();
    return changes;
  }

  dafg::IrGraphBuilder::Changes rebuildNoChanges()
  {
    const auto nodeCount = registry.knownNames.nameCount<dafg::NodeNameId>();
    const auto resCount = registry.knownNames.nameCount<dafg::ResNameId>();
    dafg::IrGraphBuilder::NodesChanged nodesChanged(nodeCount, false);
    dafg::IrGraphBuilder::ResourcesChanged resourcesChanged(resCount, false);
    auto changes = builder->build(graph, extents, extents, lastMapping, resourcesChanged, resourcesChanged, nodesChanged);
    lastMapping = graph.calculateMapping();
    return changes;
  }

  dafg::IrGraphBuilder::Changes rebuildWithExtents(dafg::multiplexing::Extents new_extents)
  {
    const auto nodeCount = registry.knownNames.nameCount<dafg::NodeNameId>();
    const auto resCount = registry.knownNames.nameCount<dafg::ResNameId>();
    dafg::IrGraphBuilder::NodesChanged nodesChanged(nodeCount, false);
    dafg::IrGraphBuilder::ResourcesChanged resourcesChanged(resCount, false);
    const auto prevExtents = extents;
    extents = new_extents;
    auto changes = builder->build(graph, extents, prevExtents, lastMapping, resourcesChanged, resourcesChanged, nodesChanged);
    lastMapping = graph.calculateMapping();
    return changes;
  }

  dafg::IrGraphBuilder::Changes rebuildWithNodesAdded(std::initializer_list<dafg::NodeNameId> node_ids,
    std::initializer_list<dafg::ResNameId> lifetimes_changed_ids)
  {
    const auto nodeCount = registry.knownNames.nameCount<dafg::NodeNameId>();
    const auto resCount = registry.knownNames.nameCount<dafg::ResNameId>();
    dafg::IrGraphBuilder::NodesChanged nodesChanged(nodeCount, false);
    for (const auto nodeId : node_ids)
    {
      validityInfo.nodeValid.set(nodeId, true);
      nodesChanged.set(nodeId, true);
    }
    {
      FRAMEMEM_REGION;
      dafg::NameResolver::NodesChanged resolverNodesChanged(nodeCount, false);
      for (const auto nodeId : node_ids)
        resolverNodesChanged.set(nodeId, true);
      nameRes.update(resolverNodesChanged);
    }
    dafg::IrGraphBuilder::ResourcesChanged resourcesChanged(resCount, false);
    dafg::IrGraphBuilder::ResourcesChanged lifetimesChanged(resCount, false);
    for (const auto resId : lifetimes_changed_ids)
      lifetimesChanged.set(resId, true);
    auto changes = builder->build(graph, extents, extents, lastMapping, resourcesChanged, lifetimesChanged, nodesChanged);
    lastMapping = graph.calculateMapping();
    return changes;
  }

  dafg::IrGraphBuilder::Changes rebuildWithNodesRemoved(std::initializer_list<dafg::NodeNameId> node_ids,
    std::initializer_list<dafg::ResNameId> lifetimes_changed_ids)
  {
    const auto nodeCount = registry.knownNames.nameCount<dafg::NodeNameId>();
    const auto resCount = registry.knownNames.nameCount<dafg::ResNameId>();
    dafg::IrGraphBuilder::NodesChanged nodesChanged(nodeCount, false);
    for (const auto nodeId : node_ids)
    {
      validityInfo.nodeValid.set(nodeId, false);
      nodesChanged.set(nodeId, true);
    }
    dafg::IrGraphBuilder::ResourcesChanged resourcesChanged(resCount, false);
    dafg::IrGraphBuilder::ResourcesChanged lifetimesChanged(resCount, false);
    for (const auto resId : lifetimes_changed_ids)
      lifetimesChanged.set(resId, true);
    auto changes = builder->build(graph, extents, extents, lastMapping, resourcesChanged, lifetimesChanged, nodesChanged);
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
    auto changes = builder->build(graph, extents, extents, lastMapping, resourcesChanged, resourcesChanged, nodesChanged);
    lastMapping = graph.calculateMapping();
    return changes;
  }

  dafg::IrGraphBuilder::Changes rebuildWithDeclarationsChanged(std::initializer_list<dafg::NodeNameId> node_ids,
    std::initializer_list<dafg::ResNameId> res_ids)
  {
    const auto nodeCount = registry.knownNames.nameCount<dafg::NodeNameId>();
    const auto resCount = registry.knownNames.nameCount<dafg::ResNameId>();
    dafg::IrGraphBuilder::NodesChanged nodesChanged(nodeCount, false);
    for (const auto nodeId : node_ids)
      nodesChanged.set(nodeId, true);
    dafg::IrGraphBuilder::ResourcesChanged resourcesChanged(resCount, false);
    for (const auto resId : res_ids)
      resourcesChanged.set(resId, true);
    dafg::IrGraphBuilder::ResourcesChanged lifetimesChanged(resCount, false);
    auto changes = builder->build(graph, extents, extents, lastMapping, resourcesChanged, lifetimesChanged, nodesChanged);
    lastMapping = graph.calculateMapping();
    return changes;
  }

  dafg::ResNameId resourceId(const char *res_name) const
  {
    return registry.knownNames.getNameId<dafg::ResNameId>(registry.knownNames.root(), res_name);
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
    dafg::ResourceSubtypeTag projected_tag, dafg::detail::TypeErasedProjector projector,
    dafg::BindingType binding_type = dafg::BindingType::ShaderVar)
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
    binding.type = binding_type;
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

  for (auto idx : f.graph.nodes.keys())
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

  for (auto idx : f.graph.nodes.keys())
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
  for (auto &&[idx, node] : f.graph.nodes.enumerate())
  {
    if (IrGraphBuilderFixture::isDestinationSentinel(node))
    {
      CHECK(node.resourceRequests.empty());
      CHECK(node.supressedRequests.empty());
    }
  }

  // Check after rebuild
  f.rebuildNoChanges();
  for (auto &&[idx, node] : f.graph.nodes.enumerate())
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

  for (auto &&[idx, node] : f.graph.nodes.enumerate())
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
  for (auto &&[idx, node] : f.graph.nodes.enumerate())
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
  for (auto &&[idx, node] : f.graph.nodes.enumerate())
    CHECK(changes1.irNodesChanged[idx]);

  // Remove node A
  auto changes2 = f.rebuildWithNodeRemoved(nodeA);

  // Destination sentinel should still be clean
  for (auto &&[idx, node] : f.graph.nodes.enumerate())
  {
    if (IrGraphBuilderFixture::isDestinationSentinel(node))
    {
      CHECK(node.resourceRequests.empty());
      CHECK(node.supressedRequests.empty());
    }
  }

  // Destination sentinel's predecessors should have changed (node A was removed)
  bool dstFound = false;
  for (auto &&[idx, node] : f.graph.nodes.enumerate())
  {
    if (IrGraphBuilderFixture::isDestinationSentinel(node))
    {
      dstFound = true;
      CHECK(changes2.irNodesChanged[idx]);
    }
  }
  CHECK(dstFound);
}

TEST_CASE("a node inserted into a branch leaves the nodes scheduled before it untouched", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  const auto pb = f.addNodeCreatingResource("producer_b", "tex_b");
  const auto pa = f.addNodeCreatingResource("producer_a", "tex_a");
  const auto mod = f.addNodeModifyingResource("modifier_a", "tex_a", dafg::SideEffects::None);
  const auto ra = f.addNodeReadingResource("reader_a", "tex_a");
  const auto texA = f.registry.knownNames.getNameId<dafg::ResNameId>(f.registry.knownNames.root(), "tex_a");
  const auto texB = f.registry.knownNames.getNameId<dafg::ResNameId>(f.registry.knownNames.root(), "tex_b");
  f.registry.nodes[pa].readResources.insert(texB);
  f.registry.nodes[pa].resourceRequests[texB] = f.registry.nodes[ra].resourceRequests[texA];
  f.depData.resourceLifetimes[texB].readers.push_back(pa);
  f.finalize();
  f.validityInfo.nodeValid.set(mod, false);
  f.build();

  const auto slotOf = [&f](dafg::NodeNameId node) { return f.lastMapping.mapNode(node, dafg::intermediate::MultiplexingIndex{0}); };
  const auto slots = eastl::array{slotOf(pb), slotOf(pa)};

  const auto changes = f.rebuildWithNodesAdded({mod}, {texA});
  CHECK((eastl::array{slotOf(pb), slotOf(pa)} == slots));
  CHECK_FALSE(changes.irNodesChanged[slotOf(pb)]);
  CHECK(changes.irNodesChanged[slotOf(mod)]);
  CHECK(f.graph.nodes[slotOf(pa)].predecessors.contains(slotOf(pb)));
  CHECK(f.graph.nodes[slotOf(ra)].predecessors.contains(slotOf(mod)));
}

TEST_CASE("a new reader of two branches keeps the slots of both branches", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  const auto pa = f.addNodeCreatingResource("producer_a", "tex_a");
  const auto ra = f.addNodeReadingResource("reader_a", "tex_a");
  const auto pb = f.addNodeCreatingResource("producer_b", "tex_b");
  const auto rb = f.addNodeReadingResource("reader_b", "tex_b");
  const auto both = f.addNodeReadingResource("reader_ab", "tex_a");
  const auto texA = f.registry.knownNames.getNameId<dafg::ResNameId>(f.registry.knownNames.root(), "tex_a");
  const auto texB = f.registry.knownNames.getNameId<dafg::ResNameId>(f.registry.knownNames.root(), "tex_b");
  f.registry.nodes[both].readResources.insert(texB);
  f.registry.nodes[both].resourceRequests[texB] = f.registry.nodes[both].resourceRequests[texA];
  f.depData.resourceLifetimes[texB].readers.push_back(both);
  f.finalize();
  f.validityInfo.nodeValid.set(both, false);
  f.build();

  const auto slotOf = [&f](dafg::NodeNameId node) { return f.lastMapping.mapNode(node, dafg::intermediate::MultiplexingIndex{0}); };
  const auto slots = eastl::array{slotOf(pa), slotOf(ra), slotOf(pb), slotOf(rb)};

  const auto changes = f.rebuildWithNodesAdded({both}, {texA, texB});
  CHECK((eastl::array{slotOf(pa), slotOf(ra), slotOf(pb), slotOf(rb)} == slots));
  CHECK(changes.irNodesChanged[slotOf(both)]);
  CHECK(f.graph.nodes[slotOf(both)].predecessors.contains(slotOf(pa)));
  CHECK(f.graph.nodes[slotOf(both)].predecessors.contains(slotOf(pb)));
  CHECK(changes.irResourceRequestsChanged[f.lastMapping.mapRes(texA, dafg::intermediate::MultiplexingIndex{0})]);
  CHECK(changes.irResourceRequestsChanged[f.lastMapping.mapRes(texB, dafg::intermediate::MultiplexingIndex{0})]);
}

TEST_CASE("IR resources compare by value", "[irGraphBuilder]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  f.addNodeCreatingResource("producer", "tex");
  f.addNodeReadingResource("consumer", "tex");
  f.finalize();
  f.build();

  const auto resId = f.registry.knownNames.getNameId<dafg::ResNameId>(f.registry.knownNames.root(), "tex");
  const auto idx = f.lastMapping.mapRes(resId, dafg::intermediate::MultiplexingIndex{0});
  const dafg::intermediate::Resource before = f.graph.resources[idx];

  f.build();
  CHECK((f.graph.resources[idx] == before));

  auto otherActivation = before;
  auto &activation = otherActivation.asScheduled().getGpuDescription().asBasicRes.activation;
  activation = activation == ResourceActivationAction::DISCARD_AS_UAV ? ResourceActivationAction::DISCARD_AS_RTV_DSV
                                                                      : ResourceActivationAction::DISCARD_AS_UAV;
  CHECK_FALSE((otherActivation == before));

  f.registry.resources[resId].createdResData->clearValue = make_clear_value(1u, 0u, 0u, 0u);
  f.build();
  CHECK_FALSE((f.graph.resources[idx] == before));
}

TEST_CASE("blob callables compare by identity", "[irGraphBuilder]")
{
  int counter = 0;
  const auto increment = [&counter](void *) { ++counter; };
  const auto incrementCopy = increment;

  const dafg::intermediate::CtorFunc first{increment};
  const dafg::intermediate::CtorFunc same{increment};
  const dafg::intermediate::CtorFunc other{incrementCopy};
  const dafg::intermediate::CtorFunc empty{};

  CHECK((first == same));
  CHECK_FALSE((first == other));
  CHECK_FALSE((first == empty));
  CHECK((empty == dafg::intermediate::CtorFunc{}));
}

TEST_CASE("external resources compare every field of the texture info", "[irGraphBuilder]")
{
  using dafg::intermediate::BufferInfo;
  using dafg::intermediate::ExternalResource;

  TextureInfo info;
  info.w = 4;
  info.h = 8;
  info.d = 2;
  info.a = 6;
  info.mipLevels = 3;
  info.type = D3DResourceType::CUBETEX;
  info.isCommitted = 1;
  info.cflg = TEXFMT_R8G8B8A8;
  CHECK((ExternalResource{info} == ExternalResource{info}));

  const auto differsAfter = [&info](auto change) {
    auto other = info;
    change(other);
    return !(ExternalResource{other} == ExternalResource{info});
  };
  CHECK(differsAfter([](TextureInfo &i) { i.w++; }));
  CHECK(differsAfter([](TextureInfo &i) { i.h++; }));
  CHECK(differsAfter([](TextureInfo &i) { i.d++; }));
  CHECK(differsAfter([](TextureInfo &i) { i.a++; }));
  CHECK(differsAfter([](TextureInfo &i) { i.mipLevels++; }));
  CHECK(differsAfter([](TextureInfo &i) { i.type = D3DResourceType::TEX; }));
  CHECK(differsAfter([](TextureInfo &i) { i.isCommitted = 0; }));
  CHECK(differsAfter([](TextureInfo &i) { i.cflg |= TEXCF_RTARGET; }));

  CHECK_FALSE((ExternalResource{info} == ExternalResource{BufferInfo{0}}));
  CHECK((ExternalResource{BufferInfo{1}} == ExternalResource{BufferInfo{1}}));
  CHECK_FALSE((ExternalResource{BufferInfo{1}} == ExternalResource{BufferInfo{2}}));
}

TEST_CASE("blob resources compare by value across rebuilds", "[irGraphBuilder]")
{
  TestRuntime testRuntime{};
  auto &typeDb = dafg::Runtime::get().getTypeDb();
  typeDb.registerNativeType(dafg::tag_for<int>(), dafg::detail::make_rtti<int>());
  typeDb.registerNativeType(dafg::tag_for<float>(), dafg::detail::make_rtti<float>());

  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.addNodeCreatingResource("producer", "blob");
  f.addNodeReadingResource("consumer", "blob");
  const auto resId = f.registry.knownNames.getNameId<dafg::ResNameId>(f.registry.knownNames.root(), "blob");
  auto &created = *f.registry.resources[resId].createdResData;
  created.creationInfo = dafg::BlobDescription{dafg::tag_for<int>(), nullptr};
  created.type = dafg::ResourceType::Blob;
  f.finalize();
  f.build();

  const auto idx = f.lastMapping.mapRes(resId, dafg::intermediate::MultiplexingIndex{0});
  REQUIRE(f.graph.resources[idx].isScheduled());
  REQUIRE(f.graph.resources[idx].asScheduled().isCpuResource());
  const dafg::intermediate::Resource before = f.graph.resources[idx];

  f.build();
  CHECK((f.graph.resources[idx] == before));

  eastl::get<dafg::BlobDescription>(created.creationInfo).ctorOverride =
    eastl::make_unique<dafg::BlobDescription::CtorT>([](void *) {});
  f.build();
  CHECK_FALSE((f.graph.resources[idx] == before));

  created.creationInfo = dafg::BlobDescription{dafg::tag_for<float>(), nullptr};
  f.build();
  CHECK_FALSE((f.graph.resources[idx] == before));
}

TEST_CASE("a branch that leaves and comes back keeps the slots of the other branches", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  const auto p1 = f.addNodeCreatingResource("producer_1", "tex_1");
  const auto r1 = f.addNodeReadingResource("reader_1", "tex_1");
  const auto p2 = f.addNodeCreatingResource("producer_2", "tex_2");
  const auto r2 = f.addNodeReadingResource("reader_2", "tex_2");
  const auto p3 = f.addNodeCreatingResource("producer_3", "tex_3");
  const auto r3 = f.addNodeReadingResource("reader_3", "tex_3");
  f.finalize();
  f.build();

  const auto tex2 = f.registry.knownNames.getNameId<dafg::ResNameId>(f.registry.knownNames.root(), "tex_2");
  const auto slotOf = [&f](dafg::NodeNameId node) { return f.lastMapping.mapNode(node, dafg::intermediate::MultiplexingIndex{0}); };
  const auto resSlotOf = [&f](dafg::ResNameId res) { return f.lastMapping.mapRes(res, dafg::intermediate::MultiplexingIndex{0}); };
  const auto slots = eastl::array{slotOf(p1), slotOf(r1), slotOf(p3), slotOf(r3)};
  const auto tex2Slot = resSlotOf(tex2);

  const auto gone = f.rebuildWithNodesRemoved({p2, r2}, {tex2});
  for (const auto node : {p1, r1, p3, r3})
  {
    CHECK_FALSE(gone.irNodesChanged[slotOf(node)]);
  }
  CHECK((eastl::array{slotOf(p1), slotOf(r1), slotOf(p3), slotOf(r3)} == slots));
  CHECK_FALSE(gone.irResourceRequestsChanged.test(tex2Slot, false));

  const auto back = f.rebuildWithNodesAdded({p2, r2}, {tex2});
  for (const auto node : {p1, r1, p3, r3})
  {
    CHECK_FALSE(back.irNodesChanged[slotOf(node)]);
  }
  CHECK((eastl::array{slotOf(p1), slotOf(r1), slotOf(p3), slotOf(r3)} == slots));
  CHECK(back.irNodesChanged[slotOf(p2)]);
  CHECK(back.irNodesChanged[slotOf(r2)]);
  CHECK(back.irResourceRequestsChanged[resSlotOf(tex2)]);
  CHECK(back.irResourcesChanged[resSlotOf(tex2)]);
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

TEST_CASE("reader leaving a lifetime changes the requests but not the value", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  auto producer = f.addNodeCreatingResource("producer", "tex");
  auto readerA = f.addNodeReadingResource("reader_a", "tex");
  auto readerB = f.addNodeReadingResource("reader_b", "tex");
  f.finalize();
  f.build();

  const auto tex = f.resourceId("tex");
  auto changes = f.rebuildWithNodesRemoved({readerB}, {tex});

  const auto texIdx = f.lastMapping.mapRes(tex, {});
  CHECK(changes.irResourceRequestsChanged[texIdx]);
  CHECK_FALSE(changes.irResourcesChanged[texIdx]);

  CHECK_FALSE(changes.irNodesChanged[f.lastMapping.mapNode(producer, {})]);
  CHECK_FALSE(changes.irNodesChanged[f.lastMapping.mapNode(readerA, {})]);
}

TEST_CASE("modifier leaving a lifetime changes the value", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  auto producer = f.addNodeCreatingResource("producer", "tex");
  auto reader = f.addNodeReadingResource("reader", "tex");
  auto modifier = f.addNodeModifyingResource("modifier", "tex");
  f.finalize();
  f.build();

  const auto tex = f.resourceId("tex");
  auto changes = f.rebuildWithNodesRemoved({modifier}, {tex});

  const auto texIdx = f.lastMapping.mapRes(tex, {});
  CHECK(changes.irResourceRequestsChanged[texIdx]);
  CHECK(changes.irResourcesChanged[texIdx]);
  CHECK(changes.irNodesChanged[f.lastMapping.mapNode(producer, {})]);
  CHECK(changes.irNodesChanged[f.lastMapping.mapNode(reader, {})]);
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
  for (auto &&[idx, irNode] : f.graph.nodes.enumerate())
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
  for (auto &&[idx, irNode] : f.graph.nodes.enumerate())
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


TEST_CASE("bindless shader-var binding survives into IR with its type", "[irGraphBuilder][bindings]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // The IR builder must carry BindingType::BindlessShaderVar through unchanged.
  constexpr int kFakeShVarId = 8;
  f.addNodeCreatingResource("producer", "the_tex", dafg::SideEffects::External);
  auto consumerId = f.addNodeReadingAndBindingShVar("consumer", "the_tex", kFakeShVarId, /*is_optional*/ false,
    dafg::ResourceSubtypeTag::Invalid, &testShVarProjector, dafg::BindingType::BindlessShaderVar);

  f.finalize();
  f.build();

  bool foundBinding = false;
  for (auto &&[idx, irNode] : f.graph.nodes.enumerate())
  {
    if (irNode.frontendNode != consumerId)
      continue;

    const auto &bindings = f.graph.nodeStates[idx].bindings;
    auto it = bindings.find(kFakeShVarId);
    REQUIRE(it != bindings.end());
    CHECK(it->second.type == dafg::BindingType::BindlessShaderVar);
    CHECK(it->second.resource.has_value());
    CHECK_FALSE(it->second.optional);
    foundBinding = true;
  }
  CHECK(foundBinding);
}


TEST_CASE("missing optional bindless shader-var binding survives into IR with optional flag", "[irGraphBuilder][bindings]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // A bindless binding for a resource that no node produces. Like the plain
  // shader-var case, the IR builder must keep the binding (so the runtime sets
  // the index to -1) with no resource, the optional flag preserved and the
  // BindlessShaderVar type carried through unchanged.
  constexpr int kFakeShVarId = 9;
  auto consumerId = f.addNodeReadingAndBindingShVar("consumer", "missing_tex", kFakeShVarId, /*is_optional*/ true,
    dafg::ResourceSubtypeTag::Invalid, &testShVarProjector, dafg::BindingType::BindlessShaderVar);

  f.finalize();
  f.build();

  bool foundBinding = false;
  for (auto &&[idx, irNode] : f.graph.nodes.enumerate())
  {
    if (irNode.frontendNode != consumerId)
      continue;

    const auto &bindings = f.graph.nodeStates[idx].bindings;
    auto it = bindings.find(kFakeShVarId);
    REQUIRE(it != bindings.end());
    CHECK(it->second.type == dafg::BindingType::BindlessShaderVar);
    CHECK_FALSE(it->second.resource.has_value());
    CHECK(it->second.optional);
    foundBinding = true;
  }
  CHECK(foundBinding);
}


TEST_CASE("bindless sampler binding survives into IR with its SamplerHandle tag", "[irGraphBuilder][bindings]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  // A bindless sampler binds a SamplerHandle blob to an int var. Both the
  // BindlessShaderVar type and the SamplerHandle projected tag must survive into
  // the IR unchanged: the executor relies on that tag to route the binding to
  // register_bindless_sampler instead of the texture/buffer slot manager.
  constexpr int kFakeShVarId = 10;
  const auto kSamplerTag = dafg::tag_for<d3d::SamplerHandle>();
  f.addNodeCreatingResource("producer", "the_sampler", dafg::SideEffects::External);
  auto consumerId = f.addNodeReadingAndBindingShVar("consumer", "the_sampler", kFakeShVarId, /*is_optional*/ false, kSamplerTag,
    &testShVarProjector, dafg::BindingType::BindlessShaderVar);

  f.finalize();
  f.build();

  bool foundBinding = false;
  for (auto &&[idx, irNode] : f.graph.nodes.enumerate())
  {
    if (irNode.frontendNode != consumerId)
      continue;

    const auto &bindings = f.graph.nodeStates[idx].bindings;
    auto it = bindings.find(kFakeShVarId);
    REQUIRE(it != bindings.end());
    CHECK(it->second.type == dafg::BindingType::BindlessShaderVar);
    CHECK(it->second.resource.has_value());
    // The tag is what discriminates a bindless sampler from a bindless texture/buffer.
    CHECK(it->second.projectedTag == kSamplerTag);
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

TEST_CASE("declaration flagged without a change of value keeps every flag clear", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  auto producer = f.addNodeCreatingResource("producer", "tex");
  auto reader = f.addNodeReadingResource("reader", "tex");
  f.finalize();
  f.build();

  const auto tex = f.resourceId("tex");
  const auto texIdx = f.lastMapping.mapRes(tex, {});
  auto changes = f.rebuildWithDeclarationsChanged({}, {tex});

  CHECK(f.lastMapping.mapRes(tex, {}) == texIdx);
  CHECK_FALSE(changes.irResourcesChanged[texIdx]);
  CHECK_FALSE(changes.irResourceRequestsChanged[texIdx]);
  CHECK_FALSE(changes.irNodesChanged[f.lastMapping.mapNode(producer, {})]);
  CHECK_FALSE(changes.irNodesChanged[f.lastMapping.mapNode(reader, {})]);
}

TEST_CASE("declaration change of the created data flags the value and its requesters", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  auto producer = f.addNodeCreatingResource("producer", "tex");
  auto reader = f.addNodeReadingResource("reader", "tex");
  f.finalize();
  f.build();

  const auto tex = f.resourceId("tex");
  const auto texIdx = f.lastMapping.mapRes(tex, {});
  f.registry.resources[tex].createdResData->creationInfo = dafg::Texture2dCreateInfo{TEXFMT_A16B16G16R16F, IPoint2{8, 8}};
  auto changes = f.rebuildWithDeclarationsChanged({}, {tex});

  CHECK(f.lastMapping.mapRes(tex, {}) == texIdx);
  CHECK(changes.irResourcesChanged[texIdx]);
  CHECK_FALSE(changes.irResourceRequestsChanged[texIdx]);
  CHECK(changes.irNodesChanged[f.lastMapping.mapNode(producer, {})]);
  CHECK(changes.irNodesChanged[f.lastMapping.mapNode(reader, {})]);
}

TEST_CASE("reader leaving a renamed resource rebuilds the shared entry in place", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.addNodeCreatingResource("producer", "a");
  f.addNodeRenamingResource("renamer", "a", "b");
  auto readerB = f.addNodeReadingResource("reader_b", "b");
  auto readerB2 = f.addNodeReadingResource("reader_b2", "b");
  f.finalize();

  const auto a = f.resourceId("a");
  const auto b = f.resourceId("b");
  f.depData.renamingChains[a] = b;
  f.depData.renamingRepresentatives[b] = a;
  f.build();

  const auto sharedIdx = f.lastMapping.mapRes(a, {});
  REQUIRE(f.lastMapping.mapRes(b, {}) == sharedIdx);

  auto changes = f.rebuildWithNodesRemoved({readerB2}, {b});

  CHECK(f.lastMapping.mapRes(a, {}) == sharedIdx);
  CHECK(f.lastMapping.mapRes(b, {}) == sharedIdx);
  REQUIRE(f.graph.resources.isMapped(sharedIdx));
  const auto &chain = f.graph.resources[sharedIdx].frontendResources;
  REQUIRE(chain.size() == 2);
  CHECK(chain[0] == a);
  CHECK(chain[1] == b);
  CHECK(changes.irResourceRequestsChanged[sharedIdx]);
  CHECK_FALSE(changes.irResourcesChanged[sharedIdx]);
  CHECK_FALSE(changes.irNodesChanged[f.lastMapping.mapNode(readerB, {})]);
}

TEST_CASE("a creating node that gains a multiplexing index splits the shared entry", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.extents = {2, 1, 1, 1};
  auto producer = f.addNodeCreatingResource("producer", "tex");
  auto reader = f.addNodeReadingResource("reader", "tex");
  f.registry.nodes[producer].multiplexingMode = dafg::multiplexing::Mode::None;
  f.finalize();
  f.build();

  const auto tex = f.resourceId("tex");
  const auto secondMidx = dafg::multiplexing_index_to_ir({.superSample = 1}, f.extents);
  const auto sharedIdx = f.lastMapping.mapRes(tex, {});
  REQUIRE(f.lastMapping.mapRes(tex, secondMidx) == sharedIdx);

  f.registry.nodes[producer].multiplexingMode = dafg::multiplexing::Mode::SuperSampling;
  auto changes = f.rebuildWithDeclarationsChanged({producer}, {tex});

  CHECK(f.lastMapping.mapRes(tex, {}) == sharedIdx);
  const auto ownIdx = f.lastMapping.mapRes(tex, secondMidx);
  REQUIRE(ownIdx != sharedIdx);
  CHECK(changes.irResourcesChanged[sharedIdx]);
  CHECK(changes.irResourcesChanged[ownIdx]);

  const auto secondReaderIdx = f.lastMapping.mapNode(reader, secondMidx);
  REQUIRE(f.graph.nodes.isMapped(secondReaderIdx));
  const auto &requests = f.graph.nodes[secondReaderIdx].resourceRequests;
  CHECK(eastl::any_of(requests.begin(), requests.end(), [ownIdx](const auto &req) { return req.resource == ownIdx; }));
  CHECK(eastl::none_of(requests.begin(), requests.end(), [sharedIdx](const auto &req) { return req.resource == sharedIdx; }));
}

TEST_CASE("a reader keeps its own instance when the creator widens on a slower-varying axis", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.extents = {2, 1, 2, 1};
  auto producer = f.addNodeCreatingResource("producer", "tex");
  auto reader = f.addNodeReadingResource("reader", "tex");
  f.registry.nodes[producer].multiplexingMode = dafg::multiplexing::Mode::SuperSampling;
  f.registry.nodes[reader].multiplexingMode = dafg::multiplexing::Mode::SuperSampling | dafg::multiplexing::Mode::Viewport;
  f.finalize();
  f.build();

  const auto tex = f.resourceId("tex");
  const auto idx2 = dafg::multiplexing_index_to_ir({.superSample = 0, .subSample = 0, .viewport = 1}, f.extents);
  const auto clampedIdx = dafg::multiplexing_index_to_ir({.superSample = 0, .subSample = 0, .viewport = 0}, f.extents);

  const auto readerBefore = f.lastMapping.mapNode(reader, idx2);
  REQUIRE(f.graph.nodes.isMapped(readerBefore));
  const auto &requestsBefore = f.graph.nodes[readerBefore].resourceRequests;
  const auto heldBefore = f.lastMapping.mapRes(tex, clampedIdx);
  REQUIRE(
    eastl::any_of(requestsBefore.begin(), requestsBefore.end(), [heldBefore](const auto &req) { return req.resource == heldBefore; }));

  f.registry.nodes[producer].multiplexingMode = dafg::multiplexing::Mode::SuperSampling | dafg::multiplexing::Mode::Viewport;
  f.rebuildWithDeclarationsChanged({producer}, {tex});

  const auto ownIdx = f.lastMapping.mapRes(tex, idx2);
  REQUIRE(ownIdx != heldBefore);
  const auto readerAfter = f.lastMapping.mapNode(reader, idx2);
  REQUIRE(f.graph.nodes.isMapped(readerAfter));
  const auto &requests = f.graph.nodes[readerAfter].resourceRequests;
  CHECK(eastl::any_of(requests.begin(), requests.end(), [ownIdx](const auto &req) { return req.resource == ownIdx; }));
  CHECK(eastl::none_of(requests.begin(), requests.end(), [heldBefore](const auto &req) { return req.resource == heldBefore; }));
}

TEST_CASE("a creating node that loses a multiplexing index drops the unclaimed entry", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.extents = {2, 1, 1, 1};
  auto producer = f.addNodeCreatingResource("producer", "tex");
  auto reader = f.addNodeReadingResource("reader", "tex");
  f.registry.nodes[producer].multiplexingMode = dafg::multiplexing::Mode::SuperSampling;
  f.finalize();
  f.build();

  const auto tex = f.resourceId("tex");
  const auto secondMidx = dafg::multiplexing_index_to_ir({.superSample = 1}, f.extents);
  const auto firstIdx = f.lastMapping.mapRes(tex, {});
  const auto droppedIdx = f.lastMapping.mapRes(tex, secondMidx);
  REQUIRE(droppedIdx != firstIdx);

  f.registry.nodes[producer].multiplexingMode = dafg::multiplexing::Mode::None;
  f.rebuildWithDeclarationsChanged({producer}, {tex});

  CHECK(f.lastMapping.mapRes(tex, {}) == firstIdx);
  CHECK(f.lastMapping.mapRes(tex, secondMidx) == firstIdx);
  CHECK_FALSE(f.graph.resources.isMapped(droppedIdx));

  const auto secondReaderIdx = f.lastMapping.mapNode(reader, secondMidx);
  REQUIRE(f.graph.nodes.isMapped(secondReaderIdx));
  const auto &requests = f.graph.nodes[secondReaderIdx].resourceRequests;
  CHECK(eastl::any_of(requests.begin(), requests.end(), [firstIdx](const auto &req) { return req.resource == firstIdx; }));
  CHECK(eastl::none_of(requests.begin(), requests.end(), [droppedIdx](const auto &req) { return req.resource == droppedIdx; }));
}

TEST_CASE("reader leaving a lifetime does not flag a resource with declared but unread history", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  auto producer = f.addNodeCreatingResource("producer", "tex");
  auto readerA = f.addNodeReadingResource("reader_a", "tex");
  auto readerB = f.addNodeReadingResource("reader_b", "tex");
  const auto tex = f.resourceId("tex");
  f.registry.resources[tex].history = dafg::History::DiscardOnFirstFrame;
  f.finalize();
  f.build();

  auto changes = f.rebuildWithNodesRemoved({readerB}, {tex});

  const auto texIdx = f.lastMapping.mapRes(tex, {});
  CHECK(changes.irResourceRequestsChanged[texIdx]);
  CHECK_FALSE(changes.irResourcesChanged[texIdx]);
  CHECK(f.graph.resources[texIdx].asScheduled().history == dafg::History::No);
  CHECK_FALSE(changes.irNodesChanged[f.lastMapping.mapNode(producer, {})]);
  CHECK_FALSE(changes.irNodesChanged[f.lastMapping.mapNode(readerA, {})]);
}

TEST_CASE("no-op rebuild does not flag a renamed resource with declared but unread history", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.addNodeCreatingResource("producer", "a");
  f.addNodeRenamingResource("renamer", "a", "b");
  f.addNodeReadingResource("reader_b", "b");
  const auto a = f.resourceId("a");
  const auto b = f.resourceId("b");
  f.registry.resources[b].history = dafg::History::DiscardOnFirstFrame;
  f.finalize();

  f.depData.renamingChains[a] = b;
  f.depData.renamingRepresentatives[b] = a;
  f.build();

  const auto sharedIdx = f.lastMapping.mapRes(a, {});
  REQUIRE(f.lastMapping.mapRes(b, {}) == sharedIdx);
  CHECK(f.graph.resources[sharedIdx].asScheduled().history == dafg::History::No);

  auto changes = f.rebuildNoChanges();

  CHECK_FALSE(changes.irResourcesChanged[sharedIdx]);
  CHECK(f.graph.resources[sharedIdx].asScheduled().history == dafg::History::No);
}

TEST_CASE("randomized rebuilds keep the IR node change flags valid", "[irGraphBuilder][incremental]")
{
  // REQUIRE throws on failure, so restore the console variable on scope exit.
  struct RandomizeOrderScope
  {
    RandomizeOrderScope() { dafg::randomize_order.set(true); }
    ~RandomizeOrderScope() { dafg::randomize_order.set(false); }
  };

  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  // Enough independent nodes for the reshuffled IR slots to outgrow the
  // initial change flag capacity within a few hundred rebuilds.
  for (int i = 0; i < 62; ++i)
    f.addNode(eastl::string(eastl::string::CtorSprintf{}, "node_%d", i).c_str());
  f.finalize();
  f.build();

  // Mirrors dafg.randomize_order together with dafg.recompile_graph_every_frame.
  RandomizeOrderScope randomize;
  for (int i = 0; i < 300; ++i)
  {
    const auto changes = f.build();
    for (const auto idx : changes.irNodesChanged.trueKeys())
    {
      REQUIRE(f.graph.nodes.isMapped(idx));
    }
  }
}

TEST_CASE("color and depth resolve targets can be untracked", "[irGraphBuilder][untracked resources]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  const auto resolver = f.addNodeCreatingResource("resolver", "msaa_color");
  const auto msaaColor = f.resourceId("msaa_color");
  const auto msaaDepth = f.addResource("msaa_depth");
  const auto resolvedColor = f.addResource("resolved_color");
  const auto resolvedDepth = f.addResource("resolved_depth");

  auto &node = f.registry.nodes[resolver];
  node.resourceRequests[msaaDepth].usage = {dafg::Usage::DEPTH_ATTACHMENT, dafg::Access::READ_WRITE, dafg::Stage::PS};
  for (const auto resId : {resolvedColor, resolvedDepth})
    node.resourceRequests[resId].usage = {dafg::Usage::RESOLVE_ATTACHMENT, dafg::Access::READ_WRITE, dafg::Stage::TRANSFER};
  for (const auto resId : {msaaDepth, resolvedColor, resolvedDepth})
  {
    node.createdResources.insert(resId);
    f.depData.resourceLifetimes[resId].introducedBy = resolver;
  }

  auto &pass = node.renderingRequirements.emplace();
  pass.colorAttachments.push_back({msaaColor});
  pass.depthAttachment = {msaaDepth};
  pass.depthReadOnly = false;
  pass.resolves.emplace(msaaColor, resolvedColor);
  pass.resolves.emplace(msaaDepth, resolvedDepth);

  f.finalize();
  f.build();

  const auto isUntracked = [&f](dafg::ResNameId res_id) { return f.graph.resources[f.lastMapping.mapRes(res_id, {})].isUntracked(); };
  const bool untrackedSupported = d3d::get_driver_desc().caps.hasEnhancedResourceBarriers;
  CHECK(isUntracked(msaaColor) == untrackedSupported);
  CHECK(isUntracked(resolvedColor) == untrackedSupported);
  CHECK(isUntracked(msaaDepth) == untrackedSupported);
  CHECK(isUntracked(resolvedDepth) == untrackedSupported);
}

TEST_CASE("untracked buffer gets a UAV flag only when its history is cleared", "[irGraphBuilder][untracked resources]")
{
  const auto history = GENERATE(dafg::History::DiscardOnFirstFrame, dafg::History::ClearZeroOnFirstFrame);

  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  const auto producer = f.addNode("producer");
  const auto reader = f.addNode("reader");
  const auto buf = f.registry.knownNames.addNameId<dafg::ResNameId>(f.registry.knownNames.root(), "history_buf");
  f.registry.resources.set(buf, {.history = history,
                                  .createdResData = dafg::CreatedResourceData{
                                    .creationInfo = dafg::BufferCreateInfo{4, 1, 0, 0}, .type = dafg::ResourceType::Buffer}});
  f.validityInfo.resourceValid.set(buf, true);
  f.depData.resourceLifetimes.expandMapping(buf);
  f.depData.resourceLifetimes[buf].introducedBy = producer;
  f.depData.resourceLifetimes[buf].historyReaders.push_back(reader);

  f.registry.nodes[producer].createdResources.insert(buf);
  f.registry.nodes[producer].resourceRequests[buf].usage = {dafg::Usage::COPY, dafg::Access::READ_WRITE, dafg::Stage::TRANSFER};
  f.registry.nodes[reader].historyResourceReadRequests[buf].usage = {
    dafg::Usage::SHADER_RESOURCE, dafg::Access::READ_ONLY, dafg::Stage::PS};

  f.finalize();
  f.build();

  const auto &res = f.graph.resources[f.lastMapping.mapRes(buf, {})];
  const bool untracked = res.isUntracked();
  const bool unordered = (res.asScheduled().getGpuDescription().asBasicRes.cFlags & SBCF_BIND_UNORDERED) != 0;
  CHECK(untracked == d3d::get_driver_desc().caps.hasEnhancedResourceBarriers);
  CHECK(unordered == (untracked && history == dafg::History::ClearZeroOnFirstFrame));
}

TEST_CASE("a resource has no history pair on the first build and pairs with itself on a rebuild", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.addNodeCreatingResource("producer", "tex");
  f.addNodeReadingResource("reader_a", "tex");
  auto readerB = f.addNodeReadingResource("reader_b", "tex");
  f.finalize();
  auto first = f.build();
  const auto tex = f.resourceId("tex");
  const auto texIdx = f.lastMapping.mapRes(tex, {});
  CHECK(first.irHistoryPairing[texIdx] == dafg::intermediate::RESOURCE_NOT_MAPPED);

  auto changes = f.rebuildWithNodesRemoved({readerB}, {tex});

  CHECK(f.lastMapping.mapRes(tex, {}) == texIdx);
  CHECK(changes.irHistoryPairing[texIdx] == texIdx);
}

TEST_CASE("a rename chain that loses its last name has no history pair", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.addNodeCreatingResource("producer", "a");
  f.addNodeReadingResource("reader_a", "a");
  auto renamer = f.addNodeRenamingResource("renamer", "a", "b");
  f.finalize();
  const auto a = f.resourceId("a");
  const auto b = f.resourceId("b");
  f.depData.renamingChains[a] = b;
  f.depData.renamingRepresentatives[b] = a;
  f.build();
  const auto sharedIdx = f.lastMapping.mapRes(a, {});
  REQUIRE(f.lastMapping.mapRes(b, {}) == sharedIdx);

  f.depData.renamingChains[a] = a;
  f.depData.renamingRepresentatives[b] = b;
  f.validityInfo.resourceValid.set(b, false);
  auto changes = f.rebuildWithNodesRemoved({renamer}, {a});

  const auto aIdx = f.lastMapping.mapRes(a, {});
  REQUIRE(f.graph.resources.isMapped(aIdx));
  REQUIRE(f.graph.resources[aIdx].frontendResources.back() == a);
  CHECK(changes.irHistoryPairing[aIdx] == dafg::intermediate::RESOURCE_NOT_MAPPED);
}

TEST_CASE("a resource pruned with its producer has no history pair", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.addNodeCreatingResource("producer", "tex", dafg::SideEffects::Internal);
  auto reader = f.addNodeReadingResource("reader", "tex");
  f.addNode("sink");
  f.finalize();
  f.build();
  const auto tex = f.resourceId("tex");
  const auto texIdx = f.lastMapping.mapRes(tex, {});

  auto changes = f.rebuildWithNodesRemoved({reader}, {tex});

  REQUIRE_FALSE(f.graph.resources.isMapped(texIdx));
  for (const auto oldIdx : changes.irHistoryPairing)
    CHECK(oldIdx != texIdx);
}

TEST_CASE("an IR resource at the same multiplexing index keeps its history pair when the extents change",
  "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.addNodeCreatingResource("producer", "tex");
  f.addNodeReadingResource("reader", "tex");
  f.finalize();
  f.build();
  const auto tex = f.resourceId("tex");
  const auto texIdx = f.lastMapping.mapRes(tex, {});

  auto grown = f.rebuildWithExtents({2, 1, 1, 1});

  const auto firstIdx = f.lastMapping.mapRes(tex, {});
  const auto secondIdx = f.lastMapping.mapRes(tex, dafg::multiplexing_index_to_ir(
                                                     {
                                                       .superSample = 1,
                                                     },
                                                     f.extents));
  REQUIRE(f.graph.resources.isMapped(firstIdx));
  REQUIRE(f.graph.resources.isMapped(secondIdx));
  CHECK(grown.irHistoryPairing[firstIdx] == texIdx);
  CHECK(grown.irHistoryPairing[secondIdx] == dafg::intermediate::RESOURCE_NOT_MAPPED);

  auto shrunk = f.rebuildWithExtents({1, 1, 1, 1});

  const auto backIdx = f.lastMapping.mapRes(tex, {});
  REQUIRE(f.graph.resources.isMapped(backIdx));
  CHECK(shrunk.irHistoryPairing[backIdx] == firstIdx);
}

TEST_CASE("IR resources of the first sub camera keep their history pairs when a sub camera is added", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.extents = {2, 1, 1, 1};
  f.addNodeCreatingResource("producer", "tex");
  f.addNodeReadingResource("reader", "tex");
  f.finalize();
  f.build();
  const auto tex = f.resourceId("tex");
  const auto oldFirst = f.lastMapping.mapRes(tex, {});
  const auto oldSecond = f.lastMapping.mapRes(tex, dafg::multiplexing_index_to_ir(
                                                     {
                                                       .superSample = 1,
                                                     },
                                                     f.extents));
  REQUIRE(oldFirst != oldSecond);

  auto changes = f.rebuildWithExtents({2, 1, 1, 2});

  const auto first = f.lastMapping.mapRes(tex, dafg::multiplexing_index_to_ir(
                                                 {
                                                   .superSample = 0,
                                                   .subCamera = 0,
                                                 },
                                                 f.extents));
  const auto second = f.lastMapping.mapRes(tex, dafg::multiplexing_index_to_ir(
                                                  {
                                                    .superSample = 1,
                                                    .subCamera = 0,
                                                  },
                                                  f.extents));
  const auto third = f.lastMapping.mapRes(tex, dafg::multiplexing_index_to_ir(
                                                 {
                                                   .superSample = 0,
                                                   .subCamera = 1,
                                                 },
                                                 f.extents));
  const auto fourth = f.lastMapping.mapRes(tex, dafg::multiplexing_index_to_ir(
                                                  {
                                                    .superSample = 1,
                                                    .subCamera = 1,
                                                  },
                                                  f.extents));
  CHECK(changes.irHistoryPairing[first] == oldFirst);
  CHECK(changes.irHistoryPairing[second] == oldSecond);
  CHECK(changes.irHistoryPairing[third] == dafg::intermediate::RESOURCE_NOT_MAPPED);
  CHECK(changes.irHistoryPairing[fourth] == dafg::intermediate::RESOURCE_NOT_MAPPED);
}

TEST_CASE("an IR resource that splits out of a shared entry has no history pair", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.extents = {2, 1, 1, 1};
  auto producer = f.addNodeCreatingResource("producer", "tex");
  f.addNodeReadingResource("reader", "tex");
  f.registry.nodes[producer].multiplexingMode = dafg::multiplexing::Mode::None;
  f.finalize();
  f.build();

  const auto tex = f.resourceId("tex");
  const auto secondMidx = dafg::multiplexing_index_to_ir(
    {
      .superSample = 1,
    },
    f.extents);
  const auto sharedIdx = f.lastMapping.mapRes(tex, {});
  REQUIRE(f.lastMapping.mapRes(tex, secondMidx) == sharedIdx);

  f.registry.nodes[producer].multiplexingMode = dafg::multiplexing::Mode::SuperSampling;
  auto changes = f.rebuildWithDeclarationsChanged({producer}, {tex});

  const auto ownIdx = f.lastMapping.mapRes(tex, secondMidx);
  REQUIRE(f.lastMapping.mapRes(tex, {}) == sharedIdx);
  REQUIRE(ownIdx != sharedIdx);
  CHECK(changes.irHistoryPairing[sharedIdx] == sharedIdx);
  CHECK(changes.irHistoryPairing[ownIdx] == dafg::intermediate::RESOURCE_NOT_MAPPED);
}

TEST_CASE("a rename chain that keeps its last name pairs with its old index", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.addNodeCreatingResource("producer", "a");
  f.addNodeReadingResource("reader_a", "a");
  f.addNodeRenamingResource("renamer", "a", "b");
  f.addNodeReadingResource("reader_b", "b");
  auto secondReaderB = f.addNodeReadingResource("second_reader_b", "b");
  f.finalize();
  const auto a = f.resourceId("a");
  const auto b = f.resourceId("b");
  f.depData.renamingChains[a] = b;
  f.depData.renamingRepresentatives[b] = a;
  f.build();
  const auto sharedIdx = f.lastMapping.mapRes(a, {});
  REQUIRE(f.lastMapping.mapRes(b, {}) == sharedIdx);

  auto changes = f.rebuildWithNodesRemoved({secondReaderB}, {b});

  const auto newIdx = f.lastMapping.mapRes(b, {});
  REQUIRE(f.graph.resources.isMapped(newIdx));
  REQUIRE(f.lastMapping.mapRes(a, {}) == newIdx);
  REQUIRE(f.graph.resources[newIdx].frontendResources.front() == a);
  REQUIRE(f.graph.resources[newIdx].frontendResources.back() == b);
  REQUIRE(newIdx == sharedIdx);
  CHECK(changes.irHistoryPairing[newIdx] == sharedIdx);
}

TEST_CASE("a resource merged by a new rename pairs with the old resource of its last name", "[irGraphBuilder][incremental]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;
  f.addNodeCreatingResource("producer_a", "a");
  f.addNodeReadingResource("reader_a", "a");
  auto producerB = f.addNodeCreatingResource("producer_b", "b");
  f.addNodeReadingResource("reader_b", "b");
  auto renamer = f.addNode("renamer");
  f.finalize();
  f.build();
  const auto a = f.resourceId("a");
  const auto b = f.resourceId("b");
  const auto oldAIdx = f.lastMapping.mapRes(a, {});
  const auto oldBIdx = f.lastMapping.mapRes(b, {});
  REQUIRE(oldAIdx != oldBIdx);

  f.registry.nodes[renamer].renamedResources.emplace(b, a);
  f.depData.resourceLifetimes[a].consumedBy = renamer;
  f.depData.resourceLifetimes[b].introducedBy = renamer;
  f.depData.renamingChains[a] = b;
  f.depData.renamingRepresentatives[b] = a;
  f.validityInfo.nodeValid.set(producerB, false);
  {
    FRAMEMEM_REGION;
    dafg::NameResolver::NodesChanged resolverNodesChanged(f.registry.knownNames.nameCount<dafg::NodeNameId>(), false);
    resolverNodesChanged.set(producerB, true);
    resolverNodesChanged.set(renamer, true);
    f.nameRes.update(resolverNodesChanged);
  }

  auto changes = f.rebuildWithDeclarationsChanged({producerB, renamer}, {a, b});

  const auto mergedIdx = f.lastMapping.mapRes(a, {});
  REQUIRE(f.graph.resources.isMapped(mergedIdx));
  REQUIRE(f.lastMapping.mapRes(b, {}) == mergedIdx);
  REQUIRE(f.graph.resources[mergedIdx].frontendResources.back() == b);
  REQUIRE(mergedIdx != oldBIdx);
  CHECK(changes.irHistoryPairing[mergedIdx] == oldBIdx);
}

TEST_CASE("a resource named as debug is flagged in the IR and stays equal on a rebuild", "[irGraphBuilder]")
{
  FRAMEMEM_REGION;
  IrGraphBuilderFixture f;

  f.addNodeCreatingResource("producer", "gbuf_raw");
  f.addNodeRenamingResource("renamer", "gbuf_raw", "gbuf_Debug_view");
  f.addNodeReadingResource("consumer", "gbuf_Debug_view");
  f.addNodeCreatingResource("other", "gbuf");
  f.finalize();
  const auto raw = f.resourceId("gbuf_raw");
  const auto debugName = f.resourceId("gbuf_Debug_view");
  f.depData.renamingChains[raw] = debugName;
  f.depData.renamingRepresentatives[debugName] = raw;
  f.build();

  // Only a later name of the renaming chain is a debug one
  const auto plain = f.resourceId("gbuf");
  const auto debugIdx = f.lastMapping.mapRes(raw, dafg::intermediate::MultiplexingIndex{0});
  const auto plainIdx = f.lastMapping.mapRes(plain, dafg::intermediate::MultiplexingIndex{0});
  CHECK(f.graph.resources[debugIdx].debugRelated);
  CHECK_FALSE(f.graph.resources[plainIdx].debugRelated);

  const dafg::intermediate::Resource before = f.graph.resources[debugIdx];
  f.build();
  CHECK((f.graph.resources[debugIdx] == before));

  // A rebuild in place must not keep the flag of what the slot held before
  f.graph.resources[plainIdx].debugRelated = true;
  f.rebuildWithDeclarationsChanged({}, {plain});
  REQUIRE(f.lastMapping.mapRes(plain, dafg::intermediate::MultiplexingIndex{0}) == plainIdx);
  CHECK_FALSE(f.graph.resources[plainIdx].debugRelated);
}
