// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/scene/scene.h>
#include <daECS/io/blk.h>
#include <daECS/core/entityManager.h>
#include <memory/dag_framemem.h>
#include <ioSys/dag_dataBlock.h>

namespace ecs
{

ECS_REGISTER_EVENT(EventOnLocalSceneEntitiesCreated);

InitOnDemand<SceneManager> g_scenes;

struct DecCreationCounterOnDestroy
{
  unsigned *entCreationCounter;
  uint8_t gen;
  DecCreationCounterOnDestroy(unsigned &cnt) : entCreationCounter(&cnt), gen(cnt >> 24) { cnt++; }
  DecCreationCounterOnDestroy(DecCreationCounterOnDestroy &&rhs)
  {
    entCreationCounter = rhs.entCreationCounter;
    gen = rhs.gen;
    rhs.entCreationCounter = nullptr;
  }
  DecCreationCounterOnDestroy(const DecCreationCounterOnDestroy &rhs)
  {
    entCreationCounter = rhs.entCreationCounter;
    gen = rhs.gen;
  }
  ~DecCreationCounterOnDestroy()
  {
    if (!entCreationCounter || (*entCreationCounter >> 24) != gen)
      return;
    if ((--*entCreationCounter & ((1 << 24) - 1)) == 0)
      g_entity_mgr->broadcastEvent(EventOnLocalSceneEntitiesCreated());
    G_ASSERTF((int)*entCreationCounter >= 0, "Unbalanced (%d) entity creation cb?", *entCreationCounter);
  }
};


/*static*/
void SceneManager::loadScene(ecs::SceneManager *smgr, const DataBlock &blk, const char *path, unsigned *ent_cnt_ptr, int import_depth)
{
  auto onSceneEntityCreationScheduled = [&](ecs::EntityId eid, const char *tname, ecs::ComponentsList &&clist) {
    G_ASSERT(tname && *tname);
    if (ent_cnt_ptr)
      ++*ent_cnt_ptr;
    if (smgr)
    {
      auto &scene = smgr->scene;
      bool toBeSaved = import_depth == 0; // entities from imports aren't saved
      G_VERIFY(scene.entities.emplace(eid, Scene::EntityRecord{eastl::move(clist), scene.orderSequence++, tname, toBeSaved}).second);
      scene.numToBeSavedEntities += (int)toBeSaved;
    }
  };
  ecs::create_entity_async_cb_t onSceneEntityCreated;
  if (ent_cnt_ptr)
    onSceneEntityCreated = [decCnt = DecCreationCounterOnDestroy(*ent_cnt_ptr)](ecs::EntityId) {};
  auto onSceneImportBeginEnd = [&](const char *scene_blk, bool begin) {
    if (smgr && import_depth == 0 && begin)
    {
      auto &scene = smgr->scene;
      const uint32_t order = scene.numToBeSavedEntities == 0 ? Scene::ImportRecord::TOP_IMPORT_ORDER : scene.orderSequence++;
      scene.imports.push_back({scene_blk, order});
    }
    import_depth += (begin ? 1 : -1);
    G_ASSERTF(import_depth >= 0, "unbalanced begin/end?");
  };
  create_entities_blk(*g_entity_mgr, blk, path, onSceneEntityCreationScheduled, onSceneEntityCreated, onSceneImportBeginEnd);
}

} // namespace ecs
