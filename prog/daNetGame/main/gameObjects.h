// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/functional.h>
#include <EASTL/fixed_function.h>
#include <scene/dag_tiledScene.h>
#include <daECS/core/componentType.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <math/dag_TMatrix.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_dataBlock.h>
#include <sound/gameObjectSound.h>

struct ObjectsToPlace;

typedef eastl::unique_ptr<scene::TiledScene> GOScenePtr;
struct GameObjects
{
  // To consider: use hash of string (instead of string) as key
  ska::flat_hash_map<eastl::string, GOScenePtr> objects; // resname -> scenes (except ladders, see below). Scene ptr can't be null

  scene::TiledScene *ladders = nullptr; // Scene with all ladders (of different resnames types) combined.
                                        // Points to scene within objects ("ladders" key)

  scene::TiledScene *indoors = nullptr; // Scene with indoor volumes. Points to scene within objects ("indoors" key)

  scene::TiledScene *sounds = nullptr; // Scene with sound volumes. Points to scene within objects ("sounds" key)
  GameObjectSoundAttributes soundAttributes;

  GameObjects() = default;
  EA_NON_COPYABLE(GameObjects)

  template <typename F>
  void moveScene(const char *scn_name, F cb)
  {
    auto sceneIt = objects.find_as(scn_name);
    if (sceneIt == objects.end())
      return;
    G_ASSERT(sceneIt->second.get() != sounds);
    cb(eastl::move(sceneIt->second));
    objects.erase(sceneIt);
  }

  bool needSceneInGame(scene::TiledScene *scene) const
  {
    return scene != nullptr && (scene == ladders || scene == indoors || scene == sounds);
  }
};

void create_game_objects(Tab<ObjectsToPlace *> &&objectsToPlace);

// it will auto calculate tileSize, by calculating it's bbox and divide biggest size by split = 8
// do not call it if you think you have already better tile
void auto_rearrange_game_object_scene(scene::TiledScene &s, uint32_t split = 8, float max_tile_size = 1e5f);

typedef eastl::fixed_function<sizeof(void *) * 3, void(const TMatrix &, int /*numSteps*/)> GameObjInstCB;
void traceray_ladders(const Point3 &from, const Point3 &to, const GameObjInstCB &game_obj_inst_cb);
scene::TiledScene *create_ladders_scene(GameObjects &game_objects);

ECS_DECLARE_RELOCATABLE_TYPE(GameObjects);
