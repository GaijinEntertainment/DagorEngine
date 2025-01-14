// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_framemem.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_renderStateId.h>
#include <EASTL/hash_map.h>
#include <EASTL/span.h>
#include "dynModelRenderer.h"
#include "global_vars.h"
#include <render/debugMesh.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <shaders/dag_shStateBlockBindless.h>
#include <3d/dag_multidrawContext.h>
#include "shaders/dynModelsPackedMultidrawParams.hlsli"
#include <shaders/animchar_additional_data_types.hlsli>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_shaderConstants.h>
#include <ecs/render/updateStageRender.h>

using namespace dynmodel_renderer;

static struct DynModelRendering
{
  // if we don't support map nooverwrite it will be one buffer for each shadow cascade, dyn shadow, and two main passes (transparent &
  // opaque) +1 for 'other' if we do support, it will be just one ring buffer with fence
  dag::Vector<DynamicBufferHolder> dynamicBuffers;
  eastl::hash_map<eastl::string, DynModelRenderingState> map; // Note: intentionally not ska, since we need ref stability
  DynModelRenderingState *emplace(const char *name)
  {
    auto r = get(name);
    if (DAGOR_LIKELY(r))
      return const_cast<DynModelRenderingState *>(r);
    return &map[name];
  }
  const DynModelRenderingState *get(const char *name) const;
  void clear()
  {
    map = decltype(map)();
    dynamicBuffers = decltype(dynamicBuffers)();
    multidrawContext.close();
  }
  bool supportNoOverwrite = false;
  MultidrawContext<uint32_t> multidrawContext = {"dynmodel_multidraw"};
} dynmodel_rendering;

static void fill_node_collapser_data(const DynamicRenderableSceneInstance *scene,
  const eastl::span<Point4, ADDITIONAL_BONE_MTX_OFFSET> node_collapser_data)
{
  const DynamicRenderableSceneInstance::NodeCollapserBits &ncBits = scene->getNodeCollapserBits();
  node_collapser_data[0] = Point4(bitwise_cast<float>(ncBits[0]), bitwise_cast<float>(ncBits[1]), bitwise_cast<float>(ncBits[2]),
    bitwise_cast<float>(ncBits[3]));
  node_collapser_data[1] = Point4(bitwise_cast<float>(ncBits[4]), bitwise_cast<float>(ncBits[5]), bitwise_cast<float>(ncBits[6]),
    bitwise_cast<float>(ncBits[7]));
}

