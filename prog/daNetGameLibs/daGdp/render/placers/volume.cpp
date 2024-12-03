// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/shared_ptr.h>
#include <generic/dag_enumerate.h>
#include <drv/3d/dag_rwResource.h>
#include <3d/dag_lockSbuffer.h>
#include <render/daBfg/bfg.h>
#include <math/dag_hlsl_floatx.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <frustumCulling/frustumPlanes.h>
#include "../../shaders/dagdp_common.hlsli"
#include "../../shaders/dagdp_common_placer.hlsli"
#include "../../shaders/dagdp_volume.hlsli"
#include "volume.h"

namespace var
{
static ShaderVariableInfo draw_ranges("dagdp_volume__draw_ranges");
static ShaderVariableInfo placeables("dagdp_volume__placeables");
static ShaderVariableInfo placeable_weights("dagdp_volume__placeable_weights");
static ShaderVariableInfo renderable_indices("dagdp_volume__renderable_indices");
static ShaderVariableInfo variants("dagdp_volume__variants");

static ShaderVariableInfo debug_frustum_culling_bias("dagdp_volume__debug_frustum_culling_bias");
static ShaderVariableInfo max_placeable_bounding_radius("dagdp_volume__max_placeable_bounding_radius");
static ShaderVariableInfo num_renderables("dagdp_volume__num_renderables");
static ShaderVariableInfo num_placeables("dagdp_volume__num_placeables");

static ShaderVariableInfo prng_seed_placeable("dagdp_volume__prng_seed_placeable");
static ShaderVariableInfo prng_seed_slope("dagdp_volume__prng_seed_slope");
static ShaderVariableInfo prng_seed_scale("dagdp_volume__prng_seed_scale");
static ShaderVariableInfo prng_seed_yaw("dagdp_volume__prng_seed_yaw");
static ShaderVariableInfo prng_seed_pitch("dagdp_volume__prng_seed_pitch");
static ShaderVariableInfo prng_seed_roll("dagdp_volume__prng_seed_roll");
static ShaderVariableInfo prng_seed_triangle1("dagdp_volume__prng_seed_triangle1");
static ShaderVariableInfo prng_seed_triangle2("dagdp_volume__prng_seed_triangle2");

static ShaderVariableInfo viewport_pos("dagdp_volume__viewport_pos");
static ShaderVariableInfo viewport_max_distance("dagdp_volume__viewport_max_distance");
static ShaderVariableInfo viewport_index("dagdp_volume__viewport_index");

static ShaderVariableInfo mesh_index("dagdp_volume__mesh_index");
static ShaderVariableInfo num_dispatches("dagdp_volume__num_dispatches");

static ShaderVariableInfo mesh_params("dagdp_volume__mesh_params");
static ShaderVariableInfo areas_start_offset("dagdp_volume__areas_start_offset");
static ShaderVariableInfo areas_bottom_offset("dagdp_volume__areas_bottom_offset");
static ShaderVariableInfo areas_top_offset("dagdp_volume__areas_top_offset");
static ShaderVariableInfo areas_count("dagdp_volume__areas_count");
static ShaderVariableInfo areas("dagdp_volume__areas");
} // namespace var

using TmpName = eastl::fixed_string<char, 128>;

namespace dagdp
{

static constexpr uint32_t MAX_TRIANGLES = 1 << 20;
static constexpr uint32_t MAX_MESHES = 1 << 10;
static constexpr uint32_t ESTIMATED_PREFIX_SUM_LEVELS = 4;
static inline uint32_t divUp(uint32_t size, uint32_t stride) { return (size + stride - 1) / stride; }

struct VolumeConstants
{
  ViewInfo viewInfo;
  eastl::fixed_string<char, 32> viewResourceName;
  float maxPlaceableBoundingRadius;
  uint32_t numRenderables;
  uint32_t numPlaceables;
  uint32_t prngSeed;
  uint32_t totalCounters;
  VolumeMapping mapping;
};

// Note: not safe if FG starts using MT execution.
struct VolumeMutables
{
  uint32_t areasUsed = 0;
  RiexProcessor riexProcessor;
};

struct VolumePersistentData
{
  CommonPlacerBuffers commonBuffers;

