// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/updateStage.h>
#include <daECS/core/coreEvents.h>
#include <ecs/anim/anim.h>
#include <math/dag_geomTree.h>
#include <debug/dag_assert.h>

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
void animchar_cam_target_init_es_event_handler(const ecs::Event &, const AnimV20::AnimcharBaseComponent &animchar,
  const ecs::string *animchar_camera_target__node_name, int &animchar_camera_target__nodeIndex)
{
  const char *nodeName = animchar_camera_target__node_name ? animchar_camera_target__node_name->c_str() : "reye";
  animchar_camera_target__nodeIndex = (int)animchar.getNodeTree().findNodeIndex(nodeName);
  if (animchar_camera_target__nodeIndex < 0)
    logerr("can't find target node <%s>", nodeName);
}

ECS_AUTO_REGISTER_COMPONENT(DPoint3, "camera__look_at", nullptr, ecs::DataComponent::DONT_REPLICATE);
ECS_DEF_PULL_VAR(animchar_camera_target);

ECS_TAG(render)
ECS_AFTER(after_animchar_update_sync)
ECS_REQUIRE(eastl::true_type animchar__updatable)
ECS_REQUIRE_NOT(ecs::Tag animchar__actOnDemand)
ECS_REQUIRE_NOT(Point3 animchar_camera_target__node_offset)
static void animchar_cam_target_es(const ecs::UpdateStageInfoAct &, const AnimV20::AnimcharBaseComponent &animchar,
  int animchar_camera_target__nodeIndex, DPoint3 &camera__look_at)
{
  if (animchar_camera_target__nodeIndex < 0)
    return;
  camera__look_at = dpoint3(animchar.getNodeTree().getNodeWposRelScalar(dag::Index16(animchar_camera_target__nodeIndex))) +
                    dpoint3(as_point3(animchar.getNodeTree().getWtmOfsPtr()));
  ANIMCHAR_VERIFY_NODE_POS_S(point3(camera__look_at), animchar_camera_target__nodeIndex, animchar);
}

ECS_TAG(render)
ECS_AFTER(after_animchar_update_sync)
ECS_REQUIRE(Point3 animchar_camera_target__node_offset)
ECS_REQUIRE(eastl::true_type animchar__updatable)
ECS_REQUIRE_NOT(ecs::Tag animchar__actOnDemand)
static void animchar_cam_target_with_offset_es(const ecs::UpdateStageInfoAct &, const AnimV20::AnimcharBaseComponent &animchar,
  int animchar_camera_target__nodeIndex, const Point3 &animchar_camera_target__node_offset, DPoint3 &camera__look_at)
{
  if (animchar_camera_target__nodeIndex < 0)
    return;
  const vec3f offsetWposRel = v_mat44_mul_vec3p(animchar.getNodeTree().getNodeWtmRel(dag::Index16(animchar_camera_target__nodeIndex)),
    v_ldu(&animchar_camera_target__node_offset.x));
  camera__look_at = dpoint3(as_point3(&offsetWposRel)) + dpoint3(as_point3(animchar.getNodeTree().getWtmOfsPtr()));
  ANIMCHAR_VERIFY_NODE_POS_S(point3(camera__look_at), animchar_camera_target__nodeIndex, animchar);
}