void DynModelRenderingState::process_animchar(uint32_t start_stage,
  uint32_t end_stage,
  const DynamicRenderableSceneInstance *scene,
  const DynamicRenderableSceneResource *lodResource,
  const Point4 *bones_additional_data,
  uint32_t additional_data_size,
  bool need_previous_matrices,
  ShaderElement *shader_override,
  const uint8_t *path_filter,
  uint32_t path_filter_size,
  uint8_t render_mask,
  bool need_immediate_const_ps,
  RenderPriority priority,
  const GlobalVariableStates *gvars_state,
  TexStreamingContext texCtx)
{
  G_ASSERT(scene->getLodsResource()->getInstanceRefCount() > 0);
  int lodNo = max(scene->getCurrentLodNo(), 0);

  const bool forceNodeCollapser = render_mask & UpdateStageInfoRender::RenderPass::FORCE_NODE_COLLAPSER_ON;
  render_mask = render_mask & ~UpdateStageInfoRender::RenderPass::FORCE_NODE_COLLAPSER_ON; // for safety

  const bool addPreviousMatrices = need_previous_matrices;
  matrixStride = addPreviousMatrices ? MATRIX_ROWS * 2 : MATRIX_ROWS; // 3 rows for matrix and 3 for previous matrix
  float distSq = scene->getDistSq();
  int reqLevel = texCtx.getTexLevel(scene->getLodsResource()->getTexScale(lodNo), distSq);

  auto addMeshToList = [&](const ShaderMesh &mesh, int instId, int bindposeBufferOffset) {
#if DAGOR_DBGLEVEL > 0
    int counter = lodNo;
    if (debug_mesh::is_enabled(debug_mesh::Type::drawElements))
    {
      counter = 0;
      for (uint8_t stage = start_stage; stage <= end_stage; ++stage)
        for (const auto &elem : mesh.getElems(stage))
        {
          if (!elem.e)
            continue;
          counter++;
        }
    }
#endif
    for (uint8_t stage = start_stage; stage <= end_stage; ++stage)
      for (const auto &elem : mesh.getElems(stage))
      {
        if (!elem.e)
          continue;
        uint32_t prog;
        ShaderStateBlockId state;
        shaders::TexStateIdx tstate;
        shaders::ConstStateIdx cstate;
        shaders::RenderStateId rstate;
        ShaderElement *s = (shader_override != nullptr) ? shader_override : static_cast<ShaderElement *>(elem.e);
        int curVar;
        curVar = get_dynamic_variant_states(gvars_state, s->native(), prog, state, rstate, cstate, tstate);
        if (curVar < 0)
          continue;
        if (!is_packed_material(cstate))
          list.emplace_back(s, curVar, prog, state, rstate, tstate, cstate, elem.vertexData, reqLevel, priority,
#if DAGOR_DBGLEVEL > 0
            (uint8_t)counter,
#endif
            instId, elem.si, elem.numf, elem.baseVertex, need_immediate_const_ps, bindposeBufferOffset);
        else
          multidrawList.emplace_back(s, curVar, prog, state, rstate, tstate, cstate, elem.vertexData, reqLevel, priority,
#if DAGOR_DBGLEVEL > 0
            (uint8_t)counter,
#endif
            instId, elem.si, elem.numf, elem.baseVertex, need_immediate_const_ps, bindposeBufferOffset);
      }
  };
  // rigids
  {
    const int currentInstanceData = instanceData.size();
    int maxRigidCount = 0, currentRigidNo = 0;
    int maxBlockNo = -1;
    for (const auto &o : lodResource->getRigidsConst())
    {
      if ((!path_filter && !scene->isNodeHidden(o.nodeId)) ||
          (path_filter && (o.nodeId >= path_filter_size || (path_filter[o.nodeId] & render_mask) == render_mask)))
      {
        const int nodeId = currentRigidNo; // or may be rigid no
        const int currentBlockNo = currentRigidNo >> 8;
        const int addDataSizeSum = (currentBlockNo + 1) * additional_data_size;
        // additional data placed between blocks of 256 (or less) rigids

        if (maxRigidCount <= nodeId)
        {
          maxRigidCount = nodeId + 1;
          maxBlockNo = currentBlockNo;
          instanceData.resize(currentInstanceData + addDataSizeSum + maxRigidCount * matrixStride);
        }
        G_ASSERT(nodeId >= 0);
        Point4 *instanceDataPtr = &instanceData[currentInstanceData + addDataSizeSum + nodeId * matrixStride];
        auto writeInst = [&](const TMatrix &wtm, int i = 0) {
          vec3f vc3 = v_ldu(&wtm.col[3].x);
          v_st(&instanceDataPtr[i + 0], v_perm_xyzd(v_ldu(&wtm.col[0].x), v_rot_1(vc3)));
          v_st(&instanceDataPtr[i + 1], v_perm_xyzd(v_ldu(&wtm.col[1].x), v_rot_2(vc3)));
          v_st(&instanceDataPtr[i + 2], v_perm_xyzd(v_ldu(&wtm.col[2].x), v_rot_3(vc3)));
        };
        writeInst(scene->getNodeWtm(o.nodeId));
        if (addPreviousMatrices)
          writeInst(scene->getPrevNodeWtm(o.nodeId), 3);
        const int dataAt = currentInstanceData + addDataSizeSum + (currentRigidNo & ~0xFF) * matrixStride;
        addMeshToList(*o.mesh->getMesh(), dataAt, 0);
      }
      currentRigidNo++;
    }
    if (additional_data_size)
      for (int blockNo = 0; blockNo <= maxBlockNo; ++blockNo)
        ::memcpy(&instanceData[currentInstanceData + blockNo * (additional_data_size + 256 * matrixStride)], bones_additional_data,
          sizeof(Point4) * additional_data_size);
  }
  // skins
  if (lodResource->getSkinsCount())
  {
    // bones is probably already somewhere in the buffer (for shadows, etc).
    // but for simplification we use it anyway
    auto skins = lodResource->getSkins();
    const int currentInstanceData = instanceData.size();
    const ShaderSkinnedMesh &skin = *skins[0]->getMesh();
    instanceData.resize(instanceData.size() + additional_data_size + ADDITIONAL_BONE_MTX_OFFSET + skin.bonesCount() * matrixStride);
    vec4f *__restrict instanceDataPtr = (vec4f *__restrict)&instanceData[currentInstanceData];

    if (additional_data_size)
      memcpy(instanceDataPtr, bones_additional_data, sizeof(Point4) * additional_data_size);
    instanceDataPtr += additional_data_size;

    if (!forceNodeCollapser && (render_mask & UpdateStageInfoRender::RenderPass::RENDER_SHADOW))
      memset(instanceDataPtr, 0, ADDITIONAL_BONE_MTX_OFFSET * sizeof(Point4));
    else
      fill_node_collapser_data(scene, eastl::span(reinterpret_cast<Point4 *__restrict>(instanceDataPtr), ADDITIONAL_BONE_MTX_OFFSET));
    instanceDataPtr += ADDITIONAL_BONE_MTX_OFFSET;

    instanceDataPtr += prepare_bones_to(instanceDataPtr, skin, *scene, addPreviousMatrices);

    auto skinNodes = lodResource->getSkinNodes();
    for (int i = 0, e = skinNodes.size(); i < e; i++)
      if ((!path_filter && !scene->isNodeHidden(skinNodes[i])) ||
          (path_filter && (skinNodes[i] >= path_filter_size || ((path_filter[skinNodes[i]]) & render_mask) == render_mask)))
        addMeshToList(skins[i]->getMesh()->getShaderMesh(), currentInstanceData + additional_data_size,
          lodResource->getBindposeBufferIndex(i));
  }
}

