// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodeTracker.h"

#include <debug/backendDebug.h>
#include <math/random/dag_random.h>

#include <frontend/dumpInternalRegistry.h>
#include <frontend/dependencyDataCalculator.h>
#include <frontend/internalRegistry.h>
#include <frontend/recompilationData.h>

#include <shaders/dag_computeShaders.h>
#include <shaders/dag_meshShaders.h>

#include <common/genericPoint.h>

#include <drv/3d/dag_draw.h>


namespace dafg
{

// Useful for ensuring that not dependencies are missing.
// Will shuffle nodes while preserving all ordering constraints.
// If anything depends on a "lucky" node order, this should break it.
CONSOLE_BOOL_VAL("dafg", randomize_order, false);

NodeTracker::PreRegisteredNameSpace *NodeTracker::preRegisteredNameSpaces = nullptr;
NodeTracker::PreRegisteredNode *NodeTracker::preRegisteredNodes = nullptr;

template <class T>
static T *steal_and_reverse_list(T *&list)
{
  auto *head = eastl::exchange(list, nullptr);

  T *prev = nullptr;
  while (head)
    head = eastl::exchange(head->next, eastl::exchange(prev, head));

  return prev;
}

NodeTracker::NodeTracker(InternalRegistry &reg, const DependencyData &deps, FrontendRecompilationData &recompData) :
  registry{reg}, depData{deps}, frontendRecompilationData{recompData}
{
  // We pre-generate IDs for namespaces/nodes when pre-registering them via
  // simple searches in the linked lists w/ strcmps (because registry is not)
  // initialized yet). These IDs are immediately given out to the user.
  // Afterwards, when the runtime is initialized and we have a registry,
  // we re-register every name in these lists and assert that the IDs match.

  // This is also why we traverse the lists in reverse order, first pre-registered
  // node/ns needs to add it's name to the registry first: FIFO.

  if (auto *head = steal_and_reverse_list(preRegisteredNameSpaces))
    while (head)
    {
      const auto id = registry.knownNames.addNameId<NameSpaceNameId>(head->parent, head->name.c_str());
      G_UNUSED(id);
      G_ASSERTF(head->nameId == id, "Pre-registration mechanism is broken!");
      delete eastl::exchange(head, head->next);
    }

  if (auto *head = steal_and_reverse_list(preRegisteredNodes))
    while (head)
    {
      const auto nodeId = registry.knownNames.addNameId<NodeNameId>(head->parent, head->name.c_str());
      G_ASSERTF(nodeId == head->nameId, "Pre-registration mechanism is broken!");

      registerNode(nullptr, nodeId);

      auto &nodeData = registry.nodes[nodeId];
      nodeData.declare = eastl::move(head->declare);
      nodeData.nodeSource = eastl::move(head->nodeSource);

      delete eastl::exchange(head, head->next);
    }
}

NameSpaceNameId NodeTracker::pre_register_name_space(NameSpaceNameId parent, const char *name)
{
  for (auto *pns = preRegisteredNameSpaces; pns; pns = pns->next)
    if (pns->parent == parent && pns->name == name)
      return pns->nameId;

  auto *pns = new PreRegisteredNameSpace;
  pns->parent = parent;
  pns->nameId = static_cast<NameSpaceNameId>(preRegisteredNameSpaces ? eastl::to_underlying(preRegisteredNameSpaces->nameId) + 1 : 1);
  pns->name = name;
  // Start from 1 because 0 is the root and it's already registered
  pns->next = eastl::exchange(preRegisteredNameSpaces, pns);

  return pns->nameId;
}

detail::NodeUid NodeTracker::pre_register_node(NameSpaceNameId parent, const char *name, const char *node_source,
  detail::DeclarationCallback declare)
{
  // Check that no name with this name was already pre-registered
  for (auto *pn = preRegisteredNodes; pn; pn = pn->next)
    if (pn->parent == parent && pn->name == name)
      G_ASSERT_FAIL("Pre-registration cannot handle re-creating nodes!");

  auto *pn = new PreRegisteredNode;
  pn->parent = parent;
  pn->nameId = static_cast<NodeNameId>(preRegisteredNodes ? eastl::to_underlying(preRegisteredNodes->nameId) + 1 : 0);
  pn->name = name;
  pn->nodeSource = node_source;
  pn->declare = eastl::move(declare);

  pn->next = eastl::exchange(preRegisteredNodes, pn);

  return detail::NodeUid{pn->nameId, 0, 1};
}

void NodeTracker::registerNode(void *context, NodeNameId nodeId)
{
  checkChangesLock();

  nodesChanged = true;

  deferredDeclarationQueue.emplace(nodeId);

  // Make sure that the id is mapped, as the nodes map is the
  // single point of truth here.
  registry.nodes.get(nodeId);

  nodeToContext.set(nodeId, context);
  trackedContexts.insert(context);
}

void NodeTracker::unregisterNode(NodeNameId nodeId, uint16_t gen)
{
  checkChangesLock();

  // If there was no such node in the first place, no need to
  // invalidate caches and try to clean up
  if (!registry.nodes.get(nodeId).declare)
    return;

  // If the node was already re-registered and generation incremented,
  // we shouldn't wipe the "new" node's data
  if (gen < registry.nodes[nodeId].generation)
    return;

  if (auto ctx = nodeToContext.get(nodeId))
    collectCreatedBlobs(nodeId, deferredResourceWipeSets[ctx]);

  nodesChanged = true;

  // In case the node didn't have a chance to declare resources yet,
  // clear from it the cache
  deferredDeclarationQueue.erase(nodeId);

  auto evictResId = [this](ResNameId res_id) {
    // Invalidate all renamed versions of this resource
    for (;;)
    {
      if (registry.resources.isMapped(res_id))
      {
        // History needs to be preserved, as it is determined by
        // the virtual resource itself and not inhereted from the
        // thing that was renamed into it. Everything else should be
        // reset to the defaults.
        auto &res = registry.resources.get(res_id);
        auto oldHist = res.history;
        res = {};
        res.history = oldHist;
      }

      if (!depData.renamingChains.isMapped(res_id) || depData.renamingChains[res_id] == res_id)
        break;

      res_id = depData.renamingChains[res_id];
    }
  };

  auto &nodeData = registry.nodes[nodeId];

  // Evict all resNameIds *produced* by this node
  for (const auto &[to, _] : nodeData.renamedResources)
    evictResId(to);

  for (const auto &resId : nodeData.createdResources)
    evictResId(resId);

  // Clear node data
  nodeData = {};
  nodeData.generation = gen + 1;
  nodeToContext.set(nodeId, {});

  unregisteredNodes.emplace(nodeId);
}

void NodeTracker::collectCreatedBlobs(NodeNameId node_id, ResourceWipeSet &into) const
{
  for (const auto resId : registry.nodes[node_id].createdResources)
  {
    if (registry.resources[resId].type != ResourceType::Blob)
      continue;
    into.insert(resId);
  }
}

eastl::optional<NodeTracker::ResourceWipeSet> NodeTracker::wipeContextNodes(void *context)
{
  const auto it = trackedContexts.find(context);
  if (it == trackedContexts.end())
    return eastl::nullopt;

  debug("daFG: Wiping nodes and resources managed by context %p...", context);

  NodeTracker::ResourceWipeSet result = eastl::move(deferredResourceWipeSets[context]);
  deferredResourceWipeSets[context].clear(); // moved-from state is valid but unspecified

  for (auto [nodeId, ctx] : nodeToContext.enumerate())
  {
    if (ctx != context)
      continue;

    collectCreatedBlobs(nodeId, result);

    unregisterNode(nodeId, registry.nodes[nodeId].generation);
    debug("daFG: Wiped node %s with context %p", registry.knownNames.getName(nodeId), ctx);
  }

  trackedContexts.erase(it);

  for (auto resId : result)
    debug("daFG: Resource %s needs to be wiped", registry.knownNames.getName(resId));

  return eastl::optional{eastl::move(result)};
}

static uint32_t get_argument_value(const BlobArg &arg, const dafg::ResourceProvider &provider)
{
  auto providedRes = provider.providedResources.find(arg.res);
  G_ASSERT_RETURN(providedRes != provider.providedResources.end(), 0);

  auto blob = eastl::get_if<BlobView>(&providedRes->second);
  G_ASSERT_RETURN(blob, 0);

  return *static_cast<const uint32_t *>(arg.projector(blob->data));
}

static uint32_t get_argument_value(const DispatchAutoResolutionArg &arg, const dafg::ResourceProvider &provider)
{
  auto value = eastl::visit(
    [&](const auto &res) {
      auto scaledResolution = scale_by(res, arg.multiplier);
      return arg.projector(&scaledResolution);
    },
    provider.resolutions[arg.autoResTypeId]);
  return *static_cast<const uint32_t *>(value);
}

static uint32_t get_argument_value(const DispatchTextureResolutionArg &arg, const dafg::ResourceProvider &provider)
{
  auto providedRes = provider.providedResources.find(arg.res);
  G_ASSERT_RETURN(providedRes != provider.providedResources.end(), 0);

  auto texture = eastl::get_if<ManagedTexView>(&providedRes->second);
  G_ASSERT_RETURN(texture, 0);
  TextureInfo info{};
  (*texture)->getinfo(info);
  IPoint3 resolution{info.w, info.h, info.d};

  return *static_cast<const uint32_t *>(arg.projector(&resolution));
}

static uint32_t get_argument_value(const uint32_t &arg, const dafg::ResourceProvider &) { return arg; }

static uint32_t get_argument_value(const eastl::monostate &, const dafg::ResourceProvider &) { return 0; }

template <typename T>
static bool is_valid_argument(const T &arg)
{
  return !eastl::holds_alternative<eastl::monostate>(arg);
}

template <typename T>
static detail::ExecutionCallback get_builtin_execute(const char *shader_name, dafg::InternalRegistry &registry, const T &args)
  requires(eastl::is_base_of_v<DispatchRequirements::DirectArgs, T>)
{
  static constexpr bool isComputeDispatch =
    eastl::is_same_v<DispatchRequirements::Threads, T> || eastl::is_same_v<DispatchRequirements::Groups, T>;
  using DispatchShader = eastl::conditional_t<isComputeDispatch, ComputeShader, MeshShader>;
  return [&registry, shader = DispatchShader(shader_name), args](dafg::multiplexing::Index) {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t z = 0;
    eastl::visit(
      [&](const auto &arg_x, const auto &arg_y, const auto &arg_z) {
        x = get_argument_value(arg_x, registry.resourceProviderReference);
        y = get_argument_value(arg_y, registry.resourceProviderReference);
        z = get_argument_value(arg_z, registry.resourceProviderReference);
      },
      args.x, args.y, args.z);

    if constexpr (eastl::is_base_of_v<T, DispatchRequirements::Threads>)
      shader.dispatchThreads(x, y, z);
    else if constexpr (eastl::is_base_of_v<T, DispatchRequirements::Groups>)
      shader.dispatchGroups(x, y, z);
  };
}

template <typename T>
static detail::ExecutionCallback get_builtin_execute(const char *shader_name, dafg::InternalRegistry &registry, const T &args)
  requires(eastl::is_base_of_v<DispatchRequirements::IndirectArgs, T>)
{
  static constexpr bool isComputeDispatch = eastl::is_same_v<DispatchRequirements::IndirectArgs, T>;
  static constexpr bool isMeshIndirect = eastl::is_same_v<DispatchRequirements::MeshIndirectArgs, T>;
  static constexpr bool isMeshMultiIndirect = eastl::is_same_v<DispatchRequirements::MeshIndirectCountArgs, T>;
  using DispatchShader = eastl::conditional_t<isComputeDispatch, ComputeShader, MeshShader>;

  if constexpr (!isComputeDispatch)
  {
    if (DAGOR_UNLIKELY(!is_valid_argument(args.stride)))
      logerr("daFG: Stride is not set for %s!", shader_name);
  }

  if constexpr (isMeshIndirect)
  {
    if (DAGOR_UNLIKELY(!is_valid_argument(args.count)))
      logerr("daFG: Count is not set for %s!", shader_name);
  }
  else if constexpr (isMeshMultiIndirect)
  {
    if (DAGOR_UNLIKELY(!is_valid_argument(args.maxCount)))
      logerr("daFG: Max count is not set for %s!", shader_name);
  }

  return [&registry, shader = DispatchShader(shader_name), &args](dafg::multiplexing::Index) {
    auto providedRes = registry.resourceProviderReference.providedResources.find(args.res);
    G_ASSERT(providedRes != registry.resourceProviderReference.providedResources.end());

    auto buffer = eastl::get_if<ManagedBufView>(&providedRes->second);
    G_ASSERT(buffer);

    uint32_t offset =
      eastl::visit([&](const auto &arg) { return get_argument_value(arg, registry.resourceProviderReference); }, args.offset);

    if constexpr (isComputeDispatch)
      shader.dispatchIndirect(buffer->getBuf(), offset);
    else
    {
      uint32_t stride =
        eastl::visit([&](const auto &arg) { return get_argument_value(arg, registry.resourceProviderReference); }, args.stride);

      if constexpr (isMeshIndirect)
      {
        uint32_t count =
          eastl::visit([&](const auto &arg) { return get_argument_value(arg, registry.resourceProviderReference); }, args.count);

        shader.dispatchIndirect(buffer->getBuf(), count, offset, stride);
      }
      else
      {
        auto providedCountRes = registry.resourceProviderReference.providedResources.find(args.countRes);
        G_ASSERT(providedCountRes != registry.resourceProviderReference.providedResources.end());

        auto countbuffer = eastl::get_if<ManagedBufView>(&providedCountRes->second);
        G_ASSERT(countbuffer);

        uint32_t countOffset =
          eastl::visit([&](const auto &arg) { return get_argument_value(arg, registry.resourceProviderReference); }, args.countOffset);

        uint32_t maxCount =
          eastl::visit([&](const auto &arg) { return get_argument_value(arg, registry.resourceProviderReference); }, args.maxCount);

        shader.dispatchIndirectCount(buffer->getBuf(), offset, stride, countbuffer->getBuf(), countOffset, maxCount);
      }
    }
  };
}

static detail::ExecutionCallback get_builtin_execute(const char *shader_name, dafg::InternalRegistry &registry,
  const DrawRequirements::DirectNonIndexedArgs &args)
{
  eastl::unique_ptr<ShaderMaterial> mat(new_shader_material_by_name(shader_name));
  eastl::unique_ptr<ShaderElement> elem(mat->make_elem());

  if (DAGOR_UNLIKELY(!is_valid_argument(args.primitiveCount)))
    logerr("daFG: Primitive count is not set for %s!", shader_name);

  return [&registry, mat = eastl::move(mat), elem = eastl::move(elem), &args](dafg::multiplexing::Index) {
    uint32_t startVertex = 0;
    uint32_t primitiveCount = 0;
    uint32_t instanceCount = 0;
    uint32_t startInstance = 0;
    eastl::visit(
      [&](const auto &start_vertex, const auto &instance_count, const auto &start_instance) {
        startVertex = get_argument_value(start_vertex, registry.resourceProviderReference);
        instanceCount = get_argument_value(instance_count, registry.resourceProviderReference);
        startInstance = get_argument_value(start_instance, registry.resourceProviderReference);
      },
      args.startVertex, args.instanceCount, args.startInstance);
    eastl::visit(
      [&](const auto &primitive_count) { primitiveCount = get_argument_value(primitive_count, registry.resourceProviderReference); },
      args.primitiveCount);

    elem->setStates();
    d3d::draw_instanced(get_d3d_primitive(args.primitive), startVertex, primitiveCount, instanceCount, startInstance);
  };
}

static detail::ExecutionCallback get_builtin_execute(const char *shader_name, dafg::InternalRegistry &registry,
  const DrawRequirements::DirectIndexedArgs &args)
{
  eastl::unique_ptr<ShaderMaterial> mat(new_shader_material_by_name(shader_name));
  eastl::unique_ptr<ShaderElement> elem(mat->make_elem());

  if (DAGOR_UNLIKELY(!is_valid_argument(args.primitiveCount)))
    logerr("daFG: Primitive count is not set for %s!", shader_name);

  return [&registry, mat = eastl::move(mat), elem = eastl::move(elem), &args](dafg::multiplexing::Index) {
    uint32_t startIndex = 0;
    uint32_t baseVertex = 0;
    uint32_t primitiveCount = 0;
    uint32_t instanceCount = 0;
    uint32_t startInstance = 0;
    eastl::visit(
      [&](const auto &start_index, const auto &base_vertex, const auto &instance_count, const auto &start_instance) {
        startIndex = get_argument_value(start_index, registry.resourceProviderReference);
        baseVertex = get_argument_value(base_vertex, registry.resourceProviderReference);
        instanceCount = get_argument_value(instance_count, registry.resourceProviderReference);
        startInstance = get_argument_value(start_instance, registry.resourceProviderReference);
      },
      args.startIndex, args.baseVertex, args.instanceCount, args.startInstance);
    eastl::visit(
      [&](const auto &primitive_count) { primitiveCount = get_argument_value(primitive_count, registry.resourceProviderReference); },
      args.primitiveCount);

    elem->setStates();
    d3d::drawind_instanced(get_d3d_primitive(args.primitive), startIndex, primitiveCount, baseVertex, instanceCount, startInstance);
  };
}

template <typename T>
  requires(eastl::is_base_of_v<DrawRequirements::IndirectArgs, T>)
static detail::ExecutionCallback get_builtin_execute(const char *shader_name, dafg::InternalRegistry &registry, const T &args)
{
  eastl::unique_ptr<ShaderMaterial> mat(new_shader_material_by_name(shader_name));
  eastl::unique_ptr<ShaderElement> elem(mat->make_elem());

  if constexpr (eastl::is_base_of_v<DrawRequirements::MultiIndirectArgs, T>)
  {
    if (DAGOR_UNLIKELY(!is_valid_argument(args.drawCount)))
      logerr("daFG: Draw count is not set for %s!", shader_name);
    if (DAGOR_UNLIKELY(!is_valid_argument(args.stride)))
      logerr("daFG: Stride is not set for %s!", shader_name);
  }

  return [&registry, mat = eastl::move(mat), elem = eastl::move(elem), &args](dafg::multiplexing::Index) {
    auto providedRes = registry.resourceProviderReference.providedResources.find(args.buffer);
    G_ASSERT(providedRes != registry.resourceProviderReference.providedResources.end());

    auto buffer = eastl::get_if<ManagedBufView>(&providedRes->second);
    G_ASSERT(buffer);

    size_t offset =
      eastl::visit([&](const auto &arg) { return get_argument_value(arg, registry.resourceProviderReference); }, args.offset);

    elem->setStates();

    if constexpr (eastl::is_same_v<T, DrawRequirements::IndirectNonIndexedArgs>)
      d3d::draw_indirect(get_d3d_primitive(args.primitive), buffer->getBuf(), offset);
    else if constexpr (eastl::is_same_v<T, DrawRequirements::IndirectIndexedArgs>)
      d3d::draw_indexed_indirect(get_d3d_primitive(args.primitive), buffer->getBuf(), offset);
    else
    {
      uint32_t stride = 0;
      uint32_t count = 0;
      eastl::visit(
        [&](const auto &strideArg, const auto &countArg) {
          stride = get_argument_value(strideArg, registry.resourceProviderReference);
          count = get_argument_value(countArg, registry.resourceProviderReference);
        },
        args.stride, args.drawCount);
      if constexpr (eastl::is_same_v<T, DrawRequirements::MultiIndirectNonIndexedArgs>)
        d3d::multi_draw_indirect(get_d3d_primitive(args.primitive), buffer->getBuf(), offset, count, stride);
      else if constexpr (eastl::is_same_v<T, DrawRequirements::MultiIndirectIndexedArgs>)
        d3d::multi_draw_indexed_indirect(get_d3d_primitive(args.primitive), buffer->getBuf(), offset, count, stride);
    }
  };
}

void NodeTracker::updateNodeDeclaration(NodeNameId node_id)
{
  frontendRecompilationData.nodeUnchanged.set(node_id, false);

  auto &node = registry.nodes[node_id];

  // Reset everything first to avoid funny side-effects later on
  if (node.execute)
  {
    auto declare = eastl::move(node.declare);
    const auto gen = node.generation;
    node = {};
    node.generation = gen;
    node.declare = eastl::move(declare);
  }

  if (auto &declare = node.declare)
    node.execute = declare(node_id, &registry);

  eastl::visit(
    [&](const auto &requirements) {
      using T = eastl::remove_cvref_t<decltype(requirements)>;
      if constexpr (!eastl::is_same_v<eastl::monostate, T>)
      {
        if (eastl::exchange(node.sideEffect, SideEffects::Internal) != SideEffects::None)
          logerr("daFG: Node %s has both execute request and execution callback! This is not allowed! Ignoring execution callback...",
            registry.knownNames.getName(node_id));

        eastl::string_view shaderName = registry.knownShaders.getName(requirements.shaderId);
        if (DAGOR_UNLIKELY(shaderName.empty()))
        {
          logerr("daFG: Node %s execute request has no shader set!", registry.knownNames.getName(node_id));
          return;
        }

        eastl::visit([&](const auto &args) { node.execute = get_builtin_execute(shaderName.cbegin(), registry, args); },
          requirements.arguments);
      }
    },
    node.executeRequirements);
}

void NodeTracker::updateNodeDeclarations()
{
  frontendRecompilationData.nodeUnchanged.clear();
  frontendRecompilationData.nodeUnchanged.resize(registry.knownNames.nameCount<NodeNameId>(), true);

  for (auto nodeId : unregisteredNodes)
    frontendRecompilationData.nodeUnchanged.set(nodeId, false);
  unregisteredNodes.clear();

  if (randomize_order.get())
    eastl::random_shuffle(deferredDeclarationQueue.begin(), deferredDeclarationQueue.end(),
      [](uint32_t n) { return static_cast<uint32_t>(grnd() % n); });

  for (auto nodeId : deferredDeclarationQueue)
    updateNodeDeclaration(nodeId);

  deferredDeclarationQueue.clear();

  // Makes sure that further code doesn't go out of bounds on any of these
  registry.resources.resize(registry.knownNames.nameCount<ResNameId>());
  registry.nodes.resize(registry.knownNames.nameCount<NodeNameId>());
  registry.autoResTypes.resize(registry.knownNames.nameCount<AutoResTypeNameId>());
  registry.resourceSlots.resize(registry.knownNames.nameCount<ResNameId>());

  // If we are recompiling the entire graph before contexts are destroyed,
  // the resources will be wiped automatically if need be.
  deferredResourceWipeSets.clear();
}

void NodeTracker::scheduleAllNodeRedeclaration()
{
  for (auto [nodeId, node] : registry.nodes.enumerate())
  {
    if (node.declare)
    {
      deferredDeclarationQueue.emplace(nodeId);
      unregisteredNodes.emplace(nodeId);
    }
  }
}

void NodeTracker::dumpRawUserGraph() const { dump_internal_registry(registry); }

void NodeTracker::checkChangesLock() const
{
  if (nodeChangesLocked)
  {
    logerr("daFG: Attempted to modify framegraph structure while it was being executed!"
           "This is not supported, see callstack and remove the modification!");
  }
}

} // namespace dafg
