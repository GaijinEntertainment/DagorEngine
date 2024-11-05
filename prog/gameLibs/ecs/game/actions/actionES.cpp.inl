// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/game/actions/action.h>
#include <memory/dag_framemem.h>
#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <ecs/anim/anim.h>
#include <ecs/anim/animState.h>
#include <ecs/phys/physVars.h>
#include <anim/dag_animBlendCtrl.h>
#include <util/dag_simpleString.h>
#include <propsRegistry/propsRegistry.h>
#include <daECS/net/recipientFilters.h>
#include <daECS/net/netEvent.h>
#include <EASTL/vector_set.h>

struct NullActionListener final : public IActionListener
{
  void onAction(ecs::EntityId, int, int) override {}
  bool checkActionPrecondition(ecs::EntityId, int) override { return false; }
};
static NullActionListener null_action_listener;

static IActionListener *listener = &null_action_listener;

EntityActions::EntityActions(const ecs::EntityManager &mgr, ecs::EntityId eid_)
{
  ECS_GET_ENTITY_COMPONENT(AnimV20::AnimcharBaseComponent, animChar, eid_, animchar);
  ECS_GET_ENTITY_COMPONENT_RW(PhysVars, physVars, eid_, phys_vars);

  const AnimV20::AnimationGraph *animGraph = animChar->getAnimGraph();

  const ecs::Array &actionsAttr = mgr.get<ecs::Array>(eid_, ECS_HASH("actions__actions"));
  const ecs::Array &actionsEnabledAttr = mgr.get<ecs::Array>(eid_, ECS_HASH("actions__actions_enabled"));
  G_ASSERT(actionsAttr.size() == actionsEnabledAttr.size());
  G_UNREFERENCED(actionsEnabledAttr);

  for (auto &item : actionsAttr)
  {
    const ecs::Object &object = item.get<ecs::Object>();
    EntityAction &act = actions.push_back();
    act.name = object[ECS_HASH("name")].get<ecs::string>();
    if (object.hasMember(ECS_HASH("class")))
      act.classHash = str_hash_fnv1(object[ECS_HASH("class")].get<ecs::string>().c_str());
    act.stateIdx = animGraph->getStateIdx(object[ECS_HASH("state")].get<ecs::string>().c_str());
    act.actionDefTime = object[ECS_HASH("actionTime")].get<float>();
    act.propsId = propsreg::register_net_props(object[ECS_HASH("props")].get<ecs::string>().c_str(), "action");
    act.varId = physVars->registerVar(object[ECS_HASH("varName")].get<ecs::string>().c_str(), 0.0f);
    act.applyAtDef = object[ECS_HASH("applyAt")].get<float>();
    if (object.hasMember(ECS_HASH("blocksSprint")))
      act.blocksSprint = object[ECS_HASH("blocksSprint")].get<bool>();
    act.actionPeriod = object[ECS_HASH("actionPeriod")].getOr(-1.0f);
  }
}
EntityActions::~EntityActions() = default;

ECS_REGISTER_RELOCATABLE_TYPE(EntityActions, nullptr);
ECS_AUTO_REGISTER_COMPONENT_DEPS(EntityActions, "actions", nullptr, 0, "animchar", "phys_vars");
ECS_REGISTER_EVENT(UpdateActionsEvent);

static inline void actions_updater_es(const UpdateActionsEvent &info, ecs::EntityId eid, EntityActions &actions, PhysVars &phys_vars,
  const ecs::string *actions__animLayer)
{
  bool anyActionRunning = false;
  for (int i = 0; i < actions.actions.size(); ++i)
  {
    EntityAction &a = actions.actions[i];
    if (a.timer > 0.f)
    {
      a.timer -= info.dt;
      anyActionRunning |= a.timer > 0.f;
      if (a.timer < a.actionTime) // passes the border
      {
        ecs::HashedConstString stateName = actions__animLayer ? ECS_HASH_SLOW(actions__animLayer->c_str()) : ECS_HASH("upper");
        g_entity_mgr->sendEventImmediate(eid, EventChangeAnimState(stateName, a.stateIdx));

        float relTime = 1.f - (a.timer / a.actionTime);
        phys_vars.setVar(a.varId, relTime);
        if (a.actionPeriod > 0.f)
        {
          if (fmodf(a.prevRel, a.actionPeriod) > fmodf(relTime, a.actionPeriod))
          {
            int propsId = a.overridePropsId >= 0 ? a.overridePropsId : a.propsId;
            listener->onAction(eid, propsId, a.interpDelayTicks);
          }
        }
        else if (a.prevRel <= a.applyAt && a.applyAt < relTime)
        {
          int propsId = a.overridePropsId >= 0 ? a.overridePropsId : a.propsId;
          listener->onAction(eid, propsId, a.interpDelayTicks);
          a.overridePropsId = -1;
        }
        a.prevRel = relTime;
      }
    }
  }
  actions.anyActionRunning = anyActionRunning;
}

bool check_action_precondition(ecs::EntityId eid, int action_id)
{
  EntityActions *entActions = g_entity_mgr->getNullableRW<EntityActions>(eid, ECS_HASH("actions"));
  if (!entActions || !g_entity_mgr->get<bool>(eid, ECS_HASH("actions__enabled")))
    return false;
  EntityAction *act = safe_at(entActions->actions, action_id);
  if (!act)
    return false;
  return listener->checkActionPrecondition(eid, act->propsId);
}

void register_action_listener(IActionListener *in_listener) { listener = in_listener; }
