// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_render.h>
#include <memory/dag_framemem.h>
#include <shaders/dag_shaders.h>
#include <util/dag_convar.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <render/daFrameGraph/daFG.h>

#include <3d/dag_lockSbuffer.h>
#include <render/world/depthBounds.h>
#include <render/indoorProbeScenes.h>
#include <render/indoorProbeNodes.h>

extern class ConVarT<float, 0> indoor_probes_max_distance;
static int indoor_probe_visible_pixels_count_const_no = -1;

void IndoorProbeNodes::registerNodes(uint32_t allProbesOnLevel, IndoorProbeManager *owner)
{
  G_ASSERT_RETURN(owner, );
  if (allProbesOnLevel == 0)
    return;

  if (depth_bounds_enabled())
    debug("Indoor probes: Depth bounds are enabled");
  else
    debug("Indoor probes: Depth bounds are disabled");

  // NOTE: we use a funny trick for conditional running of nodes:
  // If a node that creates a bool blob does an early return, that
  // blob will be false by default, so we'll skip consequent nodes
  // automatically.
  // This is why we do dontUseClusterization instead of useClusterization,
  // otherwise the default would be wrong.

  auto ns = dafg::root() / "indoor_probes";

  tryStartUpdateNode = ns.registerNode("try_start_update_node", DAFG_PP_NODE_SRC, [owner](dafg::Registry registry) {
    auto shouldUpdateHndl = registry.create("should_update", dafg::History::No).blob<bool>().handle();

    return [owner, shouldUpdateHndl](dafg::multiplexing::Index multiplexing_index) {
      const bool isFirstIteration = multiplexing_index == dafg::multiplexing::Index{};
      if (!isFirstIteration)
        return;

      // TODO: can this be hardcoded as allProbesOnLevel? Not sure,
      // maybe it changes during runtime.
      const bool anyBoxes = owner->shapesContainer && (owner->shapesContainer->getAllNodesCount() != 0);
      if (!anyBoxes)
        return;

      shouldUpdateHndl.ref() = true;

      owner->framesSinceLastCulling++;
    };
  });

  using CpuIndices = dag::RelocatableFixedVector<uint32_t, NON_CELL_PROBES_COUNT>;
  using CpuMatrices = dag::RelocatableFixedVector<mat44f, NON_CELL_PROBES_COUNT>;

  cpuCheckNode = ns.registerNode("cpu_check_node", DAFG_PP_NODE_SRC, [owner](dafg::Registry registry) {
    auto resources = eastl::make_tuple(registry.read("should_update").blob<bool>().handle(),
      registry.read("current_camera").blob<CameraParams>().handle(),
      registry.create("cpu_matrices", dafg::History::No).blob<CpuMatrices>().handle(),
      registry.create("cpu_indices", dafg::History::No).blob<CpuIndices>().handle(),
      registry.create("cpu_shape_types", dafg::History::No).blob<CpuIndices>().handle(),
      registry.create("dont_use_clusterization", dafg::History::No).blob<bool>().handle(),
      registry.create("readback_ready", dafg::History::No).blob<bool>().handle());

    return [owner, resourcesPtr = eastl::make_unique<decltype(resources)>(resources)]() {
      if (!resourcesPtr)
        return;

      auto [shouldUpdateHndl, cameraHndl, cpuMatricesHndl, cpuIndicesHndl, cpuShapeTypesHndl, dontUseClusterization,
        readbackReadyHndl] = *resourcesPtr;

      if (!shouldUpdateHndl.ref())
        return;

      const bool useOcclusion = !camera_in_camera::is_lens_render_active();
      Occlusion *occlusion = useOcclusion ? cameraHndl.ref().jobsMgr->getOcclusion() : nullptr;
      auto [cpuMats, cpuInds, cpuShapes, useCluster] =
        owner->cpuCheck(occlusion, cameraHndl.ref().viewItm.getcol(3), cameraHndl.ref().viewTm, cameraHndl.ref().jitterPersp);

      cpuMatricesHndl.ref() = eastl::move(cpuMats);
      cpuIndicesHndl.ref() = eastl::move(cpuInds);
      cpuShapeTypesHndl.ref() = eastl::move(cpuShapes);
      dontUseClusterization.ref() = !useCluster;

      readbackReadyHndl.ref() = d3d::get_event_query_status(owner->cullingDoneEvent.get(), false);
    };
  });

  completePreviousFrameReadbacksNode =
    ns.registerNode("complete_previous_frame_readbacks_node", DAFG_PP_NODE_SRC, [owner](dafg::Registry registry) {
      auto readbackReadyHndl = registry.read("readback_ready").blob<bool>().handle();
      auto shouldInitiateReadbackHndl = registry.create("should_initiate_readback", dafg::History::No).blob<bool>().handle();

      // We don't modify these directly, but we can invalidate the stuff
      // that they represent in updateActiveProbes, so we need to
      // reflect that as a modification for correct ordering
      registry.modify("cpu_matrices").blob<CpuMatrices>();
      registry.modify("cpu_indices").blob<CpuIndices>();
      registry.modify("cpu_shape_types").blob<CpuIndices>();

      return [owner, readbackReadyHndl, shouldInitiateReadbackHndl]() {
        if (!readbackReadyHndl.ref())
          return;

        owner->completeCullingReadback();

        shouldInitiateReadbackHndl.ref() = owner->framesSinceLastCulling > IndoorProbeManager::GPU_CULLING_FREQUENCY;
      };
    });

  createVisiblePixelsCountNode =
    ns.registerNode("create_visible_pixels_count_node", DAFG_PP_NODE_SRC, [allProbesOnLevel](dafg::Registry registry) {
      auto shouldInitiateReadbackHndl = registry.read("should_initiate_readback").blob<bool>().handle();

      auto visiblePixelsCountHndl = registry.create("visible_pixels_count", dafg::History::No)
                                      .structuredBufferUa<uint32_t>(allProbesOnLevel)
                                      .atStage(dafg::Stage::ALL_GRAPHICS)
                                      .useAs(dafg::Usage::SHADER_RESOURCE)
                                      .handle();

      return [shouldInitiateReadbackHndl, visiblePixelsCountHndl]() {
        if (!shouldInitiateReadbackHndl.ref())
          return;

        d3d::zero_rwbufi(visiblePixelsCountHndl.get());
      };
    });

  calculateVisiblePixelsCountNode =
    ns.registerNode("calculate_visible_pixels_count", DAFG_PP_NODE_SRC, [owner](dafg::Registry registry) {
      auto shouldInitiateReadbackHndl = registry.read("should_initiate_readback").blob<bool>().handle();

      auto visiblePixelsCountHndl = registry.modify("visible_pixels_count")
                                      .buffer()
                                      .atStage(dafg::Stage::ALL_GRAPHICS)
                                      .useAs(dafg::Usage::SHADER_RESOURCE)
                                      .handle();

      // Water SSR modifies downsampled depth, but we kind of read it,
      // so we ought to do it before it gets changed. Otherwise, when
      // looking at a reflective surface inside a house through water,
      // probes would be culled away and not used :/
      registry.create("probes_ready_token", dafg::History::No).blob<OrderingToken>();

      registry.requestRenderPass().depthRw("downsampled_depth");
      shaders::OverrideState state;
      state.set(shaders::OverrideState::Z_BOUNDS_ENABLED);
      registry.requestState()
        .setFrameBlock("global_frame")
        .enableOverride(depth_bounds_enabled() ? state : shaders::OverrideState{})
        .maxVrs();

      auto cameraHndl = registry.readBlob<CameraParams>("current_camera")
                          .bindAsView<&CameraParams::viewTm>()
                          .bindAsProj<&CameraParams::jitterProjTm>()
                          .handle();

      indoor_probe_visible_pixels_count_const_no =
        ShaderGlobal::get_int(get_shader_variable_id("indoor_probe_visible_pixels_count_const_no"));

      return [owner, shouldInitiateReadbackHndl, visiblePixelsCountHndl, cameraHndl]() {
        if (!shouldInitiateReadbackHndl.ref())
          return;

        if (depth_bounds_enabled())
        {
          Point2 zn_zfar = {cameraHndl.ref().noJitterPersp.zn, cameraHndl.ref().noJitterPersp.zf};
          // inv_linearizeZ
          float zMin = (zn_zfar.y / (indoor_probes_max_distance)-1) * zn_zfar.x / (zn_zfar.y - zn_zfar.x);
          api_set_depth_bounds(zMin, 1);
        }

        STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_PS, indoor_probe_visible_pixels_count_const_no, VALUE),
          visiblePixelsCountHndl.get());
        const bool hasUavInVSCapability = d3d::get_driver_desc().caps.hasUAVOnEveryStage;
        STATE_GUARD_NULLPTR(
          hasUavInVSCapability ? d3d::set_rwbuffer(STAGE_VS, indoor_probe_visible_pixels_count_const_no, VALUE) : false,
          visiblePixelsCountHndl.get());

        owner->cullingElem->setStates(0, true);
        d3d::draw(PRIM_TRILIST, 0, 12 * owner->shapesContainer->getAllNodesCount());
      };
    });

  initiateVisiblePixelsCountReadbackNode =
    ns.registerNode("initiate_visible_pixels_readback", DAFG_PP_NODE_SRC, [owner](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);

      auto shouldInitiateReadbackHndl = registry.read("should_initiate_readback").blob<bool>().handle();

      auto visiblePixelsCountHndl =
        registry.read("visible_pixels_count").buffer().atStage(dafg::Stage::TRANSFER).useAs(dafg::Usage::COPY).handle();

      return [owner, shouldInitiateReadbackHndl, visiblePixelsCountHndl]() {
        if (!shouldInitiateReadbackHndl.ref())
          return;

        visiblePixelsCountHndl.view()->copyTo(owner->readbackBuffer.getBuf());
        issue_readback_query(owner->cullingDoneEvent.get(), owner->readbackBuffer.getBuf());
        owner->framesSinceLastCulling = 0;
        owner->needsToReadBackCulling = true;
      };
    });

  setVisibleBoxesDataNode = ns.registerNode("set_visible_boxes_data", DAFG_PP_NODE_SRC, [owner](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);

    auto dontUseClusterizationHndl = registry.read("dont_use_clusterization").blob<bool>().handle();

    auto cpuMatricesHndl = registry.read("cpu_matrices").blob<CpuMatrices>().handle();
    auto cpuIndicesHndl = registry.read("cpu_indices").blob<CpuIndices>().handle();
    auto cpuShapeTypesHndl = registry.read("cpu_shape_types").blob<CpuIndices>().handle();

    // We consolidate probe visibility data here and upload it to the
    // GPU, so this node MUST be run before anything that uses probes
    // is rendered. In the future, this blob should probably become
    // creation of the indoorVisibleProbesData buffer.
    registry.modify("probes_ready_token").blob<OrderingToken>();

    return [owner, dontUseClusterizationHndl, cpuMatricesHndl, cpuIndicesHndl, cpuShapeTypesHndl]() {
      if (!dontUseClusterizationHndl.ref())
        return;

      owner->setVisibleBoxesData(cpuIndicesHndl.ref(), cpuMatricesHndl.ref(), cpuShapeTypesHndl.ref());
    };
  });
}
