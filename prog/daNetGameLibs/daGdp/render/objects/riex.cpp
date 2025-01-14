// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/numeric.h>
#include <EASTL/fixed_string.h>
#include <EASTL/hash_set.h>
#include <dag/dag_vectorSet.h>
#include <util/dag_stlqsort.h>
#include <math/dag_hlsl_floatx.h>
#include <generic/dag_span.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <render/renderer.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/defaultVrsSettings.h>
#include <3d/dag_lockSbuffer.h>
#include <shaders/scriptSElem.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shStateBlockBindless.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <shaders/dag_shaderBlock.h>
#include <rendInst/packedMultidrawParams.hlsli>
#include <rendInst/riShaderConstBuffers.h>
#include <rendInst/rendInstGenRender.h>
#include <render/genRender.h>
#include <math/dag_hlsl_floatx.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <ecs/rendInst/riExtra.h>
#include "../../shaders/dagdp_riex.hlsli"
#include "shaders/rendinst.hlsli"
#include "riex.h"

namespace var
{
static ShaderVariableInfo num_patches("dagdp_riex__num_patches");
} // namespace var

namespace material_var
{
static ShaderVariableInfo draw_order("draw_order");
} // namespace material_var

namespace external_var
{
static ShaderVariableInfo global_transp_r("global_transp_r");
static ShaderVariableInfo instancing_type("instancing_type");
static ShaderVariableInfo rendinst_render_pass("rendinst_render_pass");
} // namespace external_var

using TmpName = eastl::fixed_string<char, 256>;

namespace dagdp
{

enum class SubPass
{
  PRE_PASS_ALL = 0,
  NORMAL_PASS_OPAQUE,
  NORMAL_PASS_DECALS,
  DYN_SHADOW_PASS_ALL,
  COUNT
};

struct SubPassRange
{
  SubPass first;
  SubPass last; // Inclusive.
};

// Utils for nice iteration.
static inline void operator++(SubPass &subpass) { subpass = SubPass(1 + eastl::to_underlying(subpass)); }
static inline SubPass operator*(SubPass subpass) { return subpass; }
static inline SubPass begin(const SubPassRange &range) { return range.first; }
static inline SubPass end(const SubPassRange &range) { return SubPass(1 + eastl::to_underlying(range.last)); }

static constexpr SubPassRange VIEW_KIND_SUBPASSES[VIEW_KIND_COUNT] = {
  {SubPass::PRE_PASS_ALL, SubPass::NORMAL_PASS_DECALS},         // MAIN_CAMERA
  {SubPass::DYN_SHADOW_PASS_ALL, SubPass::DYN_SHADOW_PASS_ALL}, // DYN_SHADOWS
};

static constexpr size_t SUBPASS_COUNT = eastl::to_underlying(SubPass::COUNT);

// There are now separate counters for static regions, and dynamic regions.
static constexpr uint32_t COUNTER_KINDS_NUM = 2;
static constexpr uint32_t COUNTER_KIND_STATIC = 0;
static constexpr uint32_t COUNTER_KIND_DYNAMIC = 1;

struct StageRange
{
  int first;
  int last; // Inclusive.
};

static constexpr const char *MAIN_CAMERA_SUBPASS_NODE_NAMES[SUBPASS_COUNT] = {
  // clang-format off
  "dagdp_riex_pre",
  "dagdp_riex_opaque",
  "dagdp_riex_decal",
  nullptr // DYN_SHADOW_PASS_ALL
  // clang-format on
};

static constexpr StageRange SUBPASS_STAGES[SUBPASS_COUNT] = {
  {ShaderMesh::STG_atest, ShaderMesh::STG_atest},      // PRE_PASS_ALL
  {ShaderMesh::STG_opaque, ShaderMesh::STG_imm_decal}, // NORMAL_PASS_OPAQUE
  {ShaderMesh::STG_decal, ShaderMesh::STG_decal},      // NORMAL_PASS_DECALS
  {ShaderMesh::STG_opaque, ShaderMesh::STG_atest},     // DYN_SHADOW_PASS_ALL
};

static constexpr rendinst::RenderPass SUBPASS_RI_RENDER_PASS[SUBPASS_COUNT] = {
  rendinst::RenderPass::Depth,  // PRE_PASS_ALL
  rendinst::RenderPass::Normal, // NORMAL_PASS_OPAQUE
  rendinst::RenderPass::Normal, // NORMAL_PASS_DECALS
  rendinst::RenderPass::Depth,  // DYN_SHADOW_PASS_ALL
};

// TODO: duplicates dag_multidrawContext.h
struct ExtendedDrawIndexedIndirectArgs
{
  uint32_t drawCallId;
  DrawIndexedIndirectArgs args;
};

// TODO: duplicates dag_multidrawContext.h
static bool uses_extended_multi_draw_struct() { return d3d::get_driver_code().is(d3d::dx12); }

// Each of these corresponds to V actual draw calls, where V is the number of viewports.
struct ProtoDrawCall
{
  InstanceRegion staticInstanceRegion;
  const ShaderMesh::RElem *rElem;
  RenderableId rId;

  int stage; // ShaderMesh::STG_*
  int drawOrder;
  bool isTree;
  ScriptedShaderElement *sElem;
  shaders::CombinedDynVariantState dvState;
  int vbIndex;
  int vbStride;
};

// Each of these corresponds to V spans of draw calls, where V is the number of viewports.
struct MultiDrawCall
{
  uint32_t byteOffset; // Relative to viewport base offset.
  uint32_t count;      // How many draw calls are in the span?

  bool isForcedSingle; // For some platforms and/or materials, we can't use multidraw.
  InstanceRegion staticInstanceRegion;

