// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "irGraphBuilder.h"

#include <memory/dag_framemem.h>
#include <util/dag_convar.h>
#include <math/random/dag_random.h>
#include <generic/dag_enumerate.h>
#include <generic/dag_fixedVectorSet.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/stack.h>

#include <runtime/runtime.h>
#include <frontend/multiplexingInternal.h>
#include <frontend/dumpInternalRegistry.h>
#include <frontend/dependencyData.h>
#include <frontend/validityInfo.h>
#include <frontend/nameResolver.h>
#include <common/resourceUsage.h>


namespace dafg
{

extern ConVarT<bool, false> randomize_order;
extern ConVarT<bool, false> pedantic;
extern ConVarT<bool, false> verbose;

void IrGraphBuilder::fixupFlagsAndActivation(ResNameId id, ResourceType type, intermediate::ClearStage &clear_stage,
  ResourceDescription &desc) const
{
  auto [firstUsage, updatedFlags, resourceUsageInfo] = findFirstUsageAndUpdatedCreationFlags(id, desc.asBasicRes.cFlags);

  const bool clearedOnActivation = clear_stage != intermediate::ClearStage::None && !resourceUsageInfo.clearedInRenderPass;

  const auto &createdResData = registry.resources[id].createdResData;

  auto desiredBehavior = !eastl::holds_alternative<eastl::monostate>(createdResData->clearValue) ? DesiredActivationBehaviour::Clear
                                                                                                 : DesiredActivationBehaviour::Discard;
  if (clearedOnActivation)
    clear_stage = intermediate::ClearStage::Activation;
  const auto channels = get_tex_format_desc(desc.asBasicRes.cFlags & TEXFMT_MASK).mainChannelsType;
  const bool is_int = channels == ChannelDType::UINT || channels == ChannelDType::SINT;
  auto activation = get_activation_from_usage(desiredBehavior, firstUsage, type, is_int);

  desc.asBasicRes.cFlags = updatedFlags;
  if (activation.has_value())
  {
    if (clear_stage == intermediate::ClearStage::RenderPass && *activation == ResourceActivationAction::CLEAR_AS_RTV_DSV)
    {
      // TODO: Support correct discards in clear_render_pass method to enable the same thing for
      // ResourceActivationAction::DISCARD_AS_RTV_DSV.
      *activation = ResourceActivationAction::REWRITE_AS_RTV_DSV;
    }
    desc.asBasicRes.activation = *activation;
  }
  else
  {
    G_ASSERTF(firstUsage.type == Usage::UNKNOWN,
      "daFG: Resource %s has a first modify usage, but no activation action could be inferred from it! "
      "This is a bug in the frame graph!",
      registry.knownNames.getName(id));

    if (!resourceUsageInfo.hasReaders && !resourceUsageInfo.hasModifiers) // just unused resource (with or without renames)
      logwarn("daFG: Could not infer an activation action for %s. It is not initialized by its producer,"
              " but no node reads or modifies it, the resource will be prunned.",
        registry.knownNames.getName(id));
    else if (resourceUsageInfo.hasReaders)
      logerr("daFG: Could not infer an activation action for %s."
             " It was read without being initialized by neither the producer nor any modifiers beforehand."
             " See above for which nodes read the uninitialized resource."
             " Please specify a proper usage and stage in at least one producer/modifier/renamer node!",
        registry.knownNames.getName(id));
    else if (resourceUsageInfo.hasModifiers)
      logerr("daFG: Could not infer an activation action for %s."
             " Please specify a proper usage and stage in at least one producer/modifier/renamer node!",
        registry.knownNames.getName(id));

    if (type == ResourceType::Texture)
    {
      if (desc.asBasicRes.cFlags & TEXCF_RTARGET)
        desc.asBasicRes.activation = ResourceActivationAction::DISCARD_AS_RTV_DSV;
      else if (desc.asBasicRes.cFlags & TEXCF_UNORDERED)
        desc.asBasicRes.activation = ResourceActivationAction::DISCARD_AS_UAV;
      else
        desc.asBasicRes.activation = ResourceActivationAction::REWRITE_AS_COPY_DESTINATION;
    }
    else if (type == ResourceType::Buffer)
      desc.asBasicRes.activation = ResourceActivationAction::DISCARD_AS_UAV;
  }
}

ResourceDescription IrGraphBuilder::createInfoToResDesc(ResNameId id, const Texture2dCreateInfo &info,
  intermediate::ClearStage &clear_stage) const
{
  TextureResourceDescription desc{};

  desc.cFlags = info.creationFlags;
  desc.mipLevels = info.mipLevels;

  ResourceDescription result;
  // NOTE: auto-resolution + auto-mips is a special case handled
  // right before resource scheduling.
  if (auto p = eastl::get_if<IPoint2>(&info.resolution))
    result = set_tex_desc_resolution(desc, *p, info.mipLevels == AUTO_MIP_COUNT);
  else
    result = desc;
  fixupFlagsAndActivation(id, ResourceType::Texture, clear_stage, result);
  return result;
}

ResourceDescription IrGraphBuilder::createInfoToResDesc(ResNameId id, const Texture3dCreateInfo &info,
  intermediate::ClearStage &clear_stage) const
{
  VolTextureResourceDescription desc{};

  desc.mipLevels = info.mipLevels;
  desc.cFlags = info.creationFlags;

  ResourceDescription result;
  if (auto p = eastl::get_if<IPoint3>(&info.resolution))
    result = set_tex_desc_resolution(desc, *p, info.mipLevels == AUTO_MIP_COUNT);
  else
    result = desc;
  fixupFlagsAndActivation(id, ResourceType::Texture, clear_stage, result);
  return result;
}

ResourceDescription IrGraphBuilder::createInfoToResDesc(ResNameId id, const BufferCreateInfo &info,
  intermediate::ClearStage &clear_stage) const
{
  BufferResourceDescription desc{};

  desc.elementCount = info.elementCount;
  desc.elementSizeInBytes = info.elementSize;
  desc.viewFormat = info.format;
  desc.cFlags = info.flags;
  desc.activation = ResourceActivationAction::DISCARD_AS_UAV;
  G_ASSERTF(clear_stage == intermediate::ClearStage::None,
    "Buffer resources cannot be cleared, but a clear value was provided for resource %s! "
    "This is a bug in the frame graph, please report it to programmers!",
    registry.knownNames.getName(id));

  ResourceDescription result{desc};
  fixupFlagsAndActivation(id, ResourceType::Buffer, clear_stage, result);
  return result;
}

ResourceDescription IrGraphBuilder::createInfoToResDesc(ResNameId, const auto &, intermediate::ClearStage &) const
{
  G_ASSERT_FAIL("Fallback createInfoToResDesc called!");
  return ResourceDescription{};
}

static constexpr auto SOURCE_SENTINEL_NODE_ID_IN_RAW_GRAPH = intermediate::NodeIndex{0};
static constexpr auto DESTINATION_SENTINEL_NODE_ID_IN_RAW_GRAPH = intermediate::NodeIndex{1};

IrGraphBuilder::Changes IrGraphBuilder::build(intermediate::Graph &graph, multiplexing::Extents extents,
  multiplexing::Extents prev_extents, const intermediate::Mapping &old_mapping, const ResourcesChanged &resources_changed,
  const NodesChanged &nodes_changed)
{
  // Fake source/destination nodes, see below
  constexpr size_t INTERNAL_FAKE_NODES = 2;

  Changes result;
  const auto irExtents = multiplexing_extents_to_ir(extents);
  const auto irNodeMaxCount =
    max(registry.knownNames.nameCount<NodeNameId>() * irExtents, old_mapping.mappedNodeIndexCount()) + INTERNAL_FAKE_NODES;
  const auto irResMaxCount = max(registry.knownNames.nameCount<ResNameId>() * irExtents, old_mapping.mappedResourceIndexCount());
  // TODO: we must prune rawGraph.nodes in future
  // Double the capacity for gaps
  result.irNodesChanged.reserve(2 * max(irNodeMaxCount, rawGraph.nodes.totalKeys()));
  result.irResourcesChanged.reserve(irResMaxCount);

  FRAMEMEM_REGION;

  // Build the intermediate graph.
  // Only valid nodes and resources are included, so all of the following
  // graph compilation process does not need to do validity assertions at
  // all.

  intermediate::Mapping mapping(registry.knownNames.nameCount<NodeNameId>(), registry.knownNames.nameCount<ResNameId>(),
    multiplexing_extents_to_ir(extents));

  rawGraph.nodes.reserve(mapping.mappedNodeIndexCount() + INTERNAL_FAKE_NODES);

  IrNodesChanged rawIrNodesChanged(mapping.mappedNodeIndexCount() + INTERNAL_FAKE_NODES, false);
  const uint32_t safeChangedNodesCount = max(rawMapping.mappedNodeIndexCount() + INTERNAL_FAKE_NODES, rawGraph.nodes.totalKeys());
  if (rawIrNodesChanged.size() < safeChangedNodesCount)
    rawIrNodesChanged.resize(safeChangedNodesCount, true);

  addNodesToGraph(rawGraph, mapping, extents, prev_extents, rawMapping, nodes_changed, rawIrNodesChanged);
  rawMapping = mapping;

  addEdgesToIrGraph(rawGraph, mapping, rawIrNodesChanged);

  const auto sinks = findSinkIrNodes(mapping, {registry.sinkExternalResources.begin(), registry.sinkExternalResources.end()});
  G_ASSERT_DO_AND_LOG(!sinks.empty(), dumpRawUserGraph(),
    "All specified frame graph sinks were skipped due to broken resource pruning! "
    "No rendering will be done! Report this to programmers!");
  // Pruning (this makes mappings out of date)
  auto [displacement, edgesToBreak] = pruneGraph(rawGraph, mapping, sinks);
  if (!edgesToBreak.empty())
  {
    dumpRawUserGraph();
    logerr("daFG: The IR graph contained dependency cycles! "
           "See above for the cycles and the full graph dump! "
           "Note that this is a bug in this game's frame graph and should be reported to programmers!");

    // Cycles must be broken, IR graph being a control flow graph is an
    // invariant that must be upheld.
    for (auto [from, to] : edgesToBreak)
    {
      rawGraph.nodes[from].predecessors.erase(to);
      rawIrNodesChanged[from] = true;
    }

    // After an edge was removed, we might get "hanging" nodes, which
    // are not permitted inside a CFG, so we have to fix that up.
    // Doing more pruning is not what we want to do here, as we are
    // desperately trying to recover and render at least something.
    for (auto [from, _] : edgesToBreak)
      if (rawGraph.nodes[from].predecessors.empty())
        rawGraph.nodes[from].predecessors.insert(SOURCE_SENTINEL_NODE_ID_IN_RAW_GRAPH);
  }

  // Save dst sentinel's predecessors before apply_node_remap clears them.
  // apply_node_remap rebuilds all predecessors from rawGraph, but the dst
  // sentinel's rawGraph predecessors are always empty (they are set only in
  // the output graph below), so apply_node_remap would lose them.
  dag::FixedVectorSet<intermediate::NodeIndex, 16> savedDstSentinelPreds;
  if (!oldDisplacement.empty())
  {
    const auto prevDstIdx = oldDisplacement[DESTINATION_SENTINEL_NODE_ID_IN_RAW_GRAPH];
    savedDstSentinelPreds = graph.nodes[prevDstIdx].predecessors;
  }

  IrNodesChanged &irNodesChanged = result.irNodesChanged;
  auto newDisplacement =
    intermediate::apply_node_remap(graph, rawGraph, displacement, oldDisplacement, rawIrNodesChanged, irNodesChanged);

  oldDisplacement.clear();
  oldDisplacement.resize(displacement.size());
  eastl::copy(newDisplacement.begin(), newDisplacement.end(), oldDisplacement.begin());

  // Connect the sentinel destination node to all nodes without successors.
  // HAS to happen here, after we prunned and remapped everything.
  {
    const auto dstIdx = newDisplacement[DESTINATION_SENTINEL_NODE_ID_IN_RAW_GRAPH];

    IdIndexedFlags<intermediate::NodeIndex, framemem_allocator> hasIncomingEdges(graph.nodes.totalKeys(), false);
    for (auto [nodeId, node] : graph.nodes.enumerate())
      for (auto pred : node.predecessors)
        hasIncomingEdges[pred] = true;

    for (auto [nodeId, node] : graph.nodes.enumerate())
      if (!hasIncomingEdges[nodeId] && nodeId != dstIdx)
        graph.nodes[dstIdx].predecessors.insert(nodeId);

    if (!irNodesChanged[dstIdx] && graph.nodes[dstIdx].predecessors != savedDstSentinelPreds)
    {
      irNodesChanged[dstIdx] = true;
      graph.nodeStates.erase(dstIdx);
      graph.nodeNames.erase(dstIdx);
    }

    // Destination sentinel must never have resource requests.
    G_ASSERT(graph.nodes[dstIdx].resourceRequests.empty());
    G_ASSERT(graph.nodes[dstIdx].supressedRequests.empty());
  }

  IrResourcesChanged &irResourcesChanged = result.irResourcesChanged;
  irResourcesChanged.resize(mapping.mappedResourceIndexCount(), false);
  graph.resources.reserve(mapping.mappedResourceIndexCount());
  addResourcesToGraph(graph, mapping, old_mapping, resources_changed, irResourcesChanged, extents);

  // Fixup requests for resources that moved.
  for (auto [nodeIdx, node] : graph.nodes.enumerate())
    if (node.frontendNode && !irNodesChanged[nodeIdx])
    {
      bool anyChanged = eastl::any_of(node.resourceRequests.begin(), node.resourceRequests.end(),
        [&](const auto &req) { return irResourcesChanged[req.resource]; });
      anyChanged = anyChanged || eastl::any_of(node.supressedRequests.begin(), node.supressedRequests.end(),
                                   [&](intermediate::ResourceIndex res_idx) { return irResourcesChanged[res_idx]; });
      if (anyChanged)
      {
        graph.nodeStates.erase(nodeIdx);
        graph.nodeNames.erase(nodeIdx);
        graph.nodes[nodeIdx].resourceRequests.clear();
        graph.nodes[nodeIdx].supressedRequests.clear();
        irNodesChanged[nodeIdx] = true;
      }
    }

  addRequestsToGraph(graph, mapping, extents, irNodesChanged);

  graph.pruneResources();

  // Clear change flags for resources that were created but then pruned.
  // This avoids spurious dirty flags for stably-pruned resources on no-op rebuilds.
  for (auto resIdx : irResourcesChanged.trueKeys())
    if (!graph.resources.isMapped(resIdx))
      irResourcesChanged.set(resIdx, false);

  addNodeStatesToGraph(graph, mapping, extents, irNodesChanged);

  fixupFalseHistoryFlags(graph, irResourcesChanged);

  setIrGraphDebugNames(graph, irNodesChanged, irResourcesChanged);

  graph.validate();

  return result;
}

const char *IrGraphBuilder::frontendNodeName(const intermediate::Node &node) const
{
  if (node.frontendNode)
    return registry.knownNames.getName(*node.frontendNode);

  return node.predecessors.empty() ? "FAKE_SRC" : "FAKE_DST";
}


template <class F, class G, class I>
void IrGraphBuilder::scanPhysicalResourceUsages(ResNameId res_id, const F &process_reader, const G &process_modifier,
  const I &process_history_reader) const
{
  FRAMEMEM_VALIDATE;
  // Save scanned resources to prevent hanging if there is cycle in renaming chains
  // Distribution of renaming chains is exponential, hence the weird container.
  dag::FixedVectorSet<ResNameId, 2, true, eastl::use_self<ResNameId>, framemem_allocator> knownCandidates;
  const auto scanNodeSlotsOrResource = [this](NodeNameId node_id, ResNameId res_id, const auto &process_node) {
    for (const auto unresolvedResId : nameResolver.unresolve(node_id, res_id))
      process_node(node_id, unresolvedResId);
  };
  auto resIdCandidate = res_id;
  while (true)
  {
    const auto introducedBy = depData.resourceLifetimes[resIdCandidate].introducedBy;
    if (validityInfo.nodeValid[introducedBy])
      scanNodeSlotsOrResource(introducedBy, resIdCandidate, process_modifier);

    // We don't want to consider broken/invalid nodes when looking
    // for an appropriate usage.
    for (const auto modifier : depData.resourceLifetimes[resIdCandidate].modificationChain)
      if (validityInfo.nodeValid[modifier])
        scanNodeSlotsOrResource(modifier, resIdCandidate, process_modifier);

    for (const auto reader : depData.resourceLifetimes[resIdCandidate].readers)
      if (validityInfo.nodeValid[reader])
        scanNodeSlotsOrResource(reader, resIdCandidate, process_reader);

    for (const auto historyReader : depData.resourceLifetimes[resIdCandidate].historyReaders)
      if (validityInfo.nodeValid[historyReader])
      {
        for (const auto unresolvedResId : nameResolver.historyUnresolve(historyReader, resIdCandidate))
          process_history_reader(historyReader, unresolvedResId);
      }

    // The renaming chain might've been "torn" by an invalid node.
    if (!validityInfo.resourceValid[depData.renamingChains[resIdCandidate]])
      break;

    resIdCandidate = depData.renamingChains[resIdCandidate];
    if (knownCandidates.find(resIdCandidate) != knownCandidates.end())
      break;
    knownCandidates.insert(resIdCandidate);
  }
}

auto IrGraphBuilder::findFirstUsageAndUpdatedCreationFlags(ResNameId res_id,
  uint32_t initial_flags) const -> FirstUsageUpdatedFlagsAndUsageInfo
{
  enum class ModifierType
  {
    Unknown,
    RenderPass,
    NonRenderPass,
  };

  const auto isModifiedByRp = [](const NodeData &node, ResNameId res_id) {
    if (const auto &rp = node.renderingRequirements)
    {
      const auto attachComp = [res_id](const auto &attachment) { return attachment.nameId == res_id; };
      return eastl::find_if(rp->colorAttachments.begin(), rp->colorAttachments.end(), attachComp) != rp->colorAttachments.end() ||
             attachComp(rp->depthAttachment);
    }
    return false;
  };

  const auto producerNodeId = depData.resourceLifetimes[res_id].introducedBy;
  const auto &producerNode = registry.nodes[producerNodeId];

  // If the node introducer do not specify usage, find the first usage
  // in the modification requests in all renamings.
  auto firstUsage = producerNode.resourceRequests.find(res_id)->second.usage;
  const auto resourceType = registry.resources[res_id].createdResData->type;

  const bool producerUsesInRenderPass = isModifiedByRp(producerNode, res_id);

  ModifierType modifierType = ModifierType::Unknown;
  ResNameId firstModifiedResId = ResNameId::Invalid;
  if (producerUsesInRenderPass)
  {
    modifierType = ModifierType::RenderPass;
    firstModifiedResId = res_id;
  }

  bool hasReaders = false;
  const auto processReaders = [this, resourceType, &firstUsage, &initial_flags, &hasReaders](NodeNameId node_id,
                                ResNameId unresolved_res_id) {
    hasReaders = true;
    const auto &node = registry.nodes[node_id];

    const auto res = node.resourceRequests.find(unresolved_res_id);
    G_ASSERTF(res != node.resourceRequests.cend(), "daFG: Node %s does not contain resource %d", registry.knownNames.getName(node_id),
      registry.knownNames.getName(nameResolver.resolve(unresolved_res_id)));
    update_creation_flags_from_usage(initial_flags, res->second.usage, resourceType);

    if (firstUsage.type == Usage::UNKNOWN)
      logerr("daFG: Resource %s is read by %s before first modify usage",
        registry.knownNames.getName(nameResolver.resolve(unresolved_res_id)), registry.knownNames.getName(node_id));
  };

  const auto processHistoryReaders = [this, resourceType, &firstUsage, &initial_flags](NodeNameId node_id,
                                       ResNameId unresolved_res_id) {
    const auto &node = registry.nodes[node_id];

    const auto res = node.historyResourceReadRequests.find(unresolved_res_id);
    G_ASSERTF(res != node.historyResourceReadRequests.cend(), "daFG: Node %s does not contain resource %d",
      registry.knownNames.getName(node_id), registry.knownNames.getName(nameResolver.resolve(unresolved_res_id)));
    update_creation_flags_from_usage(initial_flags, res->second.usage, resourceType);

    if (firstUsage.type == Usage::UNKNOWN)
      logerr("daFG: Resource %s is read by %s before first modify usage",
        registry.knownNames.getName(nameResolver.resolve(unresolved_res_id)), registry.knownNames.getName(node_id));
  };

  bool modifiersConflictReported = false;
  bool hasModifiers = producerUsesInRenderPass;
  const auto processModifiers = [this, resourceType, &firstUsage, &initial_flags, &modifierType, &modifiersConflictReported,
                                  &firstModifiedResId, &isModifiedByRp, producerUsesInRenderPass, &hasModifiers,
                                  producerNodeId](NodeNameId node_id, ResNameId unresolved_res_id) {
    const auto &node = registry.nodes[node_id];
    const auto usage = node.resourceRequests.find(unresolved_res_id)->second.usage;
    update_creation_flags_from_usage(initial_flags, usage, resourceType);

    // the code below is useless in this case
    if (node_id == producerNodeId)
      return;

    // we don't need to search first modified res (renamed or original)
    // if producer uses the resource in render pass
    if (producerUsesInRenderPass)
      return;

    hasModifiers = true;
    if (firstUsage.type == Usage::UNKNOWN)
      firstUsage = usage;

    // Not the same as an original resId, since it could be renamed.
    const ResNameId resolvedResId = nameResolver.resolve(unresolved_res_id);
    if (firstModifiedResId == ResNameId::Invalid || firstModifiedResId == resolvedResId)
    {
      const bool modifiedByRp = isModifiedByRp(node, unresolved_res_id);
      const ModifierType currentModifierType = modifiedByRp ? ModifierType::RenderPass : ModifierType::NonRenderPass;

      if (modifierType == ModifierType::Unknown)
        modifierType = currentModifierType;
      else if (modifierType != currentModifierType)
      {
        if (!modifiersConflictReported && verbose)
          logwarn("The resource %s is modified both in a render pass and in a "
                  "non-render pass node (%s). This is allowed, but suboptimal!",
            registry.knownNames.getName(resolvedResId), registry.knownNames.getName(node_id));
        modifierType = ModifierType::NonRenderPass;
        modifiersConflictReported = true;
      }

      firstModifiedResId = resolvedResId;
    }
  };

  scanPhysicalResourceUsages(res_id, processReaders, processModifiers, processHistoryReaders);

  return {firstUsage, initial_flags, {modifierType == ModifierType::RenderPass, hasReaders, hasModifiers}};
}


void IrGraphBuilder::addResourcesToGraph(intermediate::Graph &graph, intermediate::Mapping &mapping,
  const intermediate::Mapping &old_mapping, const ResourcesChanged &resource_changed, IrResourcesChanged &ir_resources_changed,
  multiplexing::Extents extents) const
{
  TIME_PROFILE(addResourcesToGraph);

  FRAMEMEM_VALIDATE;

  if (multiplexing_extents_to_ir(extents) != old_mapping.multiplexingExtent())
    ir_resources_changed.assign(ir_resources_changed.size(), true);

  // First, get rid of all resources that might have changed.
  if (multiplexing_extents_to_ir(extents) != old_mapping.multiplexingExtent())
  {
    // No point in trying to be incremental here, EVERYTHING has changed.
    graph.resources.clear();
    graph.resourceNames.clear();
  }
  else
  {
    for (auto it = graph.resources.enumerate().begin(); it != graph.resources.enumerate().end();)
    {
      auto [idx, res] = *it;
      bool shouldErase = false;
      for (auto frontendRes : res.frontendResources)
        if (resource_changed[frontendRes] && old_mapping.wasResMapped(frontendRes, res.multiplexingIndex) &&
            graph.resources.isMapped(old_mapping.mapRes(frontendRes, res.multiplexingIndex)))
        {
          shouldErase = true;
          break;
        }
      if (shouldErase)
      {
        ir_resources_changed[idx] = true;
        graph.resourceNames.erase(idx);
        it = graph.resources.erase(idx);
      }
      else
        ++it;
    }
  }

  const auto &nodeValid = validityInfo.nodeValid;
  const auto &resourceValid = validityInfo.resourceValid;

  IdIndexedFlags<ResNameId, framemem_allocator> canReuse(registry.knownNames.nameCount<ResNameId>(), false);
  for (auto resId : registry.resources.keys())
  {
    // Can only reuse valid & unchanged resources
    if (!resourceValid[resId] || resource_changed[resId])
      continue;

    // And only if it was not prunned or invalid on PREVIOUS compilation.
    if (!old_mapping.wasResMapped(resId, {}) || !graph.resources.isMapped(old_mapping.mapRes(resId, {})))
      continue;

    canReuse[resId] = true;
  }

  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    if (!nodeValid[nodeId])
      continue;


    const auto nodeMultiplexingExtents =
      extents_for_node(nodeData.multiplexingMode.value_or(registry.defaultMultiplexingMode), extents);

    auto processResource = [&extents, &nodeMultiplexingExtents, &graph, &mapping, &ir_resources_changed](ResNameId resId,
                             const multiplexing::Index &midx, const intermediate::ConcreteResource &variant) {
      const auto irMultiplexingIndex = multiplexing_index_to_ir(midx, extents);

      if (!index_inside_extents(midx, nodeMultiplexingExtents))
      {
        // Index outside of this node's multiplexing extents, redirect
        // the mapping to the intersection with the valid extents
        const auto fromIrIndex = multiplexing_index_to_ir(clamp(midx, nodeMultiplexingExtents), extents);
        mapping.mapRes(resId, irMultiplexingIndex) = mapping.mapRes(resId, fromIrIndex);
        return;
      }

      // Correct multiindex for this frontend node, multiplex the
      // resource (unique physical resource per index)
      auto [idx, irRes] = graph.resources.appendNew();
      mapping.mapRes(resId, irMultiplexingIndex) = idx;
      irRes.resource = variant;
      irRes.frontendResources = {resId};
      irRes.multiplexingIndex = irMultiplexingIndex;
      ir_resources_changed[idx] = true;
    };

    for (auto resId : nodeData.createdResources)
    {
      if (!resourceValid[resId])
        continue;

      if (canReuse[resId])
      {
        for (multiplexing::Index i = {}; index_inside_extents(i, extents); i = next_index(i, extents))
        {
          const auto oldIrIndex = old_mapping.mapRes(resId, multiplexing_index_to_ir(i, extents));
          const auto irMultiplexingIndex = multiplexing_index_to_ir(i, extents);
          mapping.mapRes(resId, irMultiplexingIndex) = oldIrIndex;
        }
        continue;
      }

      // by design, it is impossible that a resource was created but has no createdResData,
      // so just make sure it has valid type in branches below (or panic otherwise, it's a user fail)
      const auto &resData = registry.resources[resId].createdResData.value();
      const auto createdResHistory = registry.resources[resId].history;

      if (eastl::holds_alternative<Texture2dCreateInfo>(resData.creationInfo) ||
          eastl::holds_alternative<Texture3dCreateInfo>(resData.creationInfo) ||
          eastl::holds_alternative<BufferCreateInfo>(resData.creationInfo))
      {
        intermediate::ClearStage clearStage = intermediate::ClearStage::None;
        if (!eastl::holds_alternative<eastl::monostate>(resData.clearValue))
          clearStage = intermediate::ClearStage::RenderPass;
        ResourceDescription resDesc =
          eastl::visit([&](const auto &info) { return createInfoToResDesc(resId, info, clearStage); }, resData.creationInfo);

        eastl::optional<AutoResolutionData> resolvedAutoResData = resData.resolution;
        if (resolvedAutoResData)
          resolvedAutoResData->id = nameResolver.resolve(resolvedAutoResData->id);

        ResourceClearValue staticClearValue = ResourceClearValue{};
        if (auto clearValue = eastl::get_if<ResourceClearValue>(&resData.clearValue))
          staticClearValue = *clearValue;

        intermediate::ScheduledResource res{
          .description = resDesc,
          .resourceType = resData.type,
          .resolutionType = resolvedAutoResData,
          .clearStage = clearStage,
          .clearValue = staticClearValue,
          .clearFlags = resData.clearFlags,
          .history = createdResHistory,
          .autoMipCount = resolvedAutoResData ? resDesc.asBasicTexRes.mipLevels == dafg::AUTO_MIP_COUNT : false,
        };
        for (multiplexing::Index i = {}; index_inside_extents(i, extents); i = next_index(i, extents))
          processResource(resId, i, res);
      }
      else if (auto blobDesc = eastl::get_if<BlobDescription>(&resData.creationInfo))
      {
        auto *rtti = dafg::Runtime::get().getTypeDb().getRTTI(blobDesc->typeTag);
        G_ASSERTF(rtti, "RTTI for blob type w/ tag %d not found in the type database!", eastl::to_underlying(blobDesc->typeTag));
        const bool ctorOverridden = blobDesc->ctorOverride && *blobDesc->ctorOverride;
        intermediate::BlobDescription irDesc{
          blobDesc->typeTag, rtti->size, rtti->align, ctorOverridden ? *blobDesc->ctorOverride : rtti->ctor, rtti->dtor, rtti->copy};
        intermediate::ScheduledResource res{irDesc, resData.type, eastl::nullopt, intermediate::ClearStage::None, ResourceClearValue{},
          resData.clearFlags, createdResHistory};
        for (multiplexing::Index i = {}; index_inside_extents(i, extents); i = next_index(i, extents))
          processResource(resId, i, res);
      }
      else if (auto externalProvider = eastl::get_if<ExternalResourceProvider>(&resData.creationInfo))
      {
        // We "probe" the provided external resources on recompilation
        // and record their properties. Then at runtime we assert that
        // these properties have not changed.
        // Supporting runtime changes of external resource properties
        // is possible, but it would result in bad performence and
        // is generally a bad idea.
        // TODO: we should scan usages of such resources and make sure
        // that the flags are actually compatible with how they are
        // used in practice.
        switch (resData.type)
        {
          case ResourceType::Texture:
          {
            for (multiplexing::Index i = {}; index_inside_extents(i, extents); i = next_index(i, extents))
            {
              const auto tex = eastl::get<ManagedTexView>((*externalProvider)(i));

              TextureInfo info;
              tex->getinfo(info);
              intermediate::ExternalResource res{info};
              processResource(resId, i, res);
            }
          }
          break;

          case ResourceType::Buffer:
          {
            for (multiplexing::Index i = {}; index_inside_extents(i, extents); i = next_index(i, extents))
            {
              const auto buf = eastl::get<ManagedBufView>((*externalProvider)(i));

              intermediate::BufferInfo info;
              info.flags = buf->getFlags();
              intermediate::ExternalResource res{info};
              processResource(resId, i, res);
            }
          }
          break;

          case ResourceType::Blob:
          case ResourceType::Invalid: G_ASSERTF_BREAK(false, "This should never ever happen!");
        }
      }
      else if (eastl::holds_alternative<DriverDeferredTexture>(resData.creationInfo))
      {
        intermediate::DriverDeferredTexture res{};
        for (multiplexing::Index i = {}; index_inside_extents(i, extents); i = next_index(i, extents))
          processResource(resId, i, res);
      }
      else
      {
        G_ASSERTF(false, "Unknown resource creation info type %s!", registry.knownNames.getName(resId));
      }
    }
  }

