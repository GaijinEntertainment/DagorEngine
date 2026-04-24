// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1

#include "dynamicMirrorRenderer.h"
#include "dynamicMirrorNodes.h"

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <render/daFrameGraph/nodeHandle.h>
#include <ecs/render/updateStageRender.h>
#include <animChar/dag_animCharacter2.h>
#include <render/world/cameraParams.h>
#include <render/dag_cur_view.h>
#include <render/world/gbufferConsts.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/renderer.h>
#include <ecs/anim/anim.h>
#include <render/world/animCharRenderUtil.h>
#include <render/world/private_worldRenderer.h>


ECS_REGISTER_BOXED_TYPE(DynamicMirrorRenderer, nullptr);

template <typename Callable>
static void get_dynamic_mirror_render_ecs_query(ecs::EntityManager &manager, Callable c);
template <typename Callable>
static void get_dynamic_mirror_ecs_query(ecs::EntityManager &manager, Callable c);

static bool init_dynamic_mirrors_on_demand(DynamicMirrorRenderer &dynamic_mirror_renderer,
  dafg::NodeHandle &dynamic_mirror_prepare_node,
  dafg::NodeHandle &dynamic_mirror_prepass_node,
  dafg::NodeHandle &dynamic_mirror_end_prepass_node,
  dafg::NodeHandle &dynamic_mirror_render_dynamic_node,
  dafg::NodeHandle &dynamic_mirror_render_static_node,
  dafg::NodeHandle &dynamic_mirror_render_ground_node,
  dafg::NodeHandle &dynamic_mirror_resolve_gbuf_node,
  dafg::NodeHandle &dynamic_mirror_resolve_node,
  dafg::NodeHandle &dynamic_mirror_envi_node)
{
  auto worldRenderer = static_cast<WorldRenderer *>(get_world_renderer());
  if (worldRenderer == nullptr)
    return false;
  if (dynamic_mirror_renderer.isInitialized())
    return true;
  uint32_t gbuf_fmts[NO_MOTION_GBUFFER_RT_COUNT];
  uint32_t gbuf_cnt = 0;
  const char *resolveShader = nullptr;
  if (!worldRenderer->isThinGBuffer())
  {
    resolveShader = NO_MOTION_GBUFFER_RESOLVE_SHADER;
    gbuf_cnt = NO_MOTION_GBUFFER_RT_COUNT;
    for (uint32_t i = 0; i < gbuf_cnt; ++i)
      gbuf_fmts[i] = FULL_GBUFFER_FORMATS[i];
  }
  else
  {
    resolveShader = THIN_GBUFFER_RESOLVE_SHADER;
    gbuf_cnt = THIN_GBUFFER_RT_COUNT;
    for (uint32_t i = 0; i < gbuf_cnt; ++i)
      gbuf_fmts[i] = THIN_GBUFFER_RT_FORMATS[i];
  }

  dynamic_mirror_renderer.init();
  dynamic_mirror_prepare_node = create_dynamic_mirror_prepare_node(dynamic_mirror_renderer, 0, gbuf_cnt, gbuf_fmts, TEXFMT_DEPTH32);
  dynamic_mirror_prepass_node = create_dynamic_mirror_prepass_node(dynamic_mirror_renderer);
  dynamic_mirror_end_prepass_node = create_dynamic_mirror_end_prepass_node();
  dynamic_mirror_render_dynamic_node = create_dynamic_mirror_render_dynamic_node();
  dynamic_mirror_render_static_node = create_dynamic_mirror_render_static_node(dynamic_mirror_renderer);
  dynamic_mirror_render_ground_node = create_dynamic_mirror_render_ground_node();
  dynamic_mirror_resolve_gbuf_node = create_dynamic_mirror_gbuf_resolve_node(resolveShader);
  dynamic_mirror_resolve_node = create_dynamic_mirror_resolve_node(dynamic_mirror_renderer);
  dynamic_mirror_envi_node = create_dynamic_mirror_envi_node(dynamic_mirror_renderer);
  return true;
}

