//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_string.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/vector.h>
#include <daECS/core/component.h>
#include <daECS/core/event.h>
#include <daECS/io/blk.h>
#include <generic/dag_initOnDemand.h>

class DataBlock;

namespace ecs
{

typedef eastl::vector<eastl::pair<eastl::string, ecs::ChildComponent>> ComponentsList;

// Sent when all entities that were requested for creation from within (one or more) `loadScene` calls are actually created
ECS_BROADCAST_EVENT_TYPE(EventOnLocalSceneEntitiesCreated)

class SceneManager;
class Scene
{
public:
  struct EntityRecord
  {
    ComponentsList clist;
    uint32_t order : 31;
    uint32_t toBeSaved : 1;
    eastl::string templateName;
    uint32_t sceneIdx : 30;
    uint32_t loadType : 2;
    EntityRecord(ComponentsList &&cl, uint32_t o, const char *tn, bool tbs, uint32_t sI = 0, uint32_t l = 0) :
      clist(eastl::move(cl)), order(o), toBeSaved(tbs ? 1u : 0u), templateName(tn), sceneIdx(sI), loadType(l)
    {}
  };
  typedef ska::flat_hash_map<EntityId, EntityRecord, EidHash> EMap;

  enum LoadType
  {
    UNKNOWN = 0,
    COMMON,
    CLIENT,
    IMPORT
  };

  struct SceneRecord
  {
    static constexpr uint32_t TOP_IMPORT_ORDER = ~0u;
    static constexpr uint32_t NO_PARENT = ~0u;
    eastl::string path;
    int importDepth;
    uint32_t order;
    uint32_t loadType;
    ska::flat_hash_set<EntityId, EidHash> entities;
    uint32_t imports;
    uint32_t parent;
    bool editable;
    SceneRecord(const char *p, int i, uint32_t o, uint32_t l, uint32_t par) :
      order(o), path(p), importDepth(i), loadType(l), imports(0), parent(par), editable(false)
    {}

    bool hasParent() const { return parent != NO_PARENT; }
  };
  typedef eastl::vector<SceneRecord> ScenesList;

  typename EMap::const_iterator begin() const { return entities.begin(); }
  typename EMap::const_iterator end() const { return entities.end(); }
  int entitiesCount() { return entities.size(); }
  const EntityRecord *findEntityRecord(ecs::EntityId eid) const;
  const ecs::ComponentsList *findComponentsList(ecs::EntityId eid) const;
  EntityRecord *findEntityRecordForModify(ecs::EntityId eid);
  void eraseEntityRecord(ecs::EntityId eid);
  void insertEmptyEntityRecord(ecs::EntityId eid, const char *tname);
  void insertEntityRecord(ecs::EntityId eid, const char *tname, ComponentsList const &comps);
  void cloneEntityRecord(ecs::EntityId source_eid, ecs::EntityId dest_eid, const char *template_name);

  const ScenesList &getCommonRecordList() const { return common; }
  const ScenesList &getClientRecordList() const { return client; }
  const ScenesList &getImportsRecordList() const { return imports; }
  bool isChildScene(uint32_t load_type, uint32_t idx);
  uint32_t getNumToBeSavedEntities() const { return numToBeSavedEntities; }

  void getTargetSceneId(uint32_t &load_type, uint32_t &index) const
  {
    load_type = targetLoadType;
    index = targetIndex;
  }
  void setTargetSceneId(uint32_t load_type, uint32_t index);

  bool hasUnsavedChanges() const { return unsavedChanges; }
  bool hasUnsavedChildScenes() const { return unsavedChildScenes; }
  void setNewChangesApplied(bool child_scenes)
  {
    unsavedChanges = true;
    unsavedChildScenes = unsavedChildScenes || child_scenes;
  }
  void setAllChangesWereSaved(bool child_scenes)
  {
    unsavedChanges = false;
    unsavedChildScenes = unsavedChildScenes && !child_scenes;
  }

private:
  bool tryGetSceneRecordList(uint32_t loadType, ScenesList *&sceneRecords)
  {
    if (loadType == Scene::COMMON)
      sceneRecords = &common;
    else if (loadType == Scene::CLIENT)
      sceneRecords = &client;
    else if (loadType == Scene::IMPORT)
      sceneRecords = &imports;
    else
    {
      sceneRecords = nullptr;
      return false;
    }

    return true;
  }

