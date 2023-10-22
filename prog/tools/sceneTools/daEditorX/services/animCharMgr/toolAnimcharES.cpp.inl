#include <ecs/core/entitySystem.h>
#include <ecs/anim/anim.h>
#include <animChar/dag_animCharVis.h>
#include <3d/dag_render.h>
#include <render/dynmodelRenderer.h>

#include <ecs/renderEvents.h>

ECS_REGISTER_EVENT(BeforeRender)
ECS_REGISTER_EVENT(RenderStage)

ECS_NO_ORDER
static __forceinline void animchar_act_es(const ecs::UpdateStageInfoAct &stage, AnimV20::AnimcharBaseComponent &animchar,
  const TMatrix &transform, AnimcharNodesMat44 &animchar_node_wtm, vec3f &animchar_render__root_pos)
{
  animchar.act(stage.dt, true);
  animchar_copy_nodes(animchar, animchar_node_wtm, animchar_render__root_pos);
}

// render

ECS_TAG(render)
static __forceinline void animchar_before_render_es(const BeforeRender &event, AnimV20::AnimcharRendComponent &animchar_render,
  const AnimcharNodesMat44 &animchar_node_wtm, vec4f &animchar_bsph, bbox3f &animchar_bbox, uint8_t &animchar_visbits,
  bool animchar_render__enabled)
{
  animchar_render.setVisible(animchar_render__enabled);
  animchar_render.beforeRender(animchar_node_wtm, animchar_bsph, animchar_bbox, event.cullingFrustum, 1.0f, event.occlusion,
    event.viewItm.getcol(3));

  animchar_visbits = animchar_render.getVisBits();
}


ECS_TAG(render)
ECS_NO_ORDER
static __forceinline void animchar_render_es(const RenderStage &stage, AnimV20::AnimcharRendComponent &animchar_render,
  const bbox3f &animchar_bbox, uint8_t animchar_visbits)
{
  if (AnimV20::AnimcharRendComponent::check_visibility(animchar_visbits, animchar_bbox, stage.stage != RenderStage::Shadow,
        stage.cullingFrustum, stage.occlusion))
  {
    dynrend::render_in_tools(animchar_render.getSceneInstance(),
      (stage.stage == RenderStage::Transparent) ? dynrend::RenderMode::Translucent : dynrend::RenderMode::Opaque);
  }
}
