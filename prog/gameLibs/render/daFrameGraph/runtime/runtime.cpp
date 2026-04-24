// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "runtime.h"

#include <EASTL/sort.h>
#include <mutex> // std::lock_guard

#include <drv/3d/dag_heap.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>

#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <memory/dag_framemem.h>

#include <render/daFrameGraph/daFG.h>

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
CONSOLE_BOOL_VAL("dafg", verbose, true);
CONSOLE_INT_VAL("dafg", test_incrementality_nodes, 0, 0, 1000);

InitOnDemand<Runtime, false> Runtime::instance;

Runtime::Runtime() // -V730
{
  if (d3d::get_driver_desc().caps.hasResourceHeaps)
    resourceAllocator.reset(new NativeResourceAllocator(nodeTracker));
  else
    resourceAllocator.reset(new PoolResourceAllocator(nodeTracker));

  nodeExec.emplace(*resourceAllocator, intermediateGraph, irMapping, registry, nameResolver, currentlyProvidedResources);

#if DAGOR_DBGLEVEL > 0
  fgVisManager =
    visualization::make_real_manager(registry, nameResolver, dependencyDataCalculator.depData, intermediateGraph, passColoring);
#else
  fgVisManager = visualization::make_dummy_manager();
#endif
}

Runtime::~Runtime()
{
  // CPU resources must be cleaned up gracefully when shutting down
  resourceScheduler.invalidateTemporalResources();
  resourceAllocator->shutdown(frameIndex % SCHEDULE_FRAME_WINDOW);
  reset_texture_visualization();
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
  return result;
}

auto Runtime::calculateDependencyData(const NodesChanged &nodes_changed)
{
  TIME_PROFILE(calculateDependencyData);
  if (verbose)
    debug("daFG: Calculating dependency data...");
  auto result = dependencyDataCalculator.recalculate(nodes_changed);
  currentStage = CompilationStage::REQUIRES_REGISTRY_VALIDATION;
  return result;
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

  currentStage = CompilationStage::REQUIRES_BARRIER_SCHEDULING;

  return schedulingNodesChanged;
}

auto Runtime::scheduleBarriers(const IrNodesChanged &nodesChanged, const IrResourcesChanged &resourcesChanged) -> IrResourcesChanged
{
  TIME_PROFILE(scheduleBarriers);
  if (verbose)
    debug("daFG: Scheduling barriers...");

  auto lifetimeChangedResources =
    barrierScheduler.scheduleEvents(allResourceEvents, intermediateGraph, passColoring, nodesChanged, resourcesChanged);

  currentStage = CompilationStage::REQUIRES_STATE_DELTA_RECALCULATION;

  return lifetimeChangedResources;
}

