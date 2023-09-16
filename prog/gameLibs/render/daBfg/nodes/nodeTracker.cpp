#include "nodeTracker.h"

#include <EASTL/numeric.h>

#include <debug/backendDebug.h>
#include <math/integer/dag_IPoint2.h>
#include <math/random/dag_random.h>
#include <perfMon/dag_statDrv.h>

#include <resourceUsage.h>
#include <multiplexingInternal.h>
#include <id/idRange.h>


// Useful for ensuring that not dependencies are missing.
// Will shuffle nodes while preserving all ordering constraints.
// If anything depends on a "lucky" node order, this should break it.
CONSOLE_BOOL_VAL("dabfg", randomize_order, false);

namespace dabfg
{

NodeTracker::NodeTracker(InternalRegistry &reg) : registry(reg) {}

void NodeTracker::registerNode(NodeNameId nodeId)
{
  checkChangesLock();

  nodesChanged = true;
  invalidate_graph_visualization();

  deferredDeclarationQueue.emplace(nodeId);

  // Make sure that the id is mapped, as the nodes map is the
  // single point of truth here.
  registry.nodes.get(nodeId);
}

void NodeTracker::unregisterNode(NodeNameId nodeId, uint32_t gen)
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

  nodesChanged = true;
  invalidate_graph_visualization();

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

      if (!renamingChains.isMapped(res_id) || renamingChains[res_id] == res_id)
        break;

      res_id = renamingChains[res_id];
    }
  };

  auto &nodeData = registry.nodes[nodeId];

  // Evict all resNameIds *produced* by this node
  for (const auto &[to, from] : nodeData.renamedResources)
    evictResId(to);

  for (const auto &resId : nodeData.createdResources)
    evictResId(resId);

  // Clear node data
  nodeData = {};
  nodeData.generation = gen + 1;
}

void NodeTracker::recalculateResourceLifetimes()
{
  resourceLifetimes.clear();
  resourceLifetimes.resize(registry.knownResourceNames.nameCount());
  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    auto markIntroducedByCurrent = [&, nodeId = nodeId](ResNameId resource) {
      auto &lifetime = resourceLifetimes[resource];

      G_ASSERT_DO_AND_LOG(lifetime.introducedBy == NodeNameId::Invalid, dumpRawUserGraph(),
        "Virtual resource '%s' was introduced twice: by '%s' and by '%s'!", registry.knownResourceNames.getName(resource),
        registry.knownNodeNames.getName(nodeId), registry.knownNodeNames.getName(lifetime.introducedBy));

      lifetime.introducedBy = nodeId;
      // See comment about nodes that both introduce and modify below.
      eastl::erase(lifetime.modificationChain, nodeId);
    };

    for (const auto &resId : nodeData.createdResources)
      markIntroducedByCurrent(resId);

    for (const auto &resId : nodeData.readResources)
      resourceLifetimes[resolve_res_id(registry, resId)].readers.push_back(nodeId);

    // Note that if the introductor/consumer node also modifies this
    // resource, we ignore the modification. This happens when a
    // node creates a rendertarget and then immediately renders to it,
    // or renames it after rendering.
    for (auto resId : nodeData.modifiedResources)
      if (resourceLifetimes[resId].introducedBy != nodeId && resourceLifetimes[resId].consumedBy != nodeId)
        resourceLifetimes[resId].modificationChain.emplace_back(nodeId);

    for (auto [produced, consumed] : nodeData.renamedResources)
    {
      // Sanity check (it's an invariant)
      G_ASSERT(produced != consumed);
      auto &consumedLifetime = resourceLifetimes[consumed];

      markIntroducedByCurrent(produced);

      G_ASSERT_DO_AND_LOG(consumedLifetime.consumedBy == NodeNameId::Invalid, dumpRawUserGraph(),
        "Virtual resource '%s' was consumed twice: by '%s' and by '%s'!", registry.knownResourceNames.getName(consumed),
        registry.knownNodeNames.getName(nodeId), registry.knownNodeNames.getName(consumedLifetime.consumedBy));

      consumedLifetime.consumedBy = nodeId;
      // See comment about nodes that both introduce/consume and modify above.
      eastl::erase(consumedLifetime.modificationChain, nodeId);
    }
  }
}

void NodeTracker::recalculateSlotRequests()
{
  slotRequests.clear();
  slotRequests.resize(registry.knownNodeNames.nameCount());
  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    for (const auto resId : nodeData.readResources)
    {
      const auto resolvedResId = resolve_res_id(registry, resId);
      if (resolvedResId != resId)
        slotRequests[nodeId][resolvedResId].push_back(resId);
    }

    for (const auto resId : nodeData.modifiedResources)
    {
      const auto resolvedResId = resolve_res_id(registry, resId);
      if (resolvedResId != resId)
        slotRequests[nodeId][resolvedResId].push_back(resId);
    }
  }
}

void NodeTracker::validateLifetimes() const
{
#if DAGOR_DBGLEVEL > 0
  for (auto [resId, lifetime] : resourceLifetimes.enumerate())
  {
    eastl::fixed_vector<NodeNameId, 32> sortedReaders(lifetime.readers.begin(), lifetime.readers.end());
    eastl::sort(sortedReaders.begin(), sortedReaders.end());

    eastl::fixed_vector<NodeNameId, 32> sortedModifiers(lifetime.modificationChain.begin(), lifetime.modificationChain.end());
    eastl::sort(sortedModifiers.begin(), sortedModifiers.end());

    dag::Vector<NodeNameId> conflicts;
    eastl::set_intersection(sortedReaders.begin(), sortedReaders.end(), sortedModifiers.begin(), sortedModifiers.end(),
      eastl::back_insert_iterator<decltype(conflicts)>(conflicts));

    eastl::string list;
    for (auto nodeId : conflicts)
    {
      list += "'";
      list += registry.knownNodeNames.getNameCstr(nodeId);
      list += "' ";
    }

    G_ASSERT_DO_AND_LOG(conflicts.empty(), dumpRawUserGraph(), "Found nodes that both modify and read resource '%s'! Offender(s): %s",
      registry.knownResourceNames.getName(resId), list.c_str());

    if (registry.resources.isMapped(resId) && registry.resources[resId].history != History::No)
      G_ASSERT_DO_AND_LOG(lifetime.consumedBy == NodeNameId::Invalid, dumpRawUserGraph(),
        "Resource '%s' was consumed by '%s' on it's frame but had it's "
        "history requested on next frame!",
        registry.knownResourceNames.getName(resId), registry.knownNodeNames.getName(lifetime.consumedBy));
  }
#endif
}