  // All of the draw calls share the below state:
  int stage; // ShaderMesh::STG_*
  bool isTree;
  ScriptedShaderElement *sElem;
  shaders::CombinedDynVariantState dvState;
  int vbIndex;
  int vbStride;
};

#define COMPARE_FIELDS     \
  CMP(stage)               \
  CMP(drawOrder)           \
  CMP(isTree)              \
  CMP(sElem)               \
  CMP(dvState.program)     \
  CMP(dvState.variant)     \
  CMP(dvState.state_index) \
  CMP(vbIndex)             \
  CMP(vbStride)

static bool operator<(const ProtoDrawCall &a, const ProtoDrawCall &b)
{
#define CMP(field)        \
  if (a.field != b.field) \
    return a.field < b.field;

  COMPARE_FIELDS
  return false;

#undef CMP
}

static bool can_coalesce(const ProtoDrawCall &a, const ProtoDrawCall &b)
{
#define CMP(field)        \
  if (a.field != b.field) \
    return false;

  COMPARE_FIELDS
  return true;

#undef CMP
}

// Never changes for the given view.
struct RiexConstants
{
  bool isExtendedArgs;
  bool isNonMultidrawIndirectionNeeded;
  uint32_t argsStride;
  uint32_t argsDwords;

  uint32_t instanceBufferElementStride;
  uint32_t instanceBufferElementsCount;

  ViewInfo viewInfo;
  uint32_t maxStaticInstancesPerViewport;
  uint32_t maxDrawCallsPerViewport;
  uint32_t numRenderables;

  bool haveDynamicRegions;

  eastl::array<bool, SUBPASS_COUNT> haveSubPass;
};

// May be recalculated for the given view, if RElem vertex data is moved.
struct RiexCache
{
  uint32_t numCounterPatches;
  uint32_t numCallsPerViewport;
  dag::VectorSet<const ScriptedShaderElement *> usedSElems;
  dag::VectorSet<shaders::ConstStateIdx> packedConstStates;
  dag::Vector<MultiDrawCall> multiCalls;
  eastl::array<dag::Span<MultiDrawCall>, SUBPASS_COUNT> multiCallSpans;
};

struct RiexRenderableViewInfo
{
  RenderableId rId;
  RiexRenderableInfo info;
  InstanceRegion instanceRegion;
};

// Used to keep everything potentially used by nodes alive.
struct RiexPersistentData
{
  UniqueBuf counterPatchesBuffer;
  UniqueBuf drawArgsBuffer;
  dag::Vector<RiexResource> resources;
  CallbackToken rElemsUpdatedToken;
  shaders::UniqueOverrideStateId afterPrePassOverride;
  int riAdditionalInstanceOffsetRegNo = -1;

  dag::Vector<RiexRenderableViewInfo> renderablesInfo;
  eastl::hash_set<const RenderableInstanceLodsResource *> usedRI;
  RiexConstants constants{};
  RiexCache cache{};

  bool isCacheValid = false;
  bool areBuffersValid = false;
};

class ExternalShaderVarsScope
{
  float global_transp_r;
  int instancing_type;
  int rendinst_render_pass;

public:
  ExternalShaderVarsScope(rendinst::RenderPass renderPass)
  {
    global_transp_r = ShaderGlobal::get_real(external_var::global_transp_r);
    instancing_type = ShaderGlobal::get_int(external_var::instancing_type);
    rendinst_render_pass = ShaderGlobal::get_int(external_var::rendinst_render_pass);
    ShaderGlobal::set_real(external_var::global_transp_r, 1.0f);
    ShaderGlobal::set_int(external_var::instancing_type, 0);
    ShaderGlobal::set_int(external_var::rendinst_render_pass, eastl::to_underlying(renderPass));
  }