void DynModelRenderingState::process_animchar(uint32_t start_stage,
  uint32_t end_stage,
  const DynamicRenderableSceneInstance *scene,
  const Point4 *bones_additional_data,
  uint32_t additional_data_size,
  bool need_previous_matrices,
  ShaderElement *shader_override,
  const uint8_t *path_filter,
  uint32_t path_filter_size,
  uint8_t render_mask,
  bool need_immediate_const_ps,
  RenderPriority priority,
  const GlobalVariableStates *gvars_state,
  TexStreamingContext texCtx)
{
  const DynamicRenderableSceneResource *lodResource = scene->getCurSceneResource();
  if (lodResource)
    process_animchar(start_stage, end_stage, // inclusive range of stages
      scene, lodResource, bones_additional_data, additional_data_size, need_previous_matrices, shader_override, path_filter,
      path_filter_size, render_mask, need_immediate_const_ps, priority, gvars_state, texCtx);
}

namespace dynmodel_renderer
{
uint32_t prepare_bones_to(
  vec4f *__restrict bones, const ShaderSkinnedMesh &skin, const DynamicRenderableSceneInstance &scene, bool previous_matrices)
{
  vec4f *bonesStart = bones;
  uint32_t bonesCount = skin.bonesCount();
#ifdef _DEBUG_TAB_
  dag::ConstSpan<TMatrix> boneOrgTms(&skin.getBoneOrgTm(0), bonesCount);
  dag::ConstSpan<TMatrix> nodeWtms(&scene.getNodeWtm(0), scene.getNodeCount());
  auto boneNodeIds = skin.getBoneNodeIds();
#else
  const TMatrix *boneOrgTms = &skin.getBoneOrgTm(0);
  const TMatrix *nodeWtms = &scene.getNodeWtm(0);
  auto boneNodeIds = skin.getBoneNodeIds().data();
#endif
  for (int i = 0; i < bonesCount; i++)
  {
    mat44f orgtm, wtm, result44;
    mat43f result43;
    v_mat44_make_from_43cu_unsafe(orgtm, boneOrgTms[i][0]);
    v_mat44_make_from_43cu_unsafe(wtm, nodeWtms[boneNodeIds[i]][0]);
    v_mat44_mul43(result44, wtm, orgtm);
    v_mat44_transpose_to_mat43(result43, result44);
    *(bones++) = result43.row0;
    *(bones++) = result43.row1;
    *(bones++) = result43.row2;
    if (previous_matrices)
      bones += 3;
  }
  if (previous_matrices)
  {
    bones = bonesStart + 3;
#ifdef _DEBUG_TAB_
    dag::ConstSpan<TMatrix> prevNodeWtms(&scene.getPrevNodeWtm(0), scene.getNodeCount());
#else
    const TMatrix *prevNodeWtms = &scene.getPrevNodeWtm(0);
#endif
    for (int i = 0; i < bonesCount; i++)
    {
      mat44f orgtm, wtm, result44;
      mat43f result43;
      v_mat44_make_from_43cu_unsafe(orgtm, boneOrgTms[i][0]);
      v_mat44_make_from_43cu_unsafe(wtm, prevNodeWtms[boneNodeIds[i]][0]);
      v_mat44_mul43(result44, wtm, orgtm);
      v_mat44_transpose_to_mat43(result43, result44);
      *(bones++) = result43.row0;
      *(bones++) = result43.row1;
      *(bones++) = result43.row2;
      bones += 3;
    }
  }
  return bonesCount * (previous_matrices ? 6 : 3);
}
} // namespace dynmodel_renderer

