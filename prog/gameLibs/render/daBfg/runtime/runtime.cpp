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

#include <render/daBfg/bfg.h>

#include <debug/backendDebug.h>
#include <frontend/multiplexingInternal.h>
#include <backend/nodeStateDeltas.h>
#include <backend/resourceScheduling/nativeResourceScheduler.h>
#include <backend/resourceScheduling/poolResourceScheduler.h>
#include <common/resourceUsage.h>
#include <id/idRange.h>

CONSOLE_BOOL_VAL("dabfg", recompile_graph, false);
CONSOLE_BOOL_VAL("dabfg", recompile_graph_every_frame, false);
CONSOLE_BOOL_VAL("dabfg", debug_graph_generation, DAGOR_DBGLEVEL > 0);


namespace dabfg
{

static DynamicResolutions collect_dynamic_resolution_updates(const InternalRegistry &registry)
{
  DynamicResolutions dynResolutions;
  for (auto [id, dynResType] : registry.autoResTypes.enumerate())
    if (dynResType.dynamicResolutionCountdown > 0)
    {
      eastl::visit(
        [&, id = id](const auto &values) {
          if constexpr (!eastl::is_same_v<eastl::decay_t<decltype(values)>, eastl::monostate>)
            dynResolutions.set(id, values.dynamicResolution);
        },
        dynResType.values);
    }
  return dynResolutions;
}

static void track_applied_dynamic_resolution_updates(InternalRegistry &registry, const DynamicResolutions &dyn_resolutions)
{
  for (auto [id, val] : dyn_resolutions.enumerate())
    if (!eastl::holds_alternative<eastl::monostate>(val))
    {
      --registry.autoResTypes[id].dynamicResolutionCountdown;
      eastl::visit(
        [](auto &values) {
          if constexpr (!eastl::is_same_v<eastl::decay_t<decltype(values)>, eastl::monostate>)
            values.lastAppliedDynamicResolution = values.dynamicResolution;
        },
        registry.autoResTypes[id].values);
    }
}

static DynamicResolutions collect_applied_dynamic_resolutions(const InternalRegistry &registry)
{
  DynamicResolutions dynResolutions;
  for (auto [id, dynResType] : registry.autoResTypes.enumerate())
    eastl::visit(
      [&, id = id](const auto &values) {
        if constexpr (!eastl::is_same_v<eastl::decay_t<decltype(values)>, eastl::monostate>)
          dynResolutions.set(id, values.lastAppliedDynamicResolution);
      },
      dynResType.values);
  return dynResolutions;
}

InitOnDemand<Runtime, false> Runtime::instance;

Runtime::Runtime()
{
  if (d3d::get_driver_desc().caps.hasResourceHeaps)
    resourceScheduler.reset(new NativeResourceScheduler(nodeTracker));
  else
    resourceScheduler.reset(new PoolResourceScheduler(nodeTracker));

  nodeExec.emplace(*resourceScheduler, intermediateGraph, irMapping, registry, nameResolver, currentlyProvidedResources);
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
  debug("daBfg: Updating node declarations...");
  nodeTracker.updateNodeDeclarations();
  currentStage = CompilationStage::REQUIRES_NAME_RESOLUTION;
}

void Runtime::resolveNames()
{
  TIME_PROFILE(resolveNames);
  debug("daBfg: Resolving names...");
  nameResolver.update();
  currentStage = CompilationStage::REQUIRES_DEPENDENCY_DATA_CALCULATION;
}

void Runtime::calculateDependencyData()
{
  TIME_PROFILE(calculateDependencyData);
  debug("daBfg: Calculating dependency data...");
  dependencyDataCalculator.recalculate();
  currentStage = CompilationStage::REQUIRES_REGISTRY_VALIDATION;
}

void Runtime::validateRegistry()
{
  TIME_PROFILE(validateRegistry);
  debug("daBfg: Validating the user graph as specified in the registry...");
  registryValidator.validateRegistry();
  currentStage = CompilationStage::REQUIRES_IR_GRAPH_BUILD;
}


void Runtime::buildIrGraph()
{
  TIME_PROFILE(buildIrGraph);
  debug("daBfg: Building IR graph...");
  intermediateGraph = irGraphBuilder.build(currentMultiplexingExtents);

  currentStage = CompilationStage::REQUIRES_PASS_COLORING;
}

void Runtime::colorPasses()
{
  TIME_PROFILE(colorPasses);
  debug("daBfg: Coloring nodes with speculative render passes...");

  passColoring = perform_coloring(intermediateGraph);

  currentStage = CompilationStage::REQUIRES_NODE_SCHEDULING;
}

void Runtime::scheduleNodes()
{
  TIME_PROFILE(scheduleNodes);
  debug("daBfg: Scheduling nodes...");


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

  if (debug_graph_generation.get())
  {
    DebugPassColoration coloring;
    coloring.reserve(registry.nodes.size());

    // Debug graph visualization works with not multiplexed nodes
    dag::Vector<NodeNameId, framemem_allocator> demultiplexedNodeExecutionOrder;
    demultiplexedNodeExecutionOrder.reserve(registry.nodes.size());
    for (auto [idx, irNode] : intermediateGraph.nodes.enumerate())
      if (irNode.multiplexingIndex == 0 && irNode.frontendNode)
      {
        demultiplexedNodeExecutionOrder.emplace_back(*irNode.frontendNode);
        coloring.set(*irNode.frontendNode, passColoring[idx]);
      }

    update_graph_visualization(registry, nameResolver, dependencyDataCalculator.depData, coloring, demultiplexedNodeExecutionOrder);
  }

  currentStage = CompilationStage::REQUIRES_STATE_DELTA_RECALCULATION;
}

void Runtime::scheduleBarriers()
{
  TIME_PROFILE(scheduleBarriers);
  debug("daBfg: Scheduling barriers...");

  // This has to go here, unfortunately, as it wipes out events stored
  // by the barrier scheduler. To be refactored later.
  if (debug_graph_generation.get())
  {
    dag::Vector<NodeNameId, framemem_allocator> frontendNodeExecutionOrder;
    for (const intermediate::Node &irNode : intermediateGraph.nodes)
      if (irNode.frontendNode)
        frontendNodeExecutionOrder.emplace_back(*irNode.frontendNode);
      else
        frontendNodeExecutionOrder.emplace_back(NodeNameId::Invalid);

    update_resource_visualization(registry, frontendNodeExecutionOrder);
  }

  allResourceEvents = barrierScheduler.scheduleEvents(intermediateGraph, passColoring);
}

void Runtime::recalculateStateDeltas()
{
  TIME_PROFILE(recalculateStateDeltas);
  debug("daBfg: Recalculating state deltas...");

  perNodeStateDeltas = sd::calculate_per_node_state_deltas(intermediateGraph, allResourceEvents);

  currentStage = CompilationStage::REQUIRES_RESOURCE_SCHEDULING;
}

void Runtime::scheduleResources()
{
  TIME_PROFILE(scheduleResources);
  debug("daBfg: Scheduling resources...");

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
    switch (desc.resType)
    {
      case RES3D_TEX:
      case RES3D_ARRTEX:
      {
        const auto &values = eastl::get<ResolutionValues<IPoint2>>(registry.autoResTypes[id].values);
        desc.asTexRes.width = static_cast<uint32_t>(values.staticResolution.x * mult);
        desc.asTexRes.height = static_cast<uint32_t>(values.staticResolution.y * mult);
      }
      break;
      case RES3D_VOLTEX:
      {
        const auto &values = eastl::get<ResolutionValues<IPoint3>>(registry.autoResTypes[id].values);
        desc.asVolTexRes.width = static_cast<uint32_t>(values.staticResolution.x * mult);
        desc.asVolTexRes.height = static_cast<uint32_t>(values.staticResolution.y * mult);
        desc.asVolTexRes.depth = static_cast<uint32_t>(values.staticResolution.z * mult);
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
  debug("daBfg: Initializing history of new resources...");

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
          auto sourceMultiIndex =
            multiplexing_index_to_ir(clamp_and_wrap(multiplexing_index_from_ir(res.multiplexingIndex, currentMultiplexingExtents),
                                       multiplexingExtentsOnPreviousCompilation),
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
            if (auto activation = get_activation_from_usage(history, usage, res.getResType()))
            {
              auto tex = resourceScheduler->getTexture(prevFrame, resIdx).getBaseTex();
              d3d::activate_texture(tex, *activation, ResourceClearValue{});

              if (historySourceResIdx != intermediate::RESOURCE_NOT_MAPPED)
              {
                TextureInfo texInfo = {};
                tex->getinfo(texInfo);
                BaseTexture *prevTex = resourceScheduler->getTexture(prevFrame, historySourceResIdx).getBaseTex();
                d3d::resource_barrier({tex, RB_RW_COPY_DEST, 0, texInfo.mipLevels});
                d3d::resource_barrier({prevTex, RB_RO_COPY_SOURCE, 0, texInfo.mipLevels});

                for (int i = 0; i < texInfo.mipLevels; i++)
                  if (
                    !tex->updateSubRegion(prevTex, i, 0, 0, 0, max(1, texInfo.w >> i), max(1, texInfo.h >> i), texInfo.d, i, 0, 0, 0))
                  {
                    logerr("failed to copy historical texture data for '%s'",
                      registry.knownNames.getName(res.frontendResources.back()));
                    if (res.asScheduled().history == History::ClearZeroOnFirstFrame)
                      if (auto clearActivation = get_activation_from_usage(res.asScheduled().history, usage, res.getResType()))
                      {
                        d3d::deactivate_texture(tex);
                        d3d::activate_texture(tex, *clearActivation, ResourceClearValue{});
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
            if (auto activation = get_activation_from_usage(history, usage, res.getResType()))
            {
              auto buf = resourceScheduler->getBuffer(prevFrame, resIdx).getBuf();
              d3d::activate_buffer(buf, *activation, ResourceClearValue{});

              if (historySourceResIdx != intermediate::RESOURCE_NOT_MAPPED)
                if (!resourceScheduler->getBuffer(prevFrame, historySourceResIdx).getBuf()->copyTo(buf))
                {
                  logerr("failed to copy historical buffer data for '%s'", registry.knownNames.getName(res.frontendResources.back()));
                  if (res.asScheduled().history == History::ClearZeroOnFirstFrame)
                    if (auto clearActivation = get_activation_from_usage(res.asScheduled().history, usage, res.getResType()))
                    {
                      d3d::deactivate_buffer(buf);
                      d3d::activate_buffer(buf, *clearActivation, ResourceClearValue{});
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
                logerr("Encountered a CPU resource with history that"
                       " does not specify it's first-frame action! Asan will"
                       " NOT appreciate this!");
                break;

              case History::DiscardOnFirstFrame:
              case History::ClearZeroOnFirstFrame:
                if (historySourceResIdx != intermediate::RESOURCE_NOT_MAPPED)
                  res.asScheduled().getCpuDescription().copy(resourceScheduler->getBlob(prevFrame, resIdx).data,
                    resourceScheduler->getBlob(prevFrame, historySourceResIdx).data);
                else
                  res.asScheduled().getCpuDescription().activate(resourceScheduler->getBlob(prevFrame, resIdx).data);
                break;
            }
            break;

          case ResourceType::Invalid:
            G_ASSERT(false); // sanity check, should never happen
            break;
        }

        resourceActivated.set(resIdx, true);
      }


  multiplexingExtentsOnPreviousCompilation = currentMultiplexingExtents;
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
    auto dynResolutions = collect_dynamic_resolution_updates(registry);

    resourceScheduler->resizeAutoResTextures(curr_frame, dynResolutions);

    track_applied_dynamic_resolution_updates(registry, dynResolutions);
  }
  else
  {
    for (auto [id, dynResType] : registry.autoResTypes.enumerate())
      if (eastl::exchange(dynResType.dynamicResolutionCountdown, 0) > 0)
        logerr("daBfg: Attempted to use dynamic resolution '%s' on a platform that does not support resource heaps!",
          registry.knownNames.getName(id));
  }
}

bool Runtime::runNodes()
{
  if (DAGOR_UNLIKELY(d3d::device_lost(nullptr)))
  {
    logwarn("daBfg: frame was skipped due to an ongoing device reset");
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

void Runtime::requestCompleteResourceRescheduling()
{
  resourceScheduler->shutdown(frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW);
  markStageDirty(CompilationStage::REQUIRES_RESOURCE_SCHEDULING);
}

void Runtime::wipeBlobsBetweenFrames(eastl::span<ResNameId> resources)
{
  dag::VectorSet<ResNameId, eastl::less<ResNameId>, framemem_allocator> resourcesSet(resources.begin(), resources.end());
  resourceScheduler->emergencyWipeBlobs(frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW, resourcesSet);
  // We need to free captured activate methods, because default blobs in das hold shared ptr on GC root
  for (auto [resIdx, res] : intermediateGraph.resources.enumerate())
    if (res.isScheduled() && res.asScheduled().isCpuResource() && resourcesSet.count(res.frontendResources.front()))
    {
      eastl::get<BlobDescription>(res.asScheduled().description).activate = {};
    }
}

void before_reset(bool)
{
  validation_restart();
  Runtime::get().requestCompleteResourceRescheduling();
}

} // namespace dabfg

#include <drv/3d/dag_resetDevice.h>
REGISTER_D3D_BEFORE_RESET_FUNC(dabfg::before_reset);