  // Fill in mappings for resources produced via renaming
  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    if (!nodeValid[nodeId])
      continue;

    for (const auto &[toResId, unresolvedFromResId] : nodeData.renamedResources)
    {
      if (!resourceValid[toResId])
        continue;

      auto originalRes = depData.renamingRepresentatives[nameResolver.resolve(unresolvedFromResId)];

      for (multiplexing::Index i = {}; index_inside_extents(i, extents); i = next_index(i, extents))
      {
        const auto irMultiplexingIndex = multiplexing_index_to_ir(i, extents);
        G_ASSERT(mapping.wasResMapped(originalRes, irMultiplexingIndex));

        const auto originalIndex = mapping.mapRes(originalRes, irMultiplexingIndex);
        mapping.mapRes(toResId, irMultiplexingIndex) = originalIndex;

        auto &originalRes = graph.resources[originalIndex];

        // When renaming resources, the history flag is specified on
        // the last renamee, because it makes sense. Here, we need
        // to propagate it to the first rename. This code is not ideal,
        // as it will propagate a flag even for a resource in the middle,
        // of a chain, but it's ok cuz we validate that no resources in
        // the middle have history, so it will work only for the
        // last resource in a chain.
        if (originalRes.isScheduled() && registry.resources[toResId].history != History::No)
          originalRes.asScheduled().history = registry.resources[toResId].history;
      }
    }
  }

  for (auto [resIdx, res] : graph.resources.enumerate())
  {
    if (canReuse[res.frontendResources.front()])
      continue;

    G_ASSERT(res.frontendResources.size() == 1);

    if (res.isScheduled())
    {
      // ir resource was created from valid frontend resource, so it must have valid createdResData
      const auto &createdResData = registry.resources[res.frontendResources.front()].createdResData;
      if (const auto dynamicClearValue = eastl::get_if<DynamicParameter>(&createdResData->clearValue))
      {
        const auto clearResIndex = mapping.mapRes(nameResolver.resolve(dynamicClearValue->resource), res.multiplexingIndex);
        if (clearResIndex == intermediate::RESOURCE_NOT_MAPPED)
        {
          G_ASSERTF_CONTINUE(false, "daFG invariant failed. Clear resource %s is not mapped in the IR graph!",
            registry.knownNames.getName(nameResolver.resolve(dynamicClearValue->resource)));
        }
        intermediate::DynamicParameter irDynClearColor{clearResIndex, dynamicClearValue->projectedTag, dynamicClearValue->projector};
        res.asScheduled().clearValue = irDynClearColor;
        res.asScheduled().clearFlags = createdResData->clearFlags;
      }
    }

    // Walk along the renaming chain and append renamed frontend resources
    // (first element intentionally skipped)
    auto current = res.frontendResources.front();
    while (depData.renamingChains[current] != current)
    {
      current = depData.renamingChains[current];

      if (!resourceValid[current])
        break;

      res.frontendResources.push_back(current);
    }
  }
}

