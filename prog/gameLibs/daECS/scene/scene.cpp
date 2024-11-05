// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/scene/scene.h>
#include <daECS/io/blk.h>
#include <daECS/core/entityManager.h>
#include <memory/dag_framemem.h>
#include <ioSys/dag_dataBlock.h>

namespace ecs
{

InitOnDemand<SceneManager> g_scenes;

void SceneManager::loadScene(const DataBlock &blk, const char *path)
{
  int import_depth = 0;
  auto onSceneEntityCreated = [this, &import_depth](ecs::EntityId eid, const char *tname, ecs::ComponentsList &&clist) {
    G_ASSERT(tname && *tname);
    bool toBeSaved = import_depth == 0; // entities from imports aren't saved
    G_VERIFY(scene.entities.emplace(eid, Scene::EntityRecord{eastl::move(clist), scene.orderSequence++, tname, toBeSaved}).second);
  };
  auto onSceneImportBeginEnd = [this, &import_depth](const char *scene_blk, bool begin) {
    if (import_depth == 0 && begin)
    {
      const uint32_t order = scene.orderSequence == 0 ? Scene::ImportRecord::TOP_IMPORT_ORDER : scene.orderSequence++;
      scene.imports.push_back({scene_blk, order});
    }
    import_depth += (begin ? 1 : -1);
    G_ASSERTF(import_depth >= 0, "unbalanced begin/end?");
  };
  create_entities_blk(*g_entity_mgr, blk, path, onSceneEntityCreated, onSceneImportBeginEnd);
}

} // namespace ecs