void NodeTracker::validateSlotRequests() const
{
#if DAGOR_DBGLEVEL > 0
  for (const auto &[nodeId, slotResources] : slotRequests.enumerate())
  {
    for (const auto &[resId, slots] : slotResources)
    {
      // Sanity check. Node requests should have this slot
      // Otherwise it will add false requests and it will break resource dependencies.
      G_ASSERT(registry.nodes[nodeId].resourceRequests.contains(slots.front()));
      const auto firstSlotUsageType =
        slots.size() > 0 ? registry.nodes[nodeId].resourceRequests[slots.front()].usage.type : Usage::UNKNOWN;
      for (const auto slot : slots)
      {
        G_ASSERT(registry.nodes[nodeId].resourceRequests.contains(slot));
        if (registry.nodes[nodeId].resourceRequests[slot].usage.type != firstSlotUsageType)
          logerr("The resource '%s' was assigned to both '%s' and '%s' slots, but the node '%s' requests"
                 "the resources in those slots with different usages! This will lead to invalid barriers!",
            registry.knownResourceNames.getName(resId), registry.knownResourceNames.getName(slots.front()),
            registry.knownResourceNames.getName(slot), registry.knownNodeNames.getName(nodeId));
      }
    }
  }
#endif
}

void NodeTracker::declareNodes()
{
  if (randomize_order.get())
    eastl::random_shuffle(deferredDeclarationQueue.begin(), deferredDeclarationQueue.end(),
      [](uint32_t n) { return static_cast<uint32_t>(grnd() % n); });

  for (auto nodeId : deferredDeclarationQueue)
    if (auto &declare = registry.nodes.get(nodeId).declare)
    {
      // Reset the value first to avoid funny side-effects later on
      registry.nodes[nodeId].execute = {};
      registry.nodes[nodeId].execute = declare(nodeId, &registry);
    }

  deferredDeclarationQueue.clear();
}

void NodeTracker::gatherNodeData()
{
  recalculateResourceLifetimes();
  recalculateSlotRequests();
  validateLifetimes();
  validateSlotRequests();
  resolveRenaming();
  updateRenamedResourceProperties();
}

void NodeTracker::resolveRenaming()
{
  auto iotaRange = IdRange<ResNameId>(registry.knownResourceNames.nameCount());
  renamingRepresentatives.assign(iotaRange.begin(), iotaRange.end());
  renamingChains.assign(iotaRange.begin(), iotaRange.end());

  for (const auto &nodeData : registry.nodes)
    for (auto [to, from] : nodeData.renamedResources)
    {
      renamingRepresentatives[to] = from;
      renamingChains[from] = to;
    }

  for (auto [resId, repRef] : renamingRepresentatives.enumerate())
  {
    while (renamingRepresentatives[repRef] != repRef)
    {
      // This code will be executed at most once for every modification,
      // but modifications are bounded by resource count, so O(n)
      auto tmp = renamingRepresentatives[repRef];
      renamingRepresentatives[repRef] = renamingRepresentatives[renamingRepresentatives[repRef]];
      repRef = tmp;
    }
  }
}

void NodeTracker::updateRenamedResourceProperties()
{
  for (auto [resId, repId] : renamingRepresentatives.enumerate())
    if (resId != repId)
    {
      // TODO: I kind of hate this now, gotta rework it one day
      // The internal registry state may contain out of date or
      // invalid states, the types don't really model how this system
      // actually works.
      // Idea to try: make registry.resources contain
      // variant<ResourceData, ResNameId> where the second one means
      // "redirection" to a different resource.
      auto &res = registry.resources[resId];
      const auto &rep = registry.resources[repId];
      res.type = rep.type;
      res.resolution = rep.resolution;
    }
}