void IrGraphBuilder::addNodesToGraph(intermediate::Graph &graph, intermediate::Mapping &mapping, multiplexing::Extents extents,
  multiplexing::Extents prev_extents, const intermediate::Mapping &old_mapping, const NodesChanged &nodes_changed,
  IrNodesChanged &ir_nodes_changed) const
{
  TIME_PROFILE(addNodesToGraph);

  FRAMEMEM_VALIDATE;

  const auto &nodeValid = validityInfo.nodeValid;

  if (multiplexing_extents_to_ir(extents) != old_mapping.multiplexingExtent())
    ir_nodes_changed.assign(ir_nodes_changed.size(), true);

  if (rawGraph.nodes.empty())
  {
    static constexpr auto ZERO_MIDX = intermediate::MultiplexingIndex{0};
    // Sentinel source node -- precedes all nodes.
    const auto srcId = graph.nodes.appendNew(intermediate::Node{.multiplexingIndex = ZERO_MIDX}).first;
    G_UNUSED(srcId);
    G_ASSERT(srcId == SOURCE_SENTINEL_NODE_ID_IN_RAW_GRAPH);

    // Sentinel destination node -- succeeds all nodes.
    const auto dstId = graph.nodes.appendNew(intermediate::Node{.multiplexingIndex = ZERO_MIDX}).first;
    G_UNUSED(dstId);
    G_ASSERT(dstId == DESTINATION_SENTINEL_NODE_ID_IN_RAW_GRAPH);
  }

  IdIndexedFlags<NodeNameId, framemem_allocator> canReuse(registry.knownNames.nameCount<NodeNameId>(), false);
  for (auto nodeId : registry.nodes.keys())
  {
    // Can only resuse valid & unchanged nodes
    if (!nodeValid[nodeId] || nodes_changed[nodeId])
      continue;

    // And only if it was not prunned or invalid on PREVIOUS compilation.
    if (!old_mapping.wasNodeMapped(nodeId, {}) || !graph.nodes.isMapped(old_mapping.mapNode(nodeId, {})))
      continue;

    auto currentNodeExtents =
      extents_for_node(registry.nodes[nodeId].multiplexingMode.value_or(registry.defaultMultiplexingMode), extents);
    auto prevNodeExtents =
      extents_for_node(registry.nodes[nodeId].multiplexingMode.value_or(registry.defaultMultiplexingMode), prev_extents);
    if (currentNodeExtents != prevNodeExtents)
      continue;

    canReuse[nodeId] = true;
  }

  // Prune the graph of nodes that we no longer need.
  for (auto it = graph.nodes.enumerate().begin(); it != graph.nodes.enumerate().end();)
  {
    auto [idx, node] = *it;
    if (node.frontendNode && !canReuse[*node.frontendNode])
    {
      ir_nodes_changed[idx] = true;
      it = graph.nodes.erase(idx);
    }
    else
      ++it;
  }

  dag::Vector<NodeNameId, framemem_allocator> validNodes;
  validNodes.reserve(registry.nodes.size());
  for (auto [nodeId, _] : registry.nodes.enumerate())
    if (nodeValid[nodeId])
      validNodes.emplace_back(nodeId);

  if (randomize_order.get())
  {
    eastl::random_shuffle(validNodes.begin(), validNodes.end(), [](uint32_t n) { return static_cast<uint32_t>(grnd() % n); });
  }

  for (auto nodeId : validNodes)
  {
    if (canReuse[nodeId])
    {
      for (multiplexing::Index i = {}; index_inside_extents(i, extents); i = next_index(i, extents))
      {
        const auto irMultiplexingIndex = multiplexing_index_to_ir(i, extents);
        const auto oldIrMultiplexingIndex = multiplexing_index_to_ir(clamp(i, prev_extents), extents);
        const auto oldIrIndex = old_mapping.mapNode(nodeId, oldIrMultiplexingIndex);
        mapping.mapNode(nodeId, irMultiplexingIndex) = oldIrIndex;
      }
      continue;
    }

    const auto &nodeData = registry.nodes[nodeId];

    const auto nodeMultiplexingExtents =
      extents_for_node(nodeData.multiplexingMode.value_or(registry.defaultMultiplexingMode), extents);

    for (multiplexing::Index i = {}; index_inside_extents(i, extents); i = next_index(i, extents))
    {
      const auto irMultiplexingIndex = multiplexing_index_to_ir(i, extents);
      if (!index_inside_extents(i, nodeMultiplexingExtents))
      {
        const auto fromIrIndex = multiplexing_index_to_ir(clamp(i, nodeMultiplexingExtents), extents);
        // Note that mapping at formIrIndex is guaranteed to be correct,
        // as breaching this node's extents on some axis means that
        // we've already iterated all values up to the extent on that
        // axis.
        mapping.mapNode(nodeId, irMultiplexingIndex) = mapping.mapNode(nodeId, fromIrIndex);
        ir_nodes_changed[mapping.mapNode(nodeId, fromIrIndex)] = true;
        continue;
      }

      auto [idx, irNode] = graph.nodes.appendNew();
      mapping.mapNode(nodeId, irMultiplexingIndex) = idx;
      irNode.frontendNode = nodeId;
      irNode.priority = nodeData.priority;
      irNode.multiplexingIndex = irMultiplexingIndex;
      irNode.hasSideEffects = nodeData.sideEffect != SideEffects::None;
      ir_nodes_changed[idx] = true;
    }
  }
}

