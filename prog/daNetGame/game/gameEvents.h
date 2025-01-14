// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/event.h>
#include <daECS/core/componentTypes.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <util/dag_simpleString.h>
#include <EASTL/string.h>
#include <rendInst/riexHandle.h>
#include <ioSys/dag_dataBlock.h>

class IGenLoad;

#define DEF_UNICAST_GAME_EVENTS                                                                                           \
  DECL_GAME_EVENT(CmdUse, ecs::EntityId) /* unicast */                                                                    \
  DECL_GAME_EVENT(EventOnCollision, Point3, Point3, Point3, ecs::EntityId, Point3, float, float)                          \
  /*< EventOnCollision: delta_v, initial_vel, pos, offender, offender_initial_vel, dt, max % hp damage (-1 by default) */ \
  DECL_GAME_EVENT(EventRiExtraDestroyed, ecs::EntityId /* offender */)                                                    \
  DECL_GAME_EVENT(CmdTeleportTo, TMatrix, float) /* position/direction, speed*/                                           \
  DECL_GAME_EVENT(CmdGetActorLookDir, const Point3 ** /*out_const_lookDirPtr*/)                                           \
  DECL_GAME_EVENT(CmdGetActorUsesFpsCam, bool * /*out_is_fps*/)                                                           \
  DECL_GAME_EVENT(EventOnModsChanged)

#define DEF_BROADCAST_GAME_EVENTS                                                                                              \
  DECL_GAME_EVENT(EventOnGameInit)                                                                                             \
  DECL_GAME_EVENT(EventOnGameTerm)                                                                                             \
  DECL_GAME_EVENT(EventOnGameShutdown)                                                                                         \
  DECL_GAME_EVENT(EventOnGameAppStarted)                                                                                       \
  DECL_GAME_EVENT(EventOnGameUnloadStart)                                                                                      \
  DECL_GAME_EVENT(EventOnGameUnloadEnd)                                                                                        \
  DECL_GAME_EVENT(EventOnBeforeSceneLoad, const char * /*scene_name*/, const eastl::vector<eastl::string> & /*import_scenes*/, \
    const DataBlock & /*game_settings*/)                                                                                       \
  DECL_GAME_EVENT(EventBeforeLocationEntityCreated)                                                                            \
  DECL_GAME_EVENT(EventAfterLocationEntityDestroyed)                                                                           \
  DECL_GAME_EVENT(EventDoLoadTaggedLocationData, int /*tag*/, IGenLoad * /*crd*/, bool * /*inout_tag_loaded*/)                 \
  DECL_GAME_EVENT(EventDoFinishLocationDataLoad)                                                                               \
  DECL_GAME_EVENT(EventHeroChanged, ecs::EntityId /*new_hero*/)                                                                \
  DECL_GAME_EVENT(EventUserLoggedIn, int64_t /*user_id*/, SimpleString /*user_name*/)                                          \
  DECL_GAME_EVENT(EventUserLoggedOut)                                                                                          \
  DECL_GAME_EVENT(EventUserMMQueueJoined)                                                                                      \
  DECL_GAME_EVENT(EventRendinstDestroyed, rendinst::riex_handle_t /* riex_handle*/, TMatrix /*ri_tm*/, BBox3 /*ri_bbox*/)      \
  DECL_GAME_EVENT(EventLadderUpdate, TMatrix /* ladder_tm */)                                                                  \
  DECL_GAME_EVENT(EventOnGameUpdateAfterRenderer, float /*dt*/, float /*curTime*/)                                             \
  DECL_GAME_EVENT(EventOnGameScriptsShutdown)

#define DECL_GAME_EVENT ECS_UNICAST_EVENT_TYPE
DEF_UNICAST_GAME_EVENTS
#undef DECL_GAME_EVENT
#define DECL_GAME_EVENT ECS_BROADCAST_EVENT_TYPE
DEF_BROADCAST_GAME_EVENTS
#undef DECL_GAME_EVENT
