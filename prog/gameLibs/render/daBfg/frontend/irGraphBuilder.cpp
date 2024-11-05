// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "irGraphBuilder.h"

#include <memory/dag_framemem.h>
#include <util/dag_convar.h>
#include <math/random/dag_random.h>
#include <EASTL/stack.h>

#include <frontend/multiplexingInternal.h>
#include <frontend/dumpInternalRegistry.h>
#include <frontend/dependencyData.h>
#include <frontend/validityInfo.h>
#include <frontend/nameResolver.h>
#include <common/resourceUsage.h>


extern ConVarT<bool, false> randomize_order;

namespace dabfg
{

intermediate::Graph IrGraphBuilder::build(multiplexing::Extents extents) const
{
  FRAMEMEM_REGION;

  // Build the intermediate graph.
  // Only valid nodes and resources are included, so all of the following
  // graph compilation process does not need to do validity assertions at
  // all.

  intermediate::Graph graph;
  intermediate::Mapping mapping(registry.knownNames.nameCount<NodeNameId>(), registry.knownNames.nameCount<ResNameId>(),
    multiplexing_extents_to_ir(extents));

  graph.resources.reserve(mapping.mappedResourceIndexCount());
  addResourcesToGraph(graph, mapping, extents);

  // Fake source/destination nodes, see below
  const size_t INTERNAL_FAKE_NODES = 2;
  graph.nodes.reserve(mapping.mappedNodeIndexCount() + INTERNAL_FAKE_NODES);
  graph.nodeStates.reserve(mapping.mappedNodeIndexCount() + INTERNAL_FAKE_NODES);

  // We add a fake source node that precedes all nodes. This makes our
  // graph a proper control flow graph in the strict mathematical sense.
  const auto sourceId = graph.nodes.appendNew(intermediate::Node{}).first;
  graph.nodeStates.appendNew();

  addNodesToGraph(graph, mapping, extents);

  fixupFalseHistoryFlags(graph);

  addEdgesToIrGraph(graph, mapping);

  // Make the source node actually precede everything.
  for (auto [nodeId, node] : graph.nodes.enumerate())
    if (node.predecessors.empty() && nodeId != sourceId)
      node.predecessors.insert(sourceId);

  graph.validate();

  const auto sinks = findSinkIrNodes(graph, mapping, {registry.sinkExternalResources.begin(), registry.sinkExternalResources.end()});
  G_ASSERT_DO_AND_LOG(!sinks.empty(), dumpRawUserGraph(),
    "All specified frame graph sinks were skipped due to broken resource pruning! "
    "No rendering will be done! Report this to programmers!");
  // Pruning (this makes mappings out of date)
  auto [displacement, edgesToBreak] = pruneGraph(graph, mapping, sinks);
  if (!edgesToBreak.empty())
  {
    dumpRawUserGraph();
    logerr("The IR graph contained dependency cycles! "
           "See above for the cycles and the full graph dump! "
           "Note that this is a bug in this game's frame graph and should be reported to programmers!");

    // Cycles must be broken, IR graph being a control flow graph is an
    // invariant that must be upheld.
    for (auto [from, to] : edgesToBreak)
      graph.nodes[from].predecessors.erase(to);

    // After an edge was removed, we might get "hanging" nodes, which
    // are not permitted inside a CFG, so we have to fix that up.
    // Doing more pruning is not what we want to do here, as we are
    // desperately trying to recover and render at least something.
    for (auto [from, _] : edgesToBreak)
      if (graph.nodes[from].predecessors.empty())
        graph.nodes[from].predecessors.insert(sourceId);
  }
  graph.choseSubgraph(displacement);

  // Also add a fake destination node that succeeds all nodes, this
  // also makes our life easier later down the line than having to deal
  // with multiple sinks.
  // Note that this obviously does not ruin the topsort order.
  {
    IdIndexedFlags<intermediate::NodeIndex, framemem_allocator> hasIncomingEdges(graph.nodes.size() + 1, false);
    for (auto [nodeId, node] : graph.nodes.enumerate())
      for (auto pred : node.predecessors)
        hasIncomingEdges[pred] = true;

    graph.nodeStates.appendNew();
    const auto destinationId = graph.nodes.appendNew(intermediate::Node{0, {}, {}, {}, {}}).first;

    for (auto [nodeId, node] : graph.nodes.enumerate())
      if (!hasIncomingEdges[nodeId] && nodeId != destinationId)
        graph.nodes[destinationId].predecessors.insert(nodeId);
  }

  graph.validate();

  setIrGraphDebugNames(graph);

  return graph;
}

const char *IrGraphBuilder::frontendNodeName(const intermediate::Node &node) const
{
  if (node.frontendNode)
    return registry.knownNames.getName(*node.frontendNode);

  return node.predecessors.empty() ? "FAKE_SRC" : "FAKE_DST";
}


template <class F, class G>
void IrGraphBuilder::scanPhysicalResourceUsages(ResNameId res_id, const F &process_reader, const G &process_modifier) const
{
  FRAMEMEM_VALIDATE;
  // Save scanned resources to prevent hanging if there is cycle in renaming chains
  dag::VectorSet<ResNameId, eastl::less<ResNameId>, framemem_allocator> knownCandidates;
  const auto scanNodeSlotsOrResource = [this](NodeNameId node_id, ResNameId res_id, const auto &process_node) {
    for (const auto unresolvedResId : nameResolver.unresolve(node_id, res_id))
      process_node(node_id, unresolvedResId);
  };
  auto resIdCandidate = res_id;
  while (true)
  {
    // We don't want to consider broken/invalid nodes when looking
    // for an appropriate usage.
    for (const auto modifier : depData.resourceLifetimes[resIdCandidate].modificationChain)
      if (validityInfo.nodeValid[modifier])
        scanNodeSlotsOrResource(modifier, resIdCandidate, process_modifier);

    for (const auto reader : depData.resourceLifetimes[resIdCandidate].readers)
      if (validityInfo.nodeValid[reader])
        scanNodeSlotsOrResource(reader, resIdCandidate, process_reader);

    // The renaming chain might've been "torn" by an invalid node.
    if (!validityInfo.resourceValid[depData.renamingChains[resIdCandidate]])
      break;

    resIdCandidate = depData.renamingChains[resIdCandidate];
    if (knownCandidates.find(resIdCandidate) != knownCandidates.end())
      break;
    knownCandidates.insert(resIdCandidate);
  }
}

auto IrGraphBuilder::findFirstUsageAndUpdatedCreationFlags(ResNameId res_id, uint32_t initial_flags) const
{
  // If the node introducer do not specify usage, find the first usage
  // in the modification requests in all renamings.
  auto firstUsage = registry.nodes[depData.resourceLifetimes[res_id].introducedBy].resourceRequests.find(res_id)->second.usage;
  const auto type = registry.resources[res_id].type;
  bool hasReaders = false;

  const auto processReaders = [this, type, &firstUsage, &initial_flags, &hasReaders](NodeNameId node_id, ResNameId renamed_res_id) {
    hasReaders = true;
    const auto usage = registry.nodes[node_id].resourceRequests.find(renamed_res_id)->second.usage;
    update_creation_flags_from_usage(initial_flags, usage, type);
    if (firstUsage.type == Usage::UNKNOWN)
      logerr("daBfg: Resource %s is read by %s before first modify usage", registry.knownNames.getName(renamed_res_id),
        registry.knownNames.getName(node_id));
  };

  const auto processModifiers = [this, type, &firstUsage, &initial_flags](NodeNameId node_id, ResNameId renamed_res_id) {
    const auto usage = registry.nodes[node_id].resourceRequests.find(renamed_res_id)->second.usage;
    update_creation_flags_from_usage(initial_flags, usage, type);
    if (firstUsage.type == Usage::UNKNOWN)
      firstUsage = usage;
  };

  scanPhysicalResourceUsages(res_id, processReaders, processModifiers);

  if (firstUsage.type == Usage::UNKNOWN && hasReaders)
    logerr("daBfg: Resource %s is not initialized and it's first modify usage is UNKNOWN.", registry.knownNames.getName(res_id));

  return eastl::pair{firstUsage, initial_flags};
}


void IrGraphBuilder::addResourcesToGraph(intermediate::Graph &graph, intermediate::Mapping &mapping,
  multiplexing::Extents extents) const
{
  FRAMEMEM_VALIDATE;

  const auto &nodeValid = validityInfo.nodeValid;
  const auto &resourceValid = validityInfo.resourceValid;

  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    if (!nodeValid[nodeId])
      continue;


    const auto nodeMultiplexingExtents = extents_for_node(nodeData.multiplexingMode, extents);

    auto processResource = [&extents, &nodeMultiplexingExtents, &graph, &mapping](ResNameId resId, const multiplexing::Index &midx,
                             const intermediate::ConcreteResource &variant) {
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
    };

    for (auto resId : nodeData.createdResources)
    {
      if (!resourceValid[resId])
        continue;

      const auto &resData = registry.resources[resId];

      if (auto resDesc = eastl::get_if<ResourceDescription>(&resData.creationInfo))
      {
        auto [firstUsage, updatedFlags] = findFirstUsageAndUpdatedCreationFlags(resId, resDesc->asBasicRes.cFlags);

        auto activation = get_activation_from_usage(History::DiscardOnFirstFrame, firstUsage, resData.type);

        ResourceDescription updatedDesc = *resDesc;
        updatedDesc.asBasicRes.cFlags = updatedFlags;
        if (activation)
        {
          updatedDesc.asBasicRes.activation = *activation;
        }
        else
        {
          logerr("Could not infer an activation action for %s from the first usage of a"
                 " v2 API resource, either an application error or a bug in frame graph!",
            registry.knownNames.getName(resId));
          if (resData.type == ResourceType::Texture)
            updatedDesc.asBasicRes.activation = ResourceActivationAction::DISCARD_AS_RTV_DSV;
          else if (resData.type == ResourceType::Buffer)
            updatedDesc.asBasicRes.activation = ResourceActivationAction::DISCARD_AS_UAV;
        }

        eastl::optional<AutoResolutionData> resolvedAutoResData = resData.resolution;
        if (resolvedAutoResData)
          resolvedAutoResData->id = nameResolver.resolve(resolvedAutoResData->id);

        intermediate::ScheduledResource res{updatedDesc, resData.type, resolvedAutoResData, resData.history};
        for (multiplexing::Index i = {}; index_inside_extents(i, extents); i = next_index(i, extents))
          processResource(resId, i, res);
      }
      else if (auto blobDesc = eastl::get_if<BlobDescription>(&resData.creationInfo))
      {
        intermediate::ScheduledResource res{*blobDesc, resData.type, eastl::nullopt, resData.history};
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
        // to propagate it to the first renamee. This code is not ideal,
        // as it will propagate a flag even for a resource in the middle,
        // of a chain, but it's ok cuz we validate that no resources in
        // the middle have history, so the if will work only for the
        // last resource in a chain.
        if (originalRes.isScheduled() && registry.resources[toResId].history != History::No)
          originalRes.asScheduled().history = registry.resources[toResId].history;
      }
    }
  }

