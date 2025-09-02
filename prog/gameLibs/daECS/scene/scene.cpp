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
void SceneManager::recordScene(ecs::SceneManager *smgr, uint32_t load_type, const char *path, int import_depth, const uint32_t order,
  eastl::vector<uint32_t> &loadRecordIndices)
{
  auto &scene = smgr->scene;
  Scene::ScenesList *sceneRecords;
  if (scene.tryGetSceneRecordList(load_type, sceneRecords))
  {
    uint32_t parent = Scene::SceneRecord::NO_PARENT;
    if (!loadRecordIndices.empty())
    {
      parent = loadRecordIndices.back();
      sceneRecords->at(parent).imports++;
    }
    const size_t index = sceneRecords->size();
    loadRecordIndices.push_back(index);
    Scene::SceneRecord &record = sceneRecords->emplace_back(path, import_depth, order, load_type, parent);
    if (import_depth == 0)
    {
      record.editable = true;
      scene.setTargetSceneId(load_type, index);
    }
  }
}

/*static*/
void SceneManager::loadScene(ecs::SceneManager *smgr, const DataBlock &blk, const char *path, unsigned *ent_cnt_ptr, int import_depth,
  uint32_t load_type, const template_allowed_cb_t &template_allowed_cb)
{
  eastl::vector<uint32_t> loadRecordIndices;
  if (smgr)
    recordScene(smgr, load_type, path, import_depth, smgr->scene.orderSequence++, loadRecordIndices);

  auto onSceneEntityCreationScheduled = [&](ecs::EntityId eid, const char *tname, ecs::ComponentsList &&clist) {
    G_ASSERT(tname && *tname);
    if (ent_cnt_ptr)
      ++*ent_cnt_ptr;
    if (smgr)
    {
      auto &scene = smgr->scene;
      bool toBeSaved = import_depth == 0; // entities from imports aren't saved
      uint32_t idx = 0;
      if (!loadRecordIndices.empty())
      {
        Scene::ScenesList *sceneRecords;
        auto &scene = smgr->scene;
        if (scene.tryGetSceneRecordList(load_type, sceneRecords))
        {
          idx = loadRecordIndices.back();
          G_ASSERT(idx < sceneRecords->size());
          sceneRecords->at(idx).entities.insert(eid);
        }
      }
      G_VERIFY(
        scene.entities.emplace(eid, Scene::EntityRecord(eastl::move(clist), scene.orderSequence++, tname, toBeSaved, idx, load_type))
          .second);
      scene.numToBeSavedEntities += (int)toBeSaved;
    }
  };
  ecs::create_entity_async_cb_t onSceneEntityCreated;
  if (ent_cnt_ptr)
    onSceneEntityCreated = [decCnt = DecCreationCounterOnDestroy(*ent_cnt_ptr)](ecs::EntityId) {};
  auto onSceneImportBeginEnd = [&](const char *scene_blk, bool begin) {
    if (smgr)
    {
      auto &scene = smgr->scene;
      if (begin)
      {
        uint32_t order;
        if (import_depth == 0 && scene.numToBeSavedEntities == 0)
          order = Scene::SceneRecord::TOP_IMPORT_ORDER;
        else
          order = scene.orderSequence++;
        recordScene(smgr, load_type, scene_blk, import_depth + 1, order, loadRecordIndices);
      }
      else if (!loadRecordIndices.empty())
        loadRecordIndices.pop_back();
    }
    import_depth += (begin ? 1 : -1);
    G_ASSERTF(import_depth >= 0, "unbalanced begin/end?");
  };
  create_entities_blk(*g_entity_mgr, blk, path, onSceneEntityCreationScheduled, onSceneEntityCreated, onSceneImportBeginEnd,
    template_allowed_cb);
}

/* static */
void SceneManager::setChildSceneEditable(SceneManager *smgr, uint32_t load_type, uint32_t scene_idx, bool enable)
{
  if (!smgr)
    return;

  auto &scene = smgr->scene;
  Scene::ScenesList *sceneRecords;
  if (scene.tryGetSceneRecordList(load_type, sceneRecords))
  {
    G_ASSERT(scene_idx < sceneRecords->size());
    Scene::SceneRecord &record = sceneRecords->at(scene_idx);
    if (record.hasParent())
    {
      record.editable = enable;

      uint32_t changed = 0;
      for (auto eid : record.entities)
      {
        Scene::EntityRecord *erec = scene.findEntityRecordForModify(eid);
        if (erec)
        {
          if (erec->toBeSaved != (int)enable)
          {
            erec->toBeSaved = !erec->toBeSaved;
            changed++;
          }
        }
      }
      if (enable)
        scene.numToBeSavedEntities += changed;
      else
        scene.numToBeSavedEntities -= changed;
    }
  }
}

} // namespace ecs