void IrGraphBuilder::addRequestsToGraph(intermediate::Graph &graph, intermediate::Mapping &mapping, multiplexing::Extents extents,
  const IrNodesChanged &ir_nodes_changed) const
{
  TIME_PROFILE(addRequestsToGraph);

  const auto &resourceValid = validityInfo.resourceValid;
  const auto historyMultiplexingExtents = extents_for_node(registry.defaultHistoryMultiplexingMode, extents);

  for (auto [nodeIdx, irNode] : graph.nodes.enumerate())
  {
    if (!irNode.frontendNode || !ir_nodes_changed[nodeIdx])
      continue;

    eastl::array<dag::VectorMap<ResNameId, uint32_t, eastl::less<ResNameId>, framemem_allocator>, 2> resolvedResIdxToIndexInIrRequests;

    for (auto &req : resolvedResIdxToIndexInIrRequests)
      req.reserve(registry.resources.size());

    const auto irMultiplexingIndex = irNode.multiplexingIndex;
    const auto originalMultiplexingIndex = multiplexing_index_from_ir(irMultiplexingIndex, extents);
    const auto historyMultiplexingIndex =
      multiplexing_index_to_ir(clamp(originalMultiplexingIndex, historyMultiplexingExtents), extents);

    const auto &nodeData = registry.nodes[*irNode.frontendNode];

    auto processRequest = [&irNode = irNode, &mapping, &resourceValid, &nodeData, &resolvedResIdxToIndexInIrRequests, this,
                            irMultiplexingIndex, historyMultiplexingIndex,
                            extents](bool history, ResNameId resId, const ResourceRequest &req) {
      // Skip optional requests for missing resources
      if (req.optional && !resourceValid[resId])
        return;

      // Note that this resMapping might redirect us to a resource
      // that is less multiplexed than us. This is ok.
      const auto resMultiplexingIndex = history ? historyMultiplexingIndex : irMultiplexingIndex;
      const auto resIndex = mapping.mapRes(resId, resMultiplexingIndex);

      // On the other hand, if we are less multiplexed than the
      // requested resource, we basically get undefined behaviour:
      // we don't know which one of multiple physical resources
      // we actually want to read. Assert.
      const auto resMplexMode =
        registry.nodes[depData.resourceLifetimes[resId].introducedBy].multiplexingMode.value_or(registry.defaultMultiplexingMode);
      G_ASSERT_DO_AND_LOG(
        !less_multiplexed(nodeData.multiplexingMode.value_or(registry.defaultMultiplexingMode), resMplexMode, extents),
        dumpRawUserGraph(),
        "Node '%s' requested resource '%s', which is more"
        " multiplexed than the node due to being produced by a"
        " node that is more multiplexed. This is invalid usage.",
        registry.knownNames.getName(*irNode.frontendNode), registry.knownNames.getName(resId));


      // Note that name resolution might have mapped several requests
      // into the same resource requests, in which case we need to
      // merge them.
      if (const auto alreadyProcessedIt = resolvedResIdxToIndexInIrRequests[history].find(resId);
          alreadyProcessedIt == resolvedResIdxToIndexInIrRequests[history].end())
      {
        resolvedResIdxToIndexInIrRequests[history].emplace(resId, irNode.resourceRequests.size());

        auto &irRequest = irNode.resourceRequests.emplace_back();
        irRequest.resource = resIndex;
        irRequest.usage = intermediate::ResourceUsage{req.usage.type, req.usage.access, req.usage.stage};
        irRequest.fromLastFrame = history;
      }
      else
      {
        auto &irRequest = irNode.resourceRequests[alreadyProcessedIt->second];
        // Otherwise this node must have been marked as broken and removed.
        G_ASSERT(irRequest.usage.access == req.usage.access && irRequest.usage.type == req.usage.type);

        irRequest.usage.stage = irRequest.usage.stage | req.usage.stage;
      }
    };

    for (const auto &[resId, req] : nodeData.resourceRequests)
      processRequest(false, nameResolver.resolve(resId), req);
    for (const auto &[resId, req] : nodeData.historyResourceReadRequests)
      processRequest(true, nameResolver.resolve(resId), req);

    // NOTE: if a node doesn't actually do anything, we can avoid adding
    // barriers and prolonging resource lifetimes for it, achieveing better
    // resource utilization.
    if (registry.nodes[*irNode.frontendNode].sideEffect == SideEffects::None)
    {
      for (const auto &req : irNode.resourceRequests)
        irNode.supressedRequests.push_back(req.resource);
      irNode.resourceRequests.clear();
    }
  }
}

