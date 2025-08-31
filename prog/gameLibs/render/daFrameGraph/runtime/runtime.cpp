// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "runtime.h"

#include <EASTL/sort.h>
#include <mutex> // std::lock_guard

#include <drv/3d/dag_heap.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>

#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <memory/dag_framemem.h>

#include <render/daFrameGraph/daFG.h>

#include <debug/backendDebug.h>
#include <frontend/multiplexingInternal.h>
#include <frontend/dynamicResolution.h>
#include <backend/resourceScheduling/nativeResourceScheduler.h>
#include <backend/resourceScheduling/poolResourceScheduler.h>
#include <common/resourceUsage.h>
#include <common/genericPoint.h>
#include <id/idRange.h>

namespace dafg
{

CONSOLE_BOOL_VAL("dafg", recompile_graph, false);
CONSOLE_BOOL_VAL("dafg", recompile_graph_every_frame, false);
CONSOLE_BOOL_VAL("dafg", debug_dangling_reference, false);
CONSOLE_BOOL_VAL("dafg", pedantic, false);

InitOnDemand<Runtime, false> Runtime::instance;

Runtime::Runtime()
{
  if (d3d::get_driver_desc().caps.hasResourceHeaps)
    resourceScheduler.reset(new NativeResourceScheduler(nodeTracker, badResolutionTracker));
  else
    resourceScheduler.reset(new PoolResourceScheduler(nodeTracker, badResolutionTracker));

  nodeExec.emplace(*resourceScheduler, intermediateGraph, irMapping, registry, nameResolver, currentlyProvidedResources);

#if DAGOR_DBGLEVEL > 0
  fgVisualizer =
    visualization::make_real_visualizer(registry, nameResolver, dependencyDataCalculator.depData, intermediateGraph, passColoring);
#else
  fgVisualizer = visualization::make_dummy_visualizer();
#endif
}

Runtime::~Runtime()
{
  // CPU resources must be cleaned up gracefully when shutting down
  resourceScheduler->shutdown(frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW);
  reset_texture_visualization();
}

void Runtime::updateNodeDeclarations()
{
  TIME_PROFILE(updateNodeDeclarations);
  debug("daFG: Updating node declarations...");
  nodeTracker.updateNodeDeclarations();
  currentStage = CompilationStage::REQUIRES_NAME_RESOLUTION;
}

void Runtime::resolveNames()
{
  TIME_PROFILE(resolveNames);
  debug("daFG: Resolving names...");
  nameResolver.update();
  currentStage = CompilationStage::REQUIRES_DEPENDENCY_DATA_CALCULATION;
}

void Runtime::calculateDependencyData()
{
  TIME_PROFILE(calculateDependencyData);
  debug("daFG: Calculating dependency data...");
  dependencyDataCalculator.recalculate();
  currentStage = CompilationStage::REQUIRES_REGISTRY_VALIDATION;
}

void Runtime::validateRegistry()
{
  TIME_PROFILE(validateRegistry);
  debug("daFG: Validating the user graph as specified in the registry...");
  registryValidator.validateRegistry();
  currentStage = CompilationStage::REQUIRES_IR_GRAPH_BUILD;
}


void Runtime::buildIrGraph()
{
  TIME_PROFILE(buildIrGraph);
  debug("daFG: Building IR graph...");
  intermediateGraph = irGraphBuilder.build(currentMultiplexingExtents);

  currentStage = CompilationStage::REQUIRES_PASS_COLORING;
}

void Runtime::colorPasses()
{
  TIME_PROFILE(colorPasses);
  debug("daFG: Coloring nodes with speculative render passes...");

  passColoring = perform_coloring(intermediateGraph);

  currentStage = CompilationStage::REQUIRES_NODE_SCHEDULING;
}

void Runtime::scheduleNodes()
{
  TIME_PROFILE(scheduleNodes);
  debug("daFG: Scheduling nodes...");

  {
    // old -> new index
    auto newOrder = cullingScheduler.schedule(intermediateGraph, passColoring);
    intermediateGraph.choseSubgraph(newOrder);
    intermediateGraph.validate();

    IdIndexedMapping<intermediate::NodeIndex, PassColor, framemem_allocator> oldPassColoring(passColoring.begin(), passColoring.end());
    passColoring.clear();
    for (auto [oldIdx, newIdx] : newOrder.enumerate())
      passColoring.set(newIdx, oldPassColoring[oldIdx]);
  }

  irMapping = intermediateGraph.calculateMapping();

  currentStage = CompilationStage::REQUIRES_BARRIER_SCHEDULING;
}

void Runtime::scheduleBarriers()
{
  TIME_PROFILE(scheduleBarriers);
  debug("daFG: Scheduling barriers...");

  allResourceEvents = barrierScheduler.scheduleEvents(intermediateGraph, passColoring);

  currentStage = CompilationStage::REQUIRES_STATE_DELTA_RECALCULATION;
}

void Runtime::recalculateStateDeltas()
{
  TIME_PROFILE(recalculateStateDeltas);
  debug("daFG: Recalculating state deltas...");

  perNodeStateDeltas = deltaCalculator.calculatePerNodeStateDeltas(allResourceEvents);

  sd::Alloc::flip();

  currentStage = CompilationStage::REQUIRES_RESOURCE_SCHEDULING;
}

void Runtime::scheduleResources()
{
  TIME_PROFILE(scheduleResources);
  debug("daFG: Scheduling resources...");

  // Update automatic texture resolutions (static ones)
  for (auto resIdx : IdRange<intermediate::ResourceIndex>(intermediateGraph.resources.size()))
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

  {
    // Resource scheduler needs to know which dynamic resolution is the current one
    // to provide correctly sized resources after a recompilation.
    // If a dynamic resolution change was requested on the same frame as a
    // recompilation, then it is going to be applied in the normal way.
    // Rescheduling operates as-if no new dynamic resolution change requests came.
    const auto dynResolutions = collect_applied_dynamic_resolutions(registry);

    const auto deactivations = resourceScheduler->scheduleResources(frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW,
      intermediateGraph, allResourceEvents, dynResolutions);

    for (const auto deactivation : deactivations)
      switch (deactivation.index())
      {
        case 0: d3d::deactivate_texture(eastl::get<0>(deactivation)); break;
        case 1: d3d::deactivate_buffer(eastl::get<1>(deactivation)); break;
        case 2:
          auto [f, x] = eastl::get<2>(deactivation);
          f(x);
          break;
      }
  }

  currentStage = CompilationStage::REQUIRES_HISTORY_OF_NEW_RESOURCES_INITIALIZATION;
}

void Runtime::initializeHistoryOfNewResources()
{
  TIME_PROFILE(initializeHistoryOfNewResources);
  debug("daFG: Initializing history of new resources...");

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

  IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> resourceActivated(intermediateGraph.resources.size(), false);

  // frameIndex will be incremented after this function completes,
  // so the current index is actually the previous frame index.
  const uint32_t prevFrame = frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW;

  // Nodes are topologically sorted at this point. Find first usage
  // for history resources and activate them according as requested
  for (const auto &node : intermediateGraph.nodes)
    for (auto [resIdx, usage, lastFrame] : node.resourceRequests)
      if (lastFrame && !resourceActivated[resIdx])
      {
        const auto &res = intermediateGraph.resources[resIdx];
        // NOTE: external resources do not support history
        G_ASSERT_CONTINUE(res.isScheduled());

        if (resourceScheduler->isResourcePreserved(prevFrame, resIdx))
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

            if (resourceScheduler->isResourcePreserved(prevFrame, historySourceResIdx))
              break;
            historySourceResIdx = intermediate::RESOURCE_NOT_MAPPED;
          }
        }

