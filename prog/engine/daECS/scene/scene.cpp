// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/scene/scene.h>
#include <daECS/io/blk.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/anim/animComponent.h>
#include <memory/dag_framemem.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_math3d.h>
#include <shaders/dag_dynSceneRes.h>

namespace ecs
{

ECS_REGISTER_EVENT(EventOnLocalSceneEntitiesCreated);
ECS_REGISTER_EVENT(EventOnSceneCreated);
ECS_REGISTER_EVENT(EventOnSceneDestroyed);
ECS_REGISTER_EVENT(EventOnEntitySceneDataChanged);

InitOnDemand<SceneManager> g_scenes;

namespace
{
void startMovingHierarchy(Scene::ScenesList &records, Scene::SceneId id, Scene::ScenesList &hierarchy_to_move)
{
  if (auto it = eastl::find_if(records.begin(), records.end(), [&](auto &&val) { return val.id == id; }); it != records.end())
  {
    auto childIt = it + 1;
    while (childIt != records.end() && childIt->importDepth > it->importDepth)
    {
      ++childIt;
    }

    hierarchy_to_move.insert(hierarchy_to_move.cend(), eastl::move_iterator(it), eastl::move_iterator(childIt));
    records.erase(it, childIt);
  }
}
} // namespace

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
Scene::SceneId SceneManager::recordScene(ecs::SceneManager *smgr, uint32_t load_type, const char *path, int import_depth,
  eastl::vector<Scene::SceneId> &loadRecordIds, const TMatrix *rootTransform)
{
  auto &scene = smgr->scene;
  Scene::SceneId parent = Scene::SceneRecord::NO_PARENT;
  const Scene::SceneId newId = smgr->idGenerator.acquire();

  if (!loadRecordIds.empty())
  {
    parent = loadRecordIds.back();
    if (Scene::SceneRecord *parentRecord = scene.getSceneRecordById(parent))
    {
      parentRecord->orderedEntries.push_back(Scene::SceneRecord::OrderedEntry{.sid = newId, .isEntity = false});
    }
  }
  loadRecordIds.push_back(newId);

  Scene::SceneRecord newRecord{path, import_depth, load_type, parent, newId};

  Scene::SceneRecord *record = nullptr;
  if (auto it = eastl::find_if(scene.allScenes.begin(), scene.allScenes.end(), [&](auto &&val) { return val.id == parent; });
      it != scene.allScenes.end())
  {
    record = scene.allScenes.insert(
      eastl::find_if(it + 1, scene.allScenes.end(), [depth = it->importDepth](auto &&val) { return val.importDepth <= depth; }),
      eastl::move(newRecord));
  }
  else
  {
    record = &scene.allScenes.emplace_back(eastl::move(newRecord));
  }

  record->rootTransform = rootTransform ? eastl::optional{*rootTransform} : eastl::nullopt;

  if (import_depth == 0)
  {
    scene.setTargetSceneId(newId);
  }

  return newId;
}

/*static*/
void SceneManager::loadScene(ecs::SceneManager *smgr, const DataBlock &blk, const char *path, unsigned *ent_cnt_ptr, int import_depth,
  uint32_t load_type)
{
  eastl::vector<Scene::SceneId> loadRecordIds;
  loadSceneInternal(smgr, blk, path, ent_cnt_ptr, import_depth, load_type, loadRecordIds);
}

/*static*/
Scene::SceneId SceneManager::loadSceneInternal(ecs::SceneManager *smgr, const DataBlock &blk, const char *path, unsigned *ent_cnt_ptr,
  int import_depth, uint32_t load_type, eastl::vector<Scene::SceneId> &loadRecordIds)
{
  Scene::SceneId newSceneId = Scene::C_INVALID_SCENE_ID;
  eastl::vector<Scene::SceneId> allLoadedSceneIds;

  if (smgr)
  {
    newSceneId = recordScene(smgr, load_type, path, import_depth, loadRecordIds, nullptr);
    allLoadedSceneIds.push_back(loadRecordIds.back());
  }

  auto onSceneEntityCreationScheduled = [&](ecs::EntityId eid, const char *tname, ecs::ComponentsList &&clist) {
    G_ASSERT(tname && *tname);
    if (ent_cnt_ptr)
      ++*ent_cnt_ptr;
    if (smgr)
    {
      auto &scene = smgr->scene;
      Scene::SceneId id = Scene::C_INVALID_SCENE_ID;
      uint32_t order = 0;
      if (!loadRecordIds.empty())
      {
        id = loadRecordIds.back();
        auto &scene = smgr->scene;
        if (Scene::SceneRecord *record = scene.getSceneRecordById(id))
        {
          record->entities.insert(eid);
          order = record->orderedEntries.size();
          record->orderedEntries.push_back(Scene::SceneRecord::OrderedEntry{.eid = eid, .isEntity = true});
        }
      }
      G_VERIFY(scene.entities.emplace(eid, Scene::EntityRecord(eastl::move(clist), tname, id, load_type)).second);
    }
  };
  ecs::create_entity_async_cb_t onSceneEntityCreated;
  if (ent_cnt_ptr)
    onSceneEntityCreated = [decCnt = DecCreationCounterOnDestroy(*ent_cnt_ptr)](ecs::EntityId) {};
  auto onSceneImportBeginEnd = [&](const char *scene_blk, const TMatrix *tm, bool begin) {
    if (smgr)
    {
      if (begin)
      {
        recordScene(smgr, load_type, scene_blk, import_depth + 1, loadRecordIds, tm);
        allLoadedSceneIds.push_back(loadRecordIds.back());
      }
      else if (!loadRecordIds.empty())
        loadRecordIds.pop_back();
    }
    import_depth += (begin ? 1 : -1);
    G_ASSERTF(import_depth >= 0, "unbalanced begin/end?");
  };

  const auto onSceneBlkLoaded = [&](const DataBlock &scene_blk) {
    if (smgr)
    {
      if (!loadRecordIds.empty())
      {
        const Scene::SceneId id = loadRecordIds.back();
        if (Scene::SceneRecord *record = smgr->scene.getSceneRecordById(id))
        {
          record->transformable = scene_blk.getBool("transformable_scene", true);
          record->pivot = scene_blk.getPoint3("pivot_scene", Point3::ZERO);
          record->prettyName = scene_blk.getStr("pretty_name_scene", nullptr);
        }
      }
    }
  };

  create_entities_blk(*g_entity_mgr, blk, path, onSceneEntityCreationScheduled, onSceneEntityCreated, onSceneImportBeginEnd,
    onSceneBlkLoaded);

  for (const Scene::SceneId sid : allLoadedSceneIds)
  {
    g_entity_mgr->broadcastEvent(EventOnSceneCreated{sid});
  }

  return newSceneId;
}

Scene::SceneId SceneManager::addImportScene(Scene::SceneId parent_id, const DataBlock &scene_blk, const char *scene_path)
{
  Scene::SceneId newSceneId = Scene::C_INVALID_SCENE_ID;

  if (const Scene::SceneRecord *record = scene.getSceneRecordById(parent_id))
  {
    if (record->loadType != Scene::LoadType::IMPORT)
    {
      return newSceneId;
    }

    Scene::SceneId id = scene.getTargetSceneId();
    scene.setTargetSceneId(parent_id);

    eastl::vector<Scene::SceneId> loadRecordIds{parent_id};
    newSceneId = loadSceneInternal(this, scene_blk, scene_path, nullptr, record->importDepth + 1, Scene::IMPORT, loadRecordIds);

    scene.setNewChangesApplied(parent_id);

    scene.setTargetSceneId(id);
  }

  return newSceneId;
}

bool SceneManager::isSceneTransformable(Scene::SceneId id) const
{
  const Scene::SceneRecord *record = scene.getSceneRecordById(id);

  return record && record->loadType == Scene::LoadType::IMPORT ? record->transformable : false;
}

void SceneManager::setSceneTransformable(Scene::SceneId id, bool val)
{
  if (Scene::SceneRecord *record = scene.getSceneRecordById(id); record && record->loadType == Scene::LoadType::IMPORT)
  {
    record->transformable = val;
    getActiveScene().setNewChangesApplied(id);
  }
}

Point3 SceneManager::getScenePivot(Scene::SceneId id) const
{
  if (const Scene::SceneRecord *record = scene.getSceneRecordById(id))
  {
    return record->pivot;
  }

  return {};
}

void SceneManager::setScenePivot(Scene::SceneId id, const Point3 &pivot)
{
  if (Scene::SceneRecord *record = scene.getSceneRecordById(id); record && record->loadType == Scene::LoadType::IMPORT)
  {
    record->pivot = pivot;
    getActiveScene().setNewChangesApplied(id);
  }
}

bool SceneManager::isSceneInTransformableHierarchy(Scene::SceneId id) const
{
  while (const Scene::SceneRecord *record = scene.getSceneRecordById(id))
  {
    if (record->loadType != Scene::LoadType::IMPORT || !record->transformable)
    {
      return false;
    }

    id = record->parent;
  }

  return true;
}

const char *SceneManager::getScenePrettyName(Scene::SceneId id) const
{
  if (const Scene::SceneRecord *record = scene.getSceneRecordById(id))
  {
    return record->prettyName;
  }

  return nullptr;
}

void SceneManager::setScenePrettyName(Scene::SceneId id, const char *name)
{
  if (Scene::SceneRecord *record = scene.getSceneRecordById(id); record && record->loadType == Scene::LoadType::IMPORT)
  {
    record->prettyName = name;
    getActiveScene().setNewChangesApplied(id);
  }
}

BBox3 SceneManager::getSceneWbb(Scene::SceneId id) const
{
  if (const Scene::SceneRecord *record = scene.getSceneRecordById(id))
  {
    return record->wbb;
  }

  return {};
}

void SceneManager::setNewParent(Scene::SceneId id, Scene::SceneId new_parent_id)
{
  // validate operation
  // scene cannot be its own descendant/parent and must be valid
  {
    const Scene::SceneRecord *recordToMove = scene.getSceneRecordById(id);
    const Scene::SceneRecord *newParentRecord = scene.getSceneRecordById(new_parent_id);
    if (!recordToMove || !newParentRecord || id == new_parent_id || recordToMove->loadType != Scene::LoadType::IMPORT ||
        newParentRecord->loadType != Scene::LoadType::IMPORT)
    {
      return;
    }

    while (newParentRecord)
    {
      if (newParentRecord->parent == id)
      {
        return;
      }

      newParentRecord = scene.getSceneRecordById(newParentRecord->parent);
    }
  }

  Scene::ScenesList hierarchyToMove;
  startMovingHierarchy(scene.allScenes, id, hierarchyToMove);

  if (hierarchyToMove.empty())
  {
    return;
  }

  if (Scene::SceneRecord *oldParent = scene.getSceneRecordById(hierarchyToMove[0].parent))
  {
    eastl::erase_if(oldParent->orderedEntries, [id](auto &&val) { return val.sid == id && !val.isEntity; });
  }
  hierarchyToMove[0].parent = new_parent_id;

  if (auto it = eastl::find_if(scene.allScenes.begin(), scene.allScenes.end(), [&](auto &&val) { return val.id == new_parent_id; });
      it != scene.allScenes.end())
  {
    it->orderedEntries.push_back(Scene::SceneRecord::OrderedEntry{.sid = id, .isEntity = false});
    auto newParentMemberIt = it + 1;
    while (newParentMemberIt != scene.allScenes.end() && newParentMemberIt->importDepth > it->importDepth)
    {
      ++newParentMemberIt;
    }

    const uint32_t baseDepth = hierarchyToMove[0].importDepth;
    for (Scene::SceneRecord &movedScene : hierarchyToMove)
    {
      movedScene.importDepth = it->importDepth + 1 + (movedScene.importDepth - baseDepth);
    }
    scene.allScenes.insert(newParentMemberIt, eastl::move_iterator(hierarchyToMove.begin()),
      eastl::move_iterator(hierarchyToMove.end()));
  }

  getActiveScene().setNewChangesApplied(new_parent_id);
  getActiveScene().setNewChangesApplied(id);
}

uint32_t SceneManager::getSceneOrder(Scene::SceneId scene_id) const
{
  const Scene::SceneRecord *record = scene.getSceneRecordById(scene_id);
  if (!record || !record->hasParent())
  {
    return Scene::C_INVALID_SCENE_ID;
  }

  if (const Scene::SceneRecord *precord = scene.getSceneRecordById(record->parent))
  {
    auto it = eastl::find_if(precord->orderedEntries.cbegin(), precord->orderedEntries.cend(),
      [&](auto &&val) { return val.isEntity == false && val.sid == scene_id; });
    if (it != precord->orderedEntries.cend())
    {
      return eastl::distance(precord->orderedEntries.cbegin(), it);
    }
  }

  return Scene::C_INVALID_SCENE_ID;
}

void SceneManager::setSceneOrder(Scene::SceneId id, uint32_t order)
{
  const Scene::SceneRecord *recordToMove = scene.getSceneRecordById(id);
  if (!recordToMove->hasParent() || recordToMove->loadType != Scene::LoadType::IMPORT)
  {
    return;
  }

  Scene::SceneRecord *parent = scene.getSceneRecordById(recordToMove->parent);
  if (!parent)
  {
    return;
  }

  const Scene::SceneId parentId = parent->id;
  Scene::ScenesList hierarchyToMove;
  startMovingHierarchy(scene.allScenes, id, hierarchyToMove);

  auto it = eastl::find_if(scene.allScenes.begin(), scene.allScenes.end(), [&](auto &&val) { return val.id == parentId; });
  G_ASSERT(it != scene.allScenes.end());

  uint32_t currOrder = 0;
  uint32_t childImportDepth = it->importDepth + 1;

  do
  {
    if (it->importDepth == childImportDepth)
    {
      if (currOrder == order)
      {
        break;
      }
      ++currOrder;
    }

    ++it;
  } while (it != scene.allScenes.end() && it->importDepth >= childImportDepth);

  scene.allScenes.insert(it, eastl::move_iterator(hierarchyToMove.begin()), eastl::move_iterator(hierarchyToMove.end()));

  if (auto recordIt = eastl::find_if(parent->orderedEntries.begin(), parent->orderedEntries.end(),
        [id](auto &&val) { return val.sid == id && !val.isEntity; });
      recordIt != parent->orderedEntries.end())
  {
    recordIt->sid = Scene::C_INVALID_SCENE_ID;
  }
  parent->orderedEntries.insert(parent->orderedEntries.begin() + order,
    Scene::SceneRecord::OrderedEntry{.sid = id, .isEntity = false});
  eastl::erase_if(parent->orderedEntries, [](auto &&val) { return val.sid == Scene::C_INVALID_SCENE_ID && !val.isEntity; });

  getActiveScene().setNewChangesApplied(parent->id);
}

uint32_t SceneManager::getEntityOrder(EntityId eid)
{
  uint32_t retVal = 0;
  if (Scene::EntityRecord *erecord = scene.findEntityRecordForModify(eid))
  {
    if (Scene::SceneRecord *srecord = scene.getSceneRecordById(erecord->sceneId))
    {
      auto it = eastl::find_if(srecord->orderedEntries.begin(), srecord->orderedEntries.end(),
        [eid](auto &&val) { return val.eid == eid && val.isEntity; });
      retVal = it != srecord->orderedEntries.end() ? eastl::distance(srecord->orderedEntries.begin(), it) : 0;
    }
  }

  return retVal;
}

void SceneManager::setEntityOrder(EntityId eid, uint32_t order)
{
  if (Scene::EntityRecord *erecord = scene.findEntityRecordForModify(eid); erecord && erecord->loadType == Scene::LoadType::IMPORT)
  {
    if (Scene::SceneRecord *srecord = scene.getSceneRecordById(erecord->sceneId))
    {
      auto it = eastl::find_if(srecord->orderedEntries.begin(), srecord->orderedEntries.end(),
        [eid](auto &&val) { return val.eid == eid && val.isEntity; });
      if (it == srecord->orderedEntries.end())
      {
        return;
      }

      Scene::SceneRecord::OrderedEntry copy{*it};
      it->eid = INVALID_ENTITY_ID;
      srecord->orderedEntries.insert(srecord->orderedEntries.begin() + order, copy);
      eastl::erase_if(srecord->orderedEntries, [](auto &&val) { return val.eid == INVALID_ENTITY_ID && val.isEntity; });

      getActiveScene().setNewChangesApplied(srecord->id);
      g_entity_mgr->broadcastEvent(EventOnEntitySceneDataChanged{eid});
    }
  }
}

void SceneManager::setEntityParent(Scene::SceneId new_parent_id, const ska::flat_hash_set<ecs::EntityId, ecs::EidHash> &eids)
{
  Scene::SceneId preTargetId = scene.getTargetSceneId();

  if (new_parent_id != Scene::C_INVALID_SCENE_ID)
  {
    scene.setTargetSceneId(new_parent_id);
  }

  for (ecs::EntityId id : eids)
  {
    auto *erec = scene.findEntityRecordForModify(id);
    ComponentsList clist;
    eastl::string templateName;
    if (!erec)
    {
      templateName = g_entity_mgr->getEntityTemplateName(id);
      const ecs::ComponentsMap &compMap = g_entity_mgr->getTemplateDB().getTemplateByName(templateName)->getComponentsMap();
      for (const auto &[compId, childComp] : compMap)
      {
        clist.emplace_back(g_entity_mgr->getTemplateDB().getComponentName(compId), childComp);
      }
    }
    else
    {
      clist = erec->clist;
      templateName = erec->templateName;
    }

    scene.eraseEntityRecord(id);
    if (new_parent_id != Scene::C_INVALID_SCENE_ID)
    {
      scene.insertEntityRecord(id, templateName.c_str(), eastl::move(clist));
    }

    g_entity_mgr->broadcastEvent(EventOnEntitySceneDataChanged{id});
  }

  scene.setTargetSceneId(preTargetId);
}

void SceneManager::removeScene(Scene::SceneId id, const OnEntityDeleteCallback &callback)
{
  if (auto it = eastl::find_if(scene.allScenes.begin(), scene.allScenes.end(), [&](auto &&val) { return val.id == id; });
      it != scene.allScenes.end())
  {
    if (it->loadType != Scene::LoadType::IMPORT)
    {
      return;
    }

    eastl::vector<ecs::Scene::SceneId> deletedIds;

    if (it->hasParent())
    {
      getActiveScene().setNewChangesApplied(it->parent);
    }

    const uint32_t importDepth = it->importDepth;
    auto lastChild = it + 1;
    while (lastChild != scene.allScenes.end() && lastChild->importDepth > importDepth)
    {
      ++lastChild;
    }

    {
      auto toReleaseIt = lastChild;
      while (it != toReleaseIt)
      {
        --toReleaseIt;
        for (const EntityId entId : toReleaseIt->entities)
        {
          if (callback)
          {
            callback(entId);
          }
          g_entity_mgr->destroyEntity(entId);
          scene.entities.erase(entId);
        }
        deletedIds.push_back(toReleaseIt->id);
        idGenerator.release(toReleaseIt->id);
      }
    }

    if (Scene::SceneRecord *parentRecord = getActiveScene().getSceneRecordById(it->parent))
    {
      eastl::erase_if(parentRecord->orderedEntries, [sid = it->id](auto &&val) { return val.sid == sid && val.isEntity == false; });
    }
    scene.allScenes.erase(it, lastChild);

    for (const ecs::Scene::SceneId deletedId : deletedIds)
    {
      g_entity_mgr->broadcastEvent(EventOnSceneDestroyed{deletedId});
    }
  }
}

void SceneManager::recalculateWbbs()
{
  if (scene.allScenes.size() >= 1)
  {
    for (uint32_t i = 0; i < scene.allScenes.size(); ++i)
    {
      if (scene.allScenes.at(i).parent == Scene::SceneRecord::NO_PARENT)
      {
        calculatetSceneWbb(scene.allScenes.at(i).id);
      }
    }
  }
}

BBox3 SceneManager::calculatetSceneWbb(Scene::SceneId id)
{
  auto it = eastl::find_if(scene.allScenes.begin(), scene.allScenes.end(), [&](auto &&val) { return id == val.id; });

  if (it != scene.allScenes.end())
  {
    it->wbb = BBox3{};

    for (const ecs::EntityId entityId : it->entities)
    {
      if (!g_entity_mgr->has(entityId, ECS_HASH("transform")))
      {
        continue;
      }

      const TMatrix base = g_entity_mgr->get<TMatrix>(entityId, ECS_HASH("transform"));
      const Point3 *custom_edit_box__min = g_entity_mgr->getNullable<Point3>(entityId, ECS_HASH("custom_edit_box__min"));
      const Point3 *custom_edit_box__max = g_entity_mgr->getNullable<Point3>(entityId, ECS_HASH("custom_edit_box__max"));
      if (custom_edit_box__min != nullptr && custom_edit_box__max != nullptr)
      {
        it->wbb += base * BBox3(*custom_edit_box__min, *custom_edit_box__max);
        continue;
      }

      AnimV20::AnimcharRendComponent *animChar =
        g_entity_mgr->getNullableRW<AnimV20::AnimcharRendComponent>(entityId, ECS_HASH("animchar_render"));
      if (animChar)
      {
        BBox3 animBB = animChar->getSceneInstance()->getLocalBoundingBox();
        if (g_entity_mgr->getOr(entityId, ECS_HASH("animchar__turnDir"), false))
        {
          Point3 lim0 = it->wbb.lim[0];
          Point3 lim1 = it->wbb.lim[1];

          animBB.lim[0] = Point3(-lim1.z, lim0.y, lim0.x);
          animBB.lim[1] = Point3(-lim0.z, lim1.y, lim1.x);
        }
        it->wbb += base * animBB;
        continue;
      }

      it->wbb += base * BBox3(Point3(-0.1f, -0.1f, -0.1f), Point3(0.1f, 0.1f, 0.1f));
    }

    auto childIt = it;
    while (childIt != scene.allScenes.end())
    {
      if (childIt->parent == id)
      {
        it->wbb += calculatetSceneWbb(childIt->id);
      }

      ++childIt;
    }

    return it->wbb;
  }

  return {};
}

} // namespace ecs
