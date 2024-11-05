// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/game/generic/keyTrackAnim.h>
#include <sqrat.h>
#include <quirrel/bindQuirrelEx/autoBind.h>
#include <ecs/scripts/sqBindEvent.h>
#include <ecs/scripts/sqEntity.h>

using namespace key_track_anim;

SQ_DEF_AUTO_BINDING_MODULE(bind_anim_events, "animevents")
{
#define SQ_CONST(x) constTbl.Const(#x, (int)x)

  Sqrat::ConstTable constTbl(vm);
  SQ_CONST(ANIM_SINGLE);
  SQ_CONST(ANIM_CYCLIC);
  SQ_CONST(ANIM_EXTRAPOLATED);
  return ecs::sq::EventsBind<
#define ECS_DECL_ANIM_EVENT(x, ...) x,
    ECS_DECL_ALL_ANIM_EVENTS
#undef ECS_DECL_ANIM_EVENT
      ecs::sq::term_event_t>::bindall(vm);
}
