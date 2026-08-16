// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "runtime.h"

#include <EASTL/sort.h>
#include <mutex> // std::lock_guard

#include <drv/3d/dag_heap.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_enhanced_barrier.h>
#include <drv/3d/dag_rwResource.h>

#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <memory/dag_framemem.h>

#include <render/daFrameGraph/daFG.h>
#include <shaders/dag_refinedBlock.h>

#include <debug/backendDebug.h>
#include <frontend/multiplexingInternal.h>
#include <frontend/dynamicResolution.h>
#include <backend/resourceScheduling/nativeResourceAllocator.h>
#include <backend/resourceScheduling/poolResourceAllocator.h>
#include <common/resourceUsage.h>
#include <common/genericPoint.h>
#include <id/idRange.h>

namespace dafg
{

CONSOLE_BOOL_VAL("dafg", recompile_graph, false);
CONSOLE_BOOL_VAL("dafg", recompile_graph_every_frame, false);
CONSOLE_BOOL_VAL("dafg", debug_dangling_reference, false);
CONSOLE_BOOL_VAL("dafg", pedantic, false);
CONSOLE_INT_VAL("dafg", test_incrementality_nodes, 0, 0, 1000);
CONSOLE_BOOL_VAL("dafg", recompile_from_scratch, false);

InitOnDemand<Runtime, false> Runtime::instance;

Runtime::Runtime() // -V730
{
  if (d3d::get_driver_desc().caps.hasResourceHeaps)
    resourceAllocator.reset(new NativeResourceAllocator(nodeTracker));
  else
    resourceAllocator.reset(new PoolResourceAllocator(nodeTracker));

  nodeExec.emplace(*resourceAllocator, intermediateGraph, irMapping, registry, nameResolver, currentlyProvidedResources,
    bindlessSlotManager);

#if DAGOR_DBGLEVEL > 0
  fgVisManager = visualization::make_real_manager(registry, nameResolver, dependencyDataCalculator.depData, intermediateGraph,
    passColoring, perNodeStateDeltas);
#else
  fgVisManager = visualization::make_dummy_manager();
#endif
}

Runtime::~Runtime()
{
  // CPU resources must be cleaned up gracefully when shutting down
  resourceScheduler.invalidateTemporalResources();
  bindlessSlotManager.freeRanges();
  resourceAllocator->shutdown(frameIndex % SCHEDULE_FRAME_WINDOW);
  reset_texture_visualization();
}

static void log_node_changes(const char *msg, const auto &nodes, const auto &name_resolver)
{
  FRAMEMEM_REGION;
  eastl::basic_string<char, framemem_allocator> log = msg;
  for (auto [nodeId, changed] : nodes.enumerate())
    if (changed)
      log.append_sprintf("%s, ", name_resolver.getShortName(nodeId));

  debug("%s", log.c_str());
}

auto Runtime::updateNodeDeclarations()
{
  TIME_PROFILE(updateNodeDeclarations);
  if (verbose)
    debug("daFG: Updating node declarations...");
  auto result = nodeTracker.updateNodeDeclarations();
  currentStage = CompilationStage::REQUIRES_NAME_RESOLUTION;
  return result;
}

auto Runtime::resolveNames(const NodesChanged &nodes_changed)
{
  TIME_PROFILE(resolveNames);
  if (verbose)
    debug("daFG: Resolving names...");
  auto result = nameResolver.update(nodes_changed);
  currentStage = CompilationStage::REQUIRES_DEPENDENCY_DATA_CALCULATION;
  if (verbose)
    log_node_changes("daFG: Changed nodes: ", nodes_changed, registry.knownNames);

  return result;
}

auto Runtime::calculateDependencyData(const NodesChanged &nodes_changed)
{
  TIME_PROFILE(calculateDependencyData);
  if (verbose)
    debug("daFG: Calculating dependency data...");
  auto result = dependencyDataCalculator.recalculate(nodes_changed);
  resolveBlobTypes();
  currentStage = CompilationStage::REQUIRES_REGISTRY_VALIDATION;
  return result;
}

void Runtime::resolveBlobTypes()
{
  const auto &depData = dependencyDataCalculator.depData;

  auto resolveFromCreation = [&](ResNameId res_id) -> ResourceSubtypeTag {
    const auto resolved = nameResolver.resolve(res_id);
    const auto representative = depData.renamingRepresentatives[resolved];
    const auto &resData = registry.resources[representative];
    if (!resData.createdResData.has_value())
      return ResourceSubtypeTag::Unknown;
    if (auto *blobDesc = eastl::get_if<BlobDescription>(&resData.createdResData->creationInfo))
      return blobDesc->typeTag;
    return ResourceSubtypeTag::Unknown;
  };

  for (auto nodeId : registry.nodes.keys())
  {
    auto &nodeData = registry.nodes[nodeId];

    for (auto &[resId, req] : nodeData.resourceRequests)
      if (req.subtypeTag == ResourceSubtypeTag::Unknown)
        req.subtypeTag = resolveFromCreation(resId);

    for (auto &[resId, req] : nodeData.historyResourceReadRequests)
      if (req.subtypeTag == ResourceSubtypeTag::Unknown)
        req.subtypeTag = resolveFromCreation(resId);

    for (auto &[bindId, binding] : nodeData.bindings)
      if (binding.projectedTag == ResourceSubtypeTag::Unknown)
        binding.projectedTag = resolveFromCreation(binding.resource);
  }
}

void Runtime::validateRegistry(NodesChanged &nodeChanges, ResourcesChanged &resourceChanges)
{
  TIME_PROFILE(validateRegistry);
  if (verbose)
    debug("daFG: Validating the user graph as specified in the registry...");

  FRAMEMEM_VALIDATE;

  IdIndexedFlags<ResNameId, framemem_allocator> prevResourceValid(registryValidator.validityInfo.resourceValid);
  IdIndexedFlags<NodeNameId, framemem_allocator> prevNodeValid(registryValidator.validityInfo.nodeValid);

  registryValidator.validateRegistry();

  // When validity changes, nodes get removed/added to the graph in a roundabout
  // way, so we need to track these changes here.
  for (auto [resId, valid] : registryValidator.validityInfo.resourceValid.enumerate())
    if (valid != prevResourceValid.test(resId, true))
    {
      resourceChanges[resId] = true;

      // For nodes with optional requests whose validity didn't change,
      // we need to re-do the work that nodesWithChangedRequests does.
      const auto &lifetime = dependencyDataCalculator.depData.resourceLifetimes[resId];
      const auto requestedOptionally = [this, resId = resId](NodeNameId node_id, bool history) {
        const auto &requests =
          history ? registry.nodes[node_id].historyResourceReadRequests : registry.nodes[node_id].resourceRequests;
        const auto unresolvedIds = history ? nameResolver.historyUnresolve(node_id, resId) : nameResolver.unresolve(node_id, resId);
        for (auto unresolvedResIdx : unresolvedIds)
          if (!requests.find(unresolvedResIdx)->second.optional)
            return false;
        return true;
      };
      for (const auto nodeId : lifetime.modificationChain)
        if (requestedOptionally(nodeId, false))
          nodeChanges[nodeId] = true;
      for (const auto nodeId : lifetime.readers)
        if (requestedOptionally(nodeId, false))
          nodeChanges[nodeId] = true;
      for (const auto nodeId : lifetime.historyReaders)
        if (requestedOptionally(nodeId, true))
          nodeChanges[nodeId] = true;
      if (const auto nodeId = lifetime.consumedBy; nodeId != NodeNameId::Invalid)
        if (requestedOptionally(nodeId, false))
          nodeChanges[nodeId] = true;
    }
  for (auto [nodeId, valid] : registryValidator.validityInfo.nodeValid.enumerate())
    if (valid != prevNodeValid.test(nodeId, true))
    {
      nodeChanges[nodeId] = true;
      // Also mark all resources requested by this node as changed
      // which re-does the work that resourceLifetimeChanged does.
      const auto &node = registry.nodes[nodeId];
      for (const auto [unresolvedResIdx, _] : node.resourceRequests)
        resourceChanges[nameResolver.resolve(unresolvedResIdx)] = true;
      for (const auto [unresolvedResIdx, _] : node.historyResourceReadRequests)
        resourceChanges[nameResolver.resolve(unresolvedResIdx)] = true;
    }

  currentStage = CompilationStage::REQUIRES_IR_GRAPH_BUILD;
}


auto Runtime::buildIrGraph(const ResourcesChanged &resources_changed, const NodesChanged &nodes_changed)
{
  TIME_PROFILE(buildIrGraph);
  if (verbose)
    debug("daFG: Building IR graph...");
  auto result = irGraphBuilder.build(unsortedIntermediateGraph, currentMultiplexingExtents, prevMultiplexingExtents, irMapping,
    resources_changed, nodes_changed);

  currentStage = CompilationStage::REQUIRES_PASS_COLORING;
  return result;
}

void Runtime::colorPasses(const IrNodesChanged &node_changes)
{
  TIME_PROFILE(colorPasses);
  if (verbose)
    debug("daFG: Coloring nodes with speculative render passes...");

  passColoring = passColorer.performColoring(unsortedIntermediateGraph, node_changes);

  currentStage = CompilationStage::REQUIRES_NODE_SCHEDULING;
}

Runtime::IrNodesChanged Runtime::scheduleNodes(const IrNodesChanged &irNodesChanged, const IrResourcesChanged &irResourcesChanged)
{
  TIME_PROFILE(scheduleNodes);
  if (verbose)
    debug("daFG: Scheduling nodes...");

  IrNodesChanged schedulingNodesChanged;
  schedulingNodesChanged.reserve(eastl::max(intermediateGraph.nodes.totalKeys(), unsortedIntermediateGraph.nodes.totalKeys()));

  {
    auto newOrder = cullingScheduler.schedule(unsortedIntermediateGraph, passColoring);

    // Incremental resource update (resources keep their index, no reindexing by scheduling)
    intermediateGraph.resources.updateFrom(unsortedIntermediateGraph.resources, irResourcesChanged);
    intermediateGraph.resourceNames.updateFrom(unsortedIntermediateGraph.resourceNames, irResourcesChanged);

    auto remappedOrder = intermediate::apply_node_remap(intermediateGraph, unsortedIntermediateGraph, newOrder, prevPermutation,
      irNodesChanged, schedulingNodesChanged);

    intermediateGraph.validate();

    // Remap passColoring from unsorted to sorted index space
    {
      IdIndexedMapping<intermediate::NodeIndex, PassColor, framemem_allocator> oldPassColoring(passColoring.begin(),
        passColoring.end());
      passColoring.clear();
      for (auto [unsortedIdx, sortedIdx] : remappedOrder.enumerate())
        if (sortedIdx != intermediate::NODE_NOT_MAPPED)
          passColoring.set(sortedIdx, oldPassColoring[unsortedIdx]);
    }

    // Store permutation for next invocation
    prevPermutation.assign(remappedOrder.begin(), remappedOrder.end());
  }

  irMapping = intermediateGraph.calculateMapping();

#if TIME_PROFILER_ENABLED
  if (nodeExec)
  {
    TIME_PROFILE(parseGraph);
    nodeExec->parseGraphMarks();
  }
#endif

  currentStage = CompilationStage::REQUIRES_RESOURCE_LIFETIME_CALCULATION;

  return schedulingNodesChanged;
}

auto Runtime::calculateResourceLifetimes() -> IrResourcesChanged
{
  TIME_PROFILE(calculateResourceLifetimes);
  if (verbose)
    debug("daFG: Calculating resource lifetimes...");

  auto lifetimeChangedResources = resourceLifetimeCalculator.recalculate(intermediateGraph, passColoring);

  currentStage = CompilationStage::REQUIRES_BARRIER_SCHEDULING;

  return lifetimeChangedResources;
}

void Runtime::scheduleBarriers(const IrNodesChanged &nodesChanged, const IrResourcesChanged &resourcesChanged,
  const IrResourcesChanged &lifetimeChangedResources)
{
  TIME_PROFILE(scheduleBarriers);
  if (verbose)
    debug("daFG: Scheduling barriers...");

  barrierScheduler.scheduleEvents(allResourceEvents, intermediateGraph, resourceLifetimeCalculator.lifetimes(), passColoring,
    nodesChanged, resourcesChanged, lifetimeChangedResources);

  cacheUntrackedReleaseBarriers();

  currentStage = CompilationStage::REQUIRES_STATE_DELTA_RECALCULATION;
}

void Runtime::cacheUntrackedReleaseBarriers()
{
  for (const auto &frameEvents : allResourceEvents)
    for (const auto &nodeEvents : frameEvents.values())
      for (const auto &ev : nodeEvents)
      {
        if (BarrierScheduler::barrier_kind(ev) != BarrierScheduler::Event::BarrierKind::Release)
          continue;
        if (!intermediateGraph.resources.isMapped(ev.resource) || !intermediateGraph.resources[ev.resource].isScheduled())
          continue;

        auto &releaseBarrier = intermediateGraph.resources[ev.resource].asScheduled().untrackedReleaseBarrier;
        if (auto *bufferBarrier = eastl::get_if<BarrierScheduler::Event::EnhancedBufferBarrier>(&ev.data))
          releaseBarrier.emplace<d3d::BufferBarrier>(bufferBarrier->barrier).pipelineSync.dst = d3d::PipelineStageFlag::All;
        else if (auto *textureBarrier = eastl::get_if<BarrierScheduler::Event::EnhancedTextureBarrier>(&ev.data))
          releaseBarrier.emplace<d3d::TextureBarrier>(textureBarrier->barrier).pipelineSync.dst = d3d::PipelineStageFlag::All;
      }
}

void Runtime::recalculateStateDeltas(const IrNodesChanged &nodesChanged, const IrResourcesChanged &resourcesChanged)
{
  TIME_PROFILE(recalculateStateDeltas);
  if (verbose)
    debug("daFG: Recalculating state deltas...");

  deltaCalculator.calculatePerNodeStateDeltas(perNodeStateDeltas, allResourceEvents, nodesChanged, resourcesChanged);

  // Reassign bindless texture and buffer slots for the freshly compiled graph.
  bindlessSlotManager.rebuild(intermediateGraph);

  NodeTracker::Alloc::flip();

  currentStage = CompilationStage::REQUIRES_AUTO_RESOLUTION_UPDATE;
}

void Runtime::updateAutoResolutions()
{
  TIME_PROFILE(updateAutoResolutions);
  if (verbose)
    debug("daFG: Updating automatic resolutions...");

  for (auto resIdx : intermediateGraph.resources.keys())
  {
    if (!intermediateGraph.resources[resIdx].isScheduled())
      continue;
    auto &res = intermediateGraph.resources[resIdx].asScheduled();
    if (res.resourceType != ResourceType::Texture || !res.resolutionType.has_value())
      continue;

    const auto [id, mult] = *res.resolutionType;

    // Impossible situation, sanity check
    G_ASSERT_CONTINUE(id != AutoResTypeNameId::Invalid);

    auto &desc = eastl::get<ResourceDescription>(res.description);
    switch (desc.type)
    {
      case D3DResourceType::TEX:
      case D3DResourceType::ARRTEX:
      {
        const auto &values = eastl::get<ResolutionValues<IPoint2>>(registry.autoResTypes[id].values);
        const auto scaled = scale_by(values.staticResolution, mult);
        desc.asTexRes.width = static_cast<uint32_t>(scaled.x);
        desc.asTexRes.height = static_cast<uint32_t>(scaled.y);
        if (res.autoMipCount)
          desc.asTexRes.mipLevels = auto_mip_levels_count(scaled.x, scaled.y, 1);
      }
      break;
      case D3DResourceType::VOLTEX:
      {
        const auto &values = eastl::get<ResolutionValues<IPoint3>>(registry.autoResTypes[id].values);
        const auto scaled = scale_by(values.staticResolution, mult);
        desc.asVolTexRes.width = static_cast<uint32_t>(scaled.x);
        desc.asVolTexRes.height = static_cast<uint32_t>(scaled.y);
        desc.asVolTexRes.depth = static_cast<uint32_t>(scaled.z);
        if (res.autoMipCount)
          desc.asVolTexRes.mipLevels = auto_mip_levels_count(scaled.x, scaled.y, scaled.z, 1);
      }
      break;
      default: G_ASSERT_FAIL("Impossible situation!"); break;
    }
  }

  currentStage = CompilationStage::REQUIRES_RESOURCE_SCHEDULING;
}

void Runtime::scheduleResources(const IrResourcesChanged &lifetimeChangedResources)
{
  TIME_PROFILE(scheduleResources);
  if (verbose)
    debug("daFG: Scheduling resources...");

  // Resource scheduler needs to know which dynamic resolution is the current one
  // to provide correctly sized resources after a recompilation.
  // If a dynamic resolution change was requested on the same frame as a
  // recompilation, then it is going to be applied in the normal way.
  // Rescheduling operates as-if no new dynamic resolution change requests came.
  const auto dynResolutions = collect_applied_dynamic_resolutions(registry);

  const int prevFrame = frameIndex % SCHEDULE_FRAME_WINDOW;

  FRAMEMEM_VALIDATE;

  resourceAllocator->gatherPotentialDeactivationSet(prevFrame, pendingDeactivations);

  validation_restart();

  const auto historyPairing =
    resourceScheduler.pairPreviousHistory(resourceAllocator->cachedIntermediateResources, intermediateGraph.resources);

  const auto corrections = badResolutionTracker.getTexSizeCorrections();

  const ResourceScheduler::SchedulingContext schedulingCtx{intermediateGraph, resourceLifetimeCalculator.lifetimes(),
    lifetimeChangedResources, historyPairing, corrections, *resourceAllocator, resourceAllocator->allocatedHeaps,
    intermediateGraph.resources, intermediateGraph.resourceNames};

  const auto &schedule = resourceScheduler.computeSchedule(prevFrame, schedulingCtx);

  // Don't deactivate resources that were preserved
  for (auto [idx, res] : intermediateGraph.resources.enumerate())
    if (resourceScheduler.isResourcePreserved(prevFrame, idx))
      pendingDeactivations[historyPairing[idx]] = eastl::monostate{};

  resourceAllocator->applySchedule(prevFrame, schedule, intermediateGraph, dynResolutions, corrections, pendingDeactivations);

  applyAliasSyncStages(schedule, corrections);

  // Rescheduling may swap physical resources behind the slots, so force a refresh.
  bindlessSlotManager.invalidateSlotCache();

  currentStage = CompilationStage::REQUIRES_HISTORY_UPDATE;
}

void Runtime::applyAliasSyncStages(const ResourceSchedule &schedule, const BadResolutionTracker::Corrections &corrections)
{
  TIME_PROFILE(applyAliasSyncStages);

  FRAMEMEM_VALIDATE;

  dag::Vector<intermediate::ResourceIndex, framemem_allocator> untrackedResources;
  for (auto resIdx : intermediateGraph.resources.keys())
    if (intermediateGraph.resources.isMapped(resIdx) && intermediateGraph.resources[resIdx].isScheduled() &&
        intermediateGraph.resources[resIdx].isUntracked())
      untrackedResources.push_back(resIdx);

  const auto sizeOf = [&schedule, &corrections](intermediate::ResourceIndex res_idx, int frame) {
    return corrections[frame][res_idx] != 0 ? corrections[frame][res_idx] : schedule.resourceProperties[res_idx].sizeInBytes;
  };

  const auto aliases = [&](intermediate::ResourceIndex lhs_idx, intermediate::ResourceIndex rhs_idx) {
    for (int frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    {
      const auto lhs = schedule.allocationLocations[frame][lhs_idx];
      const auto rhs = schedule.allocationLocations[frame][rhs_idx];
      if (lhs.heap != rhs.heap || lhs.heap == HeapIndex::Invalid)
        continue;
      if (lhs.offset < rhs.offset + sizeOf(rhs_idx, frame) && rhs.offset < lhs.offset + sizeOf(lhs_idx, frame))
        return true;
    }
    return false;
  };

  const auto &usageStages = barrierScheduler.usageSyncStages();
  for (auto resIdx : untrackedResources)
  {
    auto syncBefore = usageStages[resIdx].lastUse;
    auto syncAfter = usageStages[resIdx].firstUse;

    for (auto otherIdx : untrackedResources)
      if (otherIdx != resIdx && aliases(resIdx, otherIdx))
      {
        syncBefore |= usageStages[otherIdx].lastUse;
        syncAfter |= usageStages[otherIdx].firstUse;
      }

    barrierScheduler.setAliasSyncStages(allResourceEvents, resIdx, syncBefore, syncAfter);
  }
}

void Runtime::updateHistory()
{
  TIME_PROFILE(updateHistory);
  if (verbose)
    debug("daFG: Updating history...");

  // Deactivate old history resources that are no longer needed.
  for (const auto &deactivation : pendingDeactivations)
    eastl::visit(
      [](const auto &res) {
        if constexpr (eastl::is_same_v<eastl::remove_cvref_t<decltype(res)>, TextureDeactivation>)
        {
          if (res.release)
            d3d::enhanced_texture_barrier(*res.release, res.texture);
          else
          {
            TextureInfo texInfo = {};
            if (res.texture)
              res.texture->getinfo(texInfo);
            if ((texInfo.cflg & TEXCF_NO_STATE_TRACKING) == 0)
              d3d::deactivate_texture(res.texture);
          }
        }
        else if constexpr (eastl::is_same_v<eastl::remove_cvref_t<decltype(res)>, BufferDeactivation>)
        {
          if (res.release)
            d3d::enhanced_buffer_barrier(*res.release, res.buffer);
          else
          {
            const uint32_t flags = res.buffer ? res.buffer->getFlags() : 0;
            if ((flags & SBCF_NO_STATE_TRACKING) == 0)
              d3d::deactivate_buffer(res.buffer);
          }
        }
        else if constexpr (eastl::is_same_v<eastl::remove_cvref_t<decltype(res)>, BlobDeactivationRequest>)
          res.destructor(res.blob);
      },
      deactivation);
  pendingDeactivations.clear();

  // The idea here is that resources with history are active and being
  // used by nodes over 2 frames: on frame x as the normal resource,
  // and on frame x + 1 the same object becomes the history resource.
  // Therefore, if we recompile the graph between frames x and x + 1,
  // all our resources will get recreated and therefore will NOT be
  // active at the beginning of frame x + 1!
  //
  //                                    Node (reads the resource)
  //     frame x           frame x+1     o
  // [                |xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx]
  // ^               ^                   ^
  // res activated   frame ends          resource is inactive,
  //                 graph recompiled    can't read it!
  //
  // To fix this problem, we re-activate all resources that were
  // supposed to be active since the last frame, i.e. resources with
  // history. The way we reactivate them depends on the `History` flag
  // provided at creation time and the first usage on frame x + 1.
  // The History flag is used to determine whether we need a
  // potentially expensive clear, used for cumulative textures
  // (those where prev and current frame versions get blended together).

  IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> resourceActivated(intermediateGraph.resources.totalKeys(), false);

  // frameIndex will be incremented after this function completes,
  // so the current index is actually the previous frame index.
  const uint32_t prevFrame = frameIndex % SCHEDULE_FRAME_WINDOW;

  IdIndexedMapping<intermediate::ResourceIndex, eastl::optional<intermediate::ResourceUsage>, framemem_allocator>
    firstDeclaredHistoryUsage(intermediateGraph.resources.totalKeys());
  for (const auto &node : intermediateGraph.nodes.values())
    for (const auto &req : node.resourceRequests)
      if (req.fromLastFrame && req.usage.type != Usage::UNKNOWN && !firstDeclaredHistoryUsage[req.resource])
        firstDeclaredHistoryUsage[req.resource] = req.usage;

  // Nodes are topologically sorted at this point. Find first usage
  // for history resources and activate them according as requested
  for (const auto &node : intermediateGraph.nodes.values())
    for (auto [resIdx, usage, lastFrame] : node.resourceRequests)
      if (lastFrame && !resourceActivated[resIdx])
      {
        const auto &res = intermediateGraph.resources[resIdx];
        // NOTE: external resources do not support history
        G_ASSERT_CONTINUE(res.isScheduled());

        if (resourceScheduler.isResourcePreserved(prevFrame, resIdx))
          continue;

        // Map multiplexing index to previous extents, hopefully we preserved a resource there
        auto historySourceResIdx = intermediate::RESOURCE_NOT_MAPPED;
        {
          auto sourceMultiIndex = multiplexing_index_to_ir(
            clamp_and_wrap(multiplexing_index_from_ir(res.multiplexingIndex, currentMultiplexingExtents), historyMultiplexingExtents),
            currentMultiplexingExtents);

          // Find a preserved resource if possible
          for (int mi = int(eastl::to_underlying(sourceMultiIndex)); mi >= 0; mi--)
          {
            sourceMultiIndex = intermediate::MultiplexingIndex(mi);
            if (!irMapping.wasResMapped(res.frontendResources.back(), sourceMultiIndex))
              continue;

            historySourceResIdx = irMapping.mapRes(res.frontendResources.back(), sourceMultiIndex);
            if (historySourceResIdx == intermediate::RESOURCE_NOT_MAPPED)
              continue;

            if (resourceScheduler.isResourcePreserved(prevFrame, historySourceResIdx))
              break;
            historySourceResIdx = intermediate::RESOURCE_NOT_MAPPED;
          }
        }

        // Discard resource contents if we're going to copy them from history
        auto history = res.asScheduled().history;
        if (historySourceResIdx != intermediate::RESOURCE_NOT_MAPPED && history != History::No)
          history = History::DiscardOnFirstFrame;

        G_ASSERT(history != History::No);
        DesiredActivationBehaviour behavior =
          history == History::DiscardOnFirstFrame ? DesiredActivationBehaviour::Discard : DesiredActivationBehaviour::Clear;

        // Falling back to an UNKNOWN usage still yields a barrier. TODO: check if can be avoided.
        const auto historyUsage = res.isUntracked() ? firstDeclaredHistoryUsage[resIdx].value_or(usage) : usage;

        switch (res.getResType())
        {
          case ResourceType::Texture:
          {
            auto tex = resourceAllocator->getTexture(prevFrame, resIdx);

            if (res.isUntrackedTexture())
            {
              const auto activationScope = enhanced_texture_barrier_for_activation(historyUsage, d3d::PipelineStageFlag::All);
              bool copied = false;
              if (historySourceResIdx != intermediate::RESOURCE_NOT_MAPPED)
              {
                const auto *srcRelease = eastl::get_if<d3d::TextureBarrier>(
                  &intermediateGraph.resources[historySourceResIdx].asScheduled().untrackedReleaseBarrier);
                if (srcRelease)
                {
                  BaseTexture *prevTex = resourceAllocator->getTexture(prevFrame, historySourceResIdx);
                  TextureInfo texInfo = {};
                  tex->getinfo(texInfo);

                  d3d::enhanced_texture_barrier(
                    {{d3d::PipelineStageFlag::All, d3d::PipelineStageFlag::Copy}, {{}, d3d::AccessFlag::CopyWrite},
                      {d3d::TextureLayout::Undefined, d3d::TextureLayout::CopyDest}, ENTIRE_TEXTURE_SUBRESOURCE_RANGE},
                    tex);
                  d3d::enhanced_texture_barrier(
                    {{srcRelease->pipelineSync.src, d3d::PipelineStageFlag::Copy},
                      {srcRelease->memorySync.src, d3d::AccessFlag::CopyRead},
                      {srcRelease->layoutTransition.src, d3d::TextureLayout::CopySource}, ENTIRE_TEXTURE_SUBRESOURCE_RANGE},
                    prevTex);

                  copied = true;
                  for (int slice = 0; slice < texInfo.a && copied; slice++)
                    for (int mip = 0; mip < texInfo.mipLevels; mip++)
                    {
                      const int subResIdx = tex->calcSubResIdx(mip, slice, texInfo.mipLevels);
                      if (tex->updateSubRegion(prevTex, subResIdx, 0, 0, 0, max(1, texInfo.w >> mip), max(1, texInfo.h >> mip),
                            max(1, texInfo.d >> mip), subResIdx, 0, 0, 0))
                        continue;
                      logerr("daFG: failed to copy historical texture data for '%s'",
                        registry.knownNames.getName(res.frontendResources.back()));
                      copied = false;
                      break;
                    }
                }
              }

              if (copied)
                d3d::enhanced_texture_barrier(
                  {{d3d::PipelineStageFlag::Copy, activationScope.pipelineSync.dst},
                    {d3d::AccessFlag::CopyWrite, activationScope.memorySync.dst},
                    {d3d::TextureLayout::CopyDest, activationScope.layoutTransition.dst}, ENTIRE_TEXTURE_SUBRESOURCE_RANGE},
                  tex);
              else if (res.asScheduled().history == History::ClearZeroOnFirstFrame)
              {
                const auto &baseRes = res.asScheduled().getGpuDescription().asBasicRes;
                const auto channels = get_tex_format_desc(baseRes.cFlags & TEXFMT_MASK).mainChannelsType;
                const bool isInt = channels == ChannelDType::UINT || channels == ChannelDType::SINT;
                activate_untracked_texture(tex, activationScope,
                  get_history_activation(DesiredActivationBehaviour::Clear, baseRes.activation, isInt), ResourceClearValue{});
              }
              else
                d3d::enhanced_texture_barrier(activationScope, tex);
              break;
            }

            const auto &base_res = res.asScheduled().getGpuDescription().asBasicRes;
            const auto channels = get_tex_format_desc(base_res.cFlags & TEXFMT_MASK).mainChannelsType;
            auto activation =
              get_history_activation(behavior, base_res.activation, channels == ChannelDType::UINT || channels == ChannelDType::SINT);
            d3d::activate_texture(tex, activation, ResourceClearValue{});

            if (historySourceResIdx != intermediate::RESOURCE_NOT_MAPPED)
            {
              TextureInfo texInfo = {};
              tex->getinfo(texInfo);
              BaseTexture *prevTex = resourceAllocator->getTexture(prevFrame, historySourceResIdx);

              d3d::resource_barrier({tex, RB_RW_COPY_DEST, 0, 0});
              d3d::resource_barrier({prevTex, RB_RO_COPY_SOURCE, 0, 0});

              bool copyFailed = false;
              for (int slice = 0; slice < texInfo.a && !copyFailed; slice++)
                for (int mip = 0; mip < texInfo.mipLevels; mip++)
                {
                  const int subResIdx = tex->calcSubResIdx(mip, slice, texInfo.mipLevels);
                  if (tex->updateSubRegion(prevTex, subResIdx, 0, 0, 0, max(1, texInfo.w >> mip), max(1, texInfo.h >> mip),
                        max(1, texInfo.d >> mip), subResIdx, 0, 0, 0))
                    continue;

                  logerr("daFG: failed to copy historical texture data for '%s'",
                    registry.knownNames.getName(res.frontendResources.back()));
                  if (res.asScheduled().history == History::ClearZeroOnFirstFrame)
                  {
                    d3d::deactivate_texture(tex);
                    d3d::activate_texture(tex, activation, ResourceClearValue{});
                  }
                  copyFailed = true;
                  break;
                }
            }

            // TODO: these barriers might be very wrong. Everything
            // about barriers is fubar and needs to be reworked ;(
            if (auto barrier = barrier_for_transition({}, usage); barrier != RB_NONE)
              d3d::resource_barrier({tex, barrier, 0, 0});
          }
          break;

          case ResourceType::Buffer:
          {
            auto buf = resourceAllocator->getBuffer(prevFrame, resIdx);

            if (res.isUntrackedBuffer())
            {
              const auto usageScope = enhanced_buffer_barrier_for_activation(historyUsage, d3d::PipelineStageFlag::All);
              bool copied = false;
              if (historySourceResIdx != intermediate::RESOURCE_NOT_MAPPED)
              {
                auto srcBuf = resourceAllocator->getBuffer(prevFrame, historySourceResIdx);
                d3d::enhanced_buffer_barrier(
                  {{d3d::PipelineStageFlag::All, d3d::PipelineStageFlag::Copy}, {{}, d3d::AccessFlag::CopyWrite}}, buf);
                d3d::enhanced_buffer_barrier({{usageScope.pipelineSync.dst, d3d::PipelineStageFlag::Copy},
                                               {usageScope.memorySync.dst, d3d::AccessFlag::CopyRead}},
                  srcBuf);
                copied = srcBuf->copyTo(buf);
                if (!copied)
                  logerr("daFG: failed to copy historical buffer data for '%s'",
                    registry.knownNames.getName(res.frontendResources.back()));
              }
              if (copied)
                d3d::enhanced_buffer_barrier({{d3d::PipelineStageFlag::Copy, usageScope.pipelineSync.dst},
                                               {d3d::AccessFlag::CopyWrite, usageScope.memorySync.dst}},
                  buf);
              else
              {
                d3d::enhanced_buffer_barrier(
                  {{d3d::PipelineStageFlag::All, d3d::PipelineStageFlag::Clear}, {{}, d3d::AccessFlag::ClearWrite}}, buf);
                d3d::zero_rwbufi(buf);
                d3d::enhanced_buffer_barrier({{d3d::PipelineStageFlag::Clear, usageScope.pipelineSync.dst},
                                               {d3d::AccessFlag::ClearWrite, usageScope.memorySync.dst}},
                  buf);
              }
              break;
            }

            auto activation = get_history_activation(behavior, res.asScheduled().getGpuDescription().asBasicRes.activation);
            d3d::activate_buffer(buf, activation);

            if (historySourceResIdx != intermediate::RESOURCE_NOT_MAPPED)
              if (!resourceAllocator->getBuffer(prevFrame, historySourceResIdx)->copyTo(buf))
              {
                logerr("daFG: failed to copy historical buffer data for '%s'",
                  registry.knownNames.getName(res.frontendResources.back()));
                if (res.asScheduled().history == History::ClearZeroOnFirstFrame)
                {
                  d3d::deactivate_buffer(buf);
                  d3d::activate_buffer(buf, activation);
                }
              }

            if (auto barrier = barrier_for_transition({}, usage); barrier != RB_NONE)
              d3d::resource_barrier({buf, barrier});
          }
          break;

          case ResourceType::Blob:
            switch (history)
            {
              case History::No:
                logerr("daFG: Encountered a CPU resource with history that"
                       " does not specify it's first-frame action! Asan will"
                       " NOT appreciate this!");
                break;

              case History::DiscardOnFirstFrame:
              case History::ClearZeroOnFirstFrame:
                if (historySourceResIdx != intermediate::RESOURCE_NOT_MAPPED)
                  res.asScheduled().getCpuDescription().copy(resourceAllocator->getBlob(prevFrame, resIdx).data,
                    resourceAllocator->getBlob(prevFrame, historySourceResIdx).data);
                else
                  res.asScheduled().getCpuDescription().ctor(resourceAllocator->getBlob(prevFrame, resIdx).data);
                break;
            }
            break;

          case ResourceType::Invalid:
            G_ASSERT(false); // sanity check, should never happen
            break;
        }

        resourceActivated.set(resIdx, true);
      }


  historyMultiplexingExtents = extents_for_node(registry.defaultHistoryMultiplexingMode, currentMultiplexingExtents);
  currentStage = CompilationStage::REQUIRES_VISUALIZATION_UPDATE;
}

void Runtime::updateVisualization(const NodesChanged &nodes_changed)
{
  TIME_PROFILE(updateVisualization);
  if (verbose)
    debug("daFG: Updating visualization...");

  fgVisManager->updateUserGraphVisualization(nodes_changed);

  fgVisManager->updateIRGraphVisualization();

  fgVisManager->updateResourceVisualization();

  currentStage = CompilationStage::UP_TO_DATE;
}

void Runtime::setMultiplexingExtents(multiplexing::Extents extents)
{
  if (currentMultiplexingExtents != extents)
  {
    prevMultiplexingExtents = currentMultiplexingExtents;
    currentMultiplexingExtents = extents;
    markStageDirty(CompilationStage::REQUIRES_IR_GRAPH_BUILD, "multiplexing extents changed");
  }
}

void Runtime::updateDynamicResolution(int curr_frame)
{
  if (d3d::get_driver_desc().caps.hasResourceHeaps)
  {
    auto dynResUpdates = collect_dynamic_resolution_updates(registry);

    badResolutionTracker.filterOutBadResolutions(dynResUpdates, frameIndex % SCHEDULE_FRAME_WINDOW);

    resourceAllocator->resizeAutoResTextures(curr_frame, dynResUpdates);

    track_applied_dynamic_resolution_updates(registry, dynResUpdates);
  }
  else
  {
    for (auto [id, dynResType] : registry.autoResTypes.enumerate())
      if (eastl::exchange(dynResType.dynamicResolutionCountdown, 0) > 0)
        logerr("daFG: Attempted to use dynamic resolution '%s' on a platform that does not support resource heaps!",
          registry.knownNames.getName(id));
  }
}

Runtime::BlockProviderMap Runtime::applyRefinedBlockBindings(int curr_frame, int prev_frame)
{
  BlockProviderMap blockProviderMap;
  for (auto [nodeId, node] : registry.nodes.enumerate())
    for (const auto &[blockId, provider] : node.registeredRefinedBlocks)
      blockProviderMap[blockId] = &provider;

  for (auto [nodeId, node] : registry.nodes.enumerate())
  {
    if (!registryValidator.validityInfo.nodeValid[nodeId])
      continue;
    for (const auto &[blockId, bindings] : node.refinedBlockBindings)
    {
      const auto blockProviderIt = blockProviderMap.find(blockId);
      G_ASSERTF_CONTINUE(blockProviderIt != blockProviderMap.end(),
        "daFG: forBlock binding in node '%s' references unregistered block '%s'", registry.knownNames.getName(nodeId),
        registry.knownNames.getName(blockId));

      const RefinedBlockProvider &blockProvider = *blockProviderIt->second;

      for (const auto &[svId, binding] : bindings)
      {
        G_ASSERTF_CONTINUE(svId >= 0, "daFG: forBlock binding in node '%s' references invalid slot %d for block '%s'",
          registry.knownNames.getName(nodeId), svId, registry.knownNames.getName(blockId));

        const ResNameId resolvedId = nameResolver.resolve(binding.resource);
        G_ASSERTF_CONTINUE(resolvedId != ResNameId::Invalid || binding.optional,
          "daFG: forBlock resource for block '%s' var %d is not available", registry.knownNames.getName(blockId), svId);

        uint32_t prevHandleId = refined_block::PassBlockHandle{}.getId();
        for (auto midx : IdRange<intermediate::MultiplexingIndex>(irMapping.multiplexingExtent()))
        {
          if (!irMapping.wasResMapped(resolvedId, midx))
            continue;

          const intermediate::ResourceIndex resIdx = irMapping.mapRes(resolvedId, midx);
          const auto &irRes = intermediateGraph.resources[resIdx];

          G_ASSERTF_CONTINUE(irRes.isScheduled(), "daFG: External resource can't be used. Block '%s' var %d",
            registry.knownNames.getName(blockId), svId);

          multiplexing::Index blockMultiIdx = multiplexing_index_from_ir(midx, currentMultiplexingExtents);
          if (binding.history)
            blockMultiIdx = clamp(blockMultiIdx, historyMultiplexingExtents);
          refined_block::PassBlockHandle passBlock = blockProvider(blockMultiIdx);

          G_ASSERTF(prevHandleId == refined_block::PassBlockHandle{}.getId() || passBlock.getId() != prevHandleId,
            "daFG: block '%s' var %d: provider returned the same PassBlockHandle for multiple multiplexing indices;"
            " only the last index's resources will be set. Use the provider overload of registerBlock().",
            registry.knownNames.getName(blockId), svId);
          prevHandleId = passBlock.getId();

          const int frameToGet = binding.history ? prev_frame : curr_frame;

          switch (irRes.getResType())
          {
            case ResourceType::Texture: passBlock.set(svId, resourceAllocator->getTexture(frameToGet, resIdx)); break;
            case ResourceType::Buffer: passBlock.set(svId, resourceAllocator->getBuffer(frameToGet, resIdx)); break;
            case ResourceType::Blob:
            case ResourceType::Invalid:
              G_ASSERT(0); // Should never happen, sanity check
              break;
          }
        }
      }
    }
  }
  return blockProviderMap;
}

bool Runtime::runNodes(bool flush_blocks)
{
  if (DAGOR_UNLIKELY(d3d::device_lost(nullptr)))
  {
    logwarn("daFG: frame was skipped due to an ongoing device reset");
    return false;
  }

  TIME_D3D_PROFILE(ExecuteFrameGraph);
  std::lock_guard<NodeTracker> lock(nodeTracker);

  if (nodeTracker.acquireNodesChanged())
    markStageDirty(CompilationStage::REQUIRES_FULL_RECOMPILATION, "nodes changed");

  if (recompile_graph.get() || recompile_graph_every_frame.get())
  {
    recompile_graph.set(false);
    markStageDirty(CompilationStage::REQUIRES_FULL_RECOMPILATION, "recompile graph flag set");
  }

  if (badResolutionTracker.pollRescheduling())
    markStageDirty(CompilationStage::REQUIRES_RESOURCE_SCHEDULING, "bad resolution tracker requested rescheduling");

  if (debug_dangling_reference)
    debugDanglingReferences();

  if (test_incrementality_nodes.get() > 0)
    testIncrementality();

  recompile();

  const int prevFrame = (frameIndex % SCHEDULE_FRAME_WINDOW);
  const int currFrame = (++frameIndex % SCHEDULE_FRAME_WINDOW);

  updateDynamicResolution(currFrame);
  auto blockProviders = applyRefinedBlockBindings(currFrame, prevFrame);

  if (flush_blocks)
    refined_block::flush();

  const auto &frameEvents = allResourceEvents[currFrame];

  nodeExec->execute(prevFrame, currFrame, currentMultiplexingExtents, frameEvents, perNodeStateDeltas, blockProviders);

  return true;
}

void Runtime::debugDanglingReferences()
{
  {
    IdIndexedMapping<NodeNameId, NodeData> localNodes = std::move(registry.nodes);

    registry.nodes.shrink_to_fit();
    registry.nodes.reserve(localNodes.size());

    for (NodeData &node : localNodes)
      registry.nodes.push_back(eastl::move(node));
  }

  {
    IdIndexedMapping<ResNameId, ResourceData> localResources = eastl::move(registry.resources);

    registry.resources.shrink_to_fit();
    registry.resources.reserve(localResources.size());

    for (ResourceData &res : localResources)
      registry.resources.push_back(eastl::move(res));
  }

  {
    IdIndexedMapping<AutoResTypeNameId, AutoResTypeData> localAutoResTypes = eastl::move(registry.autoResTypes);
    registry.autoResTypes.shrink_to_fit();
    registry.autoResTypes = localAutoResTypes;
  }

  {
    dag::FixedVectorSet<ResNameId, 8> localSinkExternalResources = eastl::move(registry.sinkExternalResources);
    registry.sinkExternalResources.shrink_to_fit();
    registry.sinkExternalResources = localSinkExternalResources;
  }

  {
    IdIndexedMapping<ResNameId, eastl::optional<SlotData>> localResourceSlots = eastl::move(registry.resourceSlots);
    registry.resourceSlots.shrink_to_fit();
    registry.resourceSlots = localResourceSlots;
  }
}

void Runtime::testIncrementality()
{
  ska::flat_hash_map<NodeNameId, detail::DeclarationCallback> backup;
  for (int i = 0; i < test_incrementality_nodes.get(); i++)
  {
    if (registry.nodes.empty())
      break;
    const auto nodeId = static_cast<NodeNameId>(rand() % registry.nodes.size());

    auto &node = registry.nodes[nodeId];
    if (node.declare && !backup.count(nodeId))
    {
      backup[nodeId] = eastl::exchange(node.declare,
        detail::DeclarationCallback{[](NodeNameId, InternalRegistry *) -> detail::ExecutionCallback { return {}; }});
      nodeTracker.unregisterNode(nodeId, node.generation);
    }
  }

  eastl::string nodes;
  for (const auto &[nodeId, _] : backup)
    nodes.append_sprintf("'%s' (%d), ", registry.knownNames.getName(nodeId), eastl::to_underlying(nodeId));
  if (!nodes.empty())
    nodes.resize(nodes.size() - 2);
  logdbg("daFG: testIncrementality removed %zu nodes: %s, recompiling...", backup.size(), nodes.c_str());

  // Expected to logerr a lot but never assert or crash
  markStageDirty(CompilationStage::REQUIRES_FULL_RECOMPILATION);
  recompile();

  logdbg("daFG: testIncrementality restoring %zu nodes", backup.size());

  for (auto &[nodeId, declare] : backup)
  {
    registry.nodes[nodeId].declare = eastl::move(declare);
    nodeTracker.registerNode(nullptr, nodeId);
  }

  markStageDirty(CompilationStage::REQUIRES_FULL_RECOMPILATION);
}

void Runtime::resetIncrementalState()
{
  if (!recompile_from_scratch.get())
    return;

  currentStage = CompilationStage::REQUIRES_NODE_DECLARATION_UPDATE;

  intermediateGraph.clear();
  prevPermutation.clear();
  passColoring.clear();
  perNodeStateDeltas.clear();
  for (auto &events : allResourceEvents)
    events.clear();

  unsortedIntermediateGraph.clear();
  prevMultiplexingExtents = {};
  irMapping = {};

  dependencyDataCalculator.resetIncrementalState();
  irGraphBuilder.resetIncrementalState();
  passColorer.resetIncrementalState();
  resourceLifetimeCalculator.resetIncrementalState();
  barrierScheduler.resetIncrementalState();
  deltaCalculator.resetIncrementalState();
  resourceScheduler.resetIncrementalState();
  bindlessSlotManager.freeRanges();
}

void Runtime::recompile()
{
  if (currentStage == CompilationStage::UP_TO_DATE)
    return;

  TIME_PROFILE(UpdateGraph);
  FRAMEMEM_VALIDATE;

  NodesChanged nodeChanges;
  ResourcesChanged resourceChanges;

  resetIncrementalState();

  if (currentStage > CompilationStage::REQUIRES_NODE_DECLARATION_UPDATE)
  {
    nodeChanges.resize(registry.knownNames.nameCount<NodeNameId>(), false);
    resourceChanges.resize(registry.knownNames.nameCount<ResNameId>(), false);
  }

  NameResolver::NameResolutionChanged nameResolutionChanges;

  if (currentStage > CompilationStage::REQUIRES_NAME_RESOLUTION)
  {
    nameResolutionChanges.resize<ResNameId>(registry.knownNames.nameCount<ResNameId>(), false);
    nameResolutionChanges.resize<AutoResTypeNameId>(registry.knownNames.nameCount<AutoResTypeNameId>(), false);
  }

  IrGraphBuilder::IrNodesChanged unsortedIrNodesChanged;
  IrGraphBuilder::IrResourcesChanged irResourcesChanged;

  if (currentStage > CompilationStage::REQUIRES_IR_GRAPH_BUILD)
  {
    unsortedIrNodesChanged.resize(unsortedIntermediateGraph.nodes.totalKeys(), false);
    irResourcesChanged.resize(unsortedIntermediateGraph.resources.totalKeys(), false);
  }

  IrGraphBuilder::IrNodesChanged irNodesChanged;

  if (currentStage > CompilationStage::REQUIRES_NODE_SCHEDULING)
  {
    irNodesChanged.resize(intermediateGraph.nodes.totalKeys(), false);
  }

  IrResourcesChanged lifetimeChangedResources;
  if (currentStage > CompilationStage::REQUIRES_RESOURCE_LIFETIME_CALCULATION)
  {
    lifetimeChangedResources.resize(intermediateGraph.resources.totalKeys(), false);
  }

  switch (currentStage)
  {
    case CompilationStage::REQUIRES_NODE_DECLARATION_UPDATE:
    {
      auto result = updateNodeDeclarations();
      nodeChanges = eastl::move(result.nodesChanged);
      resourceChanges = eastl::move(result.resourcesChanged);
    }
      [[fallthrough]];

    case CompilationStage::REQUIRES_NAME_RESOLUTION:
    {
      auto [nameResChanges, nodesWithChangedRequests] = resolveNames(nodeChanges);
      nameResolutionChanges = eastl::move(nameResChanges);
      for (auto [nodeId, value] : nodesWithChangedRequests.enumerate())
        nodeChanges[nodeId] = nodeChanges[nodeId] || value;

      for (auto [resId, changed] : resourceChanges.enumerate())
      {
        if (nameResolutionChanges[resId])
        {
          changed = true;
          continue;
        }

        // Resources can depend on other resources, so if the name resolution
        // for these have changed, we need to propagate that to the resource itself
        // having changed.
        if (!registry.resources[resId].createdResData)
          continue;

        if (auto &resolution = registry.resources[resId].createdResData->resolution)
          if (nameResolutionChanges[resolution->id])
            changed = true;

        if (auto *clearValue = eastl::get_if<DynamicParameter>(&registry.resources[resId].createdResData->clearValue))
          if (nameResolutionChanges[clearValue->resource])
            changed = true;
      }
    }
      [[fallthrough]];

    case CompilationStage::REQUIRES_DEPENDENCY_DATA_CALCULATION:
    {
      auto resourceLifetimeChanged = calculateDependencyData(nodeChanges);
      for (auto [resId, value] : resourceLifetimeChanged.enumerate())
        resourceChanges[resId] = resourceChanges[resId] || value;
    }
      [[fallthrough]];

    case CompilationStage::REQUIRES_REGISTRY_VALIDATION:
      // Does not modify nodeChanges/resourceChanges under normal circumstances, only when something is invalid
      validateRegistry(nodeChanges, resourceChanges);
      [[fallthrough]];

    case CompilationStage::REQUIRES_IR_GRAPH_BUILD:
    {
      auto [irNodChang, irResChang] = buildIrGraph(resourceChanges, nodeChanges);
      unsortedIrNodesChanged = eastl::move(irNodChang);
      irResourcesChanged = eastl::move(irResChang);
    }
      [[fallthrough]];

    case CompilationStage::REQUIRES_PASS_COLORING: colorPasses(unsortedIrNodesChanged); [[fallthrough]];

    case CompilationStage::REQUIRES_NODE_SCHEDULING:
      irNodesChanged = scheduleNodes(unsortedIrNodesChanged, irResourcesChanged);
      [[fallthrough]];

    case CompilationStage::REQUIRES_RESOURCE_LIFETIME_CALCULATION:
      lifetimeChangedResources = calculateResourceLifetimes();
      [[fallthrough]];

    case CompilationStage::REQUIRES_BARRIER_SCHEDULING:
      scheduleBarriers(irNodesChanged, irResourcesChanged, lifetimeChangedResources);
      [[fallthrough]];

    case CompilationStage::REQUIRES_STATE_DELTA_RECALCULATION:
      recalculateStateDeltas(irNodesChanged, irResourcesChanged);
      [[fallthrough]];

    case CompilationStage::REQUIRES_AUTO_RESOLUTION_UPDATE: updateAutoResolutions(); [[fallthrough]];

    case CompilationStage::REQUIRES_RESOURCE_SCHEDULING: scheduleResources(lifetimeChangedResources); [[fallthrough]];

    case CompilationStage::REQUIRES_HISTORY_UPDATE: updateHistory(); [[fallthrough]];

    case CompilationStage::REQUIRES_VISUALIZATION_UPDATE: updateVisualization(nodeChanges); [[fallthrough]];

    case CompilationStage::UP_TO_DATE: break;
  }
}

void Runtime::invalidateHistory()
{
  resourceScheduler.invalidateTemporalResources();
  markStageDirty(CompilationStage::REQUIRES_RESOURCE_SCHEDULING, "history invalidated");
}

void Runtime::beforeDeviceReset()
{
  resourceScheduler.invalidateTemporalResources();
  bindlessSlotManager.freeRanges();
  resourceAllocator->shutdown(frameIndex % SCHEDULE_FRAME_WINDOW);
  nodeTracker.scheduleAllNodeRedeclaration();
  markStageDirty(CompilationStage::REQUIRES_NODE_DECLARATION_UPDATE, "device reset");
}

void Runtime::wipeBlobsBetweenFrames(eastl::span<ResNameId> resources)
{
  dag::VectorSet<ResNameId, eastl::less<ResNameId>, framemem_allocator> resourcesSet(resources.begin(), resources.end());
  resourceAllocator->emergencyWipeBlobs(frameIndex % SCHEDULE_FRAME_WINDOW, resourcesSet);
  // We need to free captured ctor overrides, because they can hold GC refs
  // to scripted language context objects
  for (auto [resIdx, res] : registry.resources.enumerate())
    if (res.createdResData.has_value() && res.createdResData->type == dafg::ResourceType::Blob && resourcesSet.count(resIdx))
    {
      eastl::get<BlobDescription>(res.createdResData->creationInfo).ctorOverride.reset();
    }
}

void before_reset(bool)
{
  if (!Runtime::isInitialized())
    return;
  validation_restart();
  Runtime::get().beforeDeviceReset();
}

} // namespace dafg

#include <drv/3d/dag_resetDevice.h>
REGISTER_D3D_BEFORE_RESET_FUNC(dafg::before_reset);
