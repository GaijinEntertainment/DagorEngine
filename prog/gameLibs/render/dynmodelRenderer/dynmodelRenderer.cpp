// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/dynmodelRenderer.h>

#include <3d/dag_resPtr.h>
#include <3d/dag_ringDynBuf.h>
#include <3d/dag_sbufferIDHolder.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_info.h>
#include <EASTL/array.h>
#include <EASTL/set.h>
#include <EASTL/string.h>
#include <EASTL/tuple.h>
#include <EASTL/vector_set.h>
#include <EASTL/fixed_vector.h>
#include <generic/dag_tab.h>
#include <image/dag_texPixel.h>
#include <math/dag_geomTree.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_statDrv.h>
#include <render/whiteTex.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_overrideStates.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_convar.h>
#include <memory/dag_framemem.h>
#include <generic/dag_smallTab.h>
#include <ioSys/dag_dataBlock.h>
#include <gameRes/dag_resourceNameResolve.h>

namespace dynrend
{
struct InstanceData
{
  const DynamicRenderableSceneInstance *instance;
  const DynamicRenderableSceneResource *sceneRes; // LOD and nodes visibility may be set only for the duration of the add call.
  const InitialNodes *initialNodes;
  int indexToCustomProjtm;
  int indexToPerInstanceRenderData;
  int indexToAllNodesVisibility;
  int baseOffsetRenderData;
  int instanceLod;

  InstanceData() {} //-V730
};


struct ContextData;
struct DipChunk
{
  int baseOffsetRenderData; // Can be negative.
  int nodeChunkSizeInVecs;
  int si, numf, baseVertex;

  Intervals *intervals;
  const char *shaderName;
  ShaderElement *shader;
  GlobalVertexData *vertexData;
  shaders::OverrideStateId overrideStateId;
  bool mergeOverrideState;
  D3DRESID constDataBuf; // For now one per instance.

  int numPasses;
  int forcedOrder;
  int indexToPerInstanceRenderData;
  int instanceNo;
  int numBones;

  int texLevel;

  DipChunk() {} //-V730

  Intervals *getIntervals(ContextData &ctx) const;

  bool tryMerge(const DipChunk &next)
  {
    if (numBones == 0 && next.numBones == 0 && instanceNo == next.instanceNo && shader == next.shader &&
        vertexData == next.vertexData && baseOffsetRenderData == next.baseOffsetRenderData &&
        nodeChunkSizeInVecs == next.nodeChunkSizeInVecs && baseVertex == next.baseVertex && numPasses == next.numPasses)
    {
      if (si == next.si + 3 * next.numf)
      {
        si = next.si;
        numf += next.numf;
        return true;
      }
      else if (si + 3 * numf == next.si)
      {
        numf += next.numf;
        return true;
      }
    }

    return false;
  }
};


struct InstanceChunk
{
  Point4 posMul;
  Point4 posOfs__paramsCount;
};


struct NodeChunk
{
  mat44f nodeGlobTm;
  mat44f prevNodeGlobTm;
  float nodeOpacity;
  float instanceChunkOffset; // Relative to this chunk.
  NodeExtraData extraData;

  vec4f initialNodeTm0; // Optional.
  vec4f initialNodeTm1;
  vec4f initialNodeTm2;

  // Bones follow.
};
static_assert(sizeof(NodeExtraData) == 2 * sizeof(float), "NodeExtraData must be exactly the size of 2 floats.");

const int bigNodeChunkVecs = sizeof(NodeChunk) / sizeof(vec4f);
const int smallNodeChunkVecs = bigNodeChunkVecs - 3;

static ShaderElement *replacement_shader = nullptr;
static Tab<const char *> filtered_material_names;

#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_APPLE
static const int DEFAULT_INITIAL_RING_BUFFER_SIZE = 64 << 10;
#else
static const int DEFAULT_INITIAL_RING_BUFFER_SIZE = 1 << 10;
#endif
#if _TARGET_ANDROID
static const int DEFAULT_INITIAL_MAIN_RING_BUFFER_SIZE = 64 << 10;
#else
static const int DEFAULT_INITIAL_MAIN_RING_BUFFER_SIZE = 100 << 10;
#endif
static int initialRingBufferSize = 0;
static int initialMainRingBufferSize = 0;
static int instanceDataBufferVarId = -1;

ShaderVariableInfo instance_const_data_bufferVarId("instance_const_data_buffer", true);

struct ContextGpuData
{
  TMatrix4 projToWorldTm;
  TMatrix4 projToViewTm;
};


struct ContextData
{
  eastl::string name;

  Tab<InstanceData> instances;
  Tab<bool> allNodesVisibility; // Faster than eastl::bitvector
  eastl::vector<PerInstanceRenderData> perInstanceRenderData;
  Tab<TMatrix4> customProjTms;
  float minElemRadius = 0.f;
  bool renderSkinned = true;
  bool ringBufferVarSet = false;
  int statNodes = 0;
  int statBones = 0;
  int statPreMerged = 0;
  int statPostMerged = 0;
  ContextGpuData gpuData;

  eastl::array<Tab<DipChunk>, ShaderMesh::STG_COUNT> dipChunksByStage;
  eastl::array<Tab<int>, ShaderMesh::STG_COUNT> dipChunksOrderByStage;

  Tab<char> renderDataBuffer;
  RingDynamicSB *ringBuffer = NULL;
  uint32_t ringBufferSizeInVecs = 0;
  uint32_t prevDiscardOnFrame = 0;
  int ringBufferPos = 0;

  void clear()
  {
    perInstanceRenderData.resize(1);
    instances.resize(0);
    allNodesVisibility.resize(0);
    customProjTms.resize(0);
    for (auto &stage : dipChunksByStage)
      stage.resize(0);
    for (auto &stage : dipChunksOrderByStage)
      stage.resize(0);

    minElemRadius = 0.f;
    renderSkinned = true;
    statNodes = statBones = statPreMerged = statPostMerged = 0;
  }