  void insertEntityToTargetSceneRecord(ecs::EntityId eid, bool &toBeSaved, uint32_t &loadType, uint32_t &index)
  {
    toBeSaved = true;
    loadType = UNKNOWN;
    index = 0;
    ScenesList *sceneRecords;
    if (tryGetSceneRecordList(targetLoadType, sceneRecords))
    {
      G_ASSERT(targetIndex < sceneRecords->size());
      SceneRecord &record = sceneRecords->at(targetIndex);
      record.entities.insert(eid);
      toBeSaved = record.editable;
      loadType = targetLoadType;
      index = targetIndex;
    }
  }

  void clear()
  {
    entities.clear();
    common.clear();
    client.clear();
    imports.clear();
    unsavedChanges = false;
    orderSequence = 0;
  }

  EMap entities;
  uint32_t orderSequence = 0;
  uint32_t numToBeSavedEntities = 0;
  ScenesList common;
  ScenesList client;
  ScenesList imports;
  uint32_t targetLoadType = UNKNOWN;
  uint32_t targetIndex = 0;
  bool unsavedChanges = false;
  bool unsavedChildScenes = false;
  friend class SceneManager;
};

class SceneManager
{
public:
  static inline unsigned entityCreationCounter = 0; // Note: counter:24; gen:7; msb_zero:1

  struct CounterGuard
  {
    CounterGuard() { ++entityCreationCounter; }
    ~CounterGuard() { --entityCreationCounter; }
  };
  static void resetEntityCreationCounter()
  {
    unsigned cnt = (entityCreationCounter & (0xffu << 24)) + (1 << 24);
    entityCreationCounter = ((int)cnt >= 0) ? cnt : 0; // Preserve 0 msb for overflow check
  }

  Scene &getActiveScene() { return scene; }
  static void loadScene(SceneManager *smgr, const DataBlock &scene_blk, const char *scene_path, unsigned *ent_counter_ptr,
    int import_depth, uint32_t load_type, const template_allowed_cb_t &template_allowed_cb = {});
  void loadScene(const DataBlock &scene_blk, const char *scene_path)
  {
    loadScene(this, scene_blk, scene_path, nullptr, 0, Scene::IMPORT);
  }
  static void setChildSceneEditable(SceneManager *smgr, uint32_t load_type, uint32_t scene_idx, bool enable);
  void setChildSceneEditable(uint32_t load_type, uint32_t scene_idx, bool enable)
  {
    setChildSceneEditable(this, load_type, scene_idx, enable);
  }
  void clearScene() { scene.clear(); }

private:
  Scene scene;
  friend class Scene;