  ~ExternalShaderVarsScope()
  {
    ShaderGlobal::set_real(external_var::global_transp_r, global_transp_r);
    ShaderGlobal::set_int(external_var::instancing_type, instancing_type);
    ShaderGlobal::set_int(external_var::rendinst_render_pass, rendinst_render_pass);
  }
};

template <typename Args>
void update_draw_calls(LockedBuffer<Args> &locked_buffer,
  const RiexConstants &constants,
  const dag::Vector<ProtoDrawCall, framemem_allocator> &proto_draw_calls)
{
  for (uint32_t counterKind = 0; counterKind < COUNTER_KINDS_NUM; ++counterKind)
    for (uint32_t viewportIndex = 0; viewportIndex < constants.viewInfo.maxViewports; ++viewportIndex)
    {
      uint32_t baseIndex = (viewportIndex + counterKind * constants.viewInfo.maxViewports) * proto_draw_calls.size();
      int j = 0;
      for (auto &call : proto_draw_calls)
      {
        const int i = j++;
        DrawIndexedIndirectArgs *args;
        if constexpr (eastl::is_same_v<Args, ExtendedDrawIndexedIndirectArgs>)
          args = &locked_buffer[baseIndex + i].args;
        else
          args = &locked_buffer[baseIndex + i];

        args->indexCountPerInstance = 0; // Will be patched by CS.
        args->instanceCount = 0;         // Will be patched by CS.
        args->startIndexLocation = call.rElem->si;
        args->baseVertexLocation = call.rElem->baseVertex;

        const bool isPacked = is_packed_material(call.dvState.const_state);
        const uint32_t instanceBaseIndex =
          constants.maxStaticInstancesPerViewport * viewportIndex + call.staticInstanceRegion.baseIndex;

        uint32_t drawCallId = 0;
        if (isPacked)
        {
          // Note: taken from riExtraRendererT.h
          const uint32_t materialOffset = get_material_offset(call.dvState.const_state);

          // For dynamic counters, drawCallId needs to be patched on the GPU.
          if (counterKind == COUNTER_KIND_STATIC)
            drawCallId = (instanceBaseIndex << MATRICES_OFFSET_SHIFT) | materialOffset;
        }

        if constexpr (eastl::is_same_v<Args, ExtendedDrawIndexedIndirectArgs>)
        {
          locked_buffer[baseIndex + i].drawCallId = drawCallId;
          args->startInstanceLocation = isPacked ? 0 : instanceBaseIndex;
        }
        else
          args->startInstanceLocation = isPacked ? drawCallId : instanceBaseIndex;
      }
    }
}

[[nodiscard]] static bool update_buffers(RiexPersistentData &persistent_data,
  dag::Vector<ProtoDrawCall, framemem_allocator> &proto_draw_calls)
{
  RiexConstants &constants = persistent_data.constants;

  const uint32_t drawCallsActualCount = COUNTER_KINDS_NUM * proto_draw_calls.size() * constants.viewInfo.maxViewports;
  const uint32_t drawCallsActualByteSize = drawCallsActualCount * constants.argsStride;
  UniqueBuf stagingForDrawCalls;
  {
    TmpName bufferName(TmpName::CtorSprintf(), "dagdp_riex_%s_draw_args_staging", constants.viewInfo.uniqueName.c_str());
    stagingForDrawCalls = dag::buffers::create_staging(drawCallsActualByteSize, bufferName.c_str());

    if (constants.isExtendedArgs)
    {
      auto lockedBuffer =
        lock_sbuffer<ExtendedDrawIndexedIndirectArgs>(stagingForDrawCalls.getBuf(), 0, drawCallsActualCount, VBLOCK_WRITEONLY);
      if (!lockedBuffer)
      {
        logerr("daGdp: could not lock buffer %s", bufferName.c_str());
        return false;
      }
      update_draw_calls(lockedBuffer, constants, proto_draw_calls);
    }
    else
    {
      auto lockedBuffer =
        lock_sbuffer<DrawIndexedIndirectArgs>(stagingForDrawCalls.getBuf(), 0, drawCallsActualCount, VBLOCK_WRITEONLY);
      if (!lockedBuffer)
      {
        logerr("daGdp: could not lock buffer %s", bufferName.c_str());
        return false;
      }
      update_draw_calls(lockedBuffer, constants, proto_draw_calls);
    }
  }

  const uint32_t counterPatchesActualCount = proto_draw_calls.size() * constants.viewInfo.maxViewports;
  const uint32_t counterPatchesActualByteSize = counterPatchesActualCount * sizeof(RiexPatch);
  UniqueBuf stagingForCounterPatches;
  {
    TmpName bufferName(TmpName::CtorSprintf(), "dagdp_riex_%s_counter_patches_staging", constants.viewInfo.uniqueName.c_str());
    stagingForCounterPatches = dag::buffers::create_staging(counterPatchesActualByteSize, bufferName.c_str());

    auto lockedBuffer = lock_sbuffer<RiexPatch>(stagingForCounterPatches.getBuf(), 0, counterPatchesActualCount, VBLOCK_WRITEONLY);
    if (!lockedBuffer)
    {
      logerr("daGdp: could not lock buffer %s", bufferName.c_str());
      return false;
    }

    G_STATIC_ASSERT(COUNTER_KINDS_NUM == 2);
    for (uint32_t viewportIndex = 0; viewportIndex < constants.viewInfo.maxViewports; ++viewportIndex)
    {
      const uint32_t baseIndex = viewportIndex * proto_draw_calls.size();
      const uint32_t baseIndexDynamic =
        (viewportIndex + COUNTER_KIND_DYNAMIC * constants.viewInfo.maxViewports) * proto_draw_calls.size();
      int j = 0;
      for (auto &call : proto_draw_calls)
      {
        const int i = j++;
        const bool isPacked = is_packed_material(call.dvState.const_state);

        RiexPatch &patch = lockedBuffer[baseIndex + i];
        patch.argsByteOffsetStatic = (baseIndex + i) * constants.argsStride + (constants.isExtendedArgs ? sizeof(uint32_t) : 0);
        patch.argsByteOffsetDynamic =
          (baseIndexDynamic + i) * constants.argsStride + (constants.isExtendedArgs ? sizeof(uint32_t) : 0);
        patch.localCounterIndex = viewportIndex * constants.numRenderables + call.rId;
        patch.indexCount = call.rElem->numf * 3;
        patch.materialOffset = isPacked ? get_material_offset(call.dvState.const_state) : 0;
        patch.flags = (constants.isExtendedArgs ? RIEX_PATCH_FLAG_BIT_EXTENDED_ARGS : 0) | (isPacked ? RIEX_PATCH_FLAG_BIT_PACKED : 0);
      }
    }
  }

  if (!stagingForDrawCalls->copyTo(persistent_data.drawArgsBuffer.getBuf(), 0, 0, drawCallsActualByteSize))
  {
    logerr("daGdp: could not copy draw call data from staging.");
    return false;
  }

  if (!stagingForCounterPatches->copyTo(persistent_data.counterPatchesBuffer.getBuf(), 0, 0, counterPatchesActualByteSize))
  {
    logerr("daGdp: could not copy counter patches from staging.");
    return false;
  }

  return true;
}

static void rebuild_cache(RiexPersistentData &persistent_data)
{
  FRAMEMEM_REGION;

  RiexConstants &constants = persistent_data.constants;
  RiexCache &cache = persistent_data.cache;

  cache.usedSElems.clear();
  cache.packedConstStates.clear();
  cache.numCounterPatches = 0;
  cache.numCallsPerViewport = 0;
  cache.multiCalls.clear();
  for (auto &span : cache.multiCallSpans)
    span.reset();

  dag::Vector<ProtoDrawCall, framemem_allocator> protoDrawCallsFmem; // Same for each viewport.
  protoDrawCallsFmem.reserve(constants.maxDrawCallsPerViewport);

  eastl::array<uint32_t, SUBPASS_COUNT> multiEndIndices = {};

  for (auto subPass : VIEW_KIND_SUBPASSES[eastl::to_underlying(constants.viewInfo.kind)])
  {
    const auto protoStartIndex = protoDrawCallsFmem.size();

    // Set the same state as when rendering, because `shaders::get_dynamic_variant_state` implicitly depends on it.
    ExternalShaderVarsScope externalShaderVarsScope(SUBPASS_RI_RENDER_PASS[eastl::to_underlying(subPass)]);

    auto stages = SUBPASS_STAGES[eastl::to_underlying(subPass)];
    for (int stage = stages.first; stage <= stages.last; ++stage)
    {
      for (const auto &renderable : persistent_data.renderablesInfo)
      {
        const auto &lodRes = renderable.info.lodsRes->lods[renderable.info.lodIndex];
        const ShaderMesh *shaderMesh = lodRes.scene->getMesh()->getMesh()->getMesh();

        for (const auto &rElem : shaderMesh->getElems(stage, stage))
        {
          if (rElem.vertexData->isEmpty()) // Not loaded yet, skip it.
            continue;

          ScriptedShaderElement &sElem = rElem.e->native();
          const auto dvState = shaders::get_dynamic_variant_state(sElem);

          if (dvState.variant == -1) // "Don't render".
            continue;

          auto &call = protoDrawCallsFmem.push_back();
          call.rElem = &rElem;
          call.staticInstanceRegion = renderable.instanceRegion;
          call.rId = renderable.rId;

          call.stage = stage;
          call.drawOrder = 0;
          rElem.mat->getIntVariable(material_var::draw_order.get_var_id(), call.drawOrder);
          call.isTree = renderable.info.isTree;
          call.sElem = &sElem;
          call.dvState = dvState;
          call.vbIndex = rElem.vertexData->getVbIdx();
          call.vbStride = rElem.vertexData->getStride();
        }
      }
    }

    auto addedCalls = make_span(protoDrawCallsFmem).subspan(protoStartIndex);
    stlsort::sort(addedCalls.begin(), addedCalls.end(), [](const ProtoDrawCall &a, const ProtoDrawCall &b) { return a < b; });

    const ProtoDrawCall *last = nullptr;
    MultiDrawCall *lastMulti = nullptr;
    uint32_t currentByteOffset = protoStartIndex * constants.argsStride;
    for (const auto &call : addedCalls)
    {
      if (last && !lastMulti->isForcedSingle && can_coalesce(*last, call))
        lastMulti->count++;
      else
      {
        auto &multiCall = cache.multiCalls.push_back();
        multiCall.byteOffset = currentByteOffset;
        multiCall.count = 1;

        // Non-packed materials cannot use multidraw.
        multiCall.isForcedSingle = !is_packed_material(call.dvState.const_state);
        multiCall.staticInstanceRegion = call.staticInstanceRegion;

        multiCall.stage = call.stage;
        multiCall.isTree = call.isTree;
        multiCall.sElem = call.sElem;
        multiCall.dvState = call.dvState;
        multiCall.vbIndex = call.vbIndex;
        multiCall.vbStride = call.vbStride;

        lastMulti = &multiCall;
      }

      currentByteOffset += constants.argsStride;
      last = &call;
    }

    multiEndIndices[eastl::to_underlying(subPass)] = cache.multiCalls.size();
  }

  const SubPass firstSubPass = VIEW_KIND_SUBPASSES[eastl::to_underlying(constants.viewInfo.kind)].first;
  for (auto subPass : VIEW_KIND_SUBPASSES[eastl::to_underlying(constants.viewInfo.kind)])
  {
    const int s = eastl::to_underlying(subPass);
    const uint32_t startIndex = subPass == firstSubPass ? 0 : multiEndIndices[s - 1];
    const uint32_t count = multiEndIndices[s] - startIndex;
    cache.multiCallSpans[s] = make_span(cache.multiCalls).subspan(startIndex, count);
  }

  cache.numCounterPatches = constants.viewInfo.maxViewports * protoDrawCallsFmem.size();
  cache.numCallsPerViewport = protoDrawCallsFmem.size();

  if (constants.viewInfo.kind == ViewKind::MAIN_CAMERA)
  {
    // If this node is not the main camera node, don't do updates for textures and LODs.
    for (auto &call : cache.multiCalls)
    {
      cache.usedSElems.insert(call.sElem);
      if (is_packed_material(call.dvState.const_state))
        cache.packedConstStates.insert(call.dvState.const_state);
    }
  }

  persistent_data.areBuffersValid = update_buffers(persistent_data, protoDrawCallsFmem);
  persistent_data.isCacheValid = true;
}

static void render(SubPass sub_pass,
  uint32_t viewport_index,
  const RiexPersistentData &persistent_data,
  Sbuffer *instance_data_buf,
  Sbuffer *draw_args_buf)
{
  G_ASSERT(viewport_index < persistent_data.constants.viewInfo.maxViewports);
  if (!persistent_data.areBuffersValid)
    return;

  const RiexConstants &constants = persistent_data.constants;

  // Needed as part RiShaderConstBuffers' contract.
  rendinst::render::startRenderInstancing();

  ExternalShaderVarsScope externalShaderVarsScope(SUBPASS_RI_RENDER_PASS[eastl::to_underlying(sub_pass)]);

  // Index buffer is guaranteed to be same for all RI, so we can just bind it once.
  Ibuffer *ib = unitedvdata::riUnitedVdata.getIB();
  d3d_err(d3d::setind(ib));

  // This is a special wrapper for RI-specific data that we need to init and bind.
  // See this in rendinst_inc.dshl:
  //
  // bool useCbufferParams = (cb_inst_offset.z & 1) == 0;
  // ...
  // #define GET_PER_DRAW_OFFSET asint(per_instance_data[instNo * 4 + INST_OFFSET_GETTER + 3].y);
  // ...
  // perDrawOffset = GET_PER_DRAW_OFFSET; \
  // useCbufferParams = perDrawOffset == 0; \
  //
  // (If cb_inst_offset.z is set to 1, then useCbufferParams will originally be 0)
  // When the last "unused" per-instance TM row has y == 0, the CB data will be used.
  //
  // It works without this cb as well but it might be better to leave it here as a fallback.
  // (in case perDrawOffset is 0)
  rendinst::render::RiShaderConstBuffers cb;
  cb.setInstancing(0, 4, 0x1, 0);
  cb.setOpacity(0, 1);
  const E3DCOLOR defaultColors[] = {E3DCOLOR(0x80808080), E3DCOLOR(0x80808080)};
  cb.setRandomColors(defaultColors);
  cb.setCrossDissolveRange(0);
  cb.setBoundingSphere(0, 0, 1, 1, 0);
  cb.setBoundingBox(v_make_vec4f(1, 1, 1, 0));
  cb.flushPerDraw();

  // One per-instance buffer for all drawcalls, bind it once.
  d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, instance_data_buf);