void DynModelRenderingState::coalesceDrawcalls()
{
  TIME_D3D_PROFILE(dyn_model_coalesce_drawcalls);

  const auto mergeComparator = [](const RenderElement &a, const RenderElement &b) -> bool {
    return a.vData == b.vData && a.priority == b.priority && a.rstate == b.rstate &&
           get_material_id(a.cstate) == get_material_id(b.cstate) && a.prog == b.prog;
  };

  drawcallRanges.push_back(PackedDrawCallsRange{0, 1});
  bindlessStatesToUpdateTexLevels.emplace(multidrawList[0].cstate, multidrawList[0].reqTexLevel);

  for (uint32_t i = 1, ie = multidrawList.size(); i < ie; ++i)
  {
    const auto &currentRelem = multidrawList[i];
    const bool mergeWithPrevious = mergeComparator(currentRelem, multidrawList[i - 1]);
    if (mergeWithPrevious)
      drawcallRanges.back().count++;
    else
      drawcallRanges.push_back(PackedDrawCallsRange{drawcallRanges.back().count + drawcallRanges.back().start, 1});
    auto iter = bindlessStatesToUpdateTexLevels.find(currentRelem.cstate);
    if (iter == bindlessStatesToUpdateTexLevels.end())
      bindlessStatesToUpdateTexLevels.emplace(currentRelem.cstate, currentRelem.reqTexLevel);
    else
      iter->second = max(iter->second, currentRelem.reqTexLevel);
  }
}

void DynModelRenderingState::prepareForRender()
{
  {
    TIME_PROFILE(sort_dynm);
    stlsort::sort(list.begin(), list.end(), [&](const RenderElement &a, const RenderElement &b) {
      if (a.priority != b.priority)
        return a.priority < b.priority;
      if (a.rstate != b.rstate)
        return a.rstate < b.rstate;
      if (a.tstate != b.tstate) // maybe split state into sampler state (heavy) and const buffer (cheap)?
        return a.tstate < b.tstate;
      if (a.vData != b.vData)
        return (uintptr_t)a.vData < (uintptr_t)b.vData;
      if (a.prog != b.prog)
        return a.prog < b.prog;

      if (a.state != b.state) // maybe split state into sampler state (heavy) and const buffer (cheap)?
        return a.state < b.state;
      if (a.instanceId != b.instanceId)
        return a.instanceId < b.instanceId;
      if (a.bindposeBufferOffset != b.bindposeBufferOffset)
        return a.bindposeBufferOffset < b.bindposeBufferOffset;
      if (a.bv != b.bv)
        return a.bv < b.bv;
      if (a.si != b.si)
        return a.si < b.si;
      return false;
    });
  }
  if (multidrawList.empty())
    return;
  {
    TIME_PROFILE(sort_packed_dynm);
    stlsort::sort(multidrawList.begin(), multidrawList.end(), [&](const RenderElement &a, const RenderElement &b) {
      if (get_material_id(a.cstate) != get_material_id(b.cstate))
        return get_material_id(a.cstate) < get_material_id(b.cstate);
      if (a.priority != b.priority)
        return a.priority < b.priority;
      if (a.rstate != b.rstate)
        return a.rstate < b.rstate;
      // TODO: split it on vbuffer/vstride comparator, because different vdata ofthen uses the same buffers
      if (a.vData != b.vData)
        return (uintptr_t)a.vData < (uintptr_t)b.vData;
      if (a.prog != b.prog)
        return a.prog < b.prog;

      if (a.state != b.state)
        return a.state < b.state;
      if (a.bindposeBufferOffset != b.bindposeBufferOffset)
        return a.bindposeBufferOffset < b.bindposeBufferOffset;
      return false;
    });
  }
  coalesceDrawcalls();
}

