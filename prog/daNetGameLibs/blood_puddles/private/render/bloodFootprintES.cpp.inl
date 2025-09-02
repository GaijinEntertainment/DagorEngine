// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/anim/anim.h>
#include <ecs/core/entityManager.h>

#include <math/dag_Point3.h>

#include "blood_puddles/public/render/bloodPuddles.h"

ECS_UNICAST_EVENT_TYPE(EventBloodFootStep, Point3 /*(step position)*/, Point3 /*(direction)*/, int /* (node id)*/)
ECS_REGISTER_EVENT(EventBloodFootStep)

struct FootBloodiness
{
  float strength = 0;
  void stepDecay(float strength_decay) { strength *= strength_decay; }
};

struct BloodFootprintEmitter
{
  eastl::pair<dag::Index16, FootBloodiness> leftNodeBloodiness, rightNodeBloodiness;

  bool onLoaded(const ecs::EntityManager &mgr, ecs::EntityId eid)
  {
    const AnimV20::AnimcharBaseComponent &animChar = mgr.get<AnimV20::AnimcharBaseComponent>(eid, ECS_HASH("animchar"));
    const GeomNodeTree &tree = animChar.getNodeTree();

    const ecs::string leftFootNode = mgr.get<ecs::string>(eid, ECS_HASH("blood_footprint_emitter__left_foot_node"));
    const ecs::string rightFootNode = mgr.get<ecs::string>(eid, ECS_HASH("blood_footprint_emitter__right_foot_node"));
    leftNodeBloodiness = eastl::make_pair<dag::Index16, FootBloodiness>(tree.findNodeIndex(leftFootNode.c_str()), {});
    G_ASSERTF(leftNodeBloodiness.first, "Cannot find node '%s' in node tree", leftFootNode.c_str());
    rightNodeBloodiness = eastl::make_pair<dag::Index16, FootBloodiness>(tree.findNodeIndex(rightFootNode.c_str()), {});
    G_ASSERTF(rightNodeBloodiness.first, "Cannot find node '%s' in node tree", rightFootNode.c_str());

    return true;
  }

  FootBloodiness &getBloodinessByNodeId(dag::Index16 node_id)
  {
    if (node_id == leftNodeBloodiness.first)
      return leftNodeBloodiness.second;
    else if (node_id == rightNodeBloodiness.first)
      return rightNodeBloodiness.second;
    G_ASSERTF(0, "Invalid node id for blood footprint emitter");
    return leftNodeBloodiness.second;
  }

  const FootBloodiness &getBloodinessByNodeId(dag::Index16 node_id) const
  {
    return const_cast<BloodFootprintEmitter *>(this)->getBloodinessByNodeId(node_id);
  }
};

ECS_DECLARE_RELOCATABLE_TYPE(BloodFootprintEmitter);
ECS_REGISTER_RELOCATABLE_TYPE(BloodFootprintEmitter, nullptr);
ECS_AUTO_REGISTER_COMPONENT_DEPS(BloodFootprintEmitter, "blood_footprint_emitter", nullptr, 0, "animchar");

static inline void foot_step_in_blood_puddles_es_event_handler(const EventBloodFootStep &evt,
  BloodFootprintEmitter &blood_footprint_emitter)
{
  if (!get_blood_puddles_mgr())
    return;
  const bool isBloodPuddleFresh = get_blood_puddles_mgr()->getBloodFreshness(evt.get<0>()) > 0.0f;
  auto &bloodiness = blood_footprint_emitter.getBloodinessByNodeId(dag::Index16(evt.get<2>()));
  if (isBloodPuddleFresh)
    bloodiness.strength = 1;
}

static inline void emit_blood_footprint_es_event_handler(
  const EventBloodFootStep &evt, BloodFootprintEmitter &blood_footprint_emitter, int footprintType, const TMatrix &transform)
{
  BloodPuddles *mgr = get_blood_puddles_mgr();
  if (!mgr)
    return;
  auto &bloodiness = blood_footprint_emitter.getBloodinessByNodeId(dag::Index16(evt.get<2>()));
  if (bloodiness.strength > mgr->getFootprintMinStrength())
  {
    mgr->addFootprint(evt.get<0>(), evt.get<1>(), transform.getcol(1), bloodiness.strength,
      (int)blood_footprint_emitter.leftNodeBloodiness.first == evt.get<2>(), footprintType);
    bloodiness.stepDecay(get_blood_puddles_mgr()->getFootprintStrengthDecayPerStep());
  }
}