  // perDrawInstanceData is currently bound and managed via the framegraph,
  // as using ShaderGlobal::set_buffer() in the following way (or similarly, via ExternalShaderVarsScope)
  // causes the RIs to blink in/out of existence:
  //
  // static ShaderVariableInfo perDrawDataVarId("perDrawInstanceData");
  // ShaderGlobal::set_buffer(perDrawDataVarId, get_per_draw_gathered_data());

  // Hack: BFG does not allow to specify two read-only usages.
  d3d::resource_barrier({draw_args_buf, RB_RO_SRV | RB_RO_INDIRECT_BUFFER | RB_STAGE_VERTEX});

  if (persistent_data.riAdditionalInstanceOffsetRegNo != -1)
    d3d::set_buffer(STAGE_VS, persistent_data.riAdditionalInstanceOffsetRegNo, draw_args_buf);

  Vbuffer *lastVb = nullptr;
  int lastVbStride = 0;
  shaders::CombinedDynVariantState lastDvState{};
  ScriptedShaderElement *lastSElem = nullptr;
  bool lastShaderOverride = false;
  uint32_t lastVsImmediate = ~0u;
  bool lastIsTree = false;

  // Note: naming is a bit confusing, but these constants are only used for vegetation ("trees").
  // See prog/daNetGame/shaders/vegetation.dshl
  const auto treeConsts = rendinst::render::getCommonImmediateConstants();
  const uint32_t treeImmediateConsts[3] = {0u, treeConsts[0], treeConsts[1]};