void IrGraphBuilder::addNodeStatesToGraph(intermediate::Graph &graph, intermediate::Mapping &mapping, multiplexing::Extents extents,
  const IrNodesChanged &ir_nodes_changed) const
{
  TIME_PROFILE(addNodeStatesToGraph);

  const auto historyMultiplexingExtents = extents_for_node(registry.defaultHistoryMultiplexingMode, extents);

  graph.nodeStates.reserve(graph.nodes.totalKeys());

  for (auto [nodeIdx, irNode] : graph.nodes.enumerate())
  {
    if (!ir_nodes_changed[nodeIdx])
      continue;

    if (!irNode.frontendNode)
    {
      graph.nodeStates.emplaceAt(nodeIdx);
      continue;
    }

    const auto irMultiplexingIndex = irNode.multiplexingIndex;
    const auto originalMultiplexingIndex = multiplexing_index_from_ir(irMultiplexingIndex, extents);
    const auto historyMultiplexingIndex =
      multiplexing_index_to_ir(clamp(originalMultiplexingIndex, historyMultiplexingExtents), extents);

    graph.nodeStates.emplaceAt(nodeIdx, calcNodeState(*irNode.frontendNode, irMultiplexingIndex, historyMultiplexingIndex, mapping));
  }
}

void IrGraphBuilder::fixupFalseHistoryFlags(intermediate::Graph &graph, IrResourcesChanged &ir_resources_changed) const
{
  TIME_PROFILE(fixupFalseHistoryFlags);

  FRAMEMEM_VALIDATE;

  IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> historyWasRequested;
  historyWasRequested.reserve(graph.resources.totalKeys());

  for (const auto &node : graph.nodes.values())
    for (const auto &req : node.resourceRequests)
      if (req.fromLastFrame)
        historyWasRequested.set(req.resource, true);

  for (auto [resIdx, res] : graph.resources.enumerate())
    if (res.isScheduled())
    {
      const auto userHistory = registry.resources[res.frontendResources.back()].history;
      const auto correctHistory = historyWasRequested.test(resIdx) ? userHistory : History::No;
      if (res.asScheduled().history != correctHistory)
      {
        res.asScheduled().history = correctHistory;
        ir_resources_changed.set(resIdx, true);
        if (graph.resourceNames.isMapped(resIdx))
          graph.resourceNames.erase(resIdx);
      }
    }
}