        // Discard resource contents if we're going to copy them from history
        auto history = res.asScheduled().history;
        if (historySourceResIdx != intermediate::RESOURCE_NOT_MAPPED && history != History::No)
          history = History::DiscardOnFirstFrame;

        switch (res.getResType())
        {
          case ResourceType::Texture:
          {
            const auto &base_res = res.asScheduled().getGpuDescription().asBasicRes;
            const auto channels = get_tex_format_desc(base_res.cFlags & TEXFMT_MASK).mainChannelsType;
            auto activation =
              get_history_activation(history, base_res.activation, channels == ChannelDType::UINT || channels == ChannelDType::SINT);
            auto tex = resourceScheduler->getTexture(prevFrame, resIdx).getBaseTex();
            d3d::activate_texture(tex, activation, ResourceClearValue{});

            if (historySourceResIdx != intermediate::RESOURCE_NOT_MAPPED)
            {
              TextureInfo texInfo = {};
              tex->getinfo(texInfo);
              BaseTexture *prevTex = resourceScheduler->getTexture(prevFrame, historySourceResIdx).getBaseTex();
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
            auto activation = get_history_activation(history, res.asScheduled().getGpuDescription().asBasicRes.activation);
            auto buf = resourceScheduler->getBuffer(prevFrame, resIdx).getBuf();
            d3d::activate_buffer(buf, activation);

            if (historySourceResIdx != intermediate::RESOURCE_NOT_MAPPED)
              if (!resourceScheduler->getBuffer(prevFrame, historySourceResIdx).getBuf()->copyTo(buf))
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
                  res.asScheduled().getCpuDescription().copy(resourceScheduler->getBlob(prevFrame, resIdx).data,
                    resourceScheduler->getBlob(prevFrame, historySourceResIdx).data);
                else
                  res.asScheduled().getCpuDescription().ctor(resourceScheduler->getBlob(prevFrame, resIdx).data);
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

    fgVisualizer->updateUserGraphVisualisation();

    fgVisualizer->updateIRGraphVisualisation();

    fgVisualizer->updateResourceVisualisation();
  }

  currentStage = CompilationStage::UP_TO_DATE;
}

void Runtime::setMultiplexingExtents(multiplexing::Extents extents)
{
  if (currentMultiplexingExtents != extents)
  {
    currentMultiplexingExtents = extents;
    markStageDirty(CompilationStage::REQUIRES_IR_GRAPH_BUILD);
  }
}

void Runtime::updateDynamicResolution(int curr_frame)
{
  if (d3d::get_driver_desc().caps.hasResourceHeaps)
  {
    auto dynResUpdates = collect_dynamic_resolution_updates(registry);

    badResolutionTracker.filterOutBadResolutions(dynResUpdates, frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW);

    resourceScheduler->resizeAutoResTextures(curr_frame, dynResUpdates);

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

  {
    if (currentStage > CompilationStage::REQUIRES_NODE_DECLARATION_UPDATE)
    {
      auto &nodeUnchanged = frontendRecompilationData.nodeUnchanged;
      eastl::fill(nodeUnchanged.begin(), nodeUnchanged.end(), true);
      nodeUnchanged.resize(registry.nodes.size(), true);
    }
    if (currentStage > CompilationStage::REQUIRES_NAME_RESOLUTION)
    {
      auto &resourceResolvedNameUnchanged = frontendRecompilationData.resourceResolvedNameUnchanged;
      auto &resourceRequestsForNodeUnchanged = frontendRecompilationData.resourceRequestsForNodeUnchanged;
      eastl::fill(resourceResolvedNameUnchanged.begin(), resourceResolvedNameUnchanged.end(), true);
      eastl::fill(resourceRequestsForNodeUnchanged.begin(), resourceRequestsForNodeUnchanged.end(), true);
      resourceResolvedNameUnchanged.resize(registry.resources.size(), true);
      resourceRequestsForNodeUnchanged.resize(registry.nodes.size(), true);
    }
  }

  {
    TIME_PROFILE(UpdateGraph);
    switch (currentStage)
    {
      case CompilationStage::REQUIRES_NODE_DECLARATION_UPDATE: updateNodeDeclarations(); [[fallthrough]];

      case CompilationStage::REQUIRES_NAME_RESOLUTION: resolveNames(); [[fallthrough]];

      case CompilationStage::REQUIRES_DEPENDENCY_DATA_CALCULATION: calculateDependencyData(); [[fallthrough]];

      case CompilationStage::REQUIRES_REGISTRY_VALIDATION: validateRegistry(); [[fallthrough]];

      case CompilationStage::REQUIRES_IR_GRAPH_BUILD: buildIrGraph(); [[fallthrough]];

      case CompilationStage::REQUIRES_PASS_COLORING: colorPasses(); [[fallthrough]];

      case CompilationStage::REQUIRES_NODE_SCHEDULING: scheduleNodes(); [[fallthrough]];

      case CompilationStage::REQUIRES_BARRIER_SCHEDULING: scheduleBarriers(); [[fallthrough]];

      case CompilationStage::REQUIRES_STATE_DELTA_RECALCULATION: recalculateStateDeltas(); [[fallthrough]];

      case CompilationStage::REQUIRES_RESOURCE_SCHEDULING: scheduleResources(); [[fallthrough]];

      case CompilationStage::REQUIRES_HISTORY_OF_NEW_RESOURCES_INITIALIZATION: initializeHistoryOfNewResources(); [[fallthrough]];

      case CompilationStage::REQUIRES_VISUALIZATION_UPDATE: updateVisualization(); [[fallthrough]];

      case CompilationStage::UP_TO_DATE: break;
    }
  }

  const int prevFrame = (frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW);
  const int currFrame = (++frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW);

  updateDynamicResolution(currFrame);

  const auto &frameEvents = allResourceEvents[currFrame];

  nodeExec->execute(prevFrame, currFrame, currentMultiplexingExtents, frameEvents, perNodeStateDeltas);

  return true;
}

void Runtime::invalidateHistory()
{
  resourceScheduler->invalidateTemporalResources();
  markStageDirty(CompilationStage::REQUIRES_RESOURCE_SCHEDULING);
}

void Runtime::beforeDeviceReset()
{
  resourceScheduler->shutdown(frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW);
  nodeTracker.scheduleAllNodeRedeclaration();
  markStageDirty(CompilationStage::REQUIRES_NODE_DECLARATION_UPDATE);
}

void Runtime::wipeBlobsBetweenFrames(eastl::span<ResNameId> resources)
{
  dag::VectorSet<ResNameId, eastl::less<ResNameId>, framemem_allocator> resourcesSet(resources.begin(), resources.end());
  resourceScheduler->emergencyWipeBlobs(frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW, resourcesSet);
  // We need to free captured ctor overrides, because they can hold GC refs
  // to scripted language context objects
  for (auto [resIdx, res] : registry.resources.enumerate())
    if (res.type == dafg::ResourceType::Blob && resourcesSet.count(resIdx))
    {
      eastl::get<BlobDescription>(res.creationInfo).ctorOverride.reset();
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