  void recreateRingBuffer(int new_size_in_vecs, int index)
  {
    if (ringBuffer)
    {
      if (ringBufferVarSet) // do not reset if it was not set in the first place, prevent thread race
      {
        ShaderGlobal::set_buffer(instanceDataBufferVarId, BAD_D3DRESID); // avoid reset_from_vars, since it can lead to thread race
        ringBufferVarSet = false;
      }
    }
    del_it(ringBuffer);

    ringBufferSizeInVecs = new_size_in_vecs;
    if (ringBufferSizeInVecs)
    {
      ringBuffer = new RingDynamicSB();
      String bufName(0, "per_chunk_render_data%d_%s", index, name.c_str());
      ringBuffer->init(ringBufferSizeInVecs, sizeof(vec4f), sizeof(vec4f), SBCF_BIND_SHADER_RES, TEXFMT_A32B32G32R32F, bufName.str());
    }
  }
};

Intervals *DipChunk::getIntervals(ContextData &ctx) const
{
  PerInstanceRenderData &perInstanceRenderData = ctx.perInstanceRenderData[this->indexToPerInstanceRenderData];
  return perInstanceRenderData.intervals.empty() ? NULL : &perInstanceRenderData.intervals;
}


static eastl::vector<ContextData> contexts;
static Tab<const char *> shadersRenderOrder;
static TMatrix4_vec4 prevViewTm = TMatrix4_vec4::IDENT;
static TMatrix4_vec4 prevProjTm = TMatrix4_vec4::IDENT;
static Point3 localOffsetHint;
static bool separateAtestPass = false;
UniqueBufHolder contextGpuDataBuffer;
Statistics statistics = {0};

static eastl::array<eastl::vector<eastl::fixed_vector<int, 2, false>>, 16> shaderMeshStagesByRenderFlags = {{
  // Without separateAtestPass:
  {{}}, {{ShaderMesh::STG_opaque, ShaderMesh::STG_atest}},                                                   //  OP
  {{ShaderMesh::STG_trans}},                                                                                 //  TR
  {{ShaderMesh::STG_opaque, ShaderMesh::STG_atest}, {ShaderMesh::STG_trans}},                                //  OP|TR
  {{ShaderMesh::STG_distortion}},                                                                            //  DI
  {{ShaderMesh::STG_opaque, ShaderMesh::STG_atest}, {ShaderMesh::STG_distortion}},                           //  OP|DI
  {{ShaderMesh::STG_trans}, {ShaderMesh::STG_distortion}},                                                   //  TR|DI
  {{ShaderMesh::STG_opaque, ShaderMesh::STG_atest}, {ShaderMesh::STG_trans}, {ShaderMesh::STG_distortion}},  //  OP|TR|DI
                                                                                                             // With separateAtestPass:
  {{}}, {{ShaderMesh::STG_opaque}, {ShaderMesh::STG_atest}},                                                 //  OP
  {{ShaderMesh::STG_trans}},                                                                                 //  TR
  {{ShaderMesh::STG_opaque}, {ShaderMesh::STG_atest}, {ShaderMesh::STG_trans}},                              //  OP|TR
  {{ShaderMesh::STG_distortion}},                                                                            //  DI
  {{ShaderMesh::STG_opaque}, {ShaderMesh::STG_atest}, {ShaderMesh::STG_distortion}},                         //  OP|DI
  {{ShaderMesh::STG_trans}, {ShaderMesh::STG_distortion}},                                                   //  TR|DI
  {{ShaderMesh::STG_opaque}, {ShaderMesh::STG_atest}, {ShaderMesh::STG_trans}, {ShaderMesh::STG_distortion}} //  OP|TR|DI
}};

CONSOLE_BOOL_VAL("debug", dynrendLog, false);


ContextId create_context(const char *name)
{
  G_ASSERT(contexts.size() >= (int)ContextId::FIRST_USER_CONTEXT);
  ContextData &ctx = contexts.emplace_back();
  ctx.name = name;
  ctx.recreateRingBuffer(initialRingBufferSize, &ctx - contexts.begin());
  return (ContextId)(contexts.size() - 1);
}


void delete_context(ContextId context_id)
{
  G_ASSERT(context_id >= ContextId::FIRST_USER_CONTEXT && (int)context_id < contexts.size());
  ContextData &ctx = contexts[(int)context_id];
  ctx.recreateRingBuffer(0, &ctx - contexts.begin());
  while (!contexts.back().ringBuffer)
    contexts.pop_back();
}


void init()
{
  if (is_initialized())
    return;

  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
  initialRingBufferSize = graphicsBlk->getInt("dynrendInitialRingBufferSize", DEFAULT_INITIAL_RING_BUFFER_SIZE);
  initialMainRingBufferSize = graphicsBlk->getInt("dynrendInitialMainRingBufferSize", DEFAULT_INITIAL_MAIN_RING_BUFFER_SIZE);

  ContextData &ctx0 = contexts.emplace_back();
  ctx0.name = "MAIN";
  ctx0.recreateRingBuffer(initialMainRingBufferSize, 0);

  ContextData &ctx1 = contexts.emplace_back();
  ctx1.name = "IMMEDIATE";
  ctx1.recreateRingBuffer(initialRingBufferSize, 1);

  contextGpuDataBuffer = dag::buffers::create_one_frame_cb(dag::buffers::cb_struct_reg_count<ContextGpuData>(), "context_gpu_data");
  d3d_err(contextGpuDataBuffer.getBuf());

  instanceDataBufferVarId = ::get_shader_variable_id("instance_data_buffer", true);
}


void close()
{
  for (ContextData &ctx : contexts)
    ctx.recreateRingBuffer(0, &ctx - contexts.begin());

  contexts.clear();

  contextGpuDataBuffer.close();
}


bool is_initialized() { return !contexts.empty(); }


void set_shaders_forced_render_order(const eastl::vector<eastl::string> &shader_names)
{
  clear_and_shrink(shadersRenderOrder);
  for (const eastl::string &name : shader_names)
  {
    ShaderMaterial *shmat = new_shader_material_by_name(name.c_str());
    if (shmat)
    {
      shadersRenderOrder.push_back(shmat->getShaderClassName()); // Static pointer to the name in shaders bindump (until reloaded)
      delete shmat;
    }
  }
}


void set_reduced_render(ContextId context_id, float min_elem_radius, bool render_skinned)
{
  ContextData &ctx = contexts[(int)context_id];
  ctx.minElemRadius = min_elem_radius;
  ctx.renderSkinned = render_skinned;
}

static bool check_shader_names = false;
void set_check_shader_names(bool check) { check_shader_names = check; }

void add(ContextId context_id, const DynamicRenderableSceneInstance *instance, const InitialNodes *optional_initial_nodes,
  const dynrend::PerInstanceRenderData *optional_render_data, dag::Span<int> *node_list, const TMatrix4 *customProj)
{
  G_ASSERT(instance);
  G_ASSERTF_RETURN((int)context_id >= 0 && (int)context_id < contexts.size(), , "Uninitialized dynrend context was used");
  G_ASSERT(context_id != ContextId::IMMEDIATE || is_main_thread());

  if (optional_initial_nodes && optional_initial_nodes->nodesModelTm.size() == 0) // Uninitialized.
    optional_initial_nodes = NULL;

  if (optional_initial_nodes && optional_initial_nodes->nodesModelTm.size() != instance->getNodeCount())
  {
    static bool logged = false;
    if (!logged)
      logerr("Invalid optional_initial_nodes (%d != %d)", optional_initial_nodes->nodesModelTm.size(), instance->getNodeCount());
    logged = true;
    return;
  }

  const DynamicRenderableSceneResource *sceneRes = instance->getCurSceneResource(); // int lodNo = instance->getCurrentLodNo();
  if (!sceneRes)
    return; // LODed out.

  TIME_PROFILE(dynrend_add);

  ContextData &ctx = contexts[(int)context_id];

#if DAGOR_DBGLEVEL > 0
  if (check_shader_names)
  {
    auto checkName = [&](dag::ConstSpan<ShaderMesh::RElem> elems) {
      for (auto &elem : elems)
        if (auto name = elem.mat->getShaderClassName())
          if (strstr(name, "rendinst_") == name)
          {
            String name;
            if (resolve_game_resource_name(name, instance->getLodsResource()))
              logerr("Rendinst shader used in dynrend: %s", name.data());
            else
              logerr("Rendinst shader used in dynrend: unknown");
          }
    };

    sceneRes->getMeshes(
      [&](const ShaderMesh *mesh, int, float, int) {
        if (mesh)
          checkName(mesh->getAllElems());
      },
      [&](const ShaderSkinnedMesh *mesh, int, int) {
        if (mesh)
          checkName(mesh->getShaderMesh().getAllElems());
      });
  }
#endif

  // Get nodes visibility.

  int indexToAllNodesVisibility = ctx.allNodesVisibility.size();
  ctx.allNodesVisibility.resize(indexToAllNodesVisibility + instance->getNodeCount());
  bool hasVisibleNodes = false;
  const float *opacityArray = instance->opacity_ptr();
  const bool *hiddenArray = instance->hidden_ptr();

  if (node_list == NULL)
  {
    for (int nodeId = 0; nodeId < instance->getNodeCount(); nodeId++)
    {
      bool isNodeVisible = !hiddenArray[nodeId] && opacityArray[nodeId] > 0.f; // TODO: Remove getNodeOpacity check.
      hasVisibleNodes |= isNodeVisible;
      ctx.allNodesVisibility[indexToAllNodesVisibility + nodeId] = isNodeVisible;
    }
  }
  else
  {
    for (int nodeId = 0; nodeId < instance->getNodeCount(); nodeId++)
      ctx.allNodesVisibility[indexToAllNodesVisibility + nodeId] = false;

    for (int nodeNo = 0; nodeNo < node_list->size(); nodeNo++)
    {
      int nodeId = node_list->data()[nodeNo];
      bool isNodeVisible = !hiddenArray[nodeId] && opacityArray[nodeId] > 0.f; // TODO: Remove getNodeOpacity check.
      hasVisibleNodes |= isNodeVisible;
      ctx.allNodesVisibility[indexToAllNodesVisibility + nodeId] = isNodeVisible;
    }
  }

  if (!hasVisibleNodes)
  {
    ctx.allNodesVisibility.resize(indexToAllNodesVisibility);
    return;
  }

  InstanceData &instanceData = ctx.instances.push_back();
  instanceData.instance = instance;
  instanceData.sceneRes = sceneRes;
  instanceData.initialNodes = optional_initial_nodes;
  instanceData.indexToCustomProjtm = -1;
  instanceData.instanceLod = instance->getCurrentLodNo();
  if (customProj)
  {
    instanceData.indexToCustomProjtm = ctx.customProjTms.size();
    ctx.customProjTms.push_back(*customProj);
  }
  instanceData.indexToAllNodesVisibility = indexToAllNodesVisibility;

  if (ctx.perInstanceRenderData.empty())
    ctx.perInstanceRenderData.emplace_back();

  if (optional_render_data)
  {
    instanceData.indexToPerInstanceRenderData = ctx.perInstanceRenderData.size();
    ctx.perInstanceRenderData.push_back(*optional_render_data);

#if DAGOR_DBGLEVEL > 0
    // Check the values are reasonable to catch errors in the parameters early.
    for (const Interval &interval : optional_render_data->intervals)
    {
      G_ASSERT(interval.varId > 0); // 0 is correct but unlikely value.
      G_ASSERT(interval.instNodeId >= INVALID_INST_NODE_ID && interval.instNodeId < 100000);
    }
#endif
  }
  else
  {
    instanceData.indexToPerInstanceRenderData = 0;
  }
}

// returns one of hints if any can be considered a more precise result of (lhs-rhs)
// return (lhs-rhs otherwise)
static Point3 subtractWithHint(const Point3 &lhs, const Point3 &rhs, const Point3 &hint)
{
  float eps = eastl::max({fabsf(lhs.x), fabsf(lhs.y), fabsf(lhs.z), fabsf(rhs.x), fabsf(rhs.y), fabsf(rhs.z)});
  eps *= 8 * FLT_EPSILON; // Three bits precision.
  Point3 result = lhs - rhs;
  const bool isEqual =
    is_equal_float(result.x, hint.x, eps) && is_equal_float(result.y, hint.y, eps) && is_equal_float(result.z, hint.z, eps);
  return isEqual ? hint : result;
}


static TMatrix4 calcLocalViewProj(const Point3 &model_origin, const TMatrix4 &src_view, const TMatrix4 &src_proj, const Point3 &offset)
{
  TMatrix4 viewTm = src_view;

  Point3 modelOriginInViewSpace = model_origin % viewTm;
  Point3 viewPos = -Point3(viewTm.m[3][0], viewTm.m[3][1], viewTm.m[3][2]);
  Point3 originViewPos = subtractWithHint(modelOriginInViewSpace, viewPos, offset);
  viewTm.setrow(3, originViewPos.x, originViewPos.y, originViewPos.z, 1.f);

  return viewTm * src_proj;
}


static void instanceToChunks(ContextData &ctx, const InstanceData &instance_data, const TMatrix4 &view, const TMatrix4 &proj,
  const TMatrix4 &prev_view, const TMatrix4 &prev_proj, const Point3 &offset, TexStreamingContext texCtx, int &baseOffsetRenderData)
{
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC // This function called too frequenly, too much overhead profiling
  TIME_PROFILE(instanceToChunks);
#endif

  const DynamicRenderableSceneInstance *instance = instance_data.instance;
  if (instance_data.instance->getLodsResource() == nullptr)
    return;
  const DynamicRenderableSceneResource *sceneRes = instance_data.sceneRes;
  PerInstanceRenderData *perInstanceRenderData = &ctx.perInstanceRenderData[instance_data.indexToPerInstanceRenderData];


  // Duplicate PerInstanceRenderData for per-node intervals.

  eastl::vector_set<int, eastl::less<int>, framemem_allocator> nodesWithAlternativeSetOfIntervals;
  for (const Interval &interval : perInstanceRenderData->intervals)
    if (interval.instNodeId != INVALID_INST_NODE_ID)
      nodesWithAlternativeSetOfIntervals.insert(interval.instNodeId);

  int indexToAlternativePerInstanceRenderData = instance_data.indexToPerInstanceRenderData;
  if (!nodesWithAlternativeSetOfIntervals.empty())
  {
    indexToAlternativePerInstanceRenderData = ctx.perInstanceRenderData.size();
    ctx.perInstanceRenderData.push_back(*perInstanceRenderData);
    perInstanceRenderData = &ctx.perInstanceRenderData[instance_data.indexToPerInstanceRenderData]; // Restore pointer to _main_
                                                                                                    // perInstanceRenderData.

    for (int removeIntervalNo = perInstanceRenderData->intervals.size() - 1; removeIntervalNo >= 0; removeIntervalNo--)
      if (perInstanceRenderData->intervals[removeIntervalNo].instNodeId != INVALID_INST_NODE_ID)
        erase_items(perInstanceRenderData->intervals, removeIntervalNo, 1);
  }

  Point3 posMul, posOfs;
  if (instance->getLodsResource()->isBoundPackUsed())
  {
    posMul = *((Point3 *)instance->getLodsResource()->bpC255);
    posOfs = *((Point3 *)instance->getLodsResource()->bpC254);
  }
  else
  {
    posMul = Point3(1.f, 1.f, 1.f);
    posOfs = Point3(0.f, 0.f, 0.f);
  }

  const Point3 &modelOrigin = instance->getOrigin();
  TMatrix4_vec4 viewProjMatrixRelToOrigin;
  if (instance_data.indexToCustomProjtm >= 0)
    viewProjMatrixRelToOrigin = calcLocalViewProj(modelOrigin, view, ctx.customProjTms[instance_data.indexToCustomProjtm], offset);
  else
    viewProjMatrixRelToOrigin = calcLocalViewProj(modelOrigin, view, proj, offset);
  mat44f viewProjTmRelToOrigin = (mat44f &)viewProjMatrixRelToOrigin;

  const Point3 &prevModelOrigin = instance->getOriginPrev();
  TMatrix4_vec4 prevViewProjMatrixRelToOrigin = calcLocalViewProj(prevModelOrigin, prev_view, prev_proj, offset);
  mat44f prevViewProjTmRelToOrigin = (mat44f &)prevViewProjMatrixRelToOrigin;

  const float *opacityArray = instance->opacity_ptr();

  int desiredTexLevel =
    instance_data.instanceLod >= 0
      ? texCtx.getTexLevel(instance->getLodsResource()->getTexScale(instance_data.instanceLod), instance->getDistSq())
      : 5;
  // InstanceChunk

  int instanceChunkOffset = ctx.renderDataBuffer.size() / sizeof(vec4f);
  ctx.renderDataBuffer.resize(ctx.renderDataBuffer.size() + sizeof(InstanceChunk));
  InstanceChunk &instanceChunk = *(InstanceChunk *)(ctx.renderDataBuffer.end() - sizeof(InstanceChunk));
  instanceChunk.posMul.set_xyz0(posMul);
  instanceChunk.posOfs__paramsCount.set_xyz0(posOfs);
  instanceChunk.posOfs__paramsCount.w = perInstanceRenderData->params.size() + 0.5f;


  // Params follow the instance.

  if (!perInstanceRenderData->params.empty())
  {
    int paramsSize = perInstanceRenderData->params.size() * sizeof(vec4f);
    ctx.renderDataBuffer.resize(ctx.renderDataBuffer.size() + paramsSize);
    memcpy(ctx.renderDataBuffer.end() - paramsSize, perInstanceRenderData->params.begin(), paramsSize);
  }


  // elems to DipChunks.

  auto initNodeChunk = [&](NodeChunk &node_chunk, int node_id, int base_offset) {
    node_chunk.instanceChunkOffset = instanceChunkOffset - base_offset + 0.5f;
    node_chunk.nodeOpacity = opacityArray[node_id];
  };

  auto initDipChunk = [&](DipChunk &dip_chunk, int node_id, const ShaderMesh::RElem &elem, int stg) {
    if (nodesWithAlternativeSetOfIntervals.find(node_id) == nodesWithAlternativeSetOfIntervals.end())
      dip_chunk.indexToPerInstanceRenderData = instance_data.indexToPerInstanceRenderData;
    else
      dip_chunk.indexToPerInstanceRenderData = indexToAlternativePerInstanceRenderData;
    dip_chunk.instanceNo = &instance_data - ctx.instances.begin();
    dip_chunk.forcedOrder = 0;
    dip_chunk.si = elem.si;
    dip_chunk.numf = elem.numf;
    dip_chunk.baseVertex = elem.baseVertex;
    dip_chunk.shader = replacement_shader ? replacement_shader : elem.e.get();
    dip_chunk.numPasses = 0;
    dip_chunk.shaderName = dip_chunk.shader->getShaderClassName();
    dip_chunk.vertexData = elem.vertexData;
    if (stg == ShaderMesh::STG_opaque || !(perInstanceRenderData->flags & APPLY_OVERRIDE_STATE_ID_TO_OPAQUE_ONLY))
      dip_chunk.overrideStateId = perInstanceRenderData->overrideStateId;
    else
      dip_chunk.overrideStateId = shaders::OverrideStateId();
    dip_chunk.mergeOverrideState = perInstanceRenderData->flags & MERGE_OVERRIDE_STATE;
    dip_chunk.constDataBuf = perInstanceRenderData->constDataBuf;

    dip_chunk.texLevel = desiredTexLevel;

    static int dynamic_num_passesVarId = get_shader_variable_id("dynamic_num_passes", true);
    elem.mat->getIntVariable(dynamic_num_passesVarId, dip_chunk.numPasses);
  };

  auto &&shaderMeshStagesList =
    shaderMeshStagesByRenderFlags[(separateAtestPass ? 8 : 0) +
                                  (perInstanceRenderData->flags & (RENDER_OPAQUE | RENDER_TRANS | RENDER_DISTORTION))];

  int nodeChunkVecs = instance_data.initialNodes ? bigNodeChunkVecs : smallNodeChunkVecs;

  baseOffsetRenderData = ctx.renderDataBuffer.size() / sizeof(vec4f);

  sceneRes->getMeshes(
    [&](const ShaderMesh *mesh, int node_id, float radius, int rigid_no) {
      G_ASSERT(mesh);
      G_ASSERT(node_id >= 0);
      G_ASSERT(instance_data.indexToAllNodesVisibility + node_id < ctx.allNodesVisibility.size());

      if (!ctx.allNodesVisibility[instance_data.indexToAllNodesVisibility + node_id] || radius < ctx.minElemRadius)
        return;

      mat44f nodeGlobTm, nodeTmRelToOrigin;
      v_mat44_make_from_43cu(nodeTmRelToOrigin, instance->getNodeWtmRelToOrigin(node_id)[0]);
      v_mat44_mul(nodeGlobTm, viewProjTmRelToOrigin, nodeTmRelToOrigin);

      mat44f prevNodeGlobTm, prevNodeTmRelToOrigin;
      v_mat44_make_from_43cu(prevNodeTmRelToOrigin, instance->getNodePrevWtmRelToOrigin(node_id)[0]);
      v_mat44_mul(prevNodeGlobTm, prevViewProjTmRelToOrigin, prevNodeTmRelToOrigin);

      int baseOffsetRenderData = ctx.renderDataBuffer.size() / sizeof(vec4f);
      ctx.renderDataBuffer.resize(ctx.renderDataBuffer.size() + nodeChunkVecs * sizeof(vec4f));
      NodeChunk &nodeChunk = *(NodeChunk *)(ctx.renderDataBuffer.end() - nodeChunkVecs * sizeof(vec4f));
      initNodeChunk(nodeChunk, node_id, baseOffsetRenderData);
      nodeChunk.nodeGlobTm = nodeGlobTm;
      nodeChunk.prevNodeGlobTm = prevNodeGlobTm;

      if (instance_data.initialNodes)
      {
        if (!instance_data.initialNodes->nodesModelTm.empty())
        {
          mat44f initialNodeTm = instance_data.initialNodes->nodesModelTm[node_id];
          v_mat44_transpose(initialNodeTm, initialNodeTm);
          nodeChunk.initialNodeTm0 = initialNodeTm.col0;
          nodeChunk.initialNodeTm1 = initialNodeTm.col1;
          nodeChunk.initialNodeTm2 = initialNodeTm.col2;
        }

        if (!instance_data.initialNodes->extraData.empty())
          nodeChunk.extraData = instance_data.initialNodes->extraData[node_id];
        else
          nodeChunk.extraData.flt[0] = nodeChunk.extraData.flt[1] = 0.f;
      }

      for (auto &&shaderMeshStages : shaderMeshStagesList)
      {
        dag::Span<ShaderMesh::RElem> elems = mesh->getElems(shaderMeshStages.front(), shaderMeshStages.back());
        for (unsigned int elemNo = 0; elemNo < elems.size(); elemNo++)
        {
          if (!elems[elemNo].e) // dont_render
            continue;

          if (filtered_material_names.size() && eastl::find(filtered_material_names.begin(), filtered_material_names.end(),
                                                  elems[elemNo].mat->getShaderClassName()) == filtered_material_names.end())
          {
            continue;
          }

          auto &dipChunks = ctx.dipChunksByStage[shaderMeshStages.front()];
          DipChunk &dipChunk = dipChunks.push_back();
          initDipChunk(dipChunk, node_id, elems[elemNo], shaderMeshStages.front());
          dipChunk.numBones = 0;
          dipChunk.baseOffsetRenderData = baseOffsetRenderData - (rigid_no % 256) * nodeChunkVecs;
          dipChunk.nodeChunkSizeInVecs = nodeChunkVecs;
          if (dipChunks.size() >= 2 && dipChunks[dipChunks.size() - 2].tryMerge(dipChunk))
          {
            ctx.statPreMerged++;
            dipChunks.pop_back();
            continue;
          }

          if (shaderMeshStages.front() == ShaderMesh::STG_trans)
          {
            auto found = eastl::find(shadersRenderOrder.begin(), shadersRenderOrder.end(), elems[elemNo].mat->getShaderClassName());
            if (found != shadersRenderOrder.end())
              dipChunk.forcedOrder = found - shadersRenderOrder.begin() + 1;
          }
        }
      }
      ctx.statNodes++;
    },
    [&](const ShaderSkinnedMesh *mesh, int node_id, int) {
      G_ASSERT(mesh);
      G_ASSERT(node_id >= 0);
      G_ASSERT(instance_data.indexToAllNodesVisibility + node_id < ctx.allNodesVisibility.size());
      G_ASSERT(mesh->bonesCount() > 0);

      if (!ctx.allNodesVisibility[instance_data.indexToAllNodesVisibility + node_id])
        return;

      if (!ctx.renderSkinned &&
          !(ctx.perInstanceRenderData[instance_data.indexToPerInstanceRenderData].flags & OVERRIDE_RENDER_SKINNED_CHECK))
      {
        return;
      }

      int baseOffsetRenderData = ctx.renderDataBuffer.size() / sizeof(vec4f);
      int nodeChunkSize = smallNodeChunkVecs * sizeof(vec4f) + mesh->bonesCount() * sizeof(vec4f) * 6;
      ctx.renderDataBuffer.resize(ctx.renderDataBuffer.size() + nodeChunkSize);
      NodeChunk &nodeChunk = *(NodeChunk *)(ctx.renderDataBuffer.end() - nodeChunkSize);
      initNodeChunk(nodeChunk, node_id, baseOffsetRenderData);
      nodeChunk.nodeGlobTm = viewProjTmRelToOrigin;
      nodeChunk.prevNodeGlobTm = prevViewProjTmRelToOrigin;
      if (instance_data.initialNodes && !instance_data.initialNodes->extraData.empty())
        nodeChunk.extraData = instance_data.initialNodes->extraData[node_id];

      ctx.statBones += mesh->bonesCount();
      vec4f *bones = (vec4f *)(&nodeChunk) + smallNodeChunkVecs;
      for (int boneNo = 0; boneNo < mesh->bonesCount(); boneNo++)
      {
        mat44f origTm, nodeWtmRelToOrigin, transp;
        v_mat44_make_from_43cu(origTm, mesh->getBoneOrgTm(boneNo)[0]);
        v_mat44_make_from_43cu(nodeWtmRelToOrigin, instance->getNodeWtmRelToOrigin(mesh->getNodeForBone(boneNo))[0]);
        v_mat44_mul(transp, nodeWtmRelToOrigin, origTm);
        v_mat44_transpose(transp, transp);
        *bones++ = transp.col0;
        *bones++ = transp.col1;
        *bones++ = transp.col2;

        mat44f prevNodeWtmRelToOrigin;
        v_mat44_make_from_43cu(prevNodeWtmRelToOrigin, instance->getNodePrevWtmRelToOrigin(mesh->getNodeForBone(boneNo))[0]);
        v_mat44_mul(transp, prevNodeWtmRelToOrigin, origTm);
        v_mat44_transpose(transp, transp);
        *bones++ = transp.col0;
        *bones++ = transp.col1;
        *bones++ = transp.col2;
      }

      for (auto &&shaderMeshStages : shaderMeshStagesList)
      {
        dag::Span<ShaderMesh::RElem> elems = mesh->getShaderMesh().getElems(shaderMeshStages.front(), shaderMeshStages.back());
        for (unsigned int elemNo = 0; elemNo < elems.size(); elemNo++)
        {
          if (!elems[elemNo].e) // dont_render
            continue;

          if (filtered_material_names.size() && eastl::find(filtered_material_names.begin(), filtered_material_names.end(),
                                                  elems[elemNo].mat->getShaderClassName()) == filtered_material_names.end())
          {
            continue;
          }

          DipChunk &dipChunk = ctx.dipChunksByStage[shaderMeshStages.front()].push_back();
          initDipChunk(dipChunk, node_id, elems[elemNo], shaderMeshStages.front());
          dipChunk.numBones = mesh->bonesCount();
          dipChunk.baseOffsetRenderData = baseOffsetRenderData;
          dipChunk.nodeChunkSizeInVecs = smallNodeChunkVecs; // Skinned meshes never use initial tms.
        }
      }
      ctx.statNodes++;
    });
}


class CompareDipChunks
{
  DipChunk *dipChunks;

public:
  CompareDipChunks(DipChunk *in_dipChunks) : dipChunks(in_dipChunks) {}

