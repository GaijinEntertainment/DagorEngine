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
#include <ska_hash_map/flat_hash_map2.hpp>

#include "global_vars.h"
#include <render/dynmodelRenderer.h>

extern ShaderBlockIdHolder dynamicTransSceneBlockId, dynamicSceneBlockId, dynamicDepthSceneBlockId;
using namespace dynrend;

static bool has_destr_objects_with_disintegration_animation()
{
  auto destrObjects = destructables::getDestructableObjects();
  return eastl::find_if(destrObjects.begin(), destrObjects.end(),
           [](const auto &destr) { return destr->isAlive() && destr->hasDisintegrationAnimation(); }) != destrObjects.end();
}

// Merged visible-node bboxes (camera-relative) are frame-invariant across render passes,
// so they are computed once per frame here and only looked up in the render passes.
static ska::flat_hash_map<const DynamicRenderableSceneInstance *, BBox3> destr_model_bboxes;

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static __forceinline void destructables_before_render_es(const UpdateStageInfoBeforeRender &stg)
{
  destr_model_bboxes.clear();
  vec3f vCamPos = v_ldu(&stg.camPos.x);
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

      const DynamicRenderableSceneResource *lodResource = scene->getCurSceneResource();
      if (!lodResource)
        continue;

      BBox3 localBbox = scene->getLocalBoundingBox();
      // For some reason 1st and 2nd columns of destructed ri node tms are swapped relative to the original ri.
      // But local bbox doesn't match that swap. So fix it here.
      std::swap(localBbox.lim[0].y, localBbox.lim[0].z);
      std::swap(localBbox.lim[1].y, localBbox.lim[1].z);
      bbox3f vLocalBbox = v_ldu_bbox3(localBbox);

      bbox3f vModelBbox;
      v_bbox3_init_empty(vModelBbox);
      auto addDestrNodeToBbox = [&](int node_id) {
        if (scene->isNodeHidden(node_id))
          return;

        mat44f vNodeTm;
        v_mat44_make_from_43cu_unsafe(vNodeTm, scene->getNodeWtm(node_id).array);

        bbox3f vNodeBbox;
        v_bbox3_init(vNodeBbox, vNodeTm, vLocalBbox);

        // Check middle point on top of the bbox.
        vec3f vCheckPos = v_add(v_perm_xbzw(v_bbox3_center(vNodeBbox), vNodeBbox.bmax), vCamPos);
        if (dacoll::traceht_lmesh(Point2(v_extract_x(vCheckPos), v_extract_z(vCheckPos))) > v_extract_y(vCheckPos))
        {
          scene->showNode(node_id, false);
          return;
        }

        v_bbox3_add_box(vModelBbox, vNodeBbox);
      };

      for (const auto &rigid : lodResource->getRigidsConst())
        addDestrNodeToBbox(rigid.nodeId);
      for (int skinNodeId : lodResource->getSkinNodes())
        addDestrNodeToBbox(skinNodeId);

      BBox3 modelBbox;
      v_stu_bbox3(modelBbox, vModelBbox);
      destr_model_bboxes[scene] = modelBbox;
    }
  }
}

enum class DestructablesRenderStage
{
  OPAQUE,
  DECALS,
  TRANSPARENT
};