  const auto &multiCallSpan = persistent_data.cache.multiCallSpans[eastl::to_underlying(sub_pass)];

  for (const auto &call : multiCallSpan)
  {
    constexpr auto prePassStages = SUBPASS_STAGES[eastl::to_underlying(SubPass::PRE_PASS_ALL)];
    bool shaderOverride =
      // Avoid `operator&&` because of extra branching.
      (sub_pass != SubPass::PRE_PASS_ALL && sub_pass != SubPass::DYN_SHADOW_PASS_ALL) & (call.stage >= prePassStages.first) &
      (call.stage <= prePassStages.last);

    if (shaderOverride != lastShaderOverride)
    {
      if (shaderOverride)
        shaders::overrides::set(persistent_data.afterPrePassOverride);
      else
        shaders::overrides::reset();

      lastShaderOverride = shaderOverride;
    }

    Vbuffer *vb = unitedvdata::riUnitedVdata.getVB(call.vbIndex);
    G_ASSERT(vb);

    if ((vb != lastVb) | (call.vbStride != lastVbStride)) // Avoid `operator||` because of extra branching.
    {
      // Set vertex buffer & stride.
      d3d_err(d3d::setvsrc(0, vb, call.vbStride));

      lastVb = vb;
      lastVbStride = call.vbStride;
    }

    // clang-format off
    if ((call.sElem != lastSElem)
      // Avoid `operator||` because of extra branching.
      | (call.dvState.variant != lastDvState.variant)
      | (call.dvState.program != lastDvState.program)
      | (call.dvState.state_index != lastDvState.state_index))
    // clang-format on
    {
      set_states_for_variant(*call.sElem, call.dvState.variant, call.dvState.program, call.dvState.state_index);

      lastSElem = call.sElem;
      lastDvState = call.dvState;
    }

    // When draw call ID is present, but not used, we have to skip it in the extended indirect args.
    uint32_t additionalByteOffset = call.isForcedSingle && constants.isExtendedArgs ? sizeof(uint32_t) : 0;

    for (uint32_t counterKind = 0; counterKind < COUNTER_KINDS_NUM; ++counterKind)
    {
      if (counterKind == COUNTER_KIND_STATIC && constants.maxStaticInstancesPerViewport == 0)
        continue;

      if (counterKind == COUNTER_KIND_DYNAMIC && !constants.haveDynamicRegions)
        continue;

      const uint32_t kindByteOffset =
        persistent_data.cache.numCallsPerViewport * constants.argsStride * constants.viewInfo.maxViewports * counterKind;
      const uint32_t viewportByteOffset = persistent_data.cache.numCallsPerViewport * constants.argsStride * viewport_index;
      const uint32_t totalByteOffset = call.byteOffset + kindByteOffset + viewportByteOffset + additionalByteOffset;

      uint32_t vsImmediate = 0u;
      if (call.isForcedSingle && constants.isNonMultidrawIndirectionNeeded)
      {
        const uint32_t offset = (totalByteOffset + offsetof(DrawIndexedIndirectArgs, startInstanceLocation));
        G_ASSERT(offset <= INST_OFFSET_MASK_VALUE);
        vsImmediate |= INST_OFFSET_FLAG_USE_INDIRECTION | offset;
      }

      if (vsImmediate != lastVsImmediate)
      {
        d3d::set_immediate_const(STAGE_VS, &vsImmediate, 1);
        lastVsImmediate = vsImmediate;
      }

      if (call.isTree != lastIsTree)
      {
        if (call.isTree)
          d3d::set_immediate_const(STAGE_PS, treeImmediateConsts, countof(treeImmediateConsts));
        else
          d3d::set_immediate_const(STAGE_PS, nullptr, 0);
      }

      d3d::multi_draw_indexed_indirect(PRIM_TRILIST, draw_args_buf, call.count, constants.argsStride, totalByteOffset);
    }
  }