  UniqueBuf allocsBuffer;
  UniqueBuf areasBuffer;
  UniqueBuf dispatchArgsBuffer;

  VolumeConstants constants{};
  VolumeMutables mutables{};
};

struct VolumePerViewportInfo
{
  uint32_t meshStartIndex;
  uint32_t meshEndIndex;
};

struct VolumeMeshesInfo
{
  dag::RelocatableFixedVector<VolumePerViewportInfo, 6> perViewport;
  RelevantMeshes relevantMeshes;
  bool isValid = false;

  dag::RelocatableFixedVector<MeshToProcess, ESTIMATED_PROCESSED_MESHES_PER_FRAME> meshesToProcess;
};

template <typename T>
using PerLevel = dag::RelocatableFixedVector<T, ESTIMATED_PREFIX_SUM_LEVELS, true, framemem_allocator>;

template <typename T>
using PerProcessedMesh = dag::RelocatableFixedVector<T, ESTIMATED_PROCESSED_MESHES_PER_FRAME, true, framemem_allocator>;

void create_volume_nodes(
  const ViewInfo &view_info, const ViewBuilder &view_builder, const VolumeManager &volume_manager, NodeInserter node_inserter)
{
  FRAMEMEM_REGION;

  auto &builder = volume_manager.currentBuilder;
  CommonPlacerBufferInit commonBufferInit;

  for (const auto &variant : builder.variants)
    add_variant(commonBufferInit, variant.objectGroups, variant.density, 0.0f);

  if (commonBufferInit.numPlaceables == 0)
    return;

  TmpName bufferNamePrefix(TmpName::CtorSprintf(), "dagdp_%s_volume", view_info.uniqueName.c_str());
  auto persistentData = eastl::make_shared<VolumePersistentData>();
  VolumeConstants &constants = persistentData->constants;
  constants.mapping = eastl::move(builder.mapping);
  constants.viewInfo = view_info;
  constants.viewResourceName.append_sprintf("view@%s", constants.viewInfo.uniqueName.c_str());
  constants.numRenderables = view_builder.numRenderables;
  constants.numPlaceables = commonBufferInit.numPlaceables;
  constants.prngSeed = 0; // Not implemented for now.
  constants.totalCounters = constants.viewInfo.maxViewports * constants.numRenderables;

  constants.maxPlaceableBoundingRadius = 0.0f;
  for (const auto &variant : builder.variants)
    for (const auto &objectGroup : variant.objectGroups)
      constants.maxPlaceableBoundingRadius = max(constants.maxPlaceableBoundingRadius, objectGroup.info->maxPlaceableBoundingRadius);

  bool commonSuccess = init_common_placer_buffers(commonBufferInit, bufferNamePrefix, persistentData->commonBuffers);
  if (!commonSuccess)
    return;

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_areas", bufferNamePrefix.c_str());
    persistentData->areasBuffer =
      dag::buffers::create_ua_sr_structured(sizeof(float), MAX_TRIANGLES, bufferName.c_str(), d3d::buffers::Init::Zero);
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_dispatch_args", bufferNamePrefix.c_str());
    persistentData->dispatchArgsBuffer = dag::create_sbuffer(sizeof(uint32_t), 3 * MAX_MESHES,
      SBCF_BIND_UNORDERED | SBCF_BIND_SHADER_RES | SBCF_MISC_ALLOW_RAW | SBCF_MISC_DRAWINDIRECT | SBCF_ZEROMEM, 0, bufferName.c_str());
  }

  const dabfg::NameSpace ns = dabfg::root() / "dagdp" / view_info.uniqueName.c_str() / "volume";