void Runtime::recalculateStateDeltas(const IrNodesChanged &nodesChanged, const IrResourcesChanged &resourcesChanged)
{
  TIME_PROFILE(recalculateStateDeltas);
  if (verbose)
    debug("daFG: Recalculating state deltas...");

  deltaCalculator.calculatePerNodeStateDeltas(perNodeStateDeltas, allResourceEvents, nodesChanged, resourcesChanged);

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

  const ResourceScheduler::SchedulingContext schedulingCtx{intermediateGraph, allResourceEvents, lifetimeChangedResources,
    historyPairing, corrections, *resourceAllocator, resourceAllocator->allocatedHeaps, intermediateGraph.resources,
    intermediateGraph.resourceNames};

  const auto &schedule = resourceScheduler.computeSchedule(prevFrame, schedulingCtx);

  // Don't deactivate resources that were preserved
  for (auto [idx, res] : resourceAllocator->cachedIntermediateResources.enumerate())
    if (resourceScheduler.isResourcePreserved(prevFrame, idx))
      pendingDeactivations[historyPairing[idx]] = eastl::monostate{};

  resourceAllocator->applySchedule(prevFrame, schedule, intermediateGraph, dynResolutions, corrections, pendingDeactivations);

  currentStage = CompilationStage::REQUIRES_HISTORY_UPDATE;
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
        if constexpr (eastl::is_same_v<eastl::remove_cvref_t<decltype(res)>, BaseTexture *>)
          d3d::deactivate_texture(res);
        else if constexpr (eastl::is_same_v<eastl::remove_cvref_t<decltype(res)>, Sbuffer *>)
          d3d::deactivate_buffer(res);
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

        switch (res.getResType())
        {
          case ResourceType::Texture:
          {
            const auto &base_res = res.asScheduled().getGpuDescription().asBasicRes;
            const auto channels = get_tex_format_desc(base_res.cFlags & TEXFMT_MASK).mainChannelsType;
            auto activation =
              get_history_activation(behavior, base_res.activation, channels == ChannelDType::UINT || channels == ChannelDType::SINT);
            auto tex = resourceAllocator->getTexture(prevFrame, resIdx);
            d3d::activate_texture(tex, activation, ResourceClearValue{});

            if (historySourceResIdx != intermediate::RESOURCE_NOT_MAPPED)
            {
              TextureInfo texInfo = {};
              tex->getinfo(texInfo);
              BaseTexture *prevTex = resourceAllocator->getTexture(prevFrame, historySourceResIdx);
              d3d::resource_barrier({tex, RB_RW_COPY_DEST, 0, texInfo.mipLevels});
              d3d::resource_barrier({prevTex, RB_RO_COPY_SOURCE, 0, texInfo.mipLevels});

              for (int i = 0; i < texInfo.mipLevels; i++)
                if (!tex->updateSubRegion(prevTex, i, 0, 0, 0, max(1, texInfo.w >> i), max(1, texInfo.h >> i), texInfo.d, i, 0, 0, 0))
                {
                  logerr("daFG: failed to copy historical texture data for '%s'",
                    registry.knownNames.getName(res.frontendResources.back()));
                  if (res.asScheduled().history == History::ClearZeroOnFirstFrame)
                  {
                    d3d::deactivate_texture(tex);
                    d3d::activate_texture(tex, activation, ResourceClearValue{});
                  }
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
            auto activation = get_history_activation(behavior, res.asScheduled().getGpuDescription().asBasicRes.activation);
            auto buf = resourceAllocator->getBuffer(prevFrame, resIdx);
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

void Runtime::updateVisualization()
{
  if (debug_graph_generation)
  {
    TIME_PROFILE(updateVisualization);
    debug("daFG: Updating visualization...");

    fgVisManager->updateUserGraphVisualization();

    fgVisManager->updateIRGraphVisualization();

    fgVisManager->updateResourceVisualization();
  }

  currentStage = CompilationStage::UP_TO_DATE;
}

void Runtime::setMultiplexingExtents(multiplexing::Extents extents)
{
  if (currentMultiplexingExtents != extents)
  {
    prevMultiplexingExtents = currentMultiplexingExtents;
    currentMultiplexingExtents = extents;
    markStageDirty(CompilationStage::REQUIRES_IR_GRAPH_BUILD);
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

bool Runtime::runNodes()
{
  if (DAGOR_UNLIKELY(d3d::device_lost(nullptr)))
  {
    logwarn("daFG: frame was skipped due to an ongoing device reset");
    return false;
  }

  TIME_D3D_PROFILE(ExecuteFrameGraph);
  std::lock_guard<NodeTracker> lock(nodeTracker);

  if (nodeTracker.acquireNodesChanged())
    markStageDirty(CompilationStage::REQUIRES_FULL_RECOMPILATION);

  if (recompile_graph.get() || recompile_graph_every_frame.get())
  {
    recompile_graph.set(false);
    markStageDirty(CompilationStage::REQUIRES_FULL_RECOMPILATION);
  }

  if (badResolutionTracker.pollRescheduling())
  {
    debug("daFG: Bad resolution tracker requested a resource rescheduling!");
    markStageDirty(CompilationStage::REQUIRES_RESOURCE_SCHEDULING);
  }

  if (should_update_visualization())
    markStageDirty(CompilationStage::REQUIRES_VISUALIZATION_UPDATE);

  if (debug_dangling_reference)
    debugDanglingReferences();

  if (test_incrementality_nodes.get() > 0)
    testIncrementality();

  recompile();

  const int prevFrame = (frameIndex % SCHEDULE_FRAME_WINDOW);
  const int currFrame = (++frameIndex % SCHEDULE_FRAME_WINDOW);

  updateDynamicResolution(currFrame);

  const auto &frameEvents = allResourceEvents[currFrame];

  nodeExec->execute(prevFrame, currFrame, currentMultiplexingExtents, frameEvents, perNodeStateDeltas);

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

void Runtime::recompile()
{
  TIME_PROFILE(UpdateGraph);
  FRAMEMEM_VALIDATE;

  NodesChanged nodeChanges;
  ResourcesChanged resourceChanges;

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
  if (currentStage > CompilationStage::REQUIRES_BARRIER_SCHEDULING)
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

    case CompilationStage::REQUIRES_BARRIER_SCHEDULING:
      lifetimeChangedResources = scheduleBarriers(irNodesChanged, irResourcesChanged);
      [[fallthrough]];

    case CompilationStage::REQUIRES_STATE_DELTA_RECALCULATION:
      recalculateStateDeltas(irNodesChanged, irResourcesChanged);
      [[fallthrough]];

    case CompilationStage::REQUIRES_AUTO_RESOLUTION_UPDATE: updateAutoResolutions(); [[fallthrough]];

    case CompilationStage::REQUIRES_RESOURCE_SCHEDULING: scheduleResources(lifetimeChangedResources); [[fallthrough]];

    case CompilationStage::REQUIRES_HISTORY_UPDATE: updateHistory(); [[fallthrough]];

    case CompilationStage::REQUIRES_VISUALIZATION_UPDATE: updateVisualization(); [[fallthrough]];

    case CompilationStage::UP_TO_DATE: break;
  }
}

void Runtime::invalidateHistory()
{
  resourceScheduler.invalidateTemporalResources();
  markStageDirty(CompilationStage::REQUIRES_RESOURCE_SCHEDULING);
}

void Runtime::beforeDeviceReset()
{
  resourceScheduler.invalidateTemporalResources();
  resourceAllocator->shutdown(frameIndex % SCHEDULE_FRAME_WINDOW);
  nodeTracker.scheduleAllNodeRedeclaration();
  markStageDirty(CompilationStage::REQUIRES_NODE_DECLARATION_UPDATE);
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