  for (auto [resIdx, res] : graph.resources.enumerate())
  {
    G_ASSERT(res.frontendResources.size() == 1);

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

void IrGraphBuilder::addNodesToGraph(intermediate::Graph &graph, intermediate::Mapping &mapping, multiplexing::Extents extents) const
{
  FRAMEMEM_VALIDATE;

  const auto &nodeValid = validityInfo.nodeValid;
  const auto &resourceValid = validityInfo.resourceValid;

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
    const auto &nodeData = registry.nodes[nodeId];
    // TODO: on mobile TBDR, group nodes together based on their
    // renderpass. Breaking an RP is extremely bad there.
    // Priorities will have to be reworked as well.

    const auto nodeMultiplexingExtents = extents_for_node(nodeData.multiplexingMode, extents);

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
        continue;
      }

      auto [idx, irNode] = graph.nodes.appendNew();
      mapping.mapNode(nodeId, irMultiplexingIndex) = idx;
      irNode.frontendNode = nodeId;
      irNode.priority = nodeData.priority;
      irNode.multiplexingIndex = irMultiplexingIndex;


      eastl::array<dag::VectorMap<ResNameId, uint32_t, eastl::less<ResNameId>, framemem_allocator>, 2>
        resolvedResIdxToIndexInIrRequests;

      for (auto &req : resolvedResIdxToIndexInIrRequests)
        req.reserve(registry.resources.size());

      auto processRequest = [&irNode = irNode, &mapping, &resourceValid, &nodeData, &graph, &resolvedResIdxToIndexInIrRequests, this,
                              irMultiplexingIndex, nodeId, extents](bool history, ResNameId resId, const ResourceRequest &req) {
        // Skip optional requests for missing resources
        if (req.optional && !resourceValid[resId])
          return;

        // Note that this resMapping might redirect us to a resource
        // that is less multiplexed than us. This is ok.
        const auto resIndex = mapping.mapRes(resId, irMultiplexingIndex);

        // On the other hand, if we are less multiplexed than the
        // requested resource, we basically get undefined behaviour:
        // we don't know which one of multiple physical resources
        // we actually want to read. Assert.
        const auto resMplexMode = registry.nodes[depData.resourceLifetimes[resId].introducedBy].multiplexingMode;
        G_ASSERT_DO_AND_LOG(!less_multiplexed(nodeData.multiplexingMode, resMplexMode, extents), dumpRawUserGraph(),
          "Node '%s' requested resource '%s', which is more"
          " multiplexed than the node due to being produced by a"
          " node that is more multiplexed. This is invalid usage.",
          registry.knownNames.getName(nodeId), registry.knownNames.getName(resId));


        // Note that name resolution might have mapped several requests
        // into the same resource requests, in which case we need to
        // merge them.
        if (const auto alreadyProcessedIt = resolvedResIdxToIndexInIrRequests[history].find(resId);
            alreadyProcessedIt == resolvedResIdxToIndexInIrRequests[history].end())
        {
          resolvedResIdxToIndexInIrRequests[history].emplace(resId, irNode.resourceRequests.size());

          auto &irRequest = irNode.resourceRequests.emplace_back();
          irRequest.resource = resIndex;
          irRequest.usage = intermediate::ResourceUsage{req.usage.access, req.usage.type, req.usage.stage};
          irRequest.fromLastFrame = history;
        }
        else
        {
          auto &irRequest = irNode.resourceRequests[alreadyProcessedIt->second];
          // Otherwise this node must have been marked as broken and removed.
          G_ASSERT(irRequest.usage.access == req.usage.access && irRequest.usage.type == req.usage.type);

          irRequest.usage.stage = irRequest.usage.stage | req.usage.stage;
        }

        if (history && registry.resources[resId].history == History::No)
        {
          dumpRawUserGraph();
          logerr("Node '%s' requested history for resource '%s', but the"
                 " resource was created with no history enabled. Please"
                 " specify a history behavior in the resource creation call!"
                 " Artifacts and crashes are possible otherwise!",
            registry.knownNames.getName(nodeId), registry.knownNames.getName(resId));
        }

        // NOTE: we could theoretically prune nodes that are broken
        // in this way, but that's a pretty complicated change with
        // dubious benefits.
        if (const auto &res = graph.resources[resIndex];
            res.isScheduled() && res.asScheduled().isCpuResource() &&
            !resourceTagsMatch(req.subtypeTag, res.asScheduled().getCpuDescription().typeTag))
        {
          dumpRawUserGraph();
          logerr("Node '%s' requested resource '%s' with a mismatched"
                 " subtype tag! Please make sure that you are requesting"
                 " this resource with the same type as it was produced with!",
            registry.knownNames.getName(nodeId), registry.knownNames.getName(resId));
        }
      };

      for (const auto &[resId, req] : nodeData.resourceRequests)
        processRequest(false, nameResolver.resolve(resId), req);
      for (const auto &[resId, req] : nodeData.historyResourceReadRequests)
        processRequest(true, nameResolver.resolve(resId), req);

      graph.nodeStates.set(idx, calcNodeState(nodeId, irMultiplexingIndex, mapping));
    }
  }
}