  dabfg::NodeHandle gatherNode = ns.registerNode("gather", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    const auto meshesBufferHandle = registry.create("meshes", dabfg::History::No)
                                      .structuredBufferUaSr<MeshIntersection>(MAX_MESHES)
                                      .atStage(dabfg::Stage::TRANSFER)
                                      .useAs(dabfg::Usage::COPY)
                                      .handle();

    registry.registerBuffer("areas", [persistentData](auto) { return ManagedBufView(persistentData->areasBuffer); })
      .atStage(dabfg::Stage::COMPUTE)
      .useAs(dabfg::Usage::SHADER_RESOURCE);

    const auto viewHandle = registry.readBlob<ViewPerFrameData>(constants.viewResourceName.c_str()).handle();
    const auto meshesInfoHandle = registry.createBlob<VolumeMeshesInfo>("meshesInfo", dabfg::History::No).handle();
    return [persistentData, viewHandle, meshesBufferHandle, meshesInfoHandle] {
      const auto &constants = persistentData->constants;
      auto &meshesInfo = meshesInfoHandle.ref();
      auto *meshesBuffer = meshesBufferHandle.get();
      const auto &view = viewHandle.ref();
      G_ASSERT(view.viewports.size() <= constants.viewInfo.maxViewports);

      persistentData->mutables.riexProcessor.resetCurrent();

      for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
      {
        const uint32_t meshStartIndex = meshesInfo.relevantMeshes.size();
        const auto &viewport = view.viewports[viewportIndex];
        gather_meshes(persistentData->constants.mapping, constants.viewInfo, viewport, constants.maxPlaceableBoundingRadius,
          persistentData->mutables.riexProcessor, meshesInfo.relevantMeshes);

        auto &item = meshesInfo.perViewport.push_back();
        item.meshStartIndex = meshStartIndex;
        item.meshEndIndex = meshesInfo.relevantMeshes.size();
      }

      if (const auto *nextToProcess = persistentData->mutables.riexProcessor.current(); nextToProcess != nullptr)
      {
        const dag::ConstSpan<ShaderMesh::RElem> elems =
          nextToProcess->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque);

        for (const auto &elem : elems)
        {
          auto &item = meshesInfo.meshesToProcess.push_back();
          item.startIndex = elem.si;
          item.numFaces = elem.numf;
          item.baseVertex = elem.baseVertex;
          item.stride = elem.vertexData->getStride();
          item.vbIndex = elem.vertexData->getVbIdx();
        }
      }

      if (meshesInfo.relevantMeshes.empty())
        return;

      if (meshesInfo.relevantMeshes.size() > MAX_MESHES)
      {
        logerr("daGdp: volume placement exceeded max. number of meshes.");
        return;
      }

      const bool res = meshesBuffer->updateData(0, sizeof(MeshIntersection) * meshesInfo.relevantMeshes.size(),
        meshesInfo.relevantMeshes.data(), VBLOCK_WRITEONLY);
      if (!res)
      {
        logerr("daGdp: volume meshes buffer update error.");
        return;
      }

      meshesInfo.isValid = true;
    };
  });

  dabfg::NodeHandle processNode = ns.registerNode("process", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
    const auto meshesInfoHandle = registry.readBlob<VolumeMeshesInfo>("meshesInfo").handle();
    registry.modify("areas").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp_volume__areas");
    registry.read("meshes").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp_volume__meshes");

    return [persistentData, meshesInfoHandle, shaderTri = ComputeShader("dagdp_volume_mesh_process_tri"),
             shaderUp = ComputeShader("dagdp_volume_mesh_process_up"), shaderDown = ComputeShader("dagdp_volume_mesh_process_down")] {
      FRAMEMEM_REGION;

      const auto &meshesInfo = meshesInfoHandle.ref();
      AreasIndices areasIndices;
      bool outOfMem = false;

      const auto bumpAllocate = [&used = persistentData->mutables.areasUsed, &outOfMem](uint32_t count) {
        uint32_t offset = used;
        used += count;
        if (used > MAX_TRIANGLES)
          outOfMem = true;
        return offset;
      };

      struct DispatchTri
      {
        MeshToProcess mesh;
        uint32_t areasStartOffset;
      };

      struct DispatchUpDown
      {
        uint32_t areasBottomCount;
        uint32_t areasBottomOffset;
        uint32_t areasTopOffset;
        uint32_t areasTopCount;
      };

      PerProcessedMesh<DispatchTri> dispatchTris;
      PerLevel<PerProcessedMesh<DispatchUpDown>> dispatchUpDowns; // Up forwards <=> down backwards

      for (const auto &mesh : meshesInfo.meshesToProcess)
      {
        uint32_t count = mesh.numFaces;
        uint32_t prevOffset = ~0u;
        uint32_t prevCount = 0;

        uint32_t level = 0;
        do
        {
          if (level == 0)
          {
            auto &item = dispatchTris.push_back();
            item.mesh = mesh;
            item.areasStartOffset = bumpAllocate(count);
            prevOffset = item.areasStartOffset;
            prevCount = count;
            areasIndices.push_back(item.areasStartOffset);
          }
          else
          {
            if (dispatchUpDowns.size() < level)
              dispatchUpDowns.push_back();

            auto &item = dispatchUpDowns[level - 1].push_back();
            item.areasBottomOffset = prevOffset;
            item.areasBottomCount = prevCount;
            item.areasTopOffset = bumpAllocate(count);
            item.areasTopCount = count;
            prevOffset = item.areasTopOffset;
            prevCount = item.areasTopCount;
          }

          ++level;
          count = divUp(count, DAGDP_PREFIX_SUM_GROUP_SIZE) - 1;
        } while (count > 0);
      }

      if (outOfMem)
      {
        logerr("daGdp: volume mesh processing ran out of memory! %" PRIu32 " > %" PRIu32, persistentData->mutables.areasUsed,
          MAX_TRIANGLES);
        return;
      }

      Ibuffer *ib = unitedvdata::riUnitedVdata.getIB();
      d3d::set_buffer(STAGE_CS, 0, ib);

      bool dispatchSuccess = true;
      for (const auto &dispatch : dispatchTris)
      {
        const auto &mesh = dispatch.mesh;
        Vbuffer *vb = unitedvdata::riUnitedVdata.getVB(mesh.vbIndex);
        d3d::set_buffer(STAGE_CS, 1, vb);
        ShaderGlobal::set_int4(var::mesh_params, IPoint4(mesh.startIndex, mesh.numFaces, mesh.baseVertex, mesh.stride));
        ShaderGlobal::set_int(var::areas_start_offset, dispatch.areasStartOffset);
        dispatchSuccess &= shaderTri.dispatchThreads(mesh.numFaces, 1, 1);
      }

      d3d::set_buffer(STAGE_CS, 0, nullptr);
      d3d::set_buffer(STAGE_CS, 1, nullptr);

      d3d::resource_barrier({persistentData->areasBuffer.getBuf(), RB_RW_UAV | RB_STAGE_COMPUTE});

      for (const auto &perLevel : dispatchUpDowns)
      {
        for (const auto &dispatch : perLevel)
        {
          ShaderGlobal::set_int(var::areas_bottom_offset, dispatch.areasBottomOffset);
          ShaderGlobal::set_int(var::areas_top_offset, dispatch.areasTopOffset);
          ShaderGlobal::set_int(var::areas_count, dispatch.areasTopCount);
          dispatchSuccess &= shaderUp.dispatchThreads(dispatch.areasTopCount, 1, 1);
        }

        d3d::resource_barrier({persistentData->areasBuffer.getBuf(), RB_RW_UAV | RB_STAGE_COMPUTE});
      }

      // Backwards.
      for (auto it = dispatchUpDowns.end(); it != dispatchUpDowns.begin();)
      {
        const auto &level = *--it;
        for (const auto &dispatch : level)
        {
          ShaderGlobal::set_int(var::areas_bottom_offset, dispatch.areasBottomOffset);
          ShaderGlobal::set_int(var::areas_top_offset, dispatch.areasTopOffset);
          ShaderGlobal::set_int(var::areas_count, dispatch.areasBottomCount);
          dispatchSuccess &= shaderDown.dispatchThreads(dispatch.areasBottomCount, 1, 1);
        }

        if (it != dispatchUpDowns.begin())
        {
          // Can skip this barrier on the last level.
          d3d::resource_barrier({persistentData->areasBuffer.getBuf(), RB_RW_UAV | RB_STAGE_COMPUTE});
        }
      }

      if (!dispatchSuccess)
      {
        logerr("daGdp: volume mesh processing dispatch failed!");
        return;
      }

      persistentData->mutables.riexProcessor.markCurrentAsProcessed(eastl::move(areasIndices));
    };
  });

  dabfg::NodeHandle copyArgsNode = ns.registerNode("set_args", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    registry.registerBuffer("dispatch_args", [persistentData](auto) { return ManagedBufView(persistentData->dispatchArgsBuffer); })
      .atStage(dabfg::Stage::COMPUTE)
      .bindToShaderVar("dagdp_volume__dispatch_args");

    registry.read("areas").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp_volume__areas");
    registry.read("meshes").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp_volume__meshes");
    const auto meshesInfoHandle = registry.readBlob<VolumeMeshesInfo>("meshesInfo").handle();

    return [persistentData, meshesInfoHandle, shader = ComputeShader("dagdp_volume_set_args")] {
      const auto &meshesInfo = meshesInfoHandle.ref();

      if (!meshesInfo.isValid)
        return;

      ShaderGlobal::set_buffer(var::variants, persistentData->commonBuffers.variantsBuffer.getBufId());
      ShaderGlobal::set_int(var::num_dispatches, meshesInfo.relevantMeshes.size());
      const bool dispatchSuccess = shader.dispatchThreads(meshesInfo.relevantMeshes.size(), 1, 1);
      ShaderGlobal::set_buffer(var::variants, BAD_D3DRESID);

      if (!dispatchSuccess)
        logerr("daGdp: volume set args dispatch failed!");
    };
  });

  const auto createPlaceNode = [&](bool isOptimistic) {
    return ns.registerNode(isOptimistic ? "place_stage0" : "place_stage1", DABFG_PP_NODE_SRC,
      [isOptimistic, persistentData](dabfg::Registry registry) {
        const auto &constants = persistentData->constants;
        view_multiplex(registry, constants.viewInfo.kind);
        registry.read("meshes").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp_volume__meshes");
        registry.read("areas").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp_volume__areas");
        registry.read("dispatch_args").buffer().atStage(dabfg::Stage::COMPUTE).useAs(dabfg::Usage::INDIRECTION_BUFFER);
        const auto meshesInfoHandle = registry.readBlob<VolumeMeshesInfo>("meshesInfo").handle();
        const auto viewHandle = registry.readBlob<ViewPerFrameData>(constants.viewResourceName.c_str()).handle();

        const auto ns = registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str();

        ns.read(isOptimistic ? "dyn_allocs_stage0" : "dyn_allocs_stage1")
          .buffer()
          .atStage(dabfg::Stage::COMPUTE)
          .bindToShaderVar("dagdp__dyn_allocs");

        ns.modify(isOptimistic ? "dyn_counters_stage0" : "dyn_counters_stage1")
          .buffer()
          .atStage(dabfg::Stage::COMPUTE)
          .bindToShaderVar("dagdp__dyn_counters");

        ns.modify("instance_data").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp__instance_data");

        return [persistentData, viewHandle, meshesInfoHandle, isOptimistic,
                 shader = ComputeShader(isOptimistic ? "dagdp_volume_place_stage0" : "dagdp_volume_place_stage1")] {
          const auto &constants = persistentData->constants;
          const auto &view = viewHandle.ref();
          const auto &meshesInfo = meshesInfoHandle.ref();

          if (!meshesInfo.isValid)
            return;

          bool dispatchSuccess = true;

          STATE_GUARD(ShaderGlobal::set_buffer(var::draw_ranges, VALUE), persistentData->commonBuffers.drawRangesBuffer.getBufId(),
            BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::placeables, VALUE), persistentData->commonBuffers.placeablesBuffer.getBufId(),
            BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::placeable_weights, VALUE),
            persistentData->commonBuffers.placeableWeightsBuffer.getBufId(), BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::renderable_indices, VALUE),
            persistentData->commonBuffers.renderableIndicesBuffer.getBufId(), BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::variants, VALUE), persistentData->commonBuffers.variantsBuffer.getBufId(),
            BAD_D3DRESID);

          ShaderGlobal::set_real(var::debug_frustum_culling_bias, get_frustum_culling_bias());
          ShaderGlobal::set_real(var::max_placeable_bounding_radius, constants.maxPlaceableBoundingRadius);
          ShaderGlobal::set_int(var::num_renderables, constants.numRenderables);
          ShaderGlobal::set_int(var::num_placeables, constants.numPlaceables);
          ShaderGlobal::set_int(var::prng_seed_placeable, constants.prngSeed + 0x08C2592Cu);
          ShaderGlobal::set_int(var::prng_seed_scale, constants.prngSeed + 0xDF3069FFu);
          ShaderGlobal::set_int(var::prng_seed_slope, constants.prngSeed + 0x3C1385DBu);
          ShaderGlobal::set_int(var::prng_seed_yaw, constants.prngSeed + 0x71F23960u);
          ShaderGlobal::set_int(var::prng_seed_pitch, constants.prngSeed + 0xDEB40CF0u);
          ShaderGlobal::set_int(var::prng_seed_roll, constants.prngSeed + 0xF6A38A81u);
          ShaderGlobal::set_int(var::prng_seed_triangle1, constants.prngSeed + 0x54F2A367u);
          ShaderGlobal::set_int(var::prng_seed_triangle2, constants.prngSeed + 0x45F9668Eu);

          Ibuffer *ib = unitedvdata::riUnitedVdata.getIB();
          d3d::set_buffer(STAGE_CS, 0, ib);

          uint32_t dispatchIndex = 0;
          for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
          {
            const auto &info = meshesInfo.perViewport[viewportIndex];
            const uint32_t numMeshes = info.meshEndIndex - info.meshStartIndex;

            const auto &viewport = view.viewports[viewportIndex];
            ScopeFrustumPlanesShaderVars scopedFrustumVars(viewport.frustum);

            ShaderGlobal::set_color4(var::viewport_pos, viewport.worldPos);
            ShaderGlobal::set_real(var::viewport_max_distance, min(viewport.maxDrawDistance, constants.viewInfo.maxDrawDistance));
            ShaderGlobal::set_int(var::viewport_index, viewportIndex);

            for (uint32_t meshIndex = 0; meshIndex < numMeshes; ++meshIndex)
            {
              const auto &mesh = meshesInfo.relevantMeshes[meshIndex + info.meshStartIndex];
              Vbuffer *vb = unitedvdata::riUnitedVdata.getVB(mesh.vbIndex);
              d3d::set_buffer(STAGE_CS, 1, vb);

              ShaderGlobal::set_int(var::mesh_index, info.meshStartIndex + meshIndex);

              // TODO: Performance: ideally need to merge all dispatches (regardless of viewport / vbIndex / meshIndex).
              // TODO: Performance: in the pessimistic phase, should set dispatch args to zero, based on success/failure of the
              // optimistic phase.
              int indirectOffset = 3 * sizeof(uint32_t) * dispatchIndex++;
              dispatchSuccess &= shader.dispatchIndirect(persistentData->dispatchArgsBuffer.getBuf(), indirectOffset);
            }
          }

          d3d::set_buffer(STAGE_CS, 0, nullptr);
          d3d::set_buffer(STAGE_CS, 1, nullptr);

          if (!dispatchSuccess)
            logerr("daGdp: volume place dispatch failed! (optimistic = %d)", isOptimistic);
        };
      });
  };

  // Optimistic placement.
  dabfg::NodeHandle placeStage0Node = createPlaceNode(true);

  // Pessimistic placement.
  dabfg::NodeHandle placeStage1Node = createPlaceNode(false);

  node_inserter(eastl::move(gatherNode));
  node_inserter(eastl::move(processNode));
  node_inserter(eastl::move(copyArgsNode));
  node_inserter(eastl::move(placeStage0Node));
  node_inserter(eastl::move(placeStage1Node));
}

} // namespace dagdp
