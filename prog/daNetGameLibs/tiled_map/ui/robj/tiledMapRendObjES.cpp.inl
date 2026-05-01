// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "robjTiledMap.h"
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "ui/uiEvents.h"


ROBJ_FACTORY_IMPL(RobjTiledMap, darg::RendObjEmptyParams)
ROBJ_FACTORY_IMPL(RobjTiledMapVisCone, darg::RendObjEmptyParams)
ROBJ_FACTORY_IMPL(RobjTiledMapFogOfWar, darg::RendObjEmptyParams)

ECS_TAG(render)
static void register_tiled_map_rendobj_factories_es_event_handler(const EventUiRegisterRendObjs &)
{
  add_rendobj_factory("ROBJ_TILED_MAP", ROBJ_FACTORY_PTR(RobjTiledMap));
  add_rendobj_factory("ROBJ_TILED_MAP_VIS_CONE", ROBJ_FACTORY_PTR(RobjTiledMapVisCone));
  add_rendobj_factory("ROBJ_TILED_MAP_FOG_OF_WAR", ROBJ_FACTORY_PTR(RobjTiledMapFogOfWar));
}
