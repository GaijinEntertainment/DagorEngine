// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/anim/anim.h>
#include <daECS/core/coreEvents.h>
#include <ecs/anim/animchar_visbits.h>

#include <math/dag_mathUtils.h>
#include "hideNodesEvent.h"


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void animchar_hider_at_node_created_es_event_handler(const ecs::Event &,
  ecs::EntityId eid,
  const AnimCharV20::AnimcharBaseComponent &animchar,
  const ecs::string &animchar_hider__nodeName,
  int &animchar_hider__nodeIdx)
{
  animchar_hider__nodeIdx = (int)animchar.getNodeTree().findNodeIndex(animchar_hider__nodeName.c_str());
  if (animchar_hider__nodeIdx < 0)
    logerr("Unknown animchar_hider.nodeName: '%s' in template <%s>", animchar_hider__nodeName.c_str(),
      g_entity_mgr->getEntityTemplateName(eid));
}

ECS_TAG(render)
static inline void animchar_hider_es_event_handler(const HideNodesEvent &evt,
  animchar_visbits_t &animchar_visbits,
  const vec4f &animchar_bsph,
  float animchar_hider__camThreshold ECS_REQUIRE_NOT(int animchar_hider__nodeIdx))
{
  if (!animchar_visbits)
    return;
  if (v_extract_x(v_length3_sq_x(v_sub(animchar_bsph, v_ldu(&evt.get<0>().x)))) <= sqr(animchar_hider__camThreshold))
    hide_from_main_visibility(animchar_visbits);
};

ECS_TAG(render)
static inline void animchar_hider_at_node_es_event_handler(const HideNodesEvent &evt,
  animchar_visbits_t &animchar_visbits,
  int animchar_hider__nodeIdx,
  float animchar_hider__camThreshold,
  const AnimCharV20::AnimcharBaseComponent &animchar)
{
  if (!animchar_visbits || animchar_hider__nodeIdx < 0)
    return;
  const Point3 pos = animchar.getNodeTree().getNodeWposScalar(dag::Index16(animchar_hider__nodeIdx));
  ANIMCHAR_VERIFY_NODE_POS_S(pos, animchar_hider__nodeIdx, animchar);
  if (lengthSq(pos - evt.get<0>()) <= sqr(animchar_hider__camThreshold))
    hide_from_main_visibility(animchar_visbits);
}