void DynModelRenderingState::updateDataForPackedRender() const
{
  if (bindlessStatesToUpdateTexLevels.empty())
    return;

  TIME_D3D_PROFILE(dyn_models_update_bindless_data);
  for (auto stateIdTexLevel : bindlessStatesToUpdateTexLevels)
    update_bindless_state(stateIdTexLevel.first, stateIdTexLevel.second);
}

void DynModelRenderingState::setVars(D3DRESID buf_id) const
{
  ShaderGlobal::set_int(matrices_strideVarId, matrixStride);
  ShaderGlobal::set_buffer(instance_data_bufferVarId, buf_id);
}

void DynModelRenderingState::render_no_packed(int offset) const
{
  if (list.empty())
    return;

  TIME_D3D_PROFILE(render_dynm);
#if DAGOR_DBGLEVEL > 0
  if (d3d::get_driver_desc().caps.hasWellSupportedIndirect && d3d::get_driver_desc().caps.hasBindless)
    LOGWARN_ONCE("Shader \"%s\" doesn't use multidraw indirect!", list.front().curShader->getShaderClassName());
#endif
  G_ASSERTF(ShaderGlobal::get_int(matrices_strideVarId) == matrixStride, "matrices stride should be set correctly");
  GlobalVertexData *vdata = NULL;
  int instanceId = -1;
  int bindposeBufferOffset = -1;
  bool needPsImmConst = false;
  for (auto rliI = list.begin(), e = list.end(); rliI != e;)
  {
    auto &rli = *rliI;
    int currentDipFaces = rli.numf;
    G_FAST_ASSERT(currentDipFaces);
    // begin of collapse all DIPS with pseudo bones
    const int nextInd = rli.si + rli.numf * 3;
    for (++rliI; rliI != e; rliI++)
    {
      auto &rlj = *rliI;
      if (rli.instanceId == rlj.instanceId && rli.bindposeBufferOffset == rlj.bindposeBufferOffset && rli.curVar == rlj.curVar &&
          rli.state == rlj.state && rli.prog == rlj.prog && rli.bv == rlj.bv && rli.vData == rlj.vData &&
          rli.curShader == rlj.curShader && rlj.si == nextInd)
      {
        currentDipFaces += rlj.numf;
      }
      else
        break;
    }
    // end of collapse all DIPS with pseudo bones

    debug_mesh::set_debug_value(rli.lodNo);

    const bool instanceIdChanged = instanceId != rli.instanceId || bindposeBufferOffset != rli.bindposeBufferOffset;
    const bool psImmChanged = rli.needImmediateConstPS != needPsImmConst;
    if (instanceIdChanged || psImmChanged)
    {
      instanceId = rli.instanceId;
      bindposeBufferOffset = rli.bindposeBufferOffset;
      needPsImmConst = rli.needImmediateConstPS;

      constexpr int immediateConstCount = 2;
      const uint32_t immediateConsts[immediateConstCount] = {(uint32_t)(offset + instanceId), (uint32_t)rli.bindposeBufferOffset * 3};

      if (instanceIdChanged)
        d3d::set_immediate_const(STAGE_VS, immediateConsts, immediateConstCount);

      if (rli.needImmediateConstPS)
        d3d::set_immediate_const(STAGE_PS, immediateConsts, immediateConstCount);
      else if (psImmChanged)
        d3d::set_immediate_const(STAGE_PS, nullptr, 0);
    }
    rli.curShader->setReqTexLevel(rli.reqTexLevel);
    set_states_for_variant(rli.curShader->native(), rli.curVar, rli.prog, rli.state);

    // workaround: vertex data can be empty during geom reloading
    if (!rli.vData->isEmpty())
    {
      if (vdata != rli.vData)
        (vdata = rli.vData)->setToDriver();
      d3d::drawind(PRIM_TRILIST, rli.si, currentDipFaces, rli.bv);
    }
    debug_mesh::reset_debug_value();
  }
  d3d::set_immediate_const(STAGE_VS, nullptr, 0);
  d3d::set_immediate_const(STAGE_PS, nullptr, 0);
}

