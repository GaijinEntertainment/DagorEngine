// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <ecs/render/updateStageRender.h>
#include <shaders/dag_dynSceneRes.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physObject.h>
#include <gamePhys/phys/destructableObject.h>
#include <gamePhys/phys/destructableRendObject.h>
#include <shaders/dag_shaderBlock.h>
#include <perfMon/dag_statDrv.h>
#include <scene/dag_occlusion.h>
#include <gamePhys/collision/collisionLib.h>
#include <math/dag_mathUtils.h>
#include <render/renderEvent.h>
#include <render/world/wrDispatcher.h>

#include "global_vars.h"
#include "dynModelRenderer.h"

extern ShaderBlockIdHolder dynamicSceneTransBlockId, dynamicSceneBlockId, dynamicDepthSceneBlockId;
using namespace dynmodel_renderer;

static bool has_destr_objects_with_disintegration_animation()
{
  auto destrObjects = destructables::getDestructableObjects();
  return eastl::find_if(destrObjects.begin(), destrObjects.end(),
           [](const auto &destr) { return destr->isAlive() && destr->hasDisintegrationAnimation(); }) != destrObjects.end();
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static __forceinline void destructables_before_render_es(const UpdateStageInfoBeforeRender &stg)
{
  for (const auto destr : destructables::getDestructableObjects())
  {
    if (!destr->isAlive())
      continue;
    for (int model = 0, modelCount = destr->physObj->getModelCount(); model < modelCount; ++model)
      destr->physObj->getModel(model)->savePrevNodeWtm();

    destr->physObj->beforeRender(stg.camPos);
    for (int model = 0, modelCount = destr->physObj->getModelCount(); model < modelCount; ++model)
    {
      DynamicRenderableSceneInstance *scene = destr->physObj->getModel(model);
      for (int i = 0, e = scene->getNodeCount(); i < e; i++)
      {
        TMatrix tm = scene->getNodeWtm(i);
        tm.setcol(3, tm.getcol(3) - stg.camPos);
        // we can increase precise with roundedCamPos and remainderCamPos, but not sure that
        // it is really needed for destructables
        scene->setNodeWtm(i, tm);
      }
    }
  }
}

enum class DestructablesRenderStage
{
  OPAQUE,
  DECALS,
  TRANSPARENT
};

static __forceinline void destructables_render(int render_pass,
  DestructablesRenderStage render_stage,
  bool to_depth,
  const Point3 &cam_pos,
  const Frustum &frustum,
  const Occlusion *occlusion,
  const TexStreamingContext &texCtx)
{
  DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
  uint32_t startStage = 0, endStage = 0;
  if (render_stage == DestructablesRenderStage::OPAQUE)
  {
    startStage = ShaderMesh::STG_opaque;
    endStage = ShaderMesh::STG_imm_decal;
  }
  else if (render_stage == DestructablesRenderStage::DECALS)
    startStage = endStage = ShaderMesh::STG_decal;
  else if (render_stage == DestructablesRenderStage::TRANSPARENT)
    startStage = endStage = ShaderMesh::STG_trans;
  const auto needPreviousMatrices = ((render_stage == DestructablesRenderStage::OPAQUE) && !to_depth)
                                      ? dynmodel_renderer::NeedPreviousMatrices::Yes
                                      : dynmodel_renderer::NeedPreviousMatrices::No;
  vec3f vCamPos = v_ldu(&cam_pos.x);
  for (const auto destr : destructables::getDestructableObjects())
  {
    if (!destr->physObj || !destr->isAlive())
      continue;
    for (int model = 0, modelCount = destr->physObj->getModelCount(); model < modelCount; ++model)
    {
      DynamicRenderableSceneInstance *modelDynScene = destr->physObj->getModel(model);
      const DynamicRenderableSceneResource *lodResource = modelDynScene->getCurSceneResource();
      if (!lodResource)
        continue;

      bbox3f vModelBbox;
      v_bbox3_init_empty(vModelBbox);
      BBox3 localBbox = modelDynScene->getLocalBoundingBox();
      // For some reason 1st and 2nd columns of destructed ri node tms are swapped relative to the original ri.
      // But local bbox doesn't match that swap. So fix it here.
      std::swap(localBbox.lim[0].y, localBbox.lim[0].z);
      std::swap(localBbox.lim[1].y, localBbox.lim[1].z);
      bbox3f vLocalBbox = v_ldu_bbox3(localBbox);

      auto addDestrNodeToBbox = [&](int node_id) {
        if (modelDynScene->isNodeHidden(node_id))
          return;

        mat44f vNodeTm;
        v_mat44_make_from_43cu_unsafe(vNodeTm, modelDynScene->getNodeWtm(node_id).array);

        bbox3f vNodeBbox;
        v_bbox3_init(vNodeBbox, vNodeTm, vLocalBbox);

        // Check middle point on top of the bbox.
        vec3f vCheckPos = v_add(v_perm_xbzw(v_bbox3_center(vNodeBbox), vNodeBbox.bmax), vCamPos);
        if (dacoll::traceht_lmesh(Point2(v_extract_x(vCheckPos), v_extract_z(vCheckPos))) > v_extract_y(vCheckPos))
        {
          modelDynScene->showNode(node_id, false);
          return;
        }

        v_bbox3_add_box(vModelBbox, vNodeBbox);
      };

      for (const auto &rigid : lodResource->getRigidsConst())
        addDestrNodeToBbox(rigid.nodeId);
      for (int skinNodeId : lodResource->getSkinNodes())
        addDestrNodeToBbox(skinNodeId);

      bbox3f boxCull;
      boxCull.bmin = v_add(vModelBbox.bmin, vCamPos);
      boxCull.bmax = v_add(vModelBbox.bmax, vCamPos);
      if (!frustum.testBoxB(boxCull.bmin, boxCull.bmax) || (occlusion && !occlusion->isVisibleBox(boxCull)))
        continue;

      ecs::Point4List additionalData;
      additionalData.reserve(5);
      int initialTmHashvalPos = animchar_additional_data::request_space<AAD_RAW_INITIAL_TM__HASHVAL>(additionalData, 4);
      for (int i = 0; i < 4; ++i)
        additionalData[initialTmHashvalPos + i] = destr->intialTmAndHash[i];
      int destrParamsPos = animchar_additional_data::request_space<AAD_RAW_DESTR_PARAMS>(additionalData, 1);
      additionalData[destrParamsPos] = destr->getDisintegrationParams();
      auto additionalDataView = animchar_additional_data::AnimcharAdditionalDataView::get_optional_data(&additionalData);
      state.process_animchar(startStage, endStage, modelDynScene, additionalDataView, needPreviousMatrices, {},
        dynmodel_renderer::PathFilterView::NULL_FILTER, 0, RenderPriority::HIGH, nullptr, texCtx);
    }
  }

  bool transparent = render_stage == DestructablesRenderStage::TRANSPARENT;
  state.prepareForRender();
  const DynamicBufferHolder *buffer = state.requestBuffer(
    transparent ? dynmodel_renderer::BufferType::TRANSPARENT_MAIN : dynmodel_renderer::get_buffer_type_from_render_pass(render_pass));
  if (!buffer)
    return;

  const int block = to_depth ? dynamicDepthSceneBlockId : (transparent ? dynamicSceneTransBlockId : dynamicSceneBlockId);
  state.setVars(buffer->buffer.getBufId());
  SCENE_LAYER_GUARD(block);
  state.render(buffer->curOffset);
}

void destructables::render_depth_prepass(
  const Point3 &main_cam_pos, const TMatrix &view_tm, const Frustum &culling_frustum, Occlusion *occlusion)
{
  TIME_D3D_PROFILE(destructables_render_prepass);
  if (!has_destr_objects_with_disintegration_animation())
    return;

  TMatrix vtm = view_tm;
  vtm.setcol(3, 0, 0, 0);

  d3d::settm(TM_VIEW, vtm);
  {
    STATE_GUARD_0(ShaderGlobal::set_int(enable_ri_disintegration_animationVarId, VALUE), 1);
    destructables_render(RENDER_MAIN, DestructablesRenderStage::OPAQUE, true, main_cam_pos, culling_frustum, occlusion,
      TEX_STREAMING_CTX_NULL);
  }
  d3d::settm(TM_VIEW, view_tm);
}

ECS_TAG(render)
static __forceinline void destructables_render_es(const UpdateStageInfoRender &stg)
{
  TIME_D3D_PROFILE(destructables_render);
  bool hadPrepass =
    stg.renderPass == RENDER_MAIN && WRDispatcher::usesDepthPrepass() && has_destr_objects_with_disintegration_animation();

  if (hadPrepass)
    shaders::overrides::set(WRDispatcher::getCommonOverrideStates().zFuncEqualStateId);

  TMatrix vtm = stg.viewTm;
  if (stg.renderPass == RENDER_MAIN || stg.renderPass >= RENDER_SHADOWS_CSM)
  {
    vtm.setcol(3, 0, 0, 0);
  }
  else
  {
    TMatrix itm = stg.viewItm;
    itm.setcol(3, itm.getcol(3) - stg.mainCamPos);
    vtm = inverse(itm);
  }
  d3d::settm(TM_VIEW, vtm);
  {
    STATE_GUARD_0(ShaderGlobal::set_int(enable_ri_disintegration_animationVarId, VALUE), 1);
    destructables_render(stg.renderPass, DestructablesRenderStage::OPAQUE, !(stg.hints & UpdateStageInfoRender::RENDER_COLOR),
      stg.mainCamPos, stg.cullingFrustum, stg.occlusion, stg.texCtx);
  }
  d3d::settm(TM_VIEW, stg.viewTm);

  if (hadPrepass)
    shaders::overrides::reset();
}

ECS_TAG(render)
static void destructables_render_decals_es(const RenderDecalsOnDynamic &stg)
{
  TIME_D3D_PROFILE(destructables_render_decals);
  TMatrix vtm = stg.viewTm;
  vtm.setcol(3, 0, 0, 0);
  d3d::settm(TM_VIEW, vtm);
  destructables_render(RENDER_MAIN, DestructablesRenderStage::DECALS, false, stg.mainCamPos, stg.cullingFrustum, stg.occlusion,
    stg.texCtx);
  d3d::settm(TM_VIEW, stg.viewTm);
}

ECS_TAG(render)
static __forceinline void destructables_render_trans_es(const UpdateStageInfoRenderTrans &stg)
{
  TIME_D3D_PROFILE(destructables_render_trans);
  TMatrix vtm = stg.viewTm;
  vtm.setcol(3, 0, 0, 0);
  d3d::settm(TM_VIEW, vtm);
  destructables_render(RENDER_MAIN, DestructablesRenderStage::TRANSPARENT, false, stg.viewItm.getcol(3), stg.loadGlobTm(),
    stg.occlusion, stg.texCtx);
  d3d::settm(TM_VIEW, stg.viewTm);
}

destructables::DestrRendData *destructables::init_rend_data(DynamicPhysObjectClass<PhysWorld> *, bool) { return nullptr; }
void destructables::clear_rend_data(destructables::DestrRendData *) {}
void destructables::DestrRendDataDeleter::operator()(destructables::DestrRendData *) {}

ECS_TAG(render)
static void bvh_destructables_iterate_es(BVHAdditionalAnimcharIterate &event)
{
  for (const auto destr : destructables::getDestructableObjects())
  {
    if (!destr->physObj || !destr->isAlive())
      continue;
    if (destr->getDisintegrationParams().x > 0.5) // Half desintegrated
      continue;
    for (int model = 0, modelCount = destr->physObj->getModelCount(); model < modelCount; ++model)
    {
      DynamicRenderableSceneInstance *modelDynScene = destr->physObj->getModel(model);
      DynamicRenderableSceneResource *lodResource = modelDynScene->getCurSceneResource();
      if (!lodResource)
        continue;

      auto additionalData =
        animchar_additional_data::prepare_fixed_space<AAD_RAW_INITIAL_TM__HASHVAL>(make_span_const(destr->intialTmAndHash));

      event.get<0>()({}, modelDynScene, lodResource, additionalData, VISFLG_BVH);
    }
  }
};