  // Needed as part RiShaderConstBuffers' contract.
  rendinst::render::endRenderInstancing();

  d3d::set_immediate_const(STAGE_VS, nullptr, 0);
  d3d::set_immediate_const(STAGE_PS, nullptr, 0);
  shaders::overrides::reset();
}

static auto get_draw_args_handle(dabfg::Registry registry, const RiexConstants &constants)
{
  return (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str() / "riex")
    .read("draw_args")
    .buffer()
    .atStage(dabfg::Stage::PRE_RASTER)
    .useAs(dabfg::Usage::INDIRECTION_BUFFER)
    .handle();
}

static auto get_instance_data_handle(dabfg::Registry registry, const RiexConstants &constants)
{
  return (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str())
    .read("instance_data")
    .buffer()
    .atStage(dabfg::Stage::VS)
    .useAs(dabfg::Usage::SHADER_RESOURCE)
    .handle();
}

static dabfg::NodeHandle create_main_camera_subpass_node(SubPass sub_pass,
  const eastl::shared_ptr<RiexPersistentData> &persistent_data)
{
  // Use a special namespace in order for nodes to draw into GBuffer at the correct time.
  const dabfg::NameSpace ns = dabfg::root() / "opaque" / (sub_pass == SubPass::NORMAL_PASS_DECALS ? "decorations" : "statics");
  const char *nodeName = MAIN_CAMERA_SUBPASS_NODE_NAMES[eastl::to_underlying(sub_pass)];
  G_ASSERT(nodeName);

  return ns.registerNode(nodeName, DABFG_PP_NODE_SRC, [sub_pass, persistent_data](dabfg::Registry registry) {
    const RiexConstants &constants = persistent_data->constants;

    eastl::optional<dabfg::VirtualPassRequest> pass;
    if (sub_pass == SubPass::PRE_PASS_ALL)
      pass = render_to_gbuffer_prepass(registry);
    else if (sub_pass == SubPass::NORMAL_PASS_DECALS)
      pass = render_to_gbuffer_but_sample_depth(registry);
    else
      pass = render_to_gbuffer(registry);

    auto drawArgsHandle = get_draw_args_handle(registry, constants);
    auto instanceDataHandle = get_instance_data_handle(registry, constants);

    (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str())
      .read("per_draw_gathered_data")
      .buffer()
      .atStage(dabfg::Stage::VS)
      .bindToShaderVar("perDrawInstanceData");

    // Note: set up like opaqueStaticNodes.cpp
    auto state = registry.requestState()
                   .setFrameBlock("global_frame")
                   .setSceneBlock(sub_pass == SubPass::PRE_PASS_ALL ? "rendinst_depth_scene" : "rendinst_scene")
                   .allowWireframe();

    use_default_vrs(*pass, state);

    return [sub_pass, persistent_data, instanceDataHandle, drawArgsHandle] {
      const uint32_t viewportIndex = 0; // Main camera only has one viewport.
      render(sub_pass, viewportIndex, *persistent_data, instanceDataHandle.view().getBuf(), drawArgsHandle.view().getBuf());
    };
  });
}

