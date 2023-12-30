#include "runtime.h"

#include <EASTL/sort.h>
#include <mutex> // std::lock_guard

#include <3d/dag_drv3d.h>

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


#if _TARGET_D3D_MULTI || _TARGET_C1 || _TARGET_C2
#define PLATFORM_HAS_HEAPS d3d::get_driver_desc().caps.hasResourceHeaps
#elif _TARGET_XBOX
#define PLATFORM_HAS_HEAPS true
#else
#define PLATFORM_HAS_HEAPS false
#endif

CONSOLE_BOOL_VAL("dabfg", recompile_graph, false);
CONSOLE_BOOL_VAL("dabfg", recompile_graph_every_frame, false);
CONSOLE_BOOL_VAL("dabfg", debug_graph_generation, DAGOR_DBGLEVEL > 0);


namespace dabfg
{

InitOnDemand<Runtime, false> Runtime::instance;

Runtime::Runtime()
{
  if (PLATFORM_HAS_HEAPS)
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
  currentStage = CompilationStage::REQUIRES_IR_GRAPH_BUILD;
}

void Runtime::buildIrGraph()
{
  TIME_PROFILE(buildIrGraph);
  debug("daBfg: Building IR graph...");
  intermediateGraph = irGraphBuilder.build(currentMultiplexingExtents);

  currentStage = CompilationStage::REQUIRES_NODE_SCHEDULING;
}

void Runtime::scheduleNodes()
{
  TIME_PROFILE(scheduleNodes);
  debug("daBfg: Scheduling nodes...");

  {
    // old -> new index
    auto newOrder = cullingScheduler.schedule(intermediateGraph);
    intermediateGraph.choseSubgraph(newOrder);
    intermediateGraph.validate();
  }

  irMapping = intermediateGraph.calculateMapping();

  if (debug_graph_generation.get())
  {
    // Debug graph visualization works with not multiplexed nodes
    dag::Vector<NodeNameId, framemem_allocator> demultiplexedNodeExecutionOrder;
    demultiplexedNodeExecutionOrder.reserve(registry.nodes.size());
    for (const intermediate::Node &irNode : intermediateGraph.nodes)
      if (irNode.multiplexingIndex == 0)
        demultiplexedNodeExecutionOrder.emplace_back(irNode.frontendNode);

    update_graph_visualization(registry, dependencyDataCalculator.depData, demultiplexedNodeExecutionOrder);
  }

  currentStage = CompilationStage::REQUIRES_STATE_DELTA_RECALCULATION;
}

void Runtime::recalculateStateDeltas()
{
  TIME_PROFILE(recalculateStateDeltas);
  debug("daBfg: Recalculating state deltas...");

  perNodeStateDeltas = sd::calculate_per_node_state_deltas(intermediateGraph);

  currentStage = CompilationStage::REQUIRES_RESOURCE_SCHEDULING;
}

void Runtime::scheduleResources()
{
  TIME_PROFILE(scheduleResources);
  debug("daBfg: Scheduling resources...");

  // Update automatic texture resolutions
  for (auto resIdx : IdRange<intermediate::ResourceIndex>(intermediateGraph.resources.size()))
  {
    if (!intermediateGraph.resources[resIdx].isScheduled())
      continue;
    auto &res = intermediateGraph.resources[resIdx].asScheduled();
    if (res.resourceType != ResourceType::Texture || !res.resolutionType.has_value())
      continue;

    const auto [unresolvedId, mult] = *res.resolutionType;

    // Impossible situation, sanity check
    G_ASSERT_CONTINUE(unresolvedId != AutoResTypeNameId::Invalid);

    const auto id = nameResolver.resolve(unresolvedId);

    auto &texDesc = eastl::get<ResourceDescription>(res.description).asTexRes;
    texDesc.width = static_cast<uint32_t>(registry.autoResTypes[id].staticResolution.x * mult);
    texDesc.height = static_cast<uint32_t>(registry.autoResTypes[id].staticResolution.y * mult);
  }

  if (debug_graph_generation.get())
  {
    dag::Vector<NodeNameId, framemem_allocator> frontendNodeExecutionOrder;
    for (const intermediate::Node &irNode : intermediateGraph.nodes)
      frontendNodeExecutionOrder.emplace_back(irNode.frontendNode);

    update_resource_visualization(registry, frontendNodeExecutionOrder);
  }

  {
    auto [events, deactivations] =
      resourceScheduler->scheduleResources(frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW, intermediateGraph);

    for (auto deactivation : deactivations)
      switch (deactivation.index())
      {
        case 0: d3d::deactivate_texture(eastl::get<0>(deactivation)); break;
        case 1: d3d::deactivate_buffer(eastl::get<1>(deactivation)); break;
        case 2:
          auto [f, x] = eastl::get<2>(deactivation);
          f(x);
          break;
      }

    allResourceEvents = eastl::move(events);
  }

  // After rescheduling resources are in default resolution, so update number of frames to
  // resize textures on next nodes execution.
  // NOTE: We anyway can't create textures in downscaled resolution because we need scheduling
  // with max possible resolution. Otherwise texture regions will overlap when resolution is higher.
  for (auto &[st, dyn, counter] : registry.autoResTypes)
    if (st != dyn)
      counter = ResourceScheduler::SCHEDULE_FRAME_WINDOW;

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

        switch (res.getResType())
        {
          case ResourceType::Texture:
            if (auto activation = get_activation_from_usage(res.asScheduled().history, usage, res.getResType()))
            {
              auto tex = resourceScheduler->getTexture(prevFrame, resIdx).getTex2D();
              d3d::activate_texture(tex, *activation, ResourceClearValue{});

              // TODO: these barriers might be very wrong. Everything
              // about barriers is fubar and needs to be reworked ;(
              if (auto barrier = barrier_for_transition({}, usage); barrier != RB_NONE)
                d3d::resource_barrier({tex, barrier, 0, 0});
            }
            break;

          case ResourceType::Buffer:
            if (auto activation = get_activation_from_usage(res.asScheduled().history, usage, res.getResType()))
            {
              auto buf = resourceScheduler->getBuffer(prevFrame, resIdx).getBuf();
              d3d::activate_buffer(buf, *activation, ResourceClearValue{});

              if (auto barrier = barrier_for_transition({}, usage); barrier != RB_NONE)
                d3d::resource_barrier({buf, barrier});
            }
            break;

          case ResourceType::Blob:
            switch (res.asScheduled().history)
            {
              case History::No:
                logerr("Encountered a CPU resource with history that"
                       " does not specify it's first-frame action! Asan will"
                       " NOT appreciate this!");
                break;

              case History::DiscardOnFirstFrame:
              case History::ClearZeroOnFirstFrame:
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

void Runtime::runNodes()
{
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

      case CompilationStage::REQUIRES_IR_GRAPH_BUILD: buildIrGraph(); [[fallthrough]];

      case CompilationStage::REQUIRES_NODE_SCHEDULING: scheduleNodes(); [[fallthrough]];

      case CompilationStage::REQUIRES_STATE_DELTA_RECALCULATION: recalculateStateDeltas(); [[fallthrough]];

      case CompilationStage::REQUIRES_RESOURCE_SCHEDULING: scheduleResources(); [[fallthrough]];

      case CompilationStage::REQUIRES_HISTORY_OF_NEW_RESOURCES_INITIALIZATION: initializeHistoryOfNewResources(); [[fallthrough]];

      case CompilationStage::UP_TO_DATE: break;
    }
  }

  const int prevFrame = (frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW);
  const int currFrame = (++frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW);

  if (PLATFORM_HAS_HEAPS)
  {
    DynamicResolutions dynResolutions;
    for (auto [id, dynResType] : registry.autoResTypes.enumerate())
      if (dynResType.dynamicResolutionCountdown > 0)
      {
        dynResolutions.set(id, dynResType.dynamicResolution);
        --dynResType.dynamicResolutionCountdown;
      }
    resourceScheduler->resizeAutoResTextures(currFrame, dynResolutions);
  }
  else
  {
    for (auto [id, dynResType] : registry.autoResTypes.enumerate())
      if (eastl::exchange(dynResType.dynamicResolutionCountdown, 0) > 0)
        logerr("daBfg: Attempted to use dynamic resolution '%s' on a platform that does not support resource heaps!",
          registry.knownNames.getName(id));
  }

  const auto &frameEvents = allResourceEvents[currFrame];

  nodeExec->execute(prevFrame, currFrame, currentMultiplexingExtents, frameEvents, perNodeStateDeltas);
}

void Runtime::requestCompleteResourceRescheduling()
{
  resourceScheduler->shutdown(frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW);
  markStageDirty(CompilationStage::REQUIRES_RESOURCE_SCHEDULING);
}

void before_reset(bool)
{
  validation_restart();
  Runtime::get().requestCompleteResourceRescheduling();
}

} // namespace dabfg

#include <3d/dag_drv3dReset.h>
REGISTER_D3D_BEFORE_RESET_FUNC(dabfg::before_reset);
