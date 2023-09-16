//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "event.h"
#include "ecsHash.h"
#include "internal/typesAndLimits.h"

#define ECS_DECL_UNICAST_CORE_EVENTS                                                                                          \
  /* Description:*/                                                                                                           \
  /*   Sent after entity was fully created & loaded. Sent only once. */                                                       \
  ECS_DECL_CORE_EVENT(EventEntityCreated)                                                                                     \
  /* Description:*/                                                                                                           \
  /*   As previous, but might be sent several times */                                                                        \
  ECS_DECL_CORE_EVENT(EventEntityRecreated)                                                                                   \
  /* Description:*/                                                                                                           \
  /*   Event is called on recreate if this ES will no longer apply (i.e. list of components no longer matches) */             \
  ECS_DECL_CORE_EVENT(EventComponentsDisappear)                                                                               \
  /* Description:*/                                                                                                           \
  /*   Event is called on recreate if this ES will start apply  (i.e. list of components matches)*/                           \
  ECS_DECL_CORE_EVENT(EventComponentsAppear)                                                                                  \
  /* Description:*/                                                                                                           \
  /*   Sent before entity destruction*/                                                                                       \
  ECS_DECL_CORE_EVENT(EventEntityDestroyed)                                                                                   \
  /* Description:*/                                                                                                           \
  /*   Sent after existing component is changed*/                                                                             \
  /*!!IMPORTANT!!. This is unique (opimized) event. It is called on ES only if ES requires Component that has been changed */ \
  /*I.e. unlike other events, this event won't be received by ES who is suitable to accept it (by list of components), if*/   \
  /*Component that has been changed is not listed in list of ES components*/                                                  \
  ECS_DECL_CORE_EVENT(EventComponentChanged)

#define ECS_DECL_BROADCAST_CORE_EVENTS                          \
  /* Description:*/                                             \
  /*   Sent after es order has been set*/                       \
  ECS_DECL_CORE_EVENT(EventEntityManagerEsOrderSet)             \
  /* Description:*/                                             \
  /*   Sent before&after all scene (all entities) destruction*/ \
  ECS_DECL_CORE_EVENT(EventEntityManagerBeforeClear)            \
  ECS_DECL_CORE_EVENT(EventEntityManagerAfterClear)

#define ECS_DECL_ALL_CORE_EVENTS \
  ECS_DECL_UNICAST_CORE_EVENTS   \
  ECS_DECL_BROADCAST_CORE_EVENTS

namespace ecs
{
#define ECS_DECL_CORE_EVENT(Klass, ...) ECS_BASE_DECL_EVENT_TYPE(Klass, EVCAST_UNICAST | EVFLG_CORE, __VA_ARGS__)
ECS_DECL_UNICAST_CORE_EVENTS
#undef ECS_DECL_CORE_EVENT
#define ECS_DECL_CORE_EVENT(Klass, ...) ECS_BASE_DECL_EVENT_TYPE(Klass, EVCAST_BROADCAST | EVFLG_CORE, __VA_ARGS__)
ECS_DECL_BROADCAST_CORE_EVENTS
#undef ECS_DECL_CORE_EVENT
} // namespace ecs