void riex_finalize_view(const ViewInfo &view_info, const ViewBuilder &view_builder, RiexManager &manager, NodeInserter node_inserter)
{
  auto &builder = manager.currentBuilder;

  if (builder.renderablesInfo.empty())
    return;

  const dabfg::NameSpace ns = dabfg::root() / "dagdp" / view_info.uniqueName.c_str() / "riex";
  auto persistentData = eastl::make_shared<RiexPersistentData>();
  RiexConstants &constants = persistentData->constants;

  shaders::OverrideState prePassOverrideState;
  prePassOverrideState.set(shaders::OverrideState::Z_FUNC | shaders::OverrideState::Z_WRITE_DISABLE);
  prePassOverrideState.zFunc = CMPF_EQUAL;
  persistentData->afterPrePassOverride.reset(shaders::overrides::create(prePassOverrideState));
  ShaderGlobal::get_int_by_name("ri_additional_instance_offsets_data_no", persistentData->riAdditionalInstanceOffsetRegNo);

  persistentData->rElemsUpdatedToken = unitedvdata::riUnitedVdata.on_mesh_relems_updated.subscribe(
    [self = persistentData.get()](const RenderableInstanceLodsResource *ri, bool) {
      if (self->isCacheValid && self->usedRI.find(ri) != self->usedRI.end())
        self->isCacheValid = false;
    });

  eastl::array<uint32_t, SUBPASS_COUNT> maxDrawCallsPerViewport = {};

  for (const auto &[_, rInfo] : builder.renderablesInfo)
  {
    const auto &lod = rInfo.lodsRes->lods[rInfo.lodIndex];
    const ShaderMesh *shaderMesh = lod.scene->getMesh()->getMesh()->getMesh();

    for (auto subPass : VIEW_KIND_SUBPASSES[eastl::to_underlying(constants.viewInfo.kind)])
    {
      const int s = eastl::to_underlying(subPass);
      const auto stages = SUBPASS_STAGES[s];
      maxDrawCallsPerViewport[s] += shaderMesh->getElems(stages.first, stages.last).size();
    }
  }

  constants.maxDrawCallsPerViewport = 0;
  for (auto value : maxDrawCallsPerViewport)
    constants.maxDrawCallsPerViewport += value;

  constants.maxStaticInstancesPerViewport = view_builder.maxStaticInstancesPerViewport;
  constants.haveDynamicRegions = view_builder.dynamicInstanceRegion.maxCount > 0;
  constants.viewInfo = view_info;
  constants.isExtendedArgs = uses_extended_multi_draw_struct();

  // See rendinst_inc.dshl, inst_offset_getter()
  constants.isNonMultidrawIndirectionNeeded = d3d::get_driver_code().is(d3d::dx11 || d3d::dx12);

  constants.argsStride = constants.isExtendedArgs ? sizeof(ExtendedDrawIndexedIndirectArgs) : sizeof(DrawIndexedIndirectArgs);
  constants.argsDwords = constants.argsStride / sizeof(uint32_t);
  G_ASSERT(constants.argsStride % sizeof(uint32_t) == 0);

  constants.instanceBufferElementStride = get_tex_format_desc(perInstanceFormat).bytesPerElement;
  constants.instanceBufferElementsCount = sizeof(TMatrix4) / constants.instanceBufferElementStride;
  G_ASSERT(sizeof(TMatrix4) % constants.instanceBufferElementStride == 0);
  constants.numRenderables = view_builder.numRenderables;

  for (auto subPass : VIEW_KIND_SUBPASSES[eastl::to_underlying(constants.viewInfo.kind)])
  {
    const int s = eastl::to_underlying(subPass);
    constants.haveSubPass[s] = maxDrawCallsPerViewport[s] > 0;
  }

  if (view_builder.totalMaxInstances > MAX_MATRIX_OFFSET)
  {
    // MAX_MATRIX_OFFSET only applies to packed materials, but we should enforce the same limit everywhere.
    logerr("daGdp: maximum number of RendInst Extra instances is too large: %" PRIu32, view_builder.totalMaxInstances);
    return;
  }

  persistentData->resources.reserve(builder.resources.size());
  persistentData->usedRI.reserve(builder.resources.size());
  for (auto &resource : builder.resources)
  {
    persistentData->usedRI.insert(reinterpret_cast<const RenderableInstanceLodsResource *>(resource.gameRes.get()));
    persistentData->resources.emplace_back(resource);
  }

  persistentData->renderablesInfo.reserve(builder.renderablesInfo.size());
  for (size_t i = 0; i < builder.renderablesInfo.size(); ++i)
  {
    const auto &[rId, info] = builder.renderablesInfo.at(i);
    auto &dst = persistentData->renderablesInfo.push_back();
    dst.rId = rId;
    dst.info = info;
    dst.instanceRegion = view_builder.renderablesInstanceRegions[rId];
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "dagdp_riex_%s_counter_patches", constants.viewInfo.uniqueName.c_str());
    persistentData->counterPatchesBuffer = dag::buffers::create_persistent_sr_structured(sizeof(RiexPatch),
      constants.maxDrawCallsPerViewport * constants.viewInfo.maxViewports, bufferName.c_str());
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "dagdp_riex_%s_draw_args", constants.viewInfo.uniqueName.c_str());
    // For simplicity, draw call args count and indexing are always the same, regardless of whether we have dynamic
    // or static instances. This buffer is small anyway.
    persistentData->drawArgsBuffer = dag::create_sbuffer(sizeof(uint32_t),
      constants.argsDwords * constants.maxDrawCallsPerViewport * constants.viewInfo.maxViewports * COUNTER_KINDS_NUM,
      SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_MISC_DRAWINDIRECT | SBCF_BIND_SHADER_RES, 0, bufferName.c_str());
  }

  dabfg::NodeHandle updateNode = ns.registerNode("update", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
    view_multiplex(registry, persistentData->constants.viewInfo.kind);
    registry.registerBuffer("draw_args", [persistentData](auto) { return ManagedBufView(persistentData->drawArgsBuffer); })
      .atStage(dabfg::Stage::TRANSFER);
    registry.registerBuffer("patches", [persistentData](auto) { return ManagedBufView(persistentData->counterPatchesBuffer); })
      .atStage(dabfg::Stage::TRANSFER);

    return [persistentData] {
      // Rebuilding the cache will update the draw_args, patch buffers.
      if (!persistentData->isCacheValid)
        rebuild_cache(*persistentData);

      // TODO: the code below looks like it could be implemented in an ECS system instead.

      // If this node is not the main camera node, don't do updates for textures and LODs.
      if (persistentData->constants.viewInfo.kind != ViewKind::MAIN_CAMERA)
        return;

      // Update the LOD and texture levels by forcing them to maximum quality.
      // Ideally, we would calculate the levels that are actually needed, but that would introduce a lot of complications.

      for (const auto &renderable : persistentData->renderablesInfo)
      {
        // Need to call this on every frame to make all LODs stay loaded. Source: Nikolay Savichev
        renderable.info.lodsRes->updateReqLod(0);
      }

      for (const auto sElem : persistentData->cache.usedSElems)
      {
        // This mirrors riExtraRendererT.h
        sElem->setReqTexLevel(TexStreamingContext::maxLevel);
      }

      for (const auto cState : persistentData->cache.packedConstStates)
      {
        // Technically this modifies global rendering state, but there is no direct dependency on this state from
        // the drawing node. We just need to make sure, that this is called before the draw calls are made.
        update_bindless_state(cState, TexStreamingContext::maxLevel);
      }
    };
  });

  dabfg::NodeHandle patchStaticNode = ns.registerNode("patch_static", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
    view_multiplex(registry, persistentData->constants.viewInfo.kind);
    (registry.root() / "dagdp" / persistentData->constants.viewInfo.uniqueName.c_str())
      .read("counters")
      .buffer()
      .atStage(dabfg::Stage::COMPUTE)
      .bindToShaderVar("dagdp__counters");
    registry.read("patches").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp_riex__patches");
    registry.modify("draw_args").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp_riex__draw_args");

    return [persistentData, shader = ComputeShader("dagdp_riex_patch_static")] {
      if (!persistentData->areBuffersValid)
        return;

      ShaderGlobal::set_int(var::num_patches, persistentData->cache.numCounterPatches);
      bool res = shader.dispatchThreads(persistentData->cache.numCounterPatches, 1, 1);
      G_ASSERT(res);
      G_UNUSED(res);
    };
  });

  dabfg::NodeHandle patchDynamicNode = ns.registerNode("patch_dynamic", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
    view_multiplex(registry, persistentData->constants.viewInfo.kind);

    const auto ns = registry.root() / "dagdp" / persistentData->constants.viewInfo.uniqueName.c_str();

    ns.read("dyn_allocs_stage1").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp__dyn_allocs");

    ns.read("dyn_counters_stage1").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp__dyn_counters");

    registry.read("patches").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp_riex__patches");
    registry.modify("draw_args").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp_riex__draw_args");

    return [persistentData, shader = ComputeShader("dagdp_riex_patch_dynamic")] {
      if (!persistentData->areBuffersValid)
        return;

      ShaderGlobal::set_int(var::num_patches, persistentData->cache.numCounterPatches);
      bool res = shader.dispatchThreads(persistentData->cache.numCounterPatches, 1, 1);
      G_ASSERT(res);
      G_UNUSED(res);
    };
  });

  node_inserter(eastl::move(updateNode));

  if (constants.maxStaticInstancesPerViewport > 0)
    node_inserter(eastl::move(patchStaticNode));

  if (constants.haveDynamicRegions)
    node_inserter(eastl::move(patchDynamicNode));

  switch (view_info.kind)
  {
    case ViewKind::MAIN_CAMERA:
    {
      // TODO: on mobiles, we don't want a pre-pass. But right now, simply disabling it will break alpha testing.
      for (SubPass subPass : VIEW_KIND_SUBPASSES[eastl::to_underlying(ViewKind::MAIN_CAMERA)])
        if (constants.haveSubPass[eastl::to_underlying(subPass)])
          node_inserter(create_main_camera_subpass_node(subPass, persistentData));
      break;
    }
    case ViewKind::DYN_SHADOWS:
    {
      manager.currentDynShadowViews.push_back(persistentData);
      break;
    }
    default: G_ASSERTF(false, "Unknown view kind");
  }
}