void IrGraphBuilder::fixupFalseHistoryFlags(intermediate::Graph &graph) const
{
  FRAMEMEM_VALIDATE;

  IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> historyWasRequested;
  historyWasRequested.reserve(graph.resources.size());

  for (const auto &node : graph.nodes)
    for (const auto &req : node.resourceRequests)
      if (req.fromLastFrame)
        historyWasRequested.set(req.resource, true);

  for (auto [resIdx, res] : graph.resources.enumerate())
    if (res.isScheduled() && res.asScheduled().history != History::No && !historyWasRequested.test(resIdx))
      // TODO: should this be an error?
      res.asScheduled().history = History::No;
}

void IrGraphBuilder::addEdgesToIrGraph(intermediate::Graph &graph, const intermediate::Mapping &mapping) const
{
  const auto &nodeValid = validityInfo.nodeValid;
  const auto &resourceValid = validityInfo.resourceValid;

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
          logwarn("A previous dependency of '%s', '%s'"
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
          logwarn("A follow dependency of '%s', '%s'"
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
    logerr("Some nodes had explicit dependencies on nodes that don't exist!"
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
}

auto IrGraphBuilder::findSinkIrNodes(const intermediate::Graph &graph, const intermediate::Mapping &mapping,
  eastl::span<const ResNameId> sinkResources) const -> SinkSet
{
  SinkSet result;
  const auto findIrNodes = [this, &mapping, &result](NodeNameId id) {
    for (auto midx : IdRange<intermediate::MultiplexingIndex>(mapping.multiplexingExtent()))
    {
      intermediate::NodeIndex idx = mapping.mapNode(id, midx);
      if (idx == intermediate::NODE_NOT_MAPPED)
      {
        const auto name = registry.knownNames.getName(id);
        logerr("Node '%s' was requested as a sink but did not survive IR generation", name);
        continue;
      }

      result.emplace(idx);
    }
  };

  for (auto [id, node] : registry.nodes.enumerate())
  {
    if (node.sideEffect != SideEffects::External)
      continue;

    findIrNodes(id);
  }

  for (const auto &resId : sinkResources)
  {
    const auto &resData = registry.resources[depData.renamingRepresentatives[resId]];
    // We might want a renamed version of an external resource as a sink,
    // so the check must work on the original resource
    if (resData.type == ResourceType::Invalid)
    {
      logerr("Attempted to mark a non-existent resource '%s' as a sink!", registry.knownNames.getName(resId));
      continue;
    }

    if (!eastl::holds_alternative<ExternalResourceProvider>(resData.creationInfo) &&
        !eastl::holds_alternative<DriverDeferredTexture>(resData.creationInfo))
    {
      logerr("Attempted to mark a FG-internal resource '%s' as an "
             "externally consumed resource. This is impossible, as FG "
             "resources can only be accessed from within FG nodes!",
        registry.knownNames.getName(resId));
      continue;
    }

    bool anyUnmapped = false;
    for (auto midx : IdRange<intermediate::MultiplexingIndex>(mapping.multiplexingExtent()))
    {
      intermediate::ResourceIndex idx = mapping.mapRes(resId, midx);

      if (idx == intermediate::RESOURCE_NOT_MAPPED)
      {
        anyUnmapped = true;
        continue;
      }

      // Consider all nodes that access this resource sinks
      for (auto [i, node] : graph.nodes.enumerate())
        for (const intermediate::Request &req : node.resourceRequests)
          if (req.resource == idx && req.usage.access == Access::READ_WRITE)
            result.emplace(i);
    }

    if (anyUnmapped)
      logerr("Resource '%s' was requested as a sink, but was removed from "
             "the graph due to relevant registering/renaming nodes being broken!",
        registry.knownNames.getName(resId));
  }

  return result;
}

eastl::pair<IrGraphBuilder::DisplacementFmem, IrGraphBuilder::EdgesToBreakFmem> IrGraphBuilder::pruneGraph(
  const intermediate::Graph &graph, const intermediate::Mapping &mapping, eastl::span<const intermediate::NodeIndex> sinksNodes) const
{
  // We prune the nodes using a dfs from the destination node, changing
  // the ir graph to be topsorted in the process. This not the actual
  // order that will be used when executing the graph, having a
  // topsort is just convenient for doing DP on the graph.
  static constexpr intermediate::NodeIndex NOT_VISITED = MINUS_ONE_SENTINEL_FOR<intermediate::NodeIndex>;

  eastl::pair<DisplacementFmem, EdgesToBreakFmem> result;

  auto &[outTime, edgesToBreak] = result;
  outTime.assign(graph.nodes.size(), NOT_VISITED);
  edgesToBreak.reserve(graph.nodes.size());

  FRAMEMEM_VALIDATE;

  enum class Color
  {
    WHITE,
    GRAY,
    BLACK
  };
  dag::Vector<Color, framemem_allocator> colors(graph.nodes.size(), Color::WHITE);

  // It is not enough to simply launch a DFS from every sink node, we
  // might have nodes the are not reachable from any sink, but whose
  // resources are used through history requests. We accumulate such
  // nodes into a next wave and launch a new DFS from them afterwards.
  // Note that this does not ruin the topsort, as we are only adding
  // nodes that were not reachable previously.
  dag::Vector<intermediate::NodeIndex, framemem_allocator> wave;
  wave.reserve(graph.nodes.size());
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

            eastl::bitvector<framemem_allocator> visited{graph.nodes.size(), false};
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
            logwarn("Dependency cycle detected in framegraph IR: %s", path);

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
                if (colors[modifierIndex] == Color::WHITE)
                  wave.push_back(modifierIndex);
              }

              const auto introducedByIndex = mapping.mapNode(lifetime.introducedBy, currNode.multiplexingIndex);
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

  bool anyNodePruned = false;

  for (auto [nodeId, time] : outTime.enumerate())
    if (time == NOT_VISITED)
      anyNodePruned = true;

  if (anyNodePruned)
  {
    dumpRawUserGraph();

    for (auto [nodeId, time] : outTime.enumerate())
      if (time == NOT_VISITED)
        logerr("FG: Node '%s' is pruned. See above for full user graph dump.", frontendNodeName(graph.nodes[nodeId]));
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
          G_ASSERT(registry.nodes[nodeId].resourceRequests.find(resNameId)->second.optional);
          return eastl::nullopt;
        }
        return intermediate::SubresourceRef{mapping.mapRes(resNameId, multiIndex), res.mipLevel, res.layer};
      }
      return eastl::nullopt;
    };

    to->colorAttachments.reserve(from->colorAttachments.size());
    for (const auto &color : from->colorAttachments)
      to->colorAttachments.push_back(getAttachment(color));

    to->depthAttachment = getAttachment(from->depthAttachment);
    to->depthReadOnly = from->depthReadOnly;

    to->clears.reserve(from->clears.size());
    for (const auto &[resNameId, value] : from->clears)
    {
      const auto resIndex = mapping.mapRes(nameResolver.resolve(resNameId), multiIndex);
      if (resIndex == intermediate::RESOURCE_NOT_MAPPED)
      {
        // Sanity check, node should've been skipped otherwise
        G_ASSERT(registry.nodes[nodeId].resourceRequests.find(resNameId)->second.optional);
        continue;
      }

      if (auto dynClearColor = eastl::get_if<DynamicPassParameter>(&value))
      {
        const auto clearResIndex = mapping.mapRes(nameResolver.resolve(dynClearColor->resource), multiIndex);
        if (clearResIndex == intermediate::RESOURCE_NOT_MAPPED)
        {
          // Sanity check, node should've been skipped otherwise
          G_ASSERT(registry.nodes[nodeId].resourceRequests.find(dynClearColor->resource)->second.optional);
          continue;
        }
        intermediate::DynamicPassParameter irDynClearColor{clearResIndex, dynClearColor->projectedTag, dynClearColor->projector};
        to->clears.emplace(resIndex, irDynClearColor);
      }
      else if (auto clearColor = eastl::get_if<ResourceClearValue>(&value))
        to->clears.emplace(resIndex, *clearColor);
      else
        G_ASSERT_FAIL("Impossible situation");
    }

    to->resolves.reserve(from->resolves.size());
    for (const auto &[src, dst] : from->resolves)
    {
      to->resolves.emplace(mapping.mapRes(nameResolver.resolve(src), multiIndex),
        mapping.mapRes(nameResolver.resolve(dst), multiIndex));
    }

    if (from->colorAttachments.size() == 1)
    {
      const auto resolvedResId = nameResolver.resolve(from->colorAttachments[0].nameId);
      const auto repResId = depData.renamingRepresentatives[resolvedResId];
      to->isLegacyPass = eastl::holds_alternative<DriverDeferredTexture>(registry.resources[repResId].creationInfo);
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
      const auto resIndex = mapping.mapRes(resNameId, multiIndex);

      if (resIndex == intermediate::RESOURCE_NOT_MAPPED)
      {
        // Sanity check, node should've been skipped otherwise
        G_ASSERT((bindInfo.history ? registry.nodes[nodeId].historyResourceReadRequests : registry.nodes[nodeId].resourceRequests)
                   .find(bindInfo.resource)
                   ->second.optional);

        // bind "nothing", nullptr for projector is intentional
        // cuz it's not supposed to be called for nullopt resource idx
        to.emplace(bindIdx, intermediate::Binding{bindInfo.type, eastl::nullopt, false, bindInfo.projectedTag, nullptr});
        continue;
      }

      to.emplace(bindIdx, intermediate::Binding{bindInfo.type, resIndex, bindInfo.history, bindInfo.projectedTag, bindInfo.projector});
    }
  }

  const InternalRegistry &registry;
  const NameResolver &nameResolver;
  const DependencyData &depData;
  const intermediate::Mapping &mapping;
  intermediate::MultiplexingIndex multiIndex;
  NodeNameId nodeId;
};