void IrGraphBuilder::addEdgesToIrGraph(intermediate::Graph &graph, const intermediate::Mapping &mapping,
  IrNodesChanged &ir_nodes_changed) const
{
  TIME_PROFILE(addEdgesToIrGraph);

  const auto &nodeValid = validityInfo.nodeValid;
  const auto &resourceValid = validityInfo.resourceValid;

  // Making this incremental is POSSIBLE, but quite complex, need to replace `predecessors` set with a map to ints
  // and keep track of HOW MANY edges we added due to different resource lifetimes, as well as keep old lifetimes
  // in order to know what to remove. For now, we just rebuild everything from scratch and keep track of changes.
  IdIndexedMapping<intermediate::NodeIndex, decltype(intermediate::Node::predecessors), framemem_allocator> oldPredecessors;
  oldPredecessors.resize(graph.nodes.totalKeys());
  for (auto [idx, node] : graph.nodes.enumerate())
    eastl::swap(oldPredecessors[idx], node.predecessors);

  bool anyMissingExplicitDependencies = false;

  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    if (!nodeValid[nodeId])
      continue;

    // Iterating over all multiplexing indices is redundant but simple.
    // NOTE: This heavily abuses the fact that we "redirect" mappings for
    // nodes that are undermultiplexed.
    for (auto midx : IdRange<intermediate::MultiplexingIndex>(mapping.multiplexingExtent()))
    {
      // Note the dependencies can go missing here due to broken node
      // prunning. This might lead to rendering issues, so we logerr.
      for (auto prev : nodeData.precedingNodeIds)
      {
        auto index = mapping.mapNode(prev, midx);
        if (index == intermediate::NODE_NOT_MAPPED)
        {
          logwarn("daFG: A previous dependency of '%s', '%s'"
                  " went missing during IR generation!",
            registry.knownNames.getName(nodeId), registry.knownNames.getName(prev));
          anyMissingExplicitDependencies = true;
          continue;
        }
        graph.nodes[mapping.mapNode(nodeId, midx)].predecessors.insert(index);
      }
      for (auto next : nodeData.followingNodeIds)
      {
        auto index = mapping.mapNode(next, midx);
        if (index == intermediate::NODE_NOT_MAPPED)
        {
          logwarn("daFG: A follow dependency of '%s', '%s'"
                  " went missing during IR generation!",
            registry.knownNames.getName(nodeId), registry.knownNames.getName(next));
          anyMissingExplicitDependencies = true;
          continue;
        }
        graph.nodes[index].predecessors.insert(mapping.mapNode(nodeId, midx));
      }
    }
  }

  if (anyMissingExplicitDependencies)
  {
    dumpRawUserGraph();
    logerr("daFG: Some nodes had explicit dependencies on nodes that don't exist!"
           " See above for details and full graph dump.");
  }

  // Build bipartite graphs induced by resource lifetimes:
  // introductor before all modifiers, every modifier before every reader,
  // every reader before the consumer
  //
  //                  modifier1      reader1
  //                  modifier2      reader2
  //   introductor    modifier3      reader3    consumer
  //                  modifier4      reader4
  //                  modifier5      reader5
  //

  for (const auto [resId, lifetime] : depData.resourceLifetimes.enumerate())
  {
    if (!resourceValid[resId])
      continue;

    auto addEdge = [&graph, &mapping, &nodeValid](NodeNameId from, NodeNameId to) {
      // Modifiers, readers and consumers might've been pruned out.
      if (!nodeValid[from] || !nodeValid[to])
        return;
      for (auto midx : IdRange<intermediate::MultiplexingIndex>(mapping.multiplexingExtent()))
        graph.nodes[mapping.mapNode(from, midx)].predecessors.insert(mapping.mapNode(to, midx));
    };

    const auto nodeValidPred = [&nodeValid](NodeNameId n) { return nodeValid[n]; };
    const bool anyValidReaders = eastl::any_of(lifetime.readers.begin(), lifetime.readers.end(), nodeValidPred);
    const bool anyValidModifiers = eastl::any_of(lifetime.modificationChain.begin(), lifetime.modificationChain.end(), nodeValidPred);

    // There are 4 cases of how the graph should look depending on
    // whether modifier and/or reader sets are empty.
    if (!anyValidReaders && !anyValidModifiers)
    {
      // Both parts are empty

      if (lifetime.consumedBy != NodeNameId::Invalid)
        addEdge(lifetime.consumedBy, lifetime.introducedBy);
    }
    else if (anyValidReaders != anyValidModifiers)
    {
      // One empty, one not empty
      auto &middle = anyValidReaders ? lifetime.readers : lifetime.modificationChain;

      // introductor -> middle
      for (NodeNameId middleNode : middle)
        addEdge(middleNode, lifetime.introducedBy);

      // middle -> consumer
      if (lifetime.consumedBy != NodeNameId::Invalid)
        for (NodeNameId middleNode : middle)
          addEdge(lifetime.consumedBy, middleNode);
    }
    else
    {
      // Both non-empty, build full bipartite graph

      // introductor -> modifier
      for (NodeNameId modifier : lifetime.modificationChain)
        addEdge(modifier, lifetime.introducedBy);

      // modifier -> reader
      for (NodeNameId modifier : lifetime.modificationChain)
        for (NodeNameId reader : lifetime.readers)
          addEdge(reader, modifier);

      // reader -> consumer
      if (lifetime.consumedBy != NodeNameId::Invalid)
        for (NodeNameId reader : lifetime.readers)
          addEdge(lifetime.consumedBy, reader);
    }
  }


  // Connect source sentinel node to all nodes without predecessors.
  // We could simply connect it to ALL nodes, but that would degrade the perf
  // of various graph traversals we do later.
  // Note that we cannot do the same for the destination sentinel node here,
  // because we cannot predict which nodes with no actual successors but
  // with their resources consumed via history requests will remain after prunning.
  for (auto [nodeId, node] : graph.nodes.enumerate())
    if (node.predecessors.empty() && nodeId != SOURCE_SENTINEL_NODE_ID_IN_RAW_GRAPH &&
        nodeId != DESTINATION_SENTINEL_NODE_ID_IN_RAW_GRAPH)
      node.predecessors.insert(SOURCE_SENTINEL_NODE_ID_IN_RAW_GRAPH);

  for (auto [idx, node] : graph.nodes.enumerate())
    if (oldPredecessors[idx] != node.predecessors)
      ir_nodes_changed[idx] = true;
}

