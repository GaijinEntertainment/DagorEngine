//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <daECS/core/event.h>
#include <daECS/core/internal/typesAndLimits.h>
#include <EASTL/string.h>

namespace key_track_anim
{

enum AnimType
{
  ANIM_SINGLE,
  ANIM_CYCLIC,
  ANIM_EXTRAPOLATED
};
#define ECS_DECL_ALL_ANIM_EVENTS                                           \
  /* Params:*/                                                             \
  /*   nParams[0] = first key*/                                            \
  /*   nParams[1] = first key time*/                                       \
  /*   nParams[2] = AnimType */                                            \
  ECS_DECL_ANIM_EVENT(CmdResetRotAnim, Point4, float, int)                 \
  ECS_DECL_ANIM_EVENT(CmdResetPosAnim, Point3, float, int)                 \
  /* Params:*/                                                             \
  /*   nParams[0] = first key*/                                            \
  /*   nParams[1] = first key time*/                                       \
  /*   nParams[2] = use additional time (add to last time in track)*/      \
  ECS_DECL_ANIM_EVENT(CmdAddRotAnim, Point4, float, bool)                  \
  ECS_DECL_ANIM_EVENT(CmdAddPosAnim, Point3, float, bool)                  \
  /*   nParams[0] = component name hash*/                                  \
  /*   nParams[1] = first key time*/                                       \
  /*   nParams[2] = AnimType*/                                             \
  ECS_DECL_ANIM_EVENT(CmdResetAttrFloatAnim, ecs::component_t, float, int) \
  /*   nParams[2] = use additional time (add to last time in track)*/      \
  ECS_DECL_ANIM_EVENT(CmdAddAttrFloatAnim, float, float, bool)             \
  ECS_DECL_ANIM_EVENT(CmdStopAnim)

#define ECS_DECL_ANIM_EVENT ECS_UNICAST_EVENT_TYPE
ECS_DECL_ALL_ANIM_EVENTS
#undef ECS_DECL_ANIM_EVENT


}; // namespace key_track_anim