intermediate::RequiredNodeState IrGraphBuilder::calcNodeState(NodeNameId node_id, intermediate::MultiplexingIndex multi_index,
  const intermediate::Mapping &mapping) const
{
  const auto &nodeData = registry.nodes[node_id];

  if (!nodeData.stateRequirements && !nodeData.renderingRequirements && nodeData.bindings.empty() &&
      nodeData.shaderBlockLayers.objectLayer == -1 && nodeData.shaderBlockLayers.sceneLayer == -1 &&
      nodeData.shaderBlockLayers.frameLayer == -1)
    return {};

  StateFieldSetter setter{registry, nameResolver, depData, mapping, multi_index, node_id};
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

  return result;
}

void IrGraphBuilder::setIrGraphDebugNames(intermediate::Graph &graph) const
{
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

  graph.nodeNames.resize(graph.nodes.size());
  for (auto [nodeIdx, irNode] : graph.nodes.enumerate())
  {
    const char *frontendName = frontendNodeName(irNode);
    auto &debugName = graph.nodeNames[nodeIdx];
    debugName.reserve(strlen(frontendName) + MULTI_INDEX_STRING_APPROX_SIZE);
    debugName += frontendName;
    debugName += genMultiplexingRow(irNode.multiplexingIndex).c_str();
  }

  graph.resourceNames.resize(graph.resources.size());
  for (auto [resIdx, irRes] : graph.resources.enumerate())
  {
    auto &debugName = graph.resourceNames[resIdx];

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

} // namespace dabfg