auto IrGraphBuilder::findSinkIrNodes(const intermediate::Mapping &mapping, eastl::span<const ResNameId> sinkResources) const -> SinkSet
{
  TIME_PROFILE(findSinkIrNodes);

  SinkSet result;
  const auto findIrNodes = [this, &mapping, &result](NodeNameId id) {
    if (!validityInfo.nodeValid.test(id, false))
      return;
    for (auto midx : IdRange<intermediate::MultiplexingIndex>(mapping.multiplexingExtent()))
    {
      intermediate::NodeIndex idx = mapping.mapNode(id, midx);
      result.emplace(idx);
    }
  };

  for (auto [id, node] : registry.nodes.enumerate())
    if (node.sideEffect == SideEffects::External)
      findIrNodes(id);

  for (const auto &resId : sinkResources)
  {
    if (!validityInfo.resourceValid.test(resId, false))
      continue;

    const auto &lifetime = depData.resourceLifetimes[resId];
    findIrNodes(lifetime.introducedBy);
    for (auto nodeId : lifetime.modificationChain)
      findIrNodes(nodeId);
    if (lifetime.consumedBy != NodeNameId::Invalid)
    {
      logerr("daFG: Resource '%s' is marked as a sink resource, but it has a consumer node '%s'. "
             "Sink resources logically should not have consumer nodes. Proceeding anyways.",
        registry.knownNames.getName(resId), registry.knownNames.getName(lifetime.consumedBy));
    }
  }

  return result;
}

eastl::pair<IrGraphBuilder::DisplacementFmem, IrGraphBuilder::EdgesToBreakFmem> IrGraphBuilder::pruneGraph(
  const intermediate::Graph &graph, const intermediate::Mapping &mapping, eastl::span<const intermediate::NodeIndex> sinksNodes) const
{
  TIME_PROFILE(pruneGraph);

  // We prune the nodes using a dfs from the set of sink nodes, changing
  // the ir graph to be topsorted in the process. This not the actual
  // order that will be used when executing the graph, having a
  // topsort is just convenient for doing algorithms on the graph.
  static constexpr intermediate::NodeIndex NOT_VISITED = MINUS_ONE_SENTINEL_FOR<intermediate::NodeIndex>;

  eastl::pair<DisplacementFmem, EdgesToBreakFmem> result;

  auto &[outTime, edgesToBreak] = result;
  outTime.assign(graph.nodes.totalKeys(), NOT_VISITED);
  edgesToBreak.reserve(graph.nodes.totalKeys());

  FRAMEMEM_VALIDATE;

  enum class Color
  {
    WHITE,
    GRAY,
    BLACK
  };
  dag::Vector<Color, framemem_allocator> colors(graph.nodes.totalKeys(), Color::WHITE);

  // It is not enough to simply launch a DFS from every sink node, we
  // might have nodes the are not reachable from any sink, but whose
  // resources are used through history requests. We accumulate such
  // nodes into a next wave and launch a new DFS from them afterwards.
  // Note that this does not ruin the topsort, as we are only adding
  // nodes that were not reachable previously.
  dag::Vector<intermediate::NodeIndex, framemem_allocator> wave;
  wave.reserve(graph.nodes.totalKeys());
  wave.assign(sinksNodes.begin(), sinksNodes.end());

  // Incremented each time DFS leaves a node
  eastl::underlying_type_t<intermediate::NodeIndex> timer = 0;

  while (!wave.empty())
  {
    eastl::stack<intermediate::NodeIndex, dag::Vector<intermediate::NodeIndex, framemem_allocator>> stack;
    stack.get_container().reserve(colors.size());
    for (auto id : wave)
      if (colors[id] == Color::WHITE)
        stack.push(id);
    wave.clear();

    while (!stack.empty())
    {
      const auto curr = stack.top();

      // If this is a fresh node that we haven't seen yet,
      // mark it gray and enqueue all predecessors
      // Note that we do NOT pop the curr node from the stack!!!
      if (colors[curr] == Color::WHITE)
      {
        colors[curr] = Color::GRAY;

        const auto &currNode = graph.nodes[curr];

        for (intermediate::NodeIndex next : currNode.predecessors)
          if (colors[next] == Color::WHITE)
            stack.push(next);
          else if (DAGOR_UNLIKELY(colors[next] == Color::GRAY))
          {
            // We've found a cycle, the world is sort of broken, complain.
            const auto &stackRaw = stack.get_container();
            const auto it = eastl::find(stackRaw.begin(), stackRaw.end(), next);

            eastl::string path;

            const auto appendNode = [&path, &graph, this](intermediate::NodeIndex idx) {
              const auto &node = graph.nodes[idx];
              const char *nodeName = frontendNodeName(node);
              const auto midx = graph.nodes[idx].multiplexingIndex;
              path.append_sprintf("%s {%#04X}", nodeName, midx);
            };

            eastl::bitvector<framemem_allocator> visited{graph.nodes.totalKeys(), false};
            for (auto i = it; i != stackRaw.end(); ++i)
              if (colors[*i] == Color::GRAY && !visited.test(*i, false))
              {
                visited.set(*i, true);
                appendNode(*i);
                path += " -> ";
              }
            appendNode(*it);
            // We will issue a single logerr outside of this function
            // to get good looking logs from production
            logwarn("daFG: Dependency cycle detected in framegraph IR: %s", path);

            edgesToBreak.push_back({curr, next});
          }

        // Iterate all history resource requests of this node and
        // add nodes responsible for their production to the wave.
        if (currNode.frontendNode)
          for (const auto &[resId, req] : registry.nodes[*currNode.frontendNode].historyResourceReadRequests)
            if (validityInfo.resourceValid[resId])
            {
              const auto &lifetime = depData.resourceLifetimes[resId];

              for (const auto modifierId : lifetime.modificationChain)
              {
                const auto modifierIndex = mapping.mapNode(modifierId, currNode.multiplexingIndex);
                if (modifierIndex != intermediate::NODE_NOT_MAPPED && colors[modifierIndex] == Color::WHITE)
                  wave.push_back(modifierIndex);
              }

              const auto introducedByIndex = mapping.mapNode(lifetime.introducedBy, currNode.multiplexingIndex);
              G_ASSERT(introducedByIndex != intermediate::NODE_NOT_MAPPED);
              if (colors[introducedByIndex] == Color::WHITE)
                wave.push_back(introducedByIndex);
            }

        continue;
      }

      // After all children of a node have been completely visited,
      // we will find this node at the top of the stack again,
      // this time with gray mark. Mark it black and record out time.
      // Conceptually our algorithm is "leaving" this node,
      // so we record the leaving time.
      if (colors[curr] == Color::GRAY)
      {
        outTime[curr] = static_cast<intermediate::NodeIndex>(timer++);
        colors[curr] = Color::BLACK;
      }

      // The node might have been added multiple time to the stack,
      // but only the last entry got "visited", so ignore others. E.g.:
      //       B
      //      ^ ^
      //     /   \
      //    /     \
      //   /       \
      //  A-------->C
      // Stack after each iteration:
      // A
      // ABC
      // ABCB
      // ABCB
      // ABC
      // AB  <- B is black here, because it was already visited
      // A
      stack.pop();
    }
  }

  // Sentinel destination node isn't supposed to be visited, but must still remain after pruning.
  outTime[DESTINATION_SENTINEL_NODE_ID_IN_RAW_GRAPH] = static_cast<intermediate::NodeIndex>(timer++);

  if (pedantic.get() && eastl::find(outTime.begin(), outTime.end(), NOT_VISITED) != outTime.end())
  {
    dumpRawUserGraph();

    for (auto [nodeId, time] : outTime.enumerate())
      if (time == NOT_VISITED)
        logerr("daFG: Node '%s' is pruned. See above for full user graph dump.", frontendNodeName(graph.nodes[nodeId]));
  }

  return result;
}

struct StateFieldSetter
{
  // Use this overload set to add special processing for fields
  // that change type.

  template <class T, class U>
  void set(T &to, const U &from)
  {
    to = from;
  }

  void set(intermediate::ShaderBlockBindings &to, const ShaderBlockLayersInfo &from)
  {
    to.objectLayer = from.objectLayer;
    to.sceneLayer = from.sceneLayer;
    to.frameLayer = from.frameLayer;
  }

  void set(intermediate::VrsState &to, const VrsStateRequirements &from)
  {
    to.rateX = from.rateX;
    to.rateY = from.rateY;
    to.vertexCombiner = from.vertexCombiner;
    to.pixelCombiner = from.pixelCombiner;
  }