auto NodeTracker::findValidResourcesAndNodes() const -> ValidityInfo
{
  // We build an initial marking of invalid resources/nodes and then
  // propagate the invalid marks to dependent nodes/resources using a BFS

  ValidityInfo result;
  auto &resourceValid = result.resourceValid;
  auto &nodeValid = result.nodeValid;

  resourceValid.resize(registry.knownResourceNames.nameCount(), false);
  for (auto [resId, lifetime] : resourceLifetimes.enumerate())
  {
    const bool wasIntroduced = lifetime.introducedBy != NodeNameId::Invalid;
    const bool isASlot = registry.resourceSlots.get(resId).has_value();
    if (isASlot && wasIntroduced)
      logerr("The name '%s' was both used to create/register/rename "
             "a resource in node '%s' and was filled as a resource slot. "
             "Please, use a different name for one of those!",
        registry.knownResourceNames.getName(resId), registry.knownNodeNames.getName(lifetime.introducedBy));

    const auto checkResolution = [this](AutoResTypeNameId autoResId) {
      if (!registry.autoResTypes.isMapped(autoResId))
        return false;

      auto [x, y] = registry.autoResTypes[autoResId].staticResolution;
      auto [dx, dy] = registry.autoResTypes[autoResId].dynamicResolution;
      return x > 0 && y > 0 && dx > 0 && dy > 0;
    };

    const bool autoResValid =
      !registry.resources.get(resId).resolution.has_value() || checkResolution(registry.resources.get(resId).resolution->id);

    if (!autoResValid)
      logerr("Resource '%s' was created with auto-resolution '%s', "
             "but this resolution was either set to an invalid value or "
             "not set at all!",
        registry.knownResourceNames.getName(resId),
        registry.knownAutoResolutionTypeNames.getName(registry.resources.get(resId).resolution->id));

    // Slots stay marked as invalid cuz they don't represent real resources.
    // Things that had a conflict are invalid.
    resourceValid.set(resId, wasIntroduced && !isASlot && autoResValid);
  }

  nodeValid.resize(registry.knownNodeNames.nameCount(), false);
  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    bool anyUnfilledSlotRequests = false;
    for (const auto &[resId, req] : nodeData.resourceRequests)
      if (req.slotRequest && !req.optional && !registry.resourceSlots.get(resId).has_value())
      {
        logerr("Found a broken node '%s'! "
               "Reason: it requested the resource in slot '%s' as mandatory, "
               "but this slot was never actually filled! Either a "
               "typo in the name, or this is supposed to be a regular "
               "resource request (not a slot one), or a missing call "
               "to fill_slot! Disabling this node!",
          registry.knownNodeNames.getName(nodeId), registry.knownResourceNames.getName(resId));
        anyUnfilledSlotRequests = true;
      }

    bool anyBindingTypeMismatches = false;
    for (const auto &[bindId, req] : nodeData.bindings)
    {
      // TODO: validate that subtypes match when binding matrices
      if (req.type != BindingType::ShaderVar)
        continue;

      const auto resType = registry.resources.get(resolve_res_id(registry, req.resource)).type;
      const auto svType = ShaderGlobal::get_var_type(bindId);

      static constexpr eastl::array<char const *const, 8> SHVT_MESSAGES{"is missing from the shader map", "only accepts INTs",
        "only accepts REALs", "only accepts COLOR4s", "only accepts TEXTUREs", "only accepts BUFFERs", "only accepts INT4s",
        "only accepts FLOAT4X4s"};

      G_ASSERT_CONTINUE(svType >= -1 && 1 + svType < SHVT_MESSAGES.size()); // Sanity check

      auto handleMismatch = [this, &anyBindingTypeMismatches, svType, nodeId = nodeId, svId = bindId, resId = req.resource](
                              const char *res_type_str) {
        logerr("Node '%s' requested %s '%s' to be set to"
               " shader variable '%s', but the variable %s!"
               " Either the programmers messed up, or the shader dump is out of date!",
          registry.knownNodeNames.getName(nodeId), res_type_str, registry.knownResourceNames.getName(resId),
          VariableMap::getVariableName(svId), SHVT_MESSAGES[1 + svType]);
        anyBindingTypeMismatches = false;
      };

      switch (resType)
      {
        case ResourceType::Texture:
          if (svType != SHVT_TEXTURE)
            handleMismatch("texture");
          break;

        case ResourceType::Buffer:
          if (svType != SHVT_BUFFER)
            handleMismatch("buffer");
          break;

        case ResourceType::Blob:
          // TODO: validate blob type using subtypes
          if (svType == SHVT_BUFFER || svType == SHVT_TEXTURE || svType == -1)
            handleMismatch("blob");
          break;

        // If resource is missing, either this is an optional request
        // and everything will work fine, or the unfulfilled mandatory
        // request will get caught down below.
        case ResourceType::Invalid: break;
      }
    }

    nodeValid.set(nodeId, nodeData.execute && !anyUnfilledSlotRequests && !anyBindingTypeMismatches);
  }

  // We have to do an initial pass of marking resources created by
  // broken nodes as invalid.
  for (auto [nodeId, valid] : nodeValid.enumerate())
    if (!valid)
    {
      for (auto resId : registry.nodes[nodeId].createdResources)
        resourceValid[resId] = false;
      for (auto [toId, _] : registry.nodes[nodeId].renamedResources)
        resourceValid[toId] = false;
    }

  // WARNING: do NOT use bitset::set inside a framemem_region -- it reallocates!
  FRAMEMEM_VALIDATE;
  // We need a "resource -> [nodes with mandatory dependency on it]"
  // mapping that also accounts for history resources for the BFS to be quick.
  using ResToNodeMapping =
    IdIndexedMapping<ResNameId, eastl::fixed_vector<NodeNameId, 64, true, framemem_allocator>, framemem_allocator>;
  ResToNodeMapping nodesDependentOnResource(resourceValid.size());

  // We also need a mapping that says us which resources are actually
  // other resources, just renamed. This is used to disable the renamed
  // resource even if the node that does the renaming will not be disabled
  // itself due to the renaming being optional.
  using ResToResMapping = IdIndexedMapping<ResNameId, eastl::optional<ResNameId>, framemem_allocator>;
  ResToResMapping resourceRenamingDependencies(resourceValid.size());

  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    for (const auto &[resId, req] : nodeData.resourceRequests)
      if (!req.optional)
        nodesDependentOnResource[resolve_res_id(registry, resId)].push_back(nodeId);
    for (const auto &[resId, req] : nodeData.historyResourceReadRequests)
      if (!req.optional)
        nodesDependentOnResource[resId].push_back(nodeId);

    for (const auto &[to, from] : nodeData.renamedResources)
    {
      G_ASSERT(!resourceRenamingDependencies[from].has_value());
      resourceRenamingDependencies[from] = to;
    }
  }

  // Wave of resources which are definitely invalid but who's dependent
  // nodes/resources may have not been marked as invalid yet.
  using Wave = eastl::fixed_vector<ResNameId, 64, true, framemem_allocator>;
  Wave wave;

  // Fill in the initial wave of invalid resources
  for (const auto [resId, isValid] : resourceValid.enumerate())
    if (!isValid)
      wave.emplace_back(resId);

  // Transitively disable all dependent nodes using a BFS
  while (!wave.empty())
  {
    Wave nextWave;

    for (auto resId : wave)
    {
      if (resourceRenamingDependencies[resId])
      {
        auto renamedToResId = *resourceRenamingDependencies[resId];

        if (resourceValid[renamedToResId])
        {
          resourceValid[renamedToResId] = false; //-V601
          nextWave.push_back(renamedToResId);
        }
      }

      for (auto nodeId : nodesDependentOnResource[resId])
      {
        if (!nodeValid[nodeId])
          continue;

        const bool resWasIntroduced = resourceLifetimes[resId].introducedBy != NodeNameId::Invalid;
        const bool introducedByUs = resourceLifetimes[resId].introducedBy == nodeId;

        logerr("Found a broken node '%s'! "
               "Reason: the node requested '%s' as a mandatory resource, "
               "but %s. Disabling this node!",
          registry.knownNodeNames.getName(nodeId), registry.knownResourceNames.getName(resId),
          resWasIntroduced ? (introducedByUs ? "the resource itself was broken!" : "the node that produces it was broken too!")
                           : "it was never produced by any node!");

        nodeValid[nodeId] = false; //-V601

        auto invalidateRes = [&resourceValid, &nextWave](ResNameId resId) {
          if (!resourceValid[resId])
            return;

          resourceValid[resId] = false; //-V601
          nextWave.push_back(resId);
        };

        const auto &nodeData = registry.nodes[nodeId];

        for (const auto &resId : nodeData.createdResources)
          invalidateRes(resId);

        for (const auto &[to, from] : nodeData.renamedResources)
          invalidateRes(to);
      }
    }

    wave = eastl::move(nextWave);
  }

  return result;
}

