// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/fx/fx.h"
#include <effectManager/effectManager.h>
#include "render/fx/effectEntity.h"

#include <daECS/core/coreEvents.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <ecs/anim/anim.h>
#include <EASTL/unique_ptr.h>

#include <math/dag_geomTree.h>

struct AnimCharEffectNode
{
  dag::Index16 effectNodeId;
  ecs::EntityId effectEid = ecs::INVALID_ENTITY_ID;

  bool onLoaded(const ecs::EntityManager &mgr, ecs::EntityId eid);
};

bool AnimCharEffectNode::onLoaded(const ecs::EntityManager &mgr, ecs::EntityId eid)
{
  const AnimV20::AnimcharBaseComponent &animChar = mgr.get<AnimV20::AnimcharBaseComponent>(eid, ECS_HASH("animchar"));
  const char *nodeName = mgr.getOr(eid, ECS_HASH("animchar_effect__node"), "");
  effectNodeId = *nodeName ? animChar.getNodeTree().findNodeIndex(nodeName) : dag::Index16();
  if (!effectNodeId)
  {
    G_ASSERTF(0, "Node '%s' not found in animchar", nodeName);
    return false;
  }
  return true;
}

ECS_DECLARE_RELOCATABLE_TYPE(AnimCharEffectNode);
ECS_REGISTER_RELOCATABLE_TYPE(AnimCharEffectNode, nullptr);
ECS_AUTO_REGISTER_COMPONENT_DEPS(AnimCharEffectNode, "animchar_effect", nullptr, 0, "animchar");

ECS_ON_EVENT(on_appear)
ECS_NO_ORDER
void start_animchar_effect_es_event_handler(const ecs::Event &,
  ecs::EntityId eid,
  AnimCharEffectNode &animchar_effect,
  const ecs::string &animchar_effect__effect,
  const AnimV20::AnimcharBaseComponent &animchar)
{
  TMatrix transform;
  animchar.getNodeTree().getNodeWtmScalar(animchar_effect.effectNodeId, transform);
  ecs::ComponentsInitializer attrs;
  ECS_INIT(attrs, transform, transform);
  animchar_effect.effectEid = g_entity_mgr->createEntityAsync(animchar_effect__effect.c_str(), eastl::move(attrs));

  {
    ecs::ComponentsInitializer comps;
    ECS_INIT(comps, node_attached__entity, eid);
    ECS_INIT(comps, node_attached__nodeId, (int)animchar_effect.effectNodeId);
    add_sub_template_async(animchar_effect.effectEid, "node_attached_pos", eastl::move(comps));
  }
}

ECS_ON_EVENT(on_disappear)
ECS_NO_ORDER
void end_animchar_effect_es_event_handler(const ecs::Event &, const AnimCharEffectNode &animchar_effect)
{
  g_entity_mgr->destroyEntity(animchar_effect.effectEid);
}
