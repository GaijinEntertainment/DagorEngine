// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/shared_ptr.h>
#include <generic/dag_enumerate.h>
#include <generic/dag_span.h>
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
static ShaderVariableInfo volume_variants("dagdp_volume__volume_variants");

static ShaderVariableInfo debug_frustum_culling_bias("dagdp_volume__debug_frustum_culling_bias");
static ShaderVariableInfo max_placeable_bounding_radius("dagdp_volume__max_placeable_bounding_radius");
static ShaderVariableInfo num_renderables("dagdp_volume__num_renderables");
static ShaderVariableInfo num_placeables("dagdp_volume__num_placeables");
static ShaderVariableInfo tile_start_index("dagdp_volume__tile_start_index");

static ShaderVariableInfo prng_seed_placeable("dagdp_volume__prng_seed_placeable");
static ShaderVariableInfo prng_seed_slope("dagdp_volume__prng_seed_slope");
static ShaderVariableInfo prng_seed_scale("dagdp_volume__prng_seed_scale");
static ShaderVariableInfo prng_seed_yaw("dagdp_volume__prng_seed_yaw");
static ShaderVariableInfo prng_seed_pitch("dagdp_volume__prng_seed_pitch");
static ShaderVariableInfo prng_seed_roll("dagdp_volume__prng_seed_roll");
static ShaderVariableInfo prng_seed_triangle1("dagdp_volume__prng_seed_triangle1");
static ShaderVariableInfo prng_seed_triangle2("dagdp_volume__prng_seed_triangle2");
static ShaderVariableInfo prng_seed_jitter_x("dagdp_volume__prng_seed_jitter_x");
static ShaderVariableInfo prng_seed_jitter_z("dagdp_volume__prng_seed_jitter_z");

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

static ShaderVariableInfo counts("dagdp_volume__counts");

static ShaderVariableInfo volumes("dagdp_volume__volumes");
static ShaderVariableInfo meshes("dagdp_volume__meshes");
static ShaderVariableInfo tiles("dagdp_volume__tiles");
} // namespace var

using TmpName = eastl::fixed_string<char, 128>;

namespace dagdp
{

static constexpr uint32_t MAX_TRIANGLES = 1 << 20;
static constexpr uint32_t MAX_MESHES = 1 << 10;
static constexpr uint32_t MAX_TILES = 1 << 10;
static constexpr uint32_t MAX_VOLUMES = 1 << 6;

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
  UniqueBuf volumeVariantsBuffer;
  UniqueBuf volumesBuffer;
  UniqueBuf meshesBuffer;
  UniqueBuf tilesBuffer;
  UniqueBuf terrainTileCountsBuffer;

  VolumeConstants constants{};
  VolumeMutables mutables{};
};

struct GatheredInfo
{
  struct PerViewport
  {
    uint32_t meshStartIndex;
    uint32_t meshEndIndex;
    uint32_t tileStartIndex;
    uint32_t tileEndIndex;
  };

  dag::RelocatableFixedVector<PerViewport, DAGDP_MAX_VIEWPORTS> perViewport;
  RelevantMeshes relevantMeshes;
  RelevantTiles relevantTiles;
  RelevantVolumes relevantVolumes;
  bool isValid = false;

  dag::RelocatableFixedVector<MeshToProcess, ESTIMATED_PROCESSED_MESHES_PER_FRAME> meshesToProcess;
};

template <typename T>
using PerLevel = dag::RelocatableFixedVector<T, ESTIMATED_PREFIX_SUM_LEVELS, true, framemem_allocator>;

template <typename T>
using PerProcessedMesh = dag::RelocatableFixedVector<T, ESTIMATED_PROCESSED_MESHES_PER_FRAME, true, framemem_allocator>;

