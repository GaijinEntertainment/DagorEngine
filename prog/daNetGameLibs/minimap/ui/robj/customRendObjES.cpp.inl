// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "robjMinimap.h"
#include <ecs/core/entityManager.h>
#include "ui/uiEvents.h"


ROBJ_FACTORY_IMPL(RobjMinimap, darg::RendObjEmptyParams)
ROBJ_FACTORY_IMPL(RobjMinimapVisCone, darg::RendObjEmptyParams)
ROBJ_FACTORY_IMPL(RobjMinimapRegionsGeometry, RendObjMinimapRegionsGeometryParams)
ROBJ_FACTORY_IMPL(RobjMinimapBackMap, darg::RendObjEmptyParams)

ECS_TAG(render)
static void register_game_rendobj_factories_es_event_handler(const EventUiRegisterRendObjs &)
{
  add_rendobj_factory("ROBJ_MINIMAP", ROBJ_FACTORY_PTR(RobjMinimap));
  add_rendobj_factory("ROBJ_MINIMAP_BACK", ROBJ_FACTORY_PTR(RobjMinimapBackMap));
  add_rendobj_factory("ROBJ_MINIMAP_VIS_CONE", ROBJ_FACTORY_PTR(RobjMinimapVisCone));
  add_rendobj_factory("ROBJ_MINIMAP_REGIONS_GEOMETRY", ROBJ_FACTORY_PTR(RobjMinimapRegionsGeometry));
}