void DynModelRenderingState::render_multidraw(int offset) const
{
  if (multidrawList.empty())
    return;
  TIME_D3D_PROFILE(render_dynm_indirect);

  const auto multiDrawRenderer = dynmodel_rendering.multidrawContext.fillBuffers(multidrawList.size(),
    [this](uint32_t drawcallId, uint32_t &indexCountPerInstance, uint32_t &instanceCount, uint32_t &startIndexLocation,
      int32_t &baseVertexLocation, uint32_t &perDrawData) {
      const auto &currentRelem = multidrawList[drawcallId];
      indexCountPerInstance = currentRelem.numf * 3;
      instanceCount = 1;
      startIndexLocation = currentRelem.si;
      baseVertexLocation = currentRelem.bv;
      const uint32_t instanceOffset = currentRelem.instanceId;
      if (DAGOR_UNLIKELY(instanceOffset >= MAX_MATRIX_OFFSET))
      {
        logerr("Too big offset in instance matrix buffer %d.", instanceOffset);
        instanceCount = 0;
      }
      const uint32_t materialOffset = get_material_offset(currentRelem.cstate);
      if (DAGOR_UNLIKELY(materialOffset >= MAX_MATERIAL_OFFSET))
      {
        logerr("Too big material offset %d.", materialOffset);
        instanceCount = 0;
      }
      perDrawData = (instanceOffset << MATERIAL_OFFSET_BITS) | materialOffset;
    });

  G_ASSERTF(ShaderGlobal::get_int(matrices_strideVarId) == matrixStride, "matrices stride should be set correctly");
  GlobalVertexData *vdata = NULL;
  const uint32_t immediateConst = offset;
  d3d::set_immediate_const(STAGE_VS, &immediateConst, 1);
  d3d::set_immediate_const(STAGE_PS, &immediateConst, 1);
  for (const auto &dcParams : drawcallRanges)
  {
    auto &rli = multidrawList[dcParams.start];

    debug_mesh::set_debug_value(rli.lodNo); // TODO: Support show_gbuffer lods for multidraw
    set_states_for_variant(rli.curShader->native(), rli.curVar, rli.prog, rli.state);

    // workaround: vertex data can be empty during geom reloading
    if (!rli.vData->isEmpty())
    {
      if (vdata != rli.vData)
        (vdata = rli.vData)->setToDriver();
      multiDrawRenderer.render(PRIM_TRILIST, dcParams.start, dcParams.count);
    }
    debug_mesh::reset_debug_value();
  }
  d3d::set_immediate_const(STAGE_VS, nullptr, 0);
  d3d::set_immediate_const(STAGE_PS, nullptr, 0);
}

void DynModelRenderingState::render(int offset) const
{
  render_no_packed(offset);
  render_multidraw(offset);
}

void DynModelRenderingState::swap(DynModelRenderingState &a)
{
  list.swap(a.list);
  multidrawList.swap(a.multidrawList);
  instanceData.swap(a.instanceData);
  eastl::swap(matrixStride, a.matrixStride);
}