template <typename T>
bool updateWithDiscard(dag::ConstSpan<T> items, Sbuffer *buffer, uint32_t max_size, const char *what)
{
  if (items.size() == 0)
    return true;

  if (items.size() > max_size)
  {
    logerr("daGdp: volume placement exceeded max. number of %s.", what);
    return false;
  }

  if (!buffer->updateData(0, sizeof(T) * items.size(), items.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD))
  {
    logerr("daGdp: volume %s staging buffer update error.", what);
    return false;
  }

  return true;
}

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
    TmpName bufferName(TmpName::CtorSprintf(), "%s_volume_variants", bufferNamePrefix.c_str());
    persistentData->volumeVariantsBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(VolumeVariantGpuData), builder.variants.size(), bufferName.c_str());

    dag::Vector<VolumeVariantGpuData, framemem_allocator> variantsGpuDataFmem(builder.variants.size());

    for (uint32_t i = 0; i < builder.variants.size(); ++i)
    {
      const auto &variant = builder.variants[i];
      auto &gpuData = variantsGpuDataFmem[i];
      gpuData.density = variant.density;
      gpuData.minTriangleArea2 = 2.0f * variant.minTriangleArea;
    }

    bool updated = persistentData->volumeVariantsBuffer->updateData(0, data_size(variantsGpuDataFmem), variantsGpuDataFmem.data(),
      VBLOCK_WRITEONLY);

    if (!updated)
    {
      logerr("daGdp: could not update buffer %s", bufferName.c_str());
      return;
    }
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_volumes", bufferNamePrefix.c_str());
    persistentData->volumesBuffer =
      dag::buffers::create_one_frame_sr_structured(sizeof(VolumeGpuData), MAX_VOLUMES, bufferName.c_str());
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_meshes", bufferNamePrefix.c_str());
    persistentData->meshesBuffer =
      dag::buffers::create_one_frame_sr_structured(sizeof(MeshIntersection), MAX_MESHES, bufferName.c_str());
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_tiles", bufferNamePrefix.c_str());
    persistentData->tilesBuffer =
      dag::buffers::create_one_frame_sr_structured(sizeof(VolumeTerrainTile), MAX_TILES, bufferName.c_str());
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_terrain_tile_counts", bufferNamePrefix.c_str());
    persistentData->terrainTileCountsBuffer =
      dag::buffers::create_one_frame_cb(dag::buffers::cb_array_reg_count<uint4>(DAGDP_MAX_VIEWPORTS), bufferName.c_str());
  }

  const dabfg::NameSpace ns = dabfg::root() / "dagdp" / view_info.uniqueName.c_str() / "volume";

  dabfg::NodeHandle gatherNode = ns.registerNode("gather", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    registry.registerBuffer("areas", [persistentData](auto) { return ManagedBufView(persistentData->areasBuffer); })
      .atStage(dabfg::Stage::COMPUTE)
      .useAs(dabfg::Usage::SHADER_RESOURCE);

    const auto viewHandle = registry.readBlob<ViewPerFrameData>(constants.viewResourceName.c_str()).handle();
    const auto gatheredInfoHandle = registry.createBlob<GatheredInfo>("gatheredInfo", dabfg::History::No).handle();
    return [persistentData, viewHandle, gatheredInfoHandle] {
      const auto &constants = persistentData->constants;
      auto &gatheredInfo = gatheredInfoHandle.ref();
      const auto &view = viewHandle.ref();
      G_ASSERT(view.viewports.size() <= constants.viewInfo.maxViewports);

      persistentData->mutables.riexProcessor.resetCurrent();

      for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
      {
        const uint32_t meshStartIndex = gatheredInfo.relevantMeshes.size();
        const uint32_t tileStartIndex = gatheredInfo.relevantTiles.size();
        const auto &viewport = view.viewports[viewportIndex];
        gather(persistentData->constants.mapping, constants.viewInfo, viewport, constants.maxPlaceableBoundingRadius,
          persistentData->mutables.riexProcessor, gatheredInfo.relevantMeshes, gatheredInfo.relevantTiles,
          gatheredInfo.relevantVolumes);

        auto &item = gatheredInfo.perViewport.push_back();
        item.meshStartIndex = meshStartIndex;
        item.meshEndIndex = gatheredInfo.relevantMeshes.size();
        item.tileStartIndex = tileStartIndex;
        item.tileEndIndex = gatheredInfo.relevantTiles.size();
      }

      if (const auto *nextToProcess = persistentData->mutables.riexProcessor.current(); nextToProcess != nullptr)
      {
        const dag::ConstSpan<ShaderMesh::RElem> elems =
          nextToProcess->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque);

        for (const auto &elem : elems)
        {
          auto &item = gatheredInfo.meshesToProcess.push_back();
          item.startIndex = elem.si;
          item.numFaces = elem.numf;
          item.baseVertex = elem.baseVertex;
          item.stride = elem.vertexData->getStride();
          item.vbIndex = elem.vertexData->getVbIdx();
        }
      }

      if (
        !updateWithDiscard(make_span_const(gatheredInfo.relevantMeshes), persistentData->meshesBuffer.getBuf(), MAX_MESHES, "meshes"))
        return;

      if (!updateWithDiscard(make_span_const(gatheredInfo.relevantTiles), persistentData->tilesBuffer.getBuf(), MAX_TILES, "tiles"))
        return;

      if (!updateWithDiscard(make_span_const(gatheredInfo.relevantVolumes), persistentData->volumesBuffer.getBuf(), MAX_VOLUMES,
            "volumes"))
        return;

      gatheredInfo.isValid = true;
    };
  });

  dabfg::NodeHandle processNode = ns.registerNode("process", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
    const auto gatheredInfoHandle = registry.readBlob<GatheredInfo>("gatheredInfo").handle();
    registry.modify("areas").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp_volume__areas");

    return [persistentData, gatheredInfoHandle, shaderTri = ComputeShader("dagdp_volume_mesh_process_tri"),
             shaderUp = ComputeShader("dagdp_volume_mesh_process_up"), shaderDown = ComputeShader("dagdp_volume_mesh_process_down")] {
      FRAMEMEM_REGION;

      const auto &gatheredInfo = gatheredInfoHandle.ref();
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

      for (const auto &mesh : gatheredInfo.meshesToProcess)
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

      STATE_GUARD(ShaderGlobal::set_buffer(var::meshes, VALUE), persistentData->meshesBuffer.getBufId(), BAD_D3DRESID);

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

  dabfg::NodeHandle setArgsNode = ns.registerNode("set_args", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    registry.create("dispatch_args", dabfg::History::No)
      .buffer({sizeof(uint32_t), DISPATCH_INDIRECT_NUM_ARGS * MAX_MESHES,
        SBCF_BIND_UNORDERED | SBCF_BIND_SHADER_RES | SBCF_MISC_ALLOW_RAW | SBCF_MISC_DRAWINDIRECT, 0})
      .atStage(dabfg::Stage::COMPUTE)
      .bindToShaderVar("dagdp_volume__dispatch_args");

    registry.read("areas").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp_volume__areas");

    const auto gatheredInfoHandle = registry.readBlob<GatheredInfo>("gatheredInfo").handle();

    return [persistentData, gatheredInfoHandle, shader = ComputeShader("dagdp_volume_set_args")] {
      const auto &gatheredInfo = gatheredInfoHandle.ref();

      if (!gatheredInfo.isValid)
        return;

      STATE_GUARD(ShaderGlobal::set_buffer(var::meshes, VALUE), persistentData->meshesBuffer.getBufId(), BAD_D3DRESID);
      STATE_GUARD(ShaderGlobal::set_buffer(var::variants, VALUE), persistentData->commonBuffers.variantsBuffer.getBufId(),
        BAD_D3DRESID);
      STATE_GUARD(ShaderGlobal::set_buffer(var::volume_variants, VALUE), persistentData->volumeVariantsBuffer.getBufId(),
        BAD_D3DRESID);

      ShaderGlobal::set_int(var::num_dispatches, gatheredInfo.relevantMeshes.size());
      const bool dispatchSuccess = shader.dispatchThreads(gatheredInfo.relevantMeshes.size(), 1, 1);

      if (!dispatchSuccess)
        logerr("daGdp: volume set args dispatch failed!");
    };
  });

  const auto createPlaceNode = [&](bool isOptimistic) {
    return ns.registerNode(isOptimistic ? "place_stage0" : "place_stage1", DABFG_PP_NODE_SRC,
      [isOptimistic, persistentData](dabfg::Registry registry) {
        const auto &constants = persistentData->constants;
        view_multiplex(registry, constants.viewInfo.kind);

        registry.read("areas").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp_volume__areas");
        const auto dispatchArgsHandle =
          registry.read("dispatch_args").buffer().atStage(dabfg::Stage::COMPUTE).useAs(dabfg::Usage::INDIRECTION_BUFFER).handle();
        const auto gatheredInfoHandle = registry.readBlob<GatheredInfo>("gatheredInfo").handle();
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

        return [persistentData, viewHandle, gatheredInfoHandle, dispatchArgsHandle, isOptimistic,
                 shader = ComputeShader(isOptimistic ? "dagdp_volume_place_stage0" : "dagdp_volume_place_stage1")] {
          const auto &constants = persistentData->constants;
          const auto &view = viewHandle.ref();
          const auto &gatheredInfo = gatheredInfoHandle.ref();
          auto *dispatchArgsBuffer = const_cast<Sbuffer *>(dispatchArgsHandle.get());

          if (!gatheredInfo.isValid)
            return;

          bool dispatchSuccess = true;

          STATE_GUARD(ShaderGlobal::set_buffer(var::volumes, VALUE), persistentData->volumesBuffer.getBufId(), BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::meshes, VALUE), persistentData->meshesBuffer.getBufId(), BAD_D3DRESID);

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
          STATE_GUARD(ShaderGlobal::set_buffer(var::volume_variants, VALUE), persistentData->volumeVariantsBuffer.getBufId(),
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
            const auto &info = gatheredInfo.perViewport[viewportIndex];
            const uint32_t numMeshes = info.meshEndIndex - info.meshStartIndex;

            const auto &viewport = view.viewports[viewportIndex];
            ScopeFrustumPlanesShaderVars scopedFrustumVars(viewport.frustum);

            ShaderGlobal::set_color4(var::viewport_pos, viewport.worldPos);
            ShaderGlobal::set_real(var::viewport_max_distance, min(viewport.maxDrawDistance, constants.viewInfo.maxDrawDistance));
            ShaderGlobal::set_int(var::viewport_index, viewportIndex);

            for (uint32_t meshIndex = 0; meshIndex < numMeshes; ++meshIndex)
            {
              const auto &mesh = gatheredInfo.relevantMeshes[meshIndex + info.meshStartIndex];
              Vbuffer *vb = unitedvdata::riUnitedVdata.getVB(mesh.vbIndex);
              d3d::set_buffer(STAGE_CS, 1, vb);

              ShaderGlobal::set_int(var::mesh_index, info.meshStartIndex + meshIndex);

              // TODO: Performance: ideally need to merge all dispatches (regardless of viewport / vbIndex / meshIndex).
              // TODO: Performance: in the pessimistic phase, should set dispatch args to zero, based on success/failure of the
              // optimistic phase. Should be easy to do, since they are already indirect.
              int indirectOffset = DISPATCH_INDIRECT_NUM_ARGS * sizeof(uint32_t) * dispatchIndex++;
              dispatchSuccess &= shader.dispatchIndirect(dispatchArgsBuffer, indirectOffset);
            }
          }

          d3d::set_buffer(STAGE_CS, 0, nullptr);
          d3d::set_buffer(STAGE_CS, 1, nullptr);

          if (!dispatchSuccess)
            logerr("daGdp: volume place dispatch failed! (optimistic = %d)", isOptimistic);
        };
      });
  };

  dabfg::NodeHandle terrainSetArgsNode =
    ns.registerNode("terrain_set_args", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
      const auto &constants = persistentData->constants;
      view_multiplex(registry, constants.viewInfo.kind);
      const auto gatheredInfoHandle = registry.readBlob<GatheredInfo>("gatheredInfo").handle();
      const auto viewHandle = registry.readBlob<ViewPerFrameData>(constants.viewResourceName.c_str()).handle();

      registry.create("terrain_dispatch_args", dabfg::History::No)
        .buffer({sizeof(uint32_t), DISPATCH_INDIRECT_NUM_ARGS * DAGDP_MAX_VIEWPORTS,
          SBCF_BIND_UNORDERED | SBCF_BIND_SHADER_RES | SBCF_MISC_ALLOW_RAW | SBCF_MISC_DRAWINDIRECT, 0})
        .atStage(dabfg::Stage::COMPUTE)
        .bindToShaderVar("dagdp_volume__dispatch_args");

      return [persistentData, viewHandle, gatheredInfoHandle, shader = ComputeShader("dagdp_volume_terrain_set_args")] {
        const auto &gatheredInfo = gatheredInfoHandle.ref();
        const auto &view = viewHandle.ref();

        if (!gatheredInfo.isValid)
          return;

        if (auto lockedBuffer =
              lock_sbuffer<uint4>(persistentData->terrainTileCountsBuffer.getBuf(), 0, 0, VBLOCK_DISCARD | VBLOCK_WRITEONLY))
        {
          for (int i = 0; i < view.viewports.size(); ++i)
            lockedBuffer[i].x = gatheredInfo.perViewport[i].tileEndIndex - gatheredInfo.perViewport[i].tileStartIndex;

          ShaderGlobal::set_int(var::num_dispatches, view.viewports.size());
          ShaderGlobal::set_buffer(var::counts, persistentData->terrainTileCountsBuffer.getBufId());
          const bool dispatchSuccess = shader.dispatchThreads(view.viewports.size(), 1, 1);

          if (!dispatchSuccess)
            logerr("daGdp: volume terrain set args dispatch failed!");
        }
      };
    });

  const auto createTerrainPlaceNode = [&](bool isOptimistic) {
    return ns.registerNode(isOptimistic ? "terrain_place_stage0" : "terrain_place_stage1", DABFG_PP_NODE_SRC,
      [isOptimistic, persistentData](dabfg::Registry registry) {
        const auto &constants = persistentData->constants;
        view_multiplex(registry, constants.viewInfo.kind);

        const auto dispatchArgsHandle = registry.read("terrain_dispatch_args")
                                          .buffer()
                                          .atStage(dabfg::Stage::COMPUTE)
                                          .useAs(dabfg::Usage::INDIRECTION_BUFFER)
                                          .handle();
        const auto viewHandle = registry.readBlob<ViewPerFrameData>(constants.viewResourceName.c_str()).handle();
        const auto gatheredInfoHandle = registry.readBlob<GatheredInfo>("gatheredInfo").handle();

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

        return [persistentData, viewHandle, gatheredInfoHandle, dispatchArgsHandle, isOptimistic,
                 shader = ComputeShader(isOptimistic ? "dagdp_volume_terrain_place_stage0" : "dagdp_volume_terrain_place_stage1")] {
          const auto &constants = persistentData->constants;
          const auto &view = viewHandle.ref();
          const auto &gatheredInfo = gatheredInfoHandle.ref();
          auto *dispatchArgsBuffer = const_cast<Sbuffer *>(dispatchArgsHandle.get());

          if (!gatheredInfo.isValid)
            return;

          STATE_GUARD(ShaderGlobal::set_buffer(var::volumes, VALUE), persistentData->volumesBuffer.getBufId(), BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::tiles, VALUE), persistentData->tilesBuffer.getBufId(), BAD_D3DRESID);

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
          ShaderGlobal::set_int(var::prng_seed_jitter_x, constants.prngSeed + 0xDE437762u);
          ShaderGlobal::set_int(var::prng_seed_jitter_z, constants.prngSeed + 0xA2EA427Du);

          bool dispatchSuccess = true;
          for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
          {
            const auto &viewport = view.viewports[viewportIndex];
            const auto &info = gatheredInfo.perViewport[viewportIndex];

            ShaderGlobal::set_color4(var::viewport_pos, viewport.worldPos);
            ShaderGlobal::set_real(var::viewport_max_distance, min(viewport.maxDrawDistance, constants.viewInfo.maxDrawDistance));
            ShaderGlobal::set_int(var::viewport_index, viewportIndex);
            ShaderGlobal::set_int(var::tile_start_index, info.tileStartIndex);

            // TODO: Performance: ideally need to merge all dispatches.
            // TODO: Performance: in the pessimistic phase, should set dispatch args to zero, based on success/failure of the
            // optimistic phase. Should be easy to do, since they are already indirect.
            int indirectOffset = DISPATCH_INDIRECT_NUM_ARGS * sizeof(uint32_t) * viewportIndex;
            dispatchSuccess &= shader.dispatchIndirect(dispatchArgsBuffer, indirectOffset);
          }

          if (!dispatchSuccess)
            logerr("daGdp: volume place dispatch failed! (optimistic = %d)", isOptimistic);
        };
      });
  };

  node_inserter(eastl::move(gatherNode));
  node_inserter(eastl::move(processNode));

  node_inserter(eastl::move(setArgsNode));
  node_inserter(createPlaceNode(true));
  node_inserter(createPlaceNode(false));

  node_inserter(eastl::move(terrainSetArgsNode));
  node_inserter(createTerrainPlaceNode(true));
  node_inserter(createTerrainPlaceNode(false));
}

} // namespace dagdp