void NodeTracker::setIrGraphDebugNames(intermediate::Graph &graph) const
{
  // It turns out that string concatenation is amazingly slow
  // Note that PVS warning 575 is disabled here as `getName` is
  // guaranteed to not return a null due to invariants

  // The " {64}" part
  constexpr size_t MULTI_INDEX_STRING_APPROX_SIZE = 10;

  auto genMultiplexingRow = [](uint32_t idx) {
    const eastl::string HEX_DIGITS = "0123456789ABCDEF";
    PerCompString result = " {0x$$$$} ";
    for (uint32_t i = 7; i >= 4; --i)
    {
      result[i] = HEX_DIGITS[idx & 0xF];
      idx >>= 4;
    }
    return result;
  };

  for (auto &irNode : graph.nodes)
  {
    PerCompString debugName;
    {
      size_t size = 2 + MULTI_INDEX_STRING_APPROX_SIZE;
      for (auto userNodeId : irNode.frontendNodes)
        size += registry.knownNodeNames.getName(userNodeId).size() + 2;
      debugName.reserve(size);
    }

    if (irNode.frontendNodes.size() > 1)
      debugName += '[';
    for (auto userNodeId : irNode.frontendNodes)
    {
      // EASTL is unable to append string_views to strings????
      auto name = registry.knownNodeNames.getName(userNodeId);
      debugName.resize(debugName.size() + name.size());
      eastl::copy(name.begin(), name.end(), debugName.end() - name.size());
      debugName += ", ";
    }
    debugName.resize(debugName.size() - 2);
    if (irNode.frontendNodes.size() > 1)
      debugName += ']';

    debugName += genMultiplexingRow(irNode.multiplexingIndex);
    irNode.name = eastl::move(debugName);
  }

  for (auto &irRes : graph.resources)
  {
    PerCompString debugName;

    {
      size_t size = 2 + MULTI_INDEX_STRING_APPROX_SIZE;
      for (auto userResId : irRes.frontendResources)
        size += registry.knownResourceNames.getName(userResId).size() + 2;
      debugName.reserve(size);
    }

    if (irRes.frontendResources.size() > 1)
      debugName += '[';
    for (auto userResId : irRes.frontendResources)
    {
      auto name = registry.knownResourceNames.getName(userResId);
      debugName.resize(debugName.size() + name.size());
      eastl::copy(name.begin(), name.end(), debugName.end() - name.size());
      debugName += ", ";
    }
    debugName.resize(debugName.size() - 2);
    if (irRes.frontendResources.size() > 1)
      debugName += ']';
    debugName += genMultiplexingRow(irRes.multiplexingIndex);
    irRes.name = eastl::move(debugName);
  }
}

