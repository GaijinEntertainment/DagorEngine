// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bindQuirrelEx/autoBind.h>
#include <ecs/scripts/sqEntity.h>
#include <ecs/game/zones/zoneQuery.h>

SQ_DEF_AUTO_BINDING_MODULE_EX(bind_ecs_utils, "ecs.utils", sq::VM_ALL)
{
  Sqrat::Table tbl(vm);
  tbl //
    .Func("is_point_in_capzone", game::is_point_in_capzone)
    .Func("is_entity_in_capzone", game::is_entity_in_capzone)
    .Func("is_point_in_poly_battle_area", game::is_point_in_poly_battle_area)
    /**/;
  return tbl;
}