static __forceinline void destructables_render(int /*render_pass*/,
  DestructablesRenderStage render_stage,
  bool to_depth,
  const TMatrix &view_tm,
  const TMatrix4 &proj_tm,
  const Point3 &cam_pos,
  const Frustum &frustum,
  const Occlusion *occlusion,
  const TexStreamingContext &texCtx)
{
  ContextId ctx = get_or_create_context("dynmodel_immediate");

  TMatrix4_vec4 prevView, prevProj;
  get_prev_view_proj(prevView, prevProj);
  prevView.setrow(3, 0.f, 0.f, 0.f, 1.f);
  set_context_view_proj(ctx, TMatrix4(view_tm), proj_tm, prevView, prevProj);

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
  const auto needPreviousMatrices =
    ((render_stage == DestructablesRenderStage::OPAQUE) && !to_depth) ? NeedPreviousMatrices::Yes : NeedPreviousMatrices::No;
  vec3f vCamPos = v_ldu(&cam_pos.x);
  dag::Vector<Point4, framemem_allocator> additionalData;
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

      // destructables_before_render_es caches a bbox for every alive model with a lodResource,
      // so a miss here is a logic inconsistency (a model would silently vanish), not normal
      auto cachedBbox = destr_model_bboxes.find(modelDynScene);
      G_ASSERT_CONTINUE(cachedBbox != destr_model_bboxes.end());

      bbox3f boxCull = v_ldu_bbox3(cachedBbox->second);
      boxCull.bmin = v_add(boxCull.bmin, vCamPos);
      boxCull.bmax = v_add(boxCull.bmax, vCamPos);
      if (!frustum.testBoxB(boxCull.bmin, boxCull.bmax) || (occlusion && !occlusion->isVisibleBox(boxCull)))
        continue;

      additionalData.clear();
      additionalData.reserve(7); // 5 payload + 2 metadata elements
      int initialTmHashvalPos = animchar_additional_data::request_space<AAD_RAW_INITIAL_TM__HASHVAL>(additionalData, 4);
      for (int i = 0; i < 4; ++i)
        additionalData[initialTmHashvalPos + i] = destr->intialTmAndHash[i];
      int destrParamsPos = animchar_additional_data::request_space<AAD_RAW_DESTR_PARAMS>(additionalData, 1);
      additionalData[destrParamsPos] = destr->getDisintegrationParams();
      const auto additionalDataView = animchar_additional_data::AnimcharAdditionalDataView::get_optional_data(&additionalData);
      add_animchar(ctx, startStage, endStage, modelDynScene, additionalDataView, needPreviousMatrices, {}, PathFilterView::NULL_FILTER,
        0, RenderPriority::HIGH, nullptr, texCtx);
    }
  }

  if (!prepare_render_current(ctx))
    return;

  bool transparent = render_stage == DestructablesRenderStage::TRANSPARENT;
  const int block = to_depth ? dynamicDepthSceneBlockId : (transparent ? dynamicTransSceneBlockId : dynamicSceneBlockId);
  SCENE_LAYER_GUARD(block);
  render_all_stages(ctx);
}

ECS_TAG(render)
static __forceinline void destructables_depth_prepass_es(const animchar_disintegration::RenderDisintegrationDepthPrepassEvent &event)
{
  TIME_D3D_PROFILE(destructables_render_prepass);
  if (!has_destr_objects_with_disintegration_animation())
    return;

  TMatrix vtm = event.viewTm;
  vtm.setcol(3, 0, 0, 0);

  d3d::settm(TM_VIEW, vtm);
  {
    STATE_GUARD_0(ShaderGlobal::set_int(enable_ri_disintegration_animationVarId, VALUE), 1);
    destructables_render(RENDER_MAIN, DestructablesRenderStage::OPAQUE, true, vtm, event.projTm, event.viewItm.getcol(3),
      event.cullingFrustum, event.occlusion, event.texCtx);
  }
  d3d::settm(TM_VIEW, event.viewTm);
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
    destructables_render(stg.renderPass, DestructablesRenderStage::OPAQUE, !(stg.hints & UpdateStageInfoRender::RENDER_COLOR), vtm,
      stg.projTm, stg.mainCamPos, stg.cullingFrustum, stg.occlusion, stg.texCtx);
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
  destructables_render(RENDER_MAIN, DestructablesRenderStage::DECALS, false, vtm, stg.projTm, stg.mainCamPos, stg.cullingFrustum,
    stg.occlusion, stg.texCtx);
  d3d::settm(TM_VIEW, stg.viewTm);
}

ECS_TAG(render)
static __forceinline void destructables_render_trans_es(const UpdateStageInfoRenderTrans &stg)
{
  TIME_D3D_PROFILE(destructables_render_trans);
  TMatrix vtm = stg.viewTm;
  vtm.setcol(3, 0, 0, 0);
  d3d::settm(TM_VIEW, vtm);
  destructables_render(RENDER_MAIN, DestructablesRenderStage::TRANSPARENT, false, vtm, stg.projTm, stg.viewItm.getcol(3),
    stg.loadGlobTm(), stg.occlusion, stg.texCtx);
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