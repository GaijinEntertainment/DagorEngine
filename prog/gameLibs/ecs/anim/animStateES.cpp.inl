// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entitySystem.h>
#include <ecs/core/attributeEx.h>
#include <ecs/core/entityManager.h>
#include <ecs/anim/anim.h>
#include <ecs/anim/animState.h>

#include <daECS/core/coreEvents.h>
#include <anim/dag_animBlend.h>
#include <daECS/core/updateStage.h>

#include <util/dag_convar.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>

CONSOLE_BOOL_VAL("anim", debug_anim_state, false);
ECS_REGISTER_EVENT(EventChangeAnimState);

ECS_ON_EVENT(on_appear)
static void restriction_parse_es(const ecs::Event &, ecs::EntityId eid, const AnimV20::AnimcharBaseComponent &animchar,
  const ecs::Object &human_anim_restrictions__prohibited, ecs::Array &human_anim_restriction_ids__prohibited)
{
  const AnimV20::AnimationGraph *animGraph = animchar.getAnimGraph();
  if (DAGOR_UNLIKELY(!animGraph))
  {
    G_ASSERT_LOG(animGraph, "%s: animGraph is NULL for %d<%s>", __FUNCTION__, ecs::entity_id_t(eid),
      g_entity_mgr->getEntityTemplateName(eid));
    return;
  }
  for (auto &it : human_anim_restrictions__prohibited)
  {
    int fromIdx = animGraph->getStateIdx(it.first.c_str());
    int toIdx = animGraph->getStateIdx(it.second.getStr());

    human_anim_restriction_ids__prohibited.push_back(IPoint2(fromIdx, toIdx));
  }
}

ECS_REQUIRE_NOT(ecs::auto_type animchar__lockAnimStateChange)
inline void human_anim_state_change_es_event_handler(const EventChangeAnimState &evt, ecs::Object &animchar__animState,
  const ecs::Array &human_anim_restriction_ids__prohibited, bool &animchar__animStateDirty)
{
  ecs::HashedConstString stateName = evt.get<0>();
  int currentState = animchar__animState.getMemberOr(stateName, -1);
  int nextState = evt.get<1>();

  if (currentState == nextState)
    return;

  animchar__animStateDirty = true;

  if (currentState != -1)
  {
    IPoint2 transition(currentState, nextState);
    for (auto &it : human_anim_restriction_ids__prohibited)
    {
      const IPoint2 &restriction = it.get<IPoint2>();
      if (restriction == transition)
        return;
    }
  }
  animchar__animState.addMember(evt.get<0>(), nextState);
}

ECS_REQUIRE_NOT(ecs::Array human_anim_restriction_ids__prohibited)
inline void anim_state_change_es_event_handler(const EventChangeAnimState &evt, ecs::Object &animchar__animState,
  bool &animchar__animStateDirty)
{
  ecs::HashedConstString stateName = evt.get<0>();
  int currentState = animchar__animState.getMemberOr(stateName, -1);
  int nextState = evt.get<1>();

  if (currentState == nextState)
    return;

  animchar__animStateDirty = true;
  animchar__animState.addMember(stateName, nextState);
}

ECS_NO_ORDER
ECS_TAG(render, dev)
inline void anim_state_debug_es(const ecs::UpdateStageInfoRenderDebug &, const AnimV20::AnimcharBaseComponent &animchar,
  const ecs::Object &animchar__animState, const TMatrix &transform)
{
  if (debug_anim_state.get())
  {
    const float dbgStateOffs = 1.f;
    int i = 0;
    for (const auto &st : animchar__animState)
    {
      int stId = st.second.get<int>();
      String stName = String(128, "%s: %s", st.first.c_str(), animchar.getAnimGraph()->getStateNameByStateIdx(stId));
      add_debug_text_mark(transform.getcol(3) + Point3(0.f, dbgStateOffs, 0.f), stName.str(), -1, i++);
    }
  }
}
