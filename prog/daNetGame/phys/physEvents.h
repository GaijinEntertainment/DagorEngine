// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>
#include <daECS/net/msgDecl.h>
#include <daNet/bitStream.h>
#include <math/dag_TMatrix.h>

struct UpdatePhysEvent : public ecs::Event
{
  float curTime = 0.;
  float dt = 0.;
  ECS_INSIDE_EVENT_DECL(UpdatePhysEvent, ::ecs::EVCAST_BROADCAST | ::ecs::EVFLG_PROFILE)
  UpdatePhysEvent(float _dt, float _curTime) : ECS_EVENT_CONSTRUCTOR(UpdatePhysEvent), dt(_dt), curTime(_curTime) {}
};

struct QueryPhysActorsNotCollidable : public ecs::Event
{
  // result
  bool shouldCollide = true;
  // parameters
  ecs::EntityId otherEid;

  ECS_UNICAST_EVENT_DECL(QueryPhysActorsNotCollidable)
  QueryPhysActorsNotCollidable(ecs::EntityId rhs) : ECS_EVENT_CONSTRUCTOR(QueryPhysActorsNotCollidable), otherEid(rhs) {}
};

struct QueryPhysActorsOneSideCollidable : public ecs::Event
{
  // result
  bool oneSideCollidable = false;
  // parameters
  ecs::EntityId otherEid;

  ECS_UNICAST_EVENT_DECL(QueryPhysActorsOneSideCollidable)
  QueryPhysActorsOneSideCollidable(ecs::EntityId rhs) : ECS_EVENT_CONSTRUCTOR(QueryPhysActorsOneSideCollidable), otherEid(rhs) {}
};

#define DANETGAME_PHYS_ECS_EVENTS                                                                          \
  PHYS_ECS_EVENT(EventOnEntityReset)                                                                       \
  PHYS_ECS_EVENT(CmdTeleportEntity, TMatrix /* newtm*/, bool /*hard teleport*/)                            \
  PHYS_ECS_EVENT(CmdSetSimplifiedPhys, bool /*is_simplified*/)                                             \
  PHYS_ECS_EVENT(CmdNavPhysCollisionApply, const Point3 * /*velocity*/, const Point3 * /*preudoVelDelta*/) \
  PHYS_ECS_EVENT(QueryPhysSnapshotMustBeMinVisible, int /*plrTeam*/, bool * /*out_should_be_min_visible*/)

#define PHYS_ECS_EVENT ECS_UNICAST_EVENT_TYPE
DANETGAME_PHYS_ECS_EVENTS
#undef PHYS_ECS_EVENT

ECS_UNICAST_PROFILE_EVENT_TYPE(CmdUpdateRemoteShadow, int32_t /*tick*/, float /*dt*/)
ECS_UNICAST_PROFILE_EVENT_TYPE(CmdPostPhysUpdate, int32_t /*tick*/, float /*dt*/, bool /*is_for_real*/);
ECS_UNICAST_PROFILE_EVENT_TYPE(CmdPostPhysUpdateRemoteShadow, int32_t /*tick*/, float /*dt*/);

// authority approved state
ECS_NET_DECL_MSG(AAStateMsg, danet::BitStream, float /* unit time */);
ECS_NET_DECL_MSG(AAPartialStateMsg, danet::BitStream);
// client's controls (aka usercmds)
ECS_NET_DECL_MSG(ControlsMsg, danet::BitStream);
ECS_NET_DECL_MSG(ControlsSeq, uint16_t /*controls sequence number*/);
// teleport msg
ECS_NET_DECL_MSG(TeleportMsg, TMatrix, bool /*hard teleport*/);
// phys reset
ECS_NET_DECL_MSG(PhysReset);
// Desync data
ECS_NET_DECL_MSG(DesyncData, danet::BitStream);