struct ExtensionData
{
  eastl::optional<dabfg::VirtualResourceHandle<const dynamic_shadow_render::FrameVector<int>, false, false>> mappingHandle;
  dag::Vector<dabfg::VirtualResourceHandle<const Sbuffer, true, false>> drawArgsHandles;
  dag::Vector<dabfg::VirtualResourceHandle<const Sbuffer, true, false>> instanceDataHandles;
  dag::Vector<eastl::shared_ptr<RiexPersistentData>> views;
};

void riex_finalize(RiexManager &manager)
{
  if (manager.currentDynShadowViews.size() > 0)
  {
    auto extensionData = eastl::make_shared<ExtensionData>();
    extensionData->views = eastl::move(manager.currentDynShadowViews);

    const auto declare = [extensionData](dabfg::Registry registry) {
      extensionData->drawArgsHandles.clear();
      extensionData->instanceDataHandles.clear();

      extensionData->mappingHandle =
        (registry.root() / "dagdp").readBlob<dynamic_shadow_render::FrameVector<int>>("scene_shadow_updates_mapping").handle();

      for (const auto &viewData : extensionData->views)
      {
        const RiexConstants &constants = viewData->constants;
        extensionData->drawArgsHandles.push_back(get_draw_args_handle(registry, constants));
        extensionData->instanceDataHandles.push_back(get_instance_data_handle(registry, constants));
      }
    };

    const auto execute = [extensionData](int updateIndex, int viewportIndex) {
      auto &mapping = extensionData->mappingHandle->ref();
      int viewIndex = mapping[updateIndex];
      if (viewIndex == -1)
        return;

      static const int rendinstDepthSceneBlockId = ShaderGlobal::getBlockId("rendinst_depth_scene");
      TIME_D3D_PROFILE(dagdpRiexDynShadow);
      SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);

      render(SubPass::DYN_SHADOW_PASS_ALL, viewportIndex, *extensionData->views[viewIndex],
        extensionData->instanceDataHandles[viewIndex].view().getBuf(), extensionData->drawArgsHandles[viewIndex].view().getBuf());
    };

    DynamicShadowRenderExtender::Extension extension{declare, execute};
    auto &wr = *static_cast<IRenderWorld *>(get_world_renderer());
    manager.shadowExtensionHandle = wr.registerShadowRenderExtension(eastl::move(extension));
  }
}

} // namespace dagdp