eastl::pair<intermediate::Graph, intermediate::Mapping> NodeTracker::createDiscreteGraph(const ValidityInfo &validity,
  multiplexing::Extents extents) const
{
  const auto irMultiplexingExtents = multiplexing_extents_to_ir(extents);

  eastl::pair<intermediate::Graph, intermediate::Mapping> result{
    {}, intermediate::Mapping(registry.knownNodeNames.nameCount(), registry.knownResourceNames.nameCount(), irMultiplexingExtents)};

  auto &graph = result.first;
  auto &mapping = result.second;

  const auto &nodeValid = validity.nodeValid;
  const auto &resourceValid = validity.resourceValid;

  // Group and multiplex resources
  {
    graph.resources.reserve(mapping.mappedResNameIdCount());

    for (auto [nodeId, nodeData] : registry.nodes.enumerate())
    {
      if (!nodeValid[nodeId])
        continue;


      const auto nodeMultiplexingExtents = extents_for_node(nodeData.multiplexingMode, extents);

      auto processResource = [&extents, &nodeMultiplexingExtents, &graph, &mapping](ResNameId resId,
                               const eastl::variant<intermediate::ScheduledResource, ExternalResource> &variant) {
        for (multiplexing::Index i = {}; index_inside_extents(i, extents); i = next_index(i, extents))
        {
          const auto irMultiplexingIndex = multiplexing_index_to_ir(i, extents);

          if (!index_inside_extents(i, nodeMultiplexingExtents))
          {
            // Index outside of this node's multiplexing extents, redirect
            // the mapping to the intersection with the valid extents
            const auto fromIrIndex = multiplexing_index_to_ir(clamp(i, nodeMultiplexingExtents), extents);
            mapping.mapRes(resId, irMultiplexingIndex) = mapping.mapRes(resId, fromIrIndex);
            continue;
          }

          // Correct multiindex for this frontend node, multiplex the
          // resource (unique physical resource per index)
          auto [idx, irRes] = graph.resources.appendNew();
          mapping.mapRes(resId, irMultiplexingIndex) = idx;
          irRes.resource = variant;
          irRes.frontendResources.insert(resId);
          irRes.multiplexingIndex = irMultiplexingIndex;
        }
      };

      for (auto resId : nodeData.createdResources)
      {
        if (!resourceValid[resId])
          continue;

        const auto &resData = registry.resources[resId];

        const auto scanResourceUsages = [this](ResNameId res_id, const auto &process_readers, const auto &process_modifiers) {
          FRAMEMEM_VALIDATE;
          // Save scanned resources to prevent hanging if there is cycle in renaming chains
          dag::VectorSet<ResNameId, eastl::less<ResNameId>, framemem_allocator> knownCandidates;
          const auto scanNodeSlotsOrResource = [this](NodeNameId node_id, ResNameId res_id, const auto &process_node) {
            if (const auto slotsIt = slotRequests[node_id].find(res_id); slotsIt != slotRequests[node_id].end())
              for (const auto slotId : slotsIt->second)
                process_node(node_id, slotId);
            else
              process_node(node_id, res_id);
          };
          auto resIdCandidate = res_id;
          while (true)
          {
            for (const auto modifier : resourceLifetimes[resIdCandidate].modificationChain)
              scanNodeSlotsOrResource(modifier, resIdCandidate, process_modifiers);

            for (const auto reader : resourceLifetimes[resIdCandidate].readers)
              scanNodeSlotsOrResource(reader, resIdCandidate, process_readers);

            if (!renamingChains.isMapped(resIdCandidate) || renamingChains[resIdCandidate] != resIdCandidate)
              break;
            resIdCandidate = renamingChains[resIdCandidate];
            if (knownCandidates.find(resIdCandidate) != knownCandidates.end())
              break;
            knownCandidates.insert(resIdCandidate);
          }
        };

        if (auto resDesc = eastl::get_if<ResourceDescription>(&resData.creationInfo))
        {
          // If the node introducer do not specify usage, find the first usage
          // in the modification requests in all renamings.
          auto firstUsage = registry.nodes[resourceLifetimes[resId].introducedBy].resourceRequests[resId].usage;
          auto desc = *resDesc;
          bool hasReaders = false;
          const auto processReaders = [this, type = resData.type, &firstUsage, &desc, &hasReaders](NodeNameId node_id,
                                        ResNameId res_id) {
            hasReaders = true;
            const auto usage = registry.nodes[node_id].resourceRequests[res_id].usage;
            update_creation_flags_from_usage(desc.asBasicRes.cFlags, usage, type);
            if (firstUsage.type == Usage::UNKNOWN)
              logerr("Resource %s is read by %s before first modify usage", registry.knownResourceNames.getName(res_id),
                registry.knownNodeNames.getName(node_id));
          };
          const auto processModifiers = [this, type = resData.type, &firstUsage, &desc](NodeNameId node_id, ResNameId res_id) {
            const auto usage = registry.nodes[node_id].resourceRequests[res_id].usage;
            update_creation_flags_from_usage(desc.asBasicRes.cFlags, usage, type);
            if (firstUsage.type == Usage::UNKNOWN)
              firstUsage = usage;
          };
          scanResourceUsages(resId, processReaders, processModifiers);
          if (firstUsage.type == Usage::UNKNOWN && hasReaders)
          {
            logerr("Resource %s is not initialized and it's first modify usage is UNKNOWN.",
              registry.knownResourceNames.getName(resId));
          }

          auto activation = get_activation_from_usage(History::DiscardOnFirstFrame, firstUsage, resData.type);
          if (activation)
            desc.asBasicRes.activation = *activation;
          else
          {
            logerr("Could not infer an activation action for %s from the first usage of a"
                   " v2 API resource, either an application error or a bug in frame graph!",
              registry.knownResourceNames.getName(resId));
            if (resData.type == ResourceType::Texture)
              desc.asBasicRes.activation = ResourceActivationAction::DISCARD_AS_RTV_DSV;
            else if (resData.type == ResourceType::Buffer)
              desc.asBasicRes.activation = ResourceActivationAction::DISCARD_AS_UAV;
          }
          intermediate::ScheduledResource res{desc, resData.type, resData.resolution, resData.history};
          processResource(resId, res);
        }
        else if (auto blobDesc = eastl::get_if<BlobDescription>(&resData.creationInfo))
        {
          intermediate::ScheduledResource res{*blobDesc, resData.type, eastl::nullopt, resData.history};
          processResource(resId, res);
        }
        else if (auto externalProvider = eastl::get_if<ExternalResourceProvider>(&resData.creationInfo))
        {
          switch (resData.type)
          {
            case ResourceType::Texture: processResource(resId, ManagedTexView{}); break;

            case ResourceType::Buffer: processResource(resId, ManagedBufView{}); break;

            case ResourceType::Blob:
            case ResourceType::Invalid: G_ASSERTF_BREAK(false, "This should never ever happen!");
          }
        }
      }
    }

    // Fill in mappings for resources produced via renaming
    for (auto [nodeId, nodeData] : registry.nodes.enumerate())
    {
      if (!nodeValid[nodeId])
        continue;

      for (const auto &[toResId, fromResId] : nodeData.renamedResources)
      {
        if (!resourceValid[toResId])
          continue;

        auto originalRes = renamingRepresentatives[fromResId];

        for (multiplexing::Index i = {}; index_inside_extents(i, extents); i = next_index(i, extents))
        {
          const auto irMultiplexingIndex = multiplexing_index_to_ir(i, extents);
          G_ASSERT(mapping.wasResMapped(originalRes, irMultiplexingIndex));

          const auto originalIndex = mapping.mapRes(originalRes, irMultiplexingIndex);
          mapping.mapRes(toResId, irMultiplexingIndex) = originalIndex;

          auto &originalRes = graph.resources[originalIndex];
          originalRes.frontendResources.insert(toResId);

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
  }

  // Group and multiplex nodes
  {
    dag::Vector<NodeNameId, framemem_allocator> validNodes;
    validNodes.reserve(registry.nodes.size());
    for (auto [nodeId, _] : registry.nodes.enumerate())
      if (nodeValid[nodeId])
        validNodes.emplace_back(nodeId);

    if (randomize_order.get())
    {
      eastl::random_shuffle(validNodes.begin(), validNodes.end(), [](uint32_t n) { return static_cast<uint32_t>(grnd() % n); });
    }

    graph.nodes.reserve(validNodes.size());

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
        irNode.frontendNodes.emplace_back(nodeId);
        irNode.priority = nodeData.priority;
        irNode.multiplexingIndex = irMultiplexingIndex;

        auto processRequest = [&irNode = irNode, &mapping, &resourceValid, &nodeData, &validity, &graph, this, irMultiplexingIndex,
                                nodeId](bool history, ResNameId resId, const ResourceRequest &req) {
          // Skip optional requests for missing resources
          if (req.optional && !resourceValid[resId])
            return;

          auto &irRequest = irNode.resourceRequests.emplace_back();
          // Note that this resMapping might redirect us to a resource
          // that is less multiplexed than us. This is ok.
          irRequest.resource = mapping.mapRes(resId, irMultiplexingIndex);
          irRequest.usage = req.usage;
          irRequest.fromLastFrame = history;

          // On the other hand, if we are less multiplexed than the
          // requested resource, we basically get undefined behaviour:
          // we don't know which one of multiple physical resources
          // we actually want to read. Assert.
          const auto resMplexMode = registry.nodes[resourceLifetimes[resId].introducedBy].multiplexingMode;
          G_ASSERT_DO_AND_LOG(!less_multiplexed(nodeData.multiplexingMode, resMplexMode), dumpRawUserGraph(validity),
            "Node '%s' requested resource '%s', which is more"
            " multiplexed than the node due to being produced by a"
            " node that is more multiplexed. This is invalid usage.",
            registry.knownNodeNames.getName(nodeId), registry.knownResourceNames.getName(resId));

          if (history && registry.resources[resId].history == History::No)
          {
            dumpRawUserGraph(validity);
            logerr("Node '%s' requested history for resource '%s', but the"
                   " resource was created with no history enabled. Please"
                   " specify a history behavior in the resource creation call!"
                   " Artifacts and crashes are possible otherwise!",
              registry.knownNodeNames.getName(nodeId), registry.knownResourceNames.getName(resId));
          }

          // NOTE: we could theoretically prune nodes that are broken
          // in this way, but that's a pretty complicated change with
          // dubious benefits.
          if (const auto &res = graph.resources[irRequest.resource];
              res.isScheduled() && res.asScheduled().isCpuResource() &&
              !resourceTagsMatch(req.subtypeTag, res.asScheduled().getCpuDescription().typeTag))
          {
            dumpRawUserGraph(validity);
            logerr("Node '%s' requested resource '%s' with a mismatched"
                   " subtype tag! Please make sure that you are requesting"
                   " this resource with the same type as it was produced with!",
              registry.knownNodeNames.getName(nodeId), registry.knownResourceNames.getName(resId));
          }
        };

        for (const auto &[resId, req] : nodeData.resourceRequests)
          processRequest(false, resolve_res_id(registry, resId), req);
        for (const auto &[resId, req] : nodeData.historyResourceReadRequests)
          processRequest(true, resId, req);

        graph.nodeStates.set(idx, calcNodeState(nodeId, irMultiplexingIndex, mapping));
      }
    }
  }

  return result;
}

void NodeTracker::addEdgesToIrGraph(intermediate::Graph &graph, const ValidityInfo &validity,
  const intermediate::Mapping &mapping) const
{
  const auto &nodeValid = validity.nodeValid;
  const auto &resourceValid = validity.resourceValid;

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
            registry.knownNodeNames.getName(nodeId), registry.knownNodeNames.getName(prev));
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
            registry.knownNodeNames.getName(nodeId), registry.knownNodeNames.getName(next));
          anyMissingExplicitDependencies = true;
          continue;
        }
        graph.nodes[index].predecessors.insert(mapping.mapNode(nodeId, midx));
      }
    }
  }

  if (anyMissingExplicitDependencies)
  {
    dumpRawUserGraph(validity);
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

  for (const auto [resId, lifetime] : resourceLifetimes.enumerate())
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

void NodeTracker::fixupFalseHistoryFlags(intermediate::Graph &graph) const
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

intermediate::Graph NodeTracker::emitIR(eastl::span<const ResNameId> sinkResources, multiplexing::Extents extents) const
{
  FRAMEMEM_REGION;

  // Step 1: prune broken nodes.
  // If a node requests a resource as a mandatory one but it was not
  // produced by any node, we have no choice but to disable the node.
  // This is applied recursively until no "broken" nodes are left, as a
  // disabled node may have been producing another resource which is now
  // missing as well.

  auto validityInfo = findValidResourcesAndNodes();

  // Step 2: build the intermediate graph.
  // Only valid nodes and resources are included, so all of the following
  // graph compilation process does not need to do validity assertions at
  // all.

  auto [graph, mapping] = createDiscreteGraph(validityInfo, extents);

  fixupFalseHistoryFlags(graph);

  addEdgesToIrGraph(graph, validityInfo, mapping);


  graph.validate();

  // Pruning (this makes mappings out of date)
  auto sinks = findSinkIrNodes(graph, mapping, sinkResources);
  G_ASSERT_DO_AND_LOG(!sinks.empty(), dumpRawUserGraph(validityInfo),
    "All specified frame graph sinks were skipped due to broken resource pruning! "
    "No rendering will be done! Report this to programmers!");

  auto displacement = pruneGraph(graph, mapping, {sinks.begin(), sinks.end()}, validityInfo);
  graph.choseSubgraph(displacement);
  graph.validate();

  setIrGraphDebugNames(graph);

  // NRVO does not work with structured bindings
  return eastl::move(graph);
}

IdIndexedMapping<intermediate::NodeIndex, intermediate::NodeIndex, framemem_allocator> NodeTracker::pruneGraph(
  const intermediate::Graph &graph, const intermediate::Mapping &mapping, eastl::span<const intermediate::NodeIndex> sinkNodes,
  const ValidityInfo &validity) const
{
  // We reindex nodes to go in bfs order, this makes memory access
  // pattern better when scheduling, plus the code is simpler than if we
  // tried to preserve the original order
  static constexpr intermediate::NodeIndex NOT_VISITED{~0u};
  IdIndexedMapping<intermediate::NodeIndex, intermediate::NodeIndex, framemem_allocator> visited(graph.nodes.size(), NOT_VISITED);


  FRAMEMEM_REGION;
  // This is a way to write a BFS. Instead of a queue, we store a "wave"
  // of nodes, iterate adjacent nodes of the "wave" building a the next
  // wave out of not visited ones and then repeat.
  using Wave = eastl::fixed_vector<intermediate::NodeIndex, 64, true, framemem_allocator>;
  Wave wave{sinkNodes.begin(), sinkNodes.end()};
  uint32_t lastAdded = 0;
  auto recordVisited = [&lastAdded, &visited](intermediate::NodeIndex node) {
    visited[node] = static_cast<intermediate::NodeIndex>(lastAdded++);
  };

  for (auto idx : wave)
    recordVisited(idx);

  while (!wave.empty())
  {
    Wave nextWave;

    for (auto idx : wave)
    {
      auto addToNextWave = [&visited, &recordVisited, &nextWave](intermediate::NodeIndex nodeIdx) {
        if (visited[nodeIdx] != NOT_VISITED)
          return;
        recordVisited(nodeIdx);
        nextWave.push_back(nodeIdx);
      };

      auto &currentNode = graph.nodes[idx];

      for (auto pred : currentNode.predecessors)
        addToNextWave(pred);

      // Iterate all history resource requests of this node and
      // add nodes responsible for their production to the wave.
      for (auto userNodeId : currentNode.frontendNodes)
        for (const auto &[resId, req] : registry.nodes[userNodeId].historyResourceReadRequests)
          if (validity.resourceValid[resId])
          {
            auto &lifetime = resourceLifetimes[resId];

            for (auto modifierId : lifetime.modificationChain)
              addToNextWave(mapping.mapNode(modifierId, currentNode.multiplexingIndex));

            addToNextWave(mapping.mapNode(lifetime.introducedBy, currentNode.multiplexingIndex));
          }
    }

    wave = eastl::move(nextWave);
  }

  bool anyNodePruned = false;
  for (auto [nodeId, v] : visited.enumerate())
    if (v == NOT_VISITED)
    {
      anyNodePruned = true;
      for (auto nodeNameId : graph.nodes[nodeId].frontendNodes)
        logerr("FG: Node '%s' is pruned. See below for full user graph dump.", registry.knownNodeNames.getName(nodeNameId));
    }

  if (anyNodePruned)
    dumpRawUserGraph(validity);
  return visited;
}

auto NodeTracker::findSinkIrNodes(const intermediate::Graph &graph, const intermediate::Mapping &mapping,
  eastl::span<const ResNameId> sinkResources) const -> SinkSet
{
  SinkSet result;
  const auto findIrNodes = [this, &mapping, &result](NodeNameId id) {
    for (auto midx : IdRange<intermediate::MultiplexingIndex>(mapping.multiplexingExtent()))
    {
      intermediate::NodeIndex idx = mapping.mapNode(id, midx);
      if (idx == intermediate::NODE_NOT_MAPPED)
      {
        const auto name = registry.knownNodeNames.getName(id);
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
    // We might want a renamed version of an external resource as a sink,
    // so the check must work on the original resource
    if (registry.resources[renamingRepresentatives[resId]].type == ResourceType::Invalid)
    {
      logerr("Attempted to mark a non-existent resource '%s' as a sink!", registry.knownResourceNames.getName(resId));
      continue;
    }

    if (!eastl::holds_alternative<ExternalResourceProvider>(registry.resources[renamingRepresentatives[resId]].creationInfo))
    {
      logerr("Attempted to mark a FG-internal resource '%s' as an "
             "externally consumed resource. This is impossible, as FG "
             "resources can only be accessed from within FG nodes!",
        registry.knownResourceNames.getName(resId));
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
        registry.knownResourceNames.getName(resId));
  }

  return result;
}

dag::Vector<NodeNameId> NodeTracker::getLeafNodeIds() const
{
  dag::Vector<NodeNameId> result;
  result.reserve(registry.nodes.size());
  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
    if (nodeData.createdResources.empty())
      result.push_back(nodeId);
  return result;
}

void NodeTracker::dumpRawUserGraph() const
{
  FRAMEMEM_VALIDATE;
  ValidityInfo info;
  info.resourceValid.resize(registry.resources.size(), true);
  info.nodeValid.resize(registry.nodes.size(), true);
  dumpRawUserGraph(info);
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

  void set(eastl::optional<intermediate::VrsState> &to, const eastl::optional<VrsStateRequirements> &from)
  {
    if (!from.has_value())
      return;

    to.emplace();

    to->rateX = from->rateX;
    to->rateY = from->rateY;
    to->rateTexture = mapping.mapRes(from->rateTextureResId, multiIndex);
    to->vertexCombiner = from->vertexCombiner;
    to->pixelCombiner = from->pixelCombiner;
  }

  void set(eastl::optional<intermediate::RenderPass> &to, const eastl::optional<VirtualPassRequirements> &from)
  {
    if (!from.has_value())
      return;

    to.emplace();

    const auto getAttachment = [this](VirtualSubresourceRef res) -> eastl::optional<intermediate::SubresourceRef> {
      if (res.nameId != ResNameId::Invalid)
      {
        const auto resNameId = resolve_res_id(registry, res.nameId);
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
  }

  void set(intermediate::BindingsMap &to, const BindingsMap &from)
  {
    // We are always called on valid nodes and broken bindings cannot
    // occur here
    for (const auto &[bindIdx, bindInfo] : from)
    {
      // sanity check just in case
      G_ASSERT(bindInfo.type != BindingType::Invalid);

      const ResNameId resNameId = resolve_res_id(registry, bindInfo.resource);
      const auto resIndex = mapping.mapRes(resNameId, multiIndex);

      if (resIndex == intermediate::RESOURCE_NOT_MAPPED)
      {
        // Sanity check, node should've been skipped otherwise
        G_ASSERT((bindInfo.history ? registry.nodes[nodeId].historyResourceReadRequests : registry.nodes[nodeId].resourceRequests)
                   .find(resNameId)
                   ->second.optional);

        // bind "nothing", nullptr for projector is intentional
        // cuz it's not supposed to be called for nullopt resource idx
        to.emplace(bindIdx, intermediate::Binding{bindInfo.type, eastl::nullopt, false, bindInfo.projectedTag, nullptr});
        continue;
      }

      to.emplace(bindIdx, intermediate::Binding{bindInfo.type, resIndex, bindInfo.history, bindInfo.projectedTag, bindInfo.projector});
    }
  }

  InternalRegistry &registry;
  const intermediate::Mapping &mapping;
  intermediate::MultiplexingIndex multiIndex;
  NodeNameId nodeId;
};

intermediate::RequiredNodeState NodeTracker::calcNodeState(NodeNameId node_id, intermediate::MultiplexingIndex multi_index,
  const intermediate::Mapping &mapping) const
{
  const auto &nodeData = registry.nodes[node_id];

  if (!nodeData.stateRequirements && !nodeData.renderingRequirements && nodeData.bindings.empty() &&
      nodeData.shaderBlockLayers.objectLayer == -1 && nodeData.shaderBlockLayers.sceneLayer == -1 &&
      nodeData.shaderBlockLayers.frameLayer == -1)
    return {};

  StateFieldSetter setter{registry, mapping, multi_index, node_id};
  intermediate::RequiredNodeState result;

  if (nodeData.stateRequirements)
  {
#define SET(to, from) setter.set(result.to, nodeData.stateRequirements->from)

    SET(wire, supportsWireframe);
    SET(vrs, vrsState);
    SET(shaderOverrides, pipelineStateOverride);
#undef SET
  }
  setter.set(result.pass, nodeData.renderingRequirements);
  setter.set(result.shaderBlockLayers, nodeData.shaderBlockLayers);
  setter.set(result.bindings, nodeData.bindings);

  return result;
}

void NodeTracker::dumpRawUserGraph(const ValidityInfo &info) const
{
  logwarn("Framegraph full user graph state dump:");
  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    auto logNode = [&, this](NodeNameId id) {
      logwarn("\t\t'%s'%s", registry.knownNodeNames.getName(id), !info.nodeValid.test(id) ? " (BROKEN)" : "");
    };

    auto logRes = [&, &nodeData = nodeData, this](ResNameId id) {
      const auto &req = nodeData.resourceRequests.find(id)->second;
      logwarn("\t\t%s'%s'%s", req.optional ? "optional " : "", registry.knownResourceNames.getName(id),
        !info.resourceValid.test(id) ? " (BROKEN)" : "");
    };

    auto dumpHelper = [](const auto &data, const char *heading, const auto &f) {
      if (data.empty())
        return;
      logwarn("\t%s", heading);
      for (const auto &d : data)
        f(d);
    };

    const bool broken = !info.nodeValid.test(nodeId);
    logwarn("Node '%s' (%spriority %d)", registry.knownNodeNames.getName(nodeId), broken ? "BROKEN, " : "", nodeData.priority);

    dumpHelper(nodeData.followingNodeIds, "Following nodes:", logNode);

    dumpHelper(nodeData.precedingNodeIds, "Previous nodes:", logNode);

    // TODO: implement a resource description pretty printer in d3d
    // and use it here

    dumpHelper(nodeData.createdResources, "Created resources:", logRes);

    dumpHelper(nodeData.readResources, "Read resources:", logRes);

    dumpHelper(nodeData.historyResourceReadRequests, "History read resources:", [&, this](const auto &pair) {
      logwarn("\t\t%s'%s'%s", pair.second.optional ? "optional " : "", registry.knownResourceNames.getName(pair.first),
        !info.resourceValid.test(pair.first) ? " (BROKEN)" : "");
    });

    dumpHelper(nodeData.modifiedResources, "Modified resources:", logRes);

    dumpHelper(nodeData.renamedResources, "Renamed resources:", [&](auto pair) {
      const bool firstBroken = !info.resourceValid.test(pair.first);
      const bool secondBroken = !info.resourceValid.test(pair.second);
      // Yes, second is the old resource, first is the new one, this is not a typo.
      logwarn("\t\t'%s'%s -> '%s'%s", registry.knownResourceNames.getName(pair.second), secondBroken ? " (BROKEN)" : "",
        registry.knownResourceNames.getName(pair.first), firstBroken ? " (BROKEN)" : "");
    });
  }
  logwarn("Finished dumping framegraph state.");
}

void NodeTracker::checkChangesLock() const
{
  if (nodeChangesLocked)
  {
    logerr("Attempted to modify framegraph structure while it was being executed!"
           "This is not supported, see callstack and remove the modification!");
  }
}

} // namespace dabfg
