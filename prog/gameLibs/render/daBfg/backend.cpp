#include "memory/dag_framemem.h"
#include <backend.h>

#include <EASTL/sort.h>
#include <mutex> // std::lock_guard

#include <3d/dag_drv3d.h>

#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>

#include <render/daBfg/bfg.h>

#include <debug/backendDebug.h>
#include <nodes/nodeStateDeltas.h>
#include <multiplexingInternal.h>

#include <resourceScheduling/nativeResourceScheduler.h>
#include <resourceScheduling/poolResourceScheduler.h>
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

InitOnDemand<Backend, false> Backend::instance;

Backend::Backend()
{
  if (PLATFORM_HAS_HEAPS)
    resourceScheduler.reset(new NativeResourceScheduler(nodeTracker));
  else
    resourceScheduler.reset(new PoolResourceScheduler(nodeTracker));

  nodeExec.emplace(*resourceScheduler, intermediateGraph, irMapping, registry, currentlyProvidedResources);
}

Backend::~Backend()
{
  // CPU resources must be cleaned up gracefully when shutting down
  resourceScheduler->shutdown(frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW);
}

StackRingMemAlloc<> &get_per_compilation_memalloc() { return Backend::get().perCompilationMemAlloc; }

NodeTracker &NodeTracker::get() { return Backend::get().nodeTracker; }


void Backend::gatherNodeData()
{
  TIME_PROFILE(gatherNodeData);
  nodeTracker.gatherNodeData();
  currentStage = CompilationStage::REQUIRES_IR_GENERATION;
}

void Backend::declareNodes()
{
  TIME_PROFILE(declareNodes);
  nodeTracker.declareNodes();
  currentStage = CompilationStage::REQUIRES_NODE_DATA_GATHERING;
}

void Backend::gatherGraphIR()
{
  TIME_PROFILE(gatherGraphIR);
  intermediateGraph = nodeTracker.emitIR({backbufSinkResources.begin(), backbufSinkResources.end()}, currentMultiplexingExtents);

  currentStage = CompilationStage::REQUIRES_NODE_SCHEDULING;
}

void Backend::scheduleNodes()
{
  TIME_PROFILE(scheduleNodes);

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
      for (auto nodeId : irNode.frontendNodes)
        if (irNode.multiplexingIndex == 0)
          demultiplexedNodeExecutionOrder.emplace_back(nodeId);

    update_graph_visualization(&nodeTracker, demultiplexedNodeExecutionOrder);
  }

  perNodeStateDeltas = calculate_per_node_state_deltas(intermediateGraph);

  currentStage = CompilationStage::REQUIRES_RESOURCE_SCHEDULING;
}