  void set(eastl::optional<intermediate::RenderPass> &to, const eastl::optional<VirtualPassRequirements> &from)
  {
    if (!from.has_value())
      return;

    to.emplace();

    const auto getAttachment = [this](VirtualSubresourceRef res) -> eastl::optional<intermediate::SubresourceRef> {
      if (res.nameId != ResNameId::Invalid)
      {
        const auto resNameId = nameResolver.resolve(res.nameId);
        const auto resIndex = mapping.mapRes(resNameId, multiIndex);
        if (resIndex == intermediate::RESOURCE_NOT_MAPPED)
        {
          // Sanity check, node should've been skipped otherwise
          G_ASSERT(registry.nodes[nodeId].resourceRequests.find(res.nameId)->second.optional);
          return eastl::nullopt;
        }
        return intermediate::SubresourceRef{resIndex, res.mipLevel, res.layer};
      }
      return eastl::nullopt;
    };

    to->colorAttachments.reserve(from->colorAttachments.size());
    for (const auto &color : from->colorAttachments)
      to->colorAttachments.push_back(getAttachment(color));

    to->depthAttachment = getAttachment(from->depthAttachment);
    to->depthReadOnly = from->depthReadOnly;

    to->resolves.reserve(from->resolves.size());
    for (const auto &[src, dst] : from->resolves)
    {
      to->resolves.emplace(mapping.mapRes(nameResolver.resolve(src), multiIndex),
        mapping.mapRes(nameResolver.resolve(dst), multiIndex));
    }

    if (from->colorAttachments.size() == 1)
    {
      const auto resolvedResId = nameResolver.resolve(from->colorAttachments[0].nameId);
      const auto &createdResData = registry.resources[depData.renamingRepresentatives[resolvedResId]].createdResData;
      // may be an optional color attachment
      to->isLegacyPass = createdResData.has_value() && eastl::holds_alternative<DriverDeferredTexture>(createdResData->creationInfo);
    }
    else
      to->isLegacyPass = false;

    to->vrsRateAttachment = getAttachment(from->vrsRateAttachment);
  }

  void set(intermediate::BindingsMap &to, const BindingsMap &from)
  {
    // We are always called on valid nodes and broken bindings cannot
    // occur here
    for (const auto &[bindIdx, bindInfo] : from)
    {
      // sanity check just in case
      G_ASSERT(bindInfo.type != BindingType::Invalid);

      const ResNameId resNameId = nameResolver.resolve(bindInfo.resource);
      const auto resIndex = mapping.mapRes(resNameId, bindInfo.history ? historyMultiIndex : multiIndex);

      if (resIndex == intermediate::RESOURCE_NOT_MAPPED)
      {
        // Sanity check, node should've been skipped otherwise
        G_ASSERT((bindInfo.history ? registry.nodes[nodeId].historyResourceReadRequests : registry.nodes[nodeId].resourceRequests)
                   .find(bindInfo.resource)
                   ->second.optional);

        // bind "nothing", nullptr for projector is intentional
        // cuz it's not supposed to be called for nullopt resource idx
        to.emplace(bindIdx,
          intermediate::Binding{bindInfo.type, eastl::nullopt, false, false, bindInfo.optional, bindInfo.projectedTag, nullptr});
        continue;
      }

      to.emplace(bindIdx, intermediate::Binding{bindInfo.type, resIndex, bindInfo.history, bindInfo.reset, bindInfo.optional,
                            bindInfo.projectedTag, bindInfo.projector});
    }
  }


  void set(eastl::optional<intermediate::VertexSource> &to, const eastl::optional<VertexSource> &from)
  {
    if (!from.has_value())
      return;

    const ResNameId resolvedResId = nameResolver.resolve(from->buffer);
    const intermediate::ResourceIndex resIndex = mapping.mapRes(resolvedResId, multiIndex);
    if (resIndex == intermediate::RESOURCE_NOT_MAPPED)
    {
      G_ASSERT(registry.nodes[nodeId].resourceRequests.find(resolvedResId)->second.optional);
      to.emplace(intermediate::VertexSource{eastl::nullopt, 0});
    }
    else
      to.emplace(intermediate::VertexSource{resIndex, from->stride});
  }

  void set(eastl::optional<intermediate::IndexSource> &to, const eastl::optional<IndexSource> &from)
  {
    if (!from.has_value())
      return;

    const ResNameId resolvedResId = nameResolver.resolve(from->buffer);
    const intermediate::ResourceIndex resIndex = mapping.mapRes(resolvedResId, multiIndex);
    if (resIndex == intermediate::RESOURCE_NOT_MAPPED)
    {
      G_ASSERT(registry.nodes[nodeId].resourceRequests.find(resolvedResId)->second.optional);
      to.emplace(intermediate::IndexSource{eastl::nullopt});
    }
    else
      to.emplace(intermediate::IndexSource{resIndex});
  }


  const InternalRegistry &registry;
  const NameResolver &nameResolver;
  const DependencyData &depData;
  const intermediate::Mapping &mapping;
  intermediate::MultiplexingIndex multiIndex;
  intermediate::MultiplexingIndex historyMultiIndex;
  NodeNameId nodeId;
};

intermediate::RequiredNodeState IrGraphBuilder::calcNodeState(NodeNameId node_id, intermediate::MultiplexingIndex multi_index,
  intermediate::MultiplexingIndex history_multi_index, const intermediate::Mapping &mapping) const
{
  const auto &nodeData = registry.nodes[node_id];

  const bool hasVertexSource =
    eastl::any_of(nodeData.vertexSources.begin(), nodeData.vertexSources.end(), [](const auto &vs) { return vs.has_value(); });
  if (!nodeData.stateRequirements && !nodeData.renderingRequirements && nodeData.bindings.empty() &&
      nodeData.shaderBlockLayers.objectLayer == -1 && nodeData.shaderBlockLayers.sceneLayer == -1 &&
      nodeData.shaderBlockLayers.frameLayer == -1 && !hasVertexSource && !nodeData.indexSource)
    return {};

  StateFieldSetter setter{registry, nameResolver, depData, mapping, multi_index, history_multi_index, node_id};
  intermediate::RequiredNodeState result;

  if (nodeData.stateRequirements)
  {
#define SET(to, from) setter.set(result.to, nodeData.stateRequirements->from)

    SET(wire, supportsWireframe);
    SET(vrs, vrsState);
    SET(shaderOverrides, pipelineStateOverride);
#undef SET
  }
  setter.set(result.asyncPipelines, nodeData.allowAsyncPipelines);
  setter.set(result.pass, nodeData.renderingRequirements);
  setter.set(result.shaderBlockLayers, nodeData.shaderBlockLayers);
  setter.set(result.bindings, nodeData.bindings);
  for (auto [stream, vertexSource] : enumerate(nodeData.vertexSources))
    setter.set(result.vertexSources[stream], vertexSource);
  setter.set(result.indexSource, nodeData.indexSource);

  return result;
}

void IrGraphBuilder::setIrGraphDebugNames(intermediate::Graph &graph, const IrNodesChanged &ir_nodes_changed,
  const IrResourcesChanged &ir_resources_changed) const
{
  TIME_PROFILE(setIrGraphDebugNames);

  // It turns out that string concatenation is amazingly slow

  // The " {64}" part
  constexpr size_t MULTI_INDEX_STRING_APPROX_SIZE = 10;

  auto genMultiplexingRow = [](uint32_t idx) {
    static constexpr const char *HEX_DIGITS = "0123456789ABCDEF";
    eastl::fixed_string<char, 16> result = " {0x$$$$} ";
    for (uint32_t i = 7; i >= 4; --i)
    {
      result[i] = HEX_DIGITS[idx & 0xF];
      idx >>= 4;
    }
    return result;
  };

  graph.nodeNames.reserve(graph.nodes.totalKeys());
  for (auto [nodeIdx, irNode] : graph.nodes.enumerate())
  {
    if (!ir_nodes_changed[nodeIdx])
      continue;
    const char *frontendName = frontendNodeName(irNode);

    G_ASSERT_CONTINUE(!graph.nodeNames.isMapped(nodeIdx));

    auto &debugName = *graph.nodeNames.emplaceAt(nodeIdx);
    debugName.reserve(strlen(frontendName) + MULTI_INDEX_STRING_APPROX_SIZE);
    debugName += frontendName;
    debugName += genMultiplexingRow(irNode.multiplexingIndex).c_str();
  }

  graph.resourceNames.reserve(graph.resources.totalKeys());
  for (auto [resIdx, irRes] : graph.resources.enumerate())
  {
    if (!ir_resources_changed[resIdx])
      continue;

    G_ASSERT_CONTINUE(!graph.resourceNames.isMapped(resIdx));

    auto &debugName = *graph.resourceNames.emplaceAt(resIdx);

    {
      size_t size = 2 + MULTI_INDEX_STRING_APPROX_SIZE;
      for (auto userResId : irRes.frontendResources)
      {
        const eastl::string_view name = registry.knownNames.getName(userResId);
        size += name.size() + 2;
      }
      debugName.reserve(size);
    }

    if (irRes.frontendResources.size() > 1)
      debugName += '[';
    for (auto userResId : irRes.frontendResources)
    {
      const char *namePtr = registry.knownNames.getName(userResId);
      if (namePtr[0] == '/')
        namePtr++;

      debugName += namePtr;
      debugName += ", ";
    }
    debugName.resize(debugName.size() - 2);
    if (irRes.frontendResources.size() > 1)
      debugName += ']';
    debugName += genMultiplexingRow(irRes.multiplexingIndex).c_str();
  }
}

void IrGraphBuilder::dumpRawUserGraph() const
{
  dump_internal_registry(
    registry, [this](NodeNameId id) { return validityInfo.nodeValid.test(id); },
    [this](ResNameId id) { return validityInfo.resourceValid.test(id); });
}

} // namespace dafg