  inline bool operator()(int a_order, int b_order) const
  {
    DipChunk &a = dipChunks[a_order];
    DipChunk &b = dipChunks[b_order];

    if (a.forcedOrder != b.forcedOrder)
      return a.forcedOrder < b.forcedOrder;

    if (a.intervals != b.intervals)
      return a.intervals < b.intervals;

    if (a.constDataBuf != b.constDataBuf)
      return a.constDataBuf < b.constDataBuf;

    if (a.shaderName != b.shaderName)
      return a.shaderName < b.shaderName;

    if (a.shader != b.shader)
      return a.shader < b.shader;

    if (a.texLevel != b.texLevel)
      return a.texLevel > b.texLevel; // We want instances with higher levels to come first to not do extra state changes.

    if (a.instanceNo != b.instanceNo)
      return a.instanceNo < b.instanceNo;

    if (a.vertexData != b.vertexData)
      return a.vertexData < b.vertexData;

    if (a.si != b.si)
      return a.si < b.si;

    return false;
  }
};

void prepare_render(ContextId context_id, const TMatrix4 &view, const TMatrix4 &proj, const Point3 &offset_to_origin,
  TexStreamingContext texCtx, dynrend::InstanceContextData *instanceContextData)
{
  TIME_PROFILE(dynrend_prepare_render);

  G_ASSERTF_RETURN((int)context_id >= 0 && (int)context_id < contexts.size(), , "Uninitialized dynrend context was used");
  G_ASSERT(context_id != ContextId::IMMEDIATE || is_main_thread());

  ContextData &ctx = contexts[(int)context_id];

  if (ctx.instances.empty())
    return;

  {
    TMatrix4 relativeViewTm = view;
    relativeViewTm.setrow(3, 0, 0, 0, 1.f);

    TMatrix4 worldToProjTm = relativeViewTm * proj;
    float det;
    inverse44(worldToProjTm, ctx.gpuData.projToWorldTm, det);
    inverse44(proj, ctx.gpuData.projToViewTm, det);
  }

  // Instances to chunks.

  ctx.renderDataBuffer.resize(0);
  int baseOffsetRenderData = 0;
  if (instanceContextData)
  {
    instanceContextData->contextId = ContextId(-1);
    instanceContextData->baseOffsetRenderData = -1;
  }
  for (InstanceData &instanceData : ctx.instances)
  {
    instanceToChunks(ctx, instanceData, view, proj, prevViewTm, prevProjTm, offset_to_origin, texCtx, baseOffsetRenderData);
    instanceData.baseOffsetRenderData = baseOffsetRenderData;
    if (instanceContextData && instanceContextData->instance == instanceData.instance)
    {
      instanceContextData->contextId = context_id;
      instanceContextData->baseOffsetRenderData = baseOffsetRenderData;
    }
  }

  if (ctx.renderDataBuffer.empty())
  {
    ctx.clear();
    return;
  }
  FRAMEMEM_REGION;


  // Copy render data to ring buffer.

  int sizeOfAllChunks = ctx.renderDataBuffer.size() / sizeof(vec4f);
  if (sizeOfAllChunks > ctx.ringBufferSizeInVecs)
  {
    ctx.recreateRingBuffer(3 * sizeOfAllChunks, &ctx - contexts.begin());
    debug("dynrend: '%s' ring buffer size = %dK", ctx.name.c_str(), (ctx.ringBufferSizeInVecs * sizeof(vec4f)) >> 10);
  }

  if (ctx.ringBuffer->bufLeft() < sizeOfAllChunks)
  {
    if (ctx.prevDiscardOnFrame == ::dagor_frame_no())
    {
      ctx.recreateRingBuffer(3 * ctx.ringBufferSizeInVecs, &ctx - contexts.begin());
      debug("dynrend: '%s' ring buffer size = %dK", ctx.name.c_str(), (ctx.ringBufferSizeInVecs * sizeof(vec4f)) >> 10);
    }
    ctx.prevDiscardOnFrame = dagor_frame_no();

    ctx.ringBuffer->resetPos();
    ctx.ringBuffer->resetCounters();
  }

  void *ringBufferData;
  {
    TIME_PROFILE(lock);
    ringBufferData = ctx.ringBuffer->lockData(sizeOfAllChunks);
    if (!ringBufferData)
    {
      ctx.clear();
      return;
    }
  }

  ctx.ringBufferPos = ctx.ringBuffer->getPos();

  {
    TIME_PROFILE(memcpy);
    memcpy(ringBufferData, ctx.renderDataBuffer.begin(), ctx.renderDataBuffer.size());
  }

  {
    TIME_PROFILE(unlock);
    ctx.ringBuffer->unlockData(sizeOfAllChunks);
  }


  // Sort.

  eastl::array<SmallTab<int, framemem_allocator>, ShaderMesh::STG_COUNT> dipChunksOrderByStage;

  {
    TIME_PROFILE(sort);
    for (int stageNo = 0; stageNo < ShaderMesh::STG_COUNT; stageNo++)
    {
      for (DipChunk &chunk : ctx.dipChunksByStage[stageNo]) // In separate pass because instanceToChunks can resize
                                                            // perInstanceRenderData.
        chunk.intervals = chunk.getIntervals(ctx);          // intervals field is used only for sorting

      dipChunksOrderByStage[stageNo].resize(ctx.dipChunksByStage[stageNo].size());
      for (int order = 0, e = dipChunksOrderByStage[stageNo].size(); order < e; order++)
        dipChunksOrderByStage[stageNo][order] = order;

      stlsort::sort(dipChunksOrderByStage[stageNo].begin(), dipChunksOrderByStage[stageNo].end(),
        CompareDipChunks(ctx.dipChunksByStage[stageNo].begin()));
    }
  }


  // Post-merge.

  {
    TIME_PROFILE(postmerge);
    for (int stageNo = 0; stageNo < ShaderMesh::STG_COUNT; stageNo++)
    {
      auto &dipChunks = ctx.dipChunksByStage[stageNo];
      auto &order = dipChunksOrderByStage[stageNo];
      auto &newOrder = ctx.dipChunksOrderByStage[stageNo];
      for (int curChunkNo = 0, e = dipChunks.size(); curChunkNo < e; curChunkNo++)
      {
        newOrder.push_back(order[curChunkNo]);
        int mergeChunkNo = curChunkNo + 1;
        for (; mergeChunkNo < e; mergeChunkNo++)
        {
          if (!dipChunks[order[curChunkNo]].tryMerge(dipChunks[order[mergeChunkNo]]))
            break;

          dipChunks[order[mergeChunkNo]].numf = 0;
          ctx.statPostMerged++;
        }
        curChunkNo = mergeChunkNo - 1;
      }
    }
  }


  if (dynrendLog.get() && ::dagor_frame_no() % 100 == 0)
    debug("%d prepare %s: instances=%d, nodes=%d, bones=%d, chunks[0]=%d, chunks[4]=%d, preMerged=%d, postMerged=%d",
      ::dagor_frame_no(), ctx.name.c_str(), ctx.instances.size(), ctx.statNodes, ctx.statBones,
      ctx.dipChunksOrderByStage[ShaderMesh::STG_opaque].size(), ctx.dipChunksOrderByStage[ShaderMesh::STG_trans].size(),
      ctx.statPreMerged, ctx.statPostMerged);
}


static void change_intervals(Intervals *from, Intervals *to)
{
  if (from)
  {
    for (Interval &interval : *from)
      ShaderGlobal::set_int(interval.varId, interval.unsetValue);
  }

  if (to)
  {
    for (Interval &interval : *to)
      ShaderGlobal::set_int(interval.varId, interval.setValue);
  }
}


static void set_override(shaders::OverrideStateId id, shaders::OverrideStateId &initial_id, bool merge_override_state)
{
  shaders::overrides::reset();
  if (merge_override_state && id && initial_id)
  {
    shaders::OverrideState initialState = shaders::overrides::get(initial_id);
    shaders::OverrideState newState = shaders::overrides::get(id);
    if (initialState != newState)
    {
      initialState.set(newState.bits);
      shaders::OverrideStateId combinedStateId = shaders::overrides::create(initialState);
      shaders::overrides::set(combinedStateId);
    }
    else
      shaders::overrides::set(id);
  }
  else if (id)
    shaders::overrides::set(id);
  else if (initial_id)
    shaders::overrides::set(initial_id);
}

void update_reprojection_data(ContextId contextId)
{
  ContextData &ctx = contexts[(int)contextId];
  if (!ctx.renderSkinned)
    return;
  contextGpuDataBuffer->updateDataWithLock(0, sizeof(ContextGpuData), &ctx.gpuData, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
}

bool set_instance_data_buffer(unsigned stage, ContextId contextId, int baseOffsetRenderData)
{
  G_ASSERT(is_main_thread());

  if ((int)contextId < 0)
    return false;

  ContextData &ctx = contexts[(int)contextId];
  if (!ctx.renderSkinned)
    return false;

  static int dynamicInstancingTypeVarId = get_shader_variable_id("dynamic_instancing_type", true);
  ShaderGlobal::set_int(dynamicInstancingTypeVarId, 1);

  ShaderGlobal::set_buffer(instanceDataBufferVarId, ctx.ringBuffer->getBufId());
  ctx.ringBufferVarSet = true;

  uint32_t offset = ctx.ringBufferPos + baseOffsetRenderData;
  uint32_t offsetAndSize = (offset << 8) | smallNodeChunkVecs;

  d3d::set_immediate_const(stage, &offsetAndSize, 1);

  return true;
}

const Point4 *get_per_instance_render_data(ContextId contextId, int indexToPerInstanceRenderData)
{
  G_ASSERT(is_main_thread());

  if ((int)contextId < 0)
    return nullptr;

  ContextData &ctx = contexts[(int)contextId];

  return ctx.perInstanceRenderData[indexToPerInstanceRenderData].params.data();
}

void render(ContextId context_id, int shader_mesh_stage)
{
  G_ASSERT(is_main_thread());
  G_ASSERTF_RETURN((int)context_id >= 0 && (int)context_id < contexts.size(), , "Uninitialized dynrend context was used");

  ContextData &ctx = contexts[(int)context_id];
  const auto &dipChunks = ctx.dipChunksByStage[shader_mesh_stage];

  if (dipChunks.empty())
    return;

  TIME_PROFILE(dynrend_render);


  // Begin render.

  static int dynamicInstancingTypeVarId = get_shader_variable_id("dynamic_instancing_type", true);
  ShaderGlobal::set_int(dynamicInstancingTypeVarId, 1);

  ShaderGlobal::set_buffer(instanceDataBufferVarId, ctx.ringBuffer->getBufId());
  ctx.ringBufferVarSet = true;

  contextGpuDataBuffer->updateDataWithLock(0, sizeof(ContextGpuData), &ctx.gpuData, VBLOCK_WRITEONLY | VBLOCK_DISCARD);

  int sceneBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);
  ShaderGlobal::setBlock(sceneBlock, ShaderGlobal::LAYER_SCENE); // Update buffer var.


  // Render sequence of DIP chunks through immediate constants

  int statInvalid = 0;
  int statShader = 0;
  int statVertexData = 0;
  int statDip = 0;
  int statTriangles = 0;
  int statIntervals = 0;
  int statConstData = 0;
  int statOverrides = 0;

  {
    TIME_PROFILE(render);

    Intervals *currentIntervals = NULL;
    ShaderElement *currentShader = NULL;
    bool currentShaderValid = false;
    GlobalVertexData *currentVertexData = NULL;
    shaders::OverrideStateId initialOverrideStateId = shaders::overrides::get_current();
    shaders::OverrideStateId currentOverrideOfInitialOverrideStateId = shaders::OverrideStateId();
    bool currentMergeOverrideState = false;
    uint32_t currentOffsetAndSize = 0x80000000;
    D3DRESID currentConstDataBuf = BAD_D3DRESID;

    for (int chunkNo : ctx.dipChunksOrderByStage[shader_mesh_stage])
    {
      const DipChunk &dipChunk = dipChunks[chunkNo];

      if (dipChunk.numf == 0)
        continue;

      Intervals *dipChunkIntervals = dipChunk.getIntervals(ctx);
      if (dipChunkIntervals != currentIntervals)
      {
        change_intervals(currentIntervals, dipChunkIntervals);
        currentIntervals = dipChunkIntervals;
        currentShader = NULL;
        statIntervals++;
      }

      D3DRESID dipChunkConstDataBuf = dipChunk.constDataBuf;
      if (dipChunkConstDataBuf != currentConstDataBuf)
      {
        ShaderGlobal::set_buffer(instance_const_data_bufferVarId, dipChunkConstDataBuf);
        currentConstDataBuf = dipChunkConstDataBuf;
        currentShader = NULL;
        statConstData++;
      }

      if (dipChunk.overrideStateId != currentOverrideOfInitialOverrideStateId ||
          dipChunk.mergeOverrideState != currentMergeOverrideState)
      {
        set_override(dipChunk.overrideStateId, initialOverrideStateId, dipChunk.mergeOverrideState);
        currentOverrideOfInitialOverrideStateId = dipChunk.overrideStateId;
        currentMergeOverrideState = dipChunk.mergeOverrideState;
        currentShader = NULL; // Changing the override requires setStates.
        statOverrides++;
      }

      bool shaderChanged = dipChunk.shader != currentShader;
      bool texLevelIncreased = dipChunk.shader->setReqTexLevel(dipChunk.texLevel);
      if (shaderChanged || texLevelIncreased)
      {
        if (shaderChanged && currentShader)
          currentShader->setReqTexLevel();
        currentShaderValid = dipChunk.shader->setStates(0, true); //-V522 (we don't add dipChunk if shader is null)
        currentShader = dipChunk.shader;
        statShader++;
      }

      if (!currentShaderValid)
      {
        statInvalid++;
        continue;
      }

      // vertexData could be used after free()
      if (dipChunk.vertexData->isEmpty())
      {
        statInvalid++;
        continue;
      }

      if (dipChunk.vertexData != currentVertexData)
      {
        dipChunk.vertexData->setToDriver();
        currentVertexData = dipChunk.vertexData;
        statVertexData++;
      }

      uint32_t offset = ctx.ringBufferPos + dipChunk.baseOffsetRenderData;
      uint32_t offsetAndSize = (offset << 8) | dipChunk.nodeChunkSizeInVecs;
      if (offsetAndSize != currentOffsetAndSize)
      {
        d3d::set_immediate_const(STAGE_VS, &offsetAndSize, 1);
        currentOffsetAndSize = offsetAndSize;
      }

      // Do instansing if > 0 to gen inst id
      if (dipChunk.numPasses > 0)
        d3d::drawind_instanced(PRIM_TRILIST, dipChunk.si, dipChunk.numf, dipChunk.baseVertex, dipChunk.numPasses, 0);
      else
        d3d::drawind(PRIM_TRILIST, dipChunk.si, dipChunk.numf, dipChunk.baseVertex);
      statDip++;
      statTriangles += dipChunk.numf * (dipChunk.numPasses > 1 ? dipChunk.numPasses : 1);
      statistics.dips++;
      statistics.triangles += dipChunk.numf * (dipChunk.numPasses > 1 ? dipChunk.numPasses : 1);
    }

    if (currentIntervals)
      change_intervals(currentIntervals, NULL);

    if (currentOverrideOfInitialOverrideStateId)
      set_override(shaders::OverrideStateId(), initialOverrideStateId, false);

    if (currentShader)
      currentShader->setReqTexLevel();
  }

  if (dynrendLog.get() && ::dagor_frame_no() % 100 == 0)
    debug("%d     render[%d] %s: chunks=%d, Shader=%d, VertexData=%d, Invalid=%d, intervals=%d, constData=%d, overrides=%d, tris=%d",
      ::dagor_frame_no(), shader_mesh_stage, ctx.name.c_str(), ctx.dipChunksOrderByStage[shader_mesh_stage].size(), statShader,
      statVertexData, statInvalid, statIntervals, statConstData, statOverrides, statTriangles);

  // End render.
  ShaderGlobal::set_buffer(instanceDataBufferVarId, BAD_D3DRESID); // avoid race if prepare happens in multiple threads
  ShaderGlobal::set_buffer(instance_const_data_bufferVarId, BAD_D3DRESID);
  ShaderGlobal::set_int(dynamicInstancingTypeVarId, 0);
  d3d::set_immediate_const(STAGE_VS, NULL, 0);
  ctx.ringBufferVarSet = false;
}


void clear(ContextId context_id)
{
  G_ASSERTF_RETURN((int)context_id >= 0 && (int)context_id < contexts.size(), , "Uninitialized dynrend context was used");

  ContextData &ctx = contexts[(int)context_id];
  if (dynrendLog.get() && ::dagor_frame_no() % 100 == 0 && !ctx.instances.empty())
    debug("%d     clear %s", ::dagor_frame_no(), ctx.name.c_str());
  ctx.clear();
}


void clear_all_contexts()
{
  for (ContextData &ctx : contexts)
    ctx.clear();
}


void set_prev_view_proj(const TMatrix4_vec4 &prev_view, const TMatrix4_vec4 &prev_proj)
{
  prevViewTm = prev_view;
  prevProjTm = prev_proj;
}


void get_prev_view_proj(TMatrix4_vec4 &prev_view, TMatrix4_vec4 &prev_proj)
{
  prev_view = prevViewTm;
  prev_proj = prevProjTm;
}


void set_local_offset_hint(const Point3 &hint) { localOffsetHint = hint; }


void enable_separate_atest_pass(bool enable) { separateAtestPass = enable; }

ShaderElement *get_replaced_shader() { return replacement_shader; }
void replace_shader(ShaderElement *element) { replacement_shader = element; }
const Tab<const char *> &get_filtered_material_names() { return filtered_material_names; }
void set_material_filters_by_name(Tab<const char *> &&material_names) { filtered_material_names = std::move(material_names); }


void verify_is_empty(ContextId context_id)
{
  static bool verifyDynrendContexts = ::dgs_get_settings()->getBlockByNameEx("debug")->getBool("verifyDynrendContexts", false);
  if (verifyDynrendContexts)
  {
    ContextData &ctx = contexts[(int)context_id];
    G_ASSERTF(ctx.instances.empty(), "dynrend: context '%s' is already in use", ctx.name.c_str());
    G_UNUSED(ctx);
  }
}


void render_one_instance(const DynamicRenderableSceneInstance *instance, RenderMode mode, TexStreamingContext texCtx,
  const InitialNodes *optional_initial_nodes, const dynrend::PerInstanceRenderData *optional_render_data)
{
  G_ASSERT(is_main_thread());
  verify_is_empty(ContextId::IMMEDIATE);

  add(ContextId::IMMEDIATE, instance, optional_initial_nodes, optional_render_data);
  TMatrix4 viewTm, projTm;
  d3d::gettm(TM_VIEW, &viewTm);
  d3d::gettm(TM_PROJ, &projTm);
  prepare_render(ContextId::IMMEDIATE, viewTm, projTm, localOffsetHint, texCtx);
  switch (mode)
  {
    case RenderMode::Opaque: render(ContextId::IMMEDIATE, ShaderMesh::STG_opaque); break;
    case RenderMode::Translucent: render(ContextId::IMMEDIATE, ShaderMesh::STG_trans); break;
    case RenderMode::Distortion: render(ContextId::IMMEDIATE, ShaderMesh::STG_distortion); break;
  }
  clear(ContextId::IMMEDIATE);
}


void opaque_flush(ContextId context_id, TexStreamingContext texCtx, bool include_atest)
{
  G_ASSERT(is_main_thread());

  TMatrix4 viewTm, projTm;
  d3d::gettm(TM_VIEW, &viewTm);
  d3d::gettm(TM_PROJ, &projTm);
  dynrend::prepare_render(context_id, viewTm, projTm, localOffsetHint, texCtx);
  dynrend::render(context_id, ShaderMesh::STG_opaque);
  if (include_atest)
  {
    dynrend::render(context_id, ShaderMesh::STG_atest);
  }
  dynrend::clear(context_id);
}


bool can_render(const DynamicRenderableSceneInstance *instance)
{
  if (!is_initialized())
    return false;

  if (!instance)
    return false;

  const DynamicRenderableSceneResource *sceneResource = instance->getCurSceneResource();
  if (!sceneResource)
    return false;

  auto rigids = sceneResource->getRigidsConst();
  auto skins = sceneResource->getSkinMeshes();
  int blockId = -1;

  for (const auto &rigid : rigids)
    if (rigid.mesh && rigid.mesh->getMesh() && rigid.mesh->getMesh()->getAllElems().size() > 0 &&
        rigid.mesh->getMesh()->getAllElems()[0].e)
    {
      blockId = rigid.mesh->getMesh()->getAllElems()[0].e->getSupportedBlock(0, ShaderGlobal::LAYER_SCENE);
      break;
    }

  if (blockId == -1)
  {
    for (const auto &skin : skins)
      if (skin && skin->getMesh() && skin->getMesh()->getShaderMesh().getAllElems().size() > 0 &&
          skin->getMesh()->getShaderMesh().getAllElems()[0].e)
      {
        blockId = skin->getMesh()->getShaderMesh().getAllElems()[0].e->getSupportedBlock(0, ShaderGlobal::LAYER_SCENE);
        break;
      }
  }

  return (blockId != -1); // Wild guess, can be replaced with a list of supported block names in case of problems.
}


bool render_in_tools(const DynamicRenderableSceneInstance *instance, RenderMode mode,
  const dynrend::PerInstanceRenderData *optional_render_data)
{
  if (!dynrend::can_render(instance))
    return false;

  dynrend::render_one_instance(instance, mode, TexStreamingContext(FLT_MAX), nullptr, optional_render_data);
  return true;
}


Statistics &get_statistics() { return statistics; }


void reset_statistics() { memset(&statistics, 0, sizeof(statistics)); }

void iterate_instances(dynrend::ContextId context_id, InstanceIterator iter, void *user_data)
{
  ContextData &ctx = contexts[(int)context_id];
  for (auto &instanceData : ctx.instances)
  {
    auto &instance = *instanceData.instance;
    auto &sceneRes = *instanceData.sceneRes;
    int nodeChunkVecs = instanceData.initialNodes ? bigNodeChunkVecs : smallNodeChunkVecs;
    iter(context_id, sceneRes, instance, ctx.perInstanceRenderData[instanceData.indexToPerInstanceRenderData], ctx.allNodesVisibility,
      instanceData.indexToAllNodesVisibility, ctx.minElemRadius, instanceData.baseOffsetRenderData, nodeChunkVecs, user_data);
  }
}

//============================================================================

InitialNodes::InitialNodes(const DynamicRenderableSceneInstance *instance, const GeomNodeTree *initial_skeleton)
{
  G_ASSERT(instance && initial_skeleton);

  nodesModelTm.resize(instance->getNodeCount());
  iterate_names(instance->getLodsResource()->getNames().node, [&](int nodeId, const char *nodeName) {
    if (auto skeletonNode = initial_skeleton->findINodeIndex(nodeName))
      nodesModelTm[nodeId] = initial_skeleton->getNodeWtmRel(skeletonNode);
    else
      v_mat44_ident(nodesModelTm[nodeId]);
  });
}

InitialNodes::InitialNodes(const DynamicRenderableSceneInstance *instance, const TMatrix &root_tm)
{
  G_ASSERT(instance);

  nodesModelTm.resize(instance->getNodeCount());
  for (int nodeId = 0; nodeId < instance->getNodeCount(); nodeId++)
    v_mat44_make_from_43cu(nodesModelTm[nodeId], root_tm.m[0]);
}

//============================================================================
} // namespace dynrend