void Backend::scheduleResources()
{
  TIME_PROFILE(scheduleResources);

  // Update automatic texture resolutions
  for (auto resIdx : IdRange<intermediate::ResourceIndex>(intermediateGraph.resources.size()))
  {
    if (!intermediateGraph.resources[resIdx].isScheduled())
      continue;
    auto &res = intermediateGraph.resources[resIdx].asScheduled();
    if (res.resourceType != ResourceType::Texture || !res.resolutionType.has_value())
      continue;

    auto [id, mult] = *res.resolutionType;

    // Impossible situation, sanity check
    G_ASSERT_CONTINUE(id != AutoResTypeNameId::Invalid);

    auto &texDesc = eastl::get<ResourceDescription>(res.description).asTexRes;
    texDesc.width = static_cast<uint32_t>(registry.autoResTypes[id].staticResolution.x * mult);
    texDesc.height = static_cast<uint32_t>(registry.autoResTypes[id].staticResolution.y * mult);
  }

  if (debug_graph_generation.get())
  {
    dag::Vector<NodeNameId, framemem_allocator> frontendNodeExecutionOrder;
    for (const intermediate::Node &irNode : intermediateGraph.nodes)
      for (auto nodeId : irNode.frontendNodes)
        frontendNodeExecutionOrder.emplace_back(nodeId);

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

  currentStage = CompilationStage::REQUIRES_FIRST_FRAME_HISTORY_HANDLING;
}

void Backend::handleFirstFrameHistory()
{
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

void Backend::setMultiplexingExtents(multiplexing::Extents extents)
{
  if (currentMultiplexingExtents != extents)
  {
    currentMultiplexingExtents = extents;
    markStageDirty(CompilationStage::REQUIRES_IR_GENERATION);
  }
}

void Backend::runNodes()
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
      case CompilationStage::REQUIRES_NODES_DECLARATION: declareNodes(); [[fallthrough]];

      case CompilationStage::REQUIRES_NODE_DATA_GATHERING: gatherNodeData(); [[fallthrough]];

      case CompilationStage::REQUIRES_IR_GENERATION: gatherGraphIR(); [[fallthrough]];

      case CompilationStage::REQUIRES_NODE_SCHEDULING: scheduleNodes(); [[fallthrough]];

      case CompilationStage::REQUIRES_RESOURCE_SCHEDULING: scheduleResources(); [[fallthrough]];

      case CompilationStage::REQUIRES_FIRST_FRAME_HISTORY_HANDLING: handleFirstFrameHistory(); [[fallthrough]];

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

  const auto &frameEvents = allResourceEvents[currFrame];

  nodeExec->execute(prevFrame, currFrame, currentMultiplexingExtents, frameEvents, perNodeStateDeltas);
}

IPoint2 Backend::getResolution(const char *typeName) const
{
  const auto id = registry.knownAutoResolutionTypeNames.getNameId(typeName);
  if (id == AutoResTypeNameId::Invalid || !registry.autoResTypes.isMapped(id))
  {
    logerr("Tried to get resolution for daBfg auto-res type %s that wasn't set yet!"
           " Please call dabfg::set_resolution(\"%s\", ...)!",
      typeName, typeName);
    return {};
  }
  return registry.autoResTypes[id].dynamicResolution;
}

void Backend::setResolution(const char *typeName, IPoint2 value)
{
  const auto id = registry.knownAutoResolutionTypeNames.addNameId(typeName);
  const AutoResType newResolution = AutoResType{value, value};
  if (!registry.autoResTypes.isMapped(id) || registry.autoResTypes[id].staticResolution != newResolution.staticResolution ||
      registry.autoResTypes[id].dynamicResolution != newResolution.dynamicResolution)
  {
    markStageDirty(CompilationStage::REQUIRES_RESOURCE_SCHEDULING);
    registry.autoResTypes.set(id, newResolution);
  }
}

void Backend::setDynamicResolution(const char *typeName, IPoint2 value)
{
  const auto id = registry.knownAutoResolutionTypeNames.getNameId(typeName);
  if (id == AutoResTypeNameId::Invalid || !registry.autoResTypes.isMapped(id))
  {
    logerr("Tried to set dynamic resolution for daBfg auto-res type %s that wasn't set yet!"
           " Please call dabfg::set_resolution(\"%s\", ...)!",
      typeName, typeName);
    return;
  }
  registry.autoResTypes[id].dynamicResolution = value;
  // We can't immediately change resolution for all textures because history textures will lose
  // their data. So update counter here and decrease it by one when change resolution only for
  // current frame textures.
  registry.autoResTypes[id].dynamicResolutionCountdown = ResourceScheduler::SCHEDULE_FRAME_WINDOW;
}

void Backend::fillSlot(NamedSlot slot, const char *res_name)
{
  const ResNameId slotNameId = registry.knownResourceNames.addNameId(slot.name);
  const ResNameId resNameId = registry.knownResourceNames.addNameId(res_name);
  registry.resourceSlots.set(slotNameId, SlotData{resNameId});
  markStageDirty(CompilationStage::REQUIRES_NODE_DATA_GATHERING);
}

void Backend::requestCompleteResourceRescheduling()
{
  resourceScheduler->shutdown(frameIndex % ResourceScheduler::SCHEDULE_FRAME_WINDOW);
  markStageDirty(CompilationStage::REQUIRES_RESOURCE_SCHEDULING);
}

void before_reset(bool)
{
  validation_restart();
  Backend::get().requestCompleteResourceRescheduling();
}

} // namespace dabfg

#include <3d/dag_drv3dReset.h>
REGISTER_D3D_BEFORE_RESET_FUNC(dabfg::before_reset);