ECS_TAG(render)
ECS_BEFORE(animchar_before_render_es)
static void prepare_mirror_es(const UpdateStageInfoBeforeRender &event,
  ecs::EntityManager &manager,
  DynamicMirrorRenderer &dynamic_mirror_renderer,
  dafg::NodeHandle &dynamic_mirror_prepare_node,
  dafg::NodeHandle &dynamic_mirror_prepass_node,
  dafg::NodeHandle &dynamic_mirror_end_prepass_node,
  dafg::NodeHandle &dynamic_mirror_render_dynamic_node,
  dafg::NodeHandle &dynamic_mirror_render_static_node,
  dafg::NodeHandle &dynamic_mirror_render_ground_node,
  dafg::NodeHandle &dynamic_mirror_resolve_gbuf_node,
  dafg::NodeHandle &dynamic_mirror_resolve_node,
  dafg::NodeHandle &dynamic_mirror_envi_node)
{
  dynamic_mirror_renderer.clearCameraData();
  if (!dynamic_mirror_renderer.hasMirrorSet())
    return;
  if (!init_dynamic_mirrors_on_demand(dynamic_mirror_renderer, dynamic_mirror_prepare_node, dynamic_mirror_prepass_node,
        dynamic_mirror_end_prepass_node, dynamic_mirror_render_dynamic_node, dynamic_mirror_render_static_node,
        dynamic_mirror_render_ground_node, dynamic_mirror_resolve_gbuf_node, dynamic_mirror_resolve_node, dynamic_mirror_envi_node))
    return;
  auto latestResolution = dynamic_mirror_renderer.getLatestResolution();
  if (latestResolution.x * latestResolution.y == 0)
    return;
  get_dynamic_mirror_ecs_query(manager,
    [&](const Point3 &dynamic_mirror_holder__mirrorNormal, const TMatrix &transform,
      const AnimV20::AnimcharRendComponent &animchar_render, AnimV20::AnimcharBaseComponent &animchar) {
      const auto &mirrors = dynamic_mirror_renderer.getMirrors();
      if (mirrors.empty())
        return;

      const auto &geomTreeNodeMap = animchar_render.getNodeMap();
      Point3 avgPos = Point3(0, 0, 0);
      for (const auto &mirror : mirrors)
      {
        auto nodeIdx = geomTreeNodeMap[mirror.rendAnimcharNodeId].nodeIdx;
        mat44f nodeWtmf;
        animchar.getNodeTree().getNodeWtm(nodeIdx, nodeWtmf);
        TMatrix nodeWtm;
        animchar.getNodeTree().mat44f_to_TMatrix(nodeWtmf, nodeWtm);
        Point3 pos = nodeWtm * mirror.boundingSphere.c;
        avgPos += pos;
      }
      avgPos /= mirrors.size();
      Plane3 mirrorPlane = Plane3(transform % dynamic_mirror_holder__mirrorNormal, avgPos);
      dynamic_mirror_renderer.prepareCameraData(event.viewItm, event.persp, mirrorPlane);
      dynamic_mirror_renderer.prepareMirrorRIVisibilityAsync(latestResolution.y);
    });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_REQUIRE(const Point3 &dynamic_mirror_holder__mirrorNormal)
static void dynamic_mirror_appear_es_event_handler(const ecs::Event &,
  ecs::EntityManager &manager,
  ecs::EntityId eid,
  AnimV20::AnimcharRendComponent &animchar_render,
  const ecs::Point4List *additional_data,
  bool animchar__renderPriority = false)
{
  get_dynamic_mirror_render_ecs_query(manager, [&](DynamicMirrorRenderer &dynamic_mirror_renderer) {
    dynamic_mirror_renderer.setMirror(eid, animchar_render.getSceneInstance(), additional_data, animchar__renderPriority);
  });
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(const Point3 &dynamic_mirror_holder__mirrorNormal)
static void dynamic_mirror_disappear_es_event_handler(const ecs::Event &, ecs::EntityManager &manager, ecs::EntityId eid)
{
  get_dynamic_mirror_render_ecs_query(manager,
    [&](DynamicMirrorRenderer &dynamic_mirror_renderer) { dynamic_mirror_renderer.unsetMirror(eid); });
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es)
static void animchar_mirror_before_render_es(const UpdateStageInfoBeforeRender &stg, ecs::EntityManager &manager)
{
  get_dynamic_mirror_render_ecs_query(manager, [&](DynamicMirrorRenderer &dynamic_mirror_renderer) {
    auto mirrorCameraData = dynamic_mirror_renderer.getCameraData();
    if (mirrorCameraData != nullptr)
    {
      vec4f mirrorCamPos = v_ldu(&mirrorCameraData->viewPos.x);
      preprocess_visible_animchars_in_frustum(stg, mirrorCameraData->frustum, mirrorCamPos, VISFLG_DYNAMIC_MIRROR);
    }
  });
}
