// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>
#include <EASTL/shared_ptr.h> // FIXME: Sqrat is not well integrated with non-copyable types (e.g. unique_ptr<>)

class DataBlock;
class LandMeshManager;
class RenderScene;


bool is_level_loaded();
bool is_level_loaded_not_empty();
bool is_level_loading();
bool is_level_unloading();
ecs::EntityId get_current_level_eid();

const char *get_rendinst_dmg_blk_fn();

const LandMeshManager *get_landmesh_manager();
RenderScene *get_rivers();

void save_weather_settings_to_screenshot(DataBlock &blk);

ECS_BROADCAST_EVENT_TYPE(EventLevelLoaded);
ECS_BROADCAST_EVENT_TYPE(EventGameObjectsCreated, ecs::EntityId); /* EntityId of game_objects entity */
ECS_BROADCAST_EVENT_TYPE(EventGameObjectsEntitiesScheduled, int); /* count of scheduled (or created if synced) entities */
// all Game Objects that are not needed anymore, can be destroyed
// this event is always called immediatele after EventGameObjectsCreated
ECS_BROADCAST_EVENT_TYPE(EventGameObjectsOptimize, ecs::EntityId); /* EntityId of game_objects entity */
ECS_UNICAST_EVENT_TYPE(EventRIGenExtraRequested);                  /* EntityId of game_objects entity */