  static void recordScene(ecs::SceneManager *smgr, uint32_t load_type, const char *path, int import_depth, const uint32_t order,
    eastl::vector<uint32_t> &loadRecordIndices);
};
extern InitOnDemand<SceneManager> g_scenes;

inline Scene::EntityRecord *Scene::findEntityRecordForModify(ecs::EntityId eid)
{
  auto it = entities.find(eid);
  return (it != entities.end()) ? &it->second : nullptr;
}

inline const Scene::EntityRecord *Scene::findEntityRecord(ecs::EntityId eid) const
{
  return const_cast<Scene *>(this)->findEntityRecordForModify(eid);
}

inline const ecs::ComponentsList *Scene::findComponentsList(ecs::EntityId eid) const
{
  auto erec = findEntityRecord(eid);
  return erec ? &erec->clist : nullptr;
}

inline void Scene::eraseEntityRecord(ecs::EntityId eid)
{
  const auto *erec = findEntityRecord(eid);
  if (erec)
  {
    bool toBeSaved = erec->toBeSaved != 0;

    ScenesList *sceneRecords;
    bool childScene = false;
    if (tryGetSceneRecordList(erec->loadType, sceneRecords))
    {
      SceneRecord &record = sceneRecords->at(erec->sceneIdx);
      record.entities.erase(eid);
      childScene = record.hasParent();
      if (childScene && !toBeSaved)
      {
        SceneRecord *parent = &record;
        do
        {
          const uint32_t parentIdx = parent->parent;
          G_ASSERT(parentIdx < sceneRecords->size());
          parent = &sceneRecords->at(parentIdx);
          toBeSaved = toBeSaved || (parent->importDepth == 0);
        } while (parent->hasParent());
      }
    }

    if (entities.erase(eid))
      if (toBeSaved)
        setNewChangesApplied(childScene);
  }
}

inline void Scene::insertEmptyEntityRecord(ecs::EntityId eid, const char *tname)
{
  bool toBeSaved;
  uint32_t loadType, sceneIdx;
  insertEntityToTargetSceneRecord(eid, toBeSaved, loadType, sceneIdx);
  entities.emplace(eid, Scene::EntityRecord(ComponentsList{}, orderSequence++, tname, toBeSaved, sceneIdx, loadType));
  setNewChangesApplied(isChildScene(loadType, sceneIdx));
}

inline void Scene::insertEntityRecord(ecs::EntityId eid, const char *tname, ComponentsList const &comps)
{
  bool toBeSaved;
  uint32_t loadType, sceneIdx;
  insertEntityToTargetSceneRecord(eid, toBeSaved, loadType, sceneIdx);
  entities.emplace(eid, Scene::EntityRecord(ComponentsList(comps), orderSequence++, tname, toBeSaved, sceneIdx, loadType));
  setNewChangesApplied(isChildScene(loadType, sceneIdx));
}

inline void Scene::cloneEntityRecord(ecs::EntityId source_eid, ecs::EntityId dest_eid, const char *template_name)
{
  auto it = entities.find(source_eid);
  if (it != entities.end())
  {
    EntityRecord rec(ComponentsList(it->second.clist), orderSequence++,
      template_name ? template_name : it->second.templateName.c_str(),
      /*toBeSaved*/ true, it->second.sceneIdx, it->second.loadType);
    entities.insert_or_assign(dest_eid, eastl::move(rec));

    ScenesList *sceneRecords;
    bool childScene = false;
    if (tryGetSceneRecordList(it->second.loadType, sceneRecords))
    {
      uint32_t idx = it->second.sceneIdx;
      G_ASSERT(idx < sceneRecords->size());
      SceneRecord &record = sceneRecords->at(idx);
      record.entities.insert(dest_eid);
      childScene = record.hasParent();
    }

    setNewChangesApplied(childScene);
  }
}

inline bool Scene::isChildScene(uint32_t load_type, uint32_t idx)
{
  Scene::ScenesList *sceneRecords;
  if (tryGetSceneRecordList(load_type, sceneRecords))
  {
    G_ASSERT(idx < sceneRecords->size());
    return sceneRecords->at(idx).hasParent();
  }
  return false;
}

inline void Scene::setTargetSceneId(uint32_t load_type, uint32_t index)
{
  Scene::ScenesList *sceneRecords;
  if (tryGetSceneRecordList(load_type, sceneRecords))
  {
    G_ASSERT(index < sceneRecords->size());
    targetLoadType = load_type;
    targetIndex = index;
  }
  else
  {
    if (load_type == UNKNOWN && index == 0)
    {
      targetLoadType = load_type;
      targetIndex = index;
    }
    else
    {
      G_ASSERT_FAIL("Trying to set invalid target scene id (%u:%u)", load_type, index);
    }
  }
}

} // namespace ecs