void DynModelRenderingState::addStateFrom(DynModelRenderingState &&s)
{
  TIME_PROFILE_DEV(add_state_dynm);
  if (s.empty())
    return;
  if (empty())
    return swap(s);

  G_ASSERT(s.matrixStride == matrixStride);

  const uint32_t instanceAt = instanceData.size();
  instanceData.insert(instanceData.end(), s.instanceData.begin(), s.instanceData.end());

  {
    const uint32_t at = list.size();
    list.insert(list.end(), s.list.begin(), s.list.end());
    for (uint32_t i = at, e = list.size(), j = 0; i < e; ++i, ++j)
      list[i].instanceId += instanceAt;
  }
  {
    const uint32_t at = multidrawList.size();
    multidrawList.insert(multidrawList.end(), s.multidrawList.begin(), s.multidrawList.end());
    for (uint32_t i = at, e = multidrawList.size(), j = 0; i < e; ++i, ++j)
      multidrawList[i].instanceId += instanceAt;
  }
  s.clear();
}

const DynModelRenderingState *DynModelRendering::get(const char *name) const
{
  auto it = map.find_as(name);
  return (it != map.end()) ? &it->second : NULL;
}

namespace dynmodel_renderer
{
void init_dynmodel_rendering(int csm_num_cascades)
{
  for (int i = 0; i < BufferType::SHADOWS_CSM + csm_num_cascades; ++i)
    dynmodel_rendering.dynamicBuffers.emplace_back();
  dynmodel_rendering.supportNoOverwrite = d3d::get_driver_desc().caps.hasNoOverwriteOnShaderResourceBuffers;
}

void close_dynmodel_rendering() { dynmodel_rendering.clear(); }

DynModelRenderingState *create_state(const char *name)
{
  auto ret = dynmodel_rendering.emplace(name ? name : "");
  ret->clear();
  return ret;
}

const DynModelRenderingState *get_state(const char *name) { return dynmodel_rendering.get(name); }

static const DynamicBufferHolder *update_dynmodel_buffer(dag::ConstSpan<Point4> inst_data, int buf_idx)
{
  G_ASSERT(!dynmodel_rendering.dynamicBuffers.empty());
  // todo: this has to be remade with one big ring buffer with fences.
  // however, on windows7 there is no NOOOVERWRITE, so we still use discard concept
  // it is also totally possible to schedule EVERYTHING we need for rendering first and then use ONE DISCARD (in one buffer)
  // it would be probably faster as well.
  // todo do it next
  // one (small) buffer is probably needed for animchar rendering out-of-regular order
  DynamicBufferHolder &holder = dynmodel_rendering.dynamicBuffers[buf_idx];
  if (inst_data.size() > holder.buffer.bufSize())
  {
    String name(64, "all_instances%d", buf_idx);
    const uint32_t alignSize = dynmodel_rendering.supportNoOverwrite ? 4095 : 255;
    // align to 64kb if supports nooverwrite and to 4096 otherwise
    const uint32_t newSize = ((inst_data.size() + holder.buffer.getPos()) + alignSize) & ~alignSize;
    bool useStructuredBuffer = ShaderGlobal::get_interval_current_value(small_sampled_buffersVarId);
    uint32_t strFlags = (useStructuredBuffer ? SBCF_MISC_STRUCTURED : 0) | SBCF_BIND_SHADER_RES;
    uint32_t format = useStructuredBuffer ? 0 : TEXFMT_A32B32G32R32F;
    const uint32_t structSize = sizeof(Point4);
    holder.buffer.init(newSize, structSize, structSize, strFlags, format, name.c_str());
  }
  holder.curOffset = holder.buffer.addData(inst_data.data(), inst_data.size());
  if (holder.curOffset == -1)
    return nullptr;
  return &holder;
}

const DynamicBufferHolder *DynModelRenderingState::requestBuffer(BufferType type) const
{
  if (empty())
    return nullptr;
  TIME_D3D_PROFILE(update_dynmodel_buffer);
  updateDataForPackedRender();
  return update_dynmodel_buffer(instanceData, type);
}
} // namespace dynmodel_renderer
