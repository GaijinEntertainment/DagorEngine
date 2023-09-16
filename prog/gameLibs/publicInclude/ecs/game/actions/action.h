//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/string.h>
#include <daECS/core/entityId.h>
#include <daECS/core/componentType.h>
#include <generic/dag_tab.h>
#include <daECS/core/event.h>

struct UpdateActionsEvent : public ecs::Event
{
  float dt = 0.;
  ECS_INSIDE_EVENT_DECL(UpdateActionsEvent, ::ecs::EVCAST_BROADCAST | ::ecs::EVFLG_PROFILE)
  UpdateActionsEvent(float _dt) : ECS_EVENT_CONSTRUCTOR(UpdateActionsEvent), dt(_dt) {}
};

namespace ecs
{
class EntityManager;
}

class IActionListener
{
public:
  virtual void onAction(ecs::EntityId eid, int action_props_idx, int interp_delay_ticks) = 0;
  virtual bool checkActionPrecondition(ecs::EntityId eid, int action_props_idx) = 0;
};

struct EntityAction
{
  int stateIdx = -1;         // for animchar
  float actionDefTime = 0.f; // total time of action
  float timer = 0.f;
  float actionTime = 0.f;
  float applyAtDef = 0.5f;
  float applyAt = 0.5f;
  float prevRel = 0.f;
  float actionPeriod = -1.f;
  int overridePropsId = -1;
  int interpDelayTicks = -1;
  int propsId = -1; // props of action
  int varId = -1;
  bool blocksSprint = true;
  ecs::string name;
  ecs::hash_str_t classHash = 0;
};

struct EntityActions
{
  Tab<EntityAction> actions;
  bool anyActionRunning = false;
  EntityActions() = delete;
  EntityActions(const ecs::EntityManager &mgr, ecs::EntityId eid);
  EntityActions(const EntityActions &) = delete;
  ~EntityActions();
};
ECS_DECLARE_RELOCATABLE_TYPE(EntityActions);

void register_action_listener(IActionListener *listener);
bool check_action_precondition(ecs::EntityId eid, int action_id);
