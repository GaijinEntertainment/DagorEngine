//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/functional.h>
#include <EASTL/vector.h>
#include <EASTL/optional.h>
#include <EASTL/stack.h>
#include <daECS/core/component.h>
#include <daECS/core/event.h>
#include <generic/dag_initOnDemand.h>
#include <math/dag_TMatrix.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>

class DataBlock;

namespace ecs
{

typedef eastl::vector<eastl::pair<eastl::string, ecs::ChildComponent>> ComponentsList;

class SceneManager;
class Scene
{
public:
  using SceneId = uint32_t;
  inline static constexpr SceneId C_INVALID_SCENE_ID = eastl::numeric_limits<SceneId>::max();

  struct EntityRecord
  {
    ComponentsList clist;
    eastl::string templateName;
    SceneId sceneId : 30;
    uint32_t loadType : 2;
    bool wasTransformChanged = false;
    EntityRecord(ComponentsList &&cl, const char *tn, SceneId sId = 0, uint32_t l = 0) :
      clist(eastl::move(cl)), templateName(tn), sceneId(sId), loadType(l)
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
    static constexpr SceneId NO_PARENT = C_INVALID_SCENE_ID;

    struct OrderedEntry
    {
      union
      {
        EntityId eid;
        SceneId sid;
      };
      bool isEntity;
    };

    eastl::string path;
    int importDepth;
    uint32_t loadType;
    ska::flat_hash_set<EntityId, EidHash> entities;
    eastl::vector<OrderedEntry> orderedEntries;
    SceneId parent;
    bool transformable = true;
    Point3 pivot = Point3::ZERO;
    eastl::optional<TMatrix> rootTransform;
    SimpleString prettyName;
    BBox3 wbb;
    SceneId id = C_INVALID_SCENE_ID;
    bool isDirty = false;

    SceneRecord(const char *p, int i, uint32_t l, SceneId par, SceneId id) : path(p), importDepth(i), loadType(l), parent(par), id(id)
    {}

    bool hasParent() const { return parent != NO_PARENT; }
  };
  using ScenesList = eastl::vector<SceneRecord>;
  using ScenesConstPtrList = eastl::vector<const SceneRecord *>;
  using ScenesPtrList = eastl::vector<SceneRecord *>;

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

  const ScenesList &getAllScenes() const { return allScenes; }
  ScenesList &getAllScenes() { return allScenes; }

  ScenesConstPtrList getScenes(LoadType load_type) const
  {
    ScenesConstPtrList ret;
    for (const SceneRecord &record : allScenes)
    {
      if (record.loadType == load_type)
      {
        ret.emplace_back(&record);
      }
    }
    return ret;
  }

  ScenesPtrList getScenesForModify(LoadType load_type)
  {
    ScenesPtrList ret;
    for (SceneRecord &record : allScenes)
    {
      if (record.loadType == load_type)
      {
        ret.emplace_back(&record);
      }
    }
    return ret;
  }

  bool isChildScene(SceneId id);

  SceneId getTargetSceneId() const { return targetId; }
  void setTargetSceneId(SceneId id);

  bool hasUnsavedChanges() const
  {
    for (const SceneRecord &record : allScenes)
    {
      if (record.loadType == LoadType::IMPORT && record.isDirty)
      {
        return true;
      }
    }

    return false;
  }

  void setNewChangesApplied(SceneId scene_id)
  {
    if (SceneRecord *record = getSceneRecordById(scene_id))
    {
      record->isDirty = true;
    }
  }

  void setAllChangesWereSaved()
  {
    for (SceneRecord &record : allScenes)
    {
      record.isDirty = false;
    }
  }

  void notifyEntityTransformWasChanged(ecs::EntityId eid)
  {
    ecs::Scene::EntityRecord *erec = findEntityRecordForModify(eid);
    if (erec && erec->loadType == LoadType::IMPORT)
    {
      erec->wasTransformChanged = true;
    }
  }

  const Scene::SceneRecord *getSceneRecordById(Scene::SceneId id) const
  {
    if (id == C_INVALID_SCENE_ID)
    {
      return nullptr;
    }

    const auto it = eastl::find_if(allScenes.cbegin(), allScenes.cend(), [id](const Scene::SceneRecord &val) { return val.id == id; });

    return it != allScenes.cend() ? it : nullptr;
  }

  Scene::SceneRecord *getSceneRecordById(Scene::SceneId id)
  {
    if (id == C_INVALID_SCENE_ID)
    {
      return nullptr;
    }

    const auto it = eastl::find_if(allScenes.begin(), allScenes.end(), [id](const Scene::SceneRecord &val) { return val.id == id; });

    return it != allScenes.end() ? it : nullptr;
  }

private:
  bool insertEntityToTargetSceneRecord(ecs::EntityId eid, uint32_t &loadType, SceneId &id)
  {
    loadType = UNKNOWN;
    id = C_INVALID_SCENE_ID;
    if (Scene::SceneRecord *record = getSceneRecordById(targetId))
    {
      if (auto result = record->entities.insert(eid); result.second)
      {
        record->orderedEntries.push_back(Scene::SceneRecord::OrderedEntry{.eid = eid, .isEntity = true});
      }
      loadType = record->loadType;
      id = record->id;
      return true;
    }

    return false;
  }

  void clear()
  {
    entities.clear();
    allScenes.clear();
    orderSequence = 0;
  }

  EMap entities;
  uint32_t orderSequence = 0;
  ScenesList allScenes;
  uint32_t targetId = C_INVALID_SCENE_ID;
  friend class SceneManager;
};

class SceneManager
{
public:
  static inline unsigned entityCreationCounter = 0; // Note: counter:24; gen:7; msb_zero:1

  using OnEntityDeleteCallback = eastl::function<void(EntityId)>;

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
  const Scene &getActiveScene() const { return scene; }
  static void loadScene(SceneManager *smgr, const DataBlock &scene_blk, const char *scene_path, unsigned *ent_counter_ptr,
    int import_depth, uint32_t load_type);
  void loadScene(const DataBlock &scene_blk, const char *scene_path)
  {
    loadScene(this, scene_blk, scene_path, nullptr, 0, Scene::IMPORT);
  }
  Scene::SceneId addImportScene(Scene::SceneId parent_id, const DataBlock &scene_blk, const char *scene_path);

  // Pivot, transformable, pretty name getters/setters work on Import scenes
  bool isSceneTransformable(Scene::SceneId id) const;
  void setSceneTransformable(Scene::SceneId id, bool val);
  Point3 getScenePivot(Scene::SceneId id) const;

  void setScenePivot(Scene::SceneId id, const Point3 &pivot);
  bool isSceneInTransformableHierarchy(Scene::SceneId id) const;

  const char *getScenePrettyName(Scene::SceneId id) const;
  void setScenePrettyName(Scene::SceneId id, const char *name);

  BBox3 getSceneWbb(Scene::SceneId id) const;

  void setNewParent(Scene::SceneId id, Scene::SceneId new_parent_id);

  uint32_t getSceneOrder(Scene::SceneId scene_id) const;
  void setSceneOrder(Scene::SceneId id, uint32_t order);
  uint32_t getEntityOrder(EntityId eid);
  void setEntityOrder(EntityId eid, uint32_t order);

  void setEntityParent(Scene::SceneId new_parent_id, const ska::flat_hash_set<ecs::EntityId, ecs::EidHash> &eids);

  void removeScene(Scene::SceneId id, const OnEntityDeleteCallback &callback = {});

  void clearScene() { scene.clear(); }
  void recalculateWbbs();

  template <typename T>
  void iterateHierarchy(Scene::SceneId scene_id, T &&callback) const
  {
    if (auto it = eastl::find_if(scene.allScenes.begin(), scene.allScenes.end(), [&](auto &&val) { return val.id == scene_id; });
        it != scene.allScenes.end())
    {
      const uint32_t depth = it->importDepth;

      do
      {
        callback(*it);
        ++it;
      } while (it != scene.allScenes.end() && it->importDepth > depth);
    }
  }

private:
  Scene scene;

  friend class Scene;

  class SceneIdGenerator final
  {
  public:
    Scene::SceneId acquire()
    {
      if (idsToReuse.empty())
      {
        return nextFreeId++;
      }
      else
      {
        const Scene::SceneId id = idsToReuse.top();
        idsToReuse.pop();
        return id;
      }
    }

    void release(Scene::SceneId id) { idsToReuse.push(id); }

  private:
    eastl::stack<Scene::SceneId> idsToReuse;
    Scene::SceneId nextFreeId = 0;
  };

  SceneIdGenerator idGenerator;

  BBox3 calculatetSceneWbb(Scene::SceneId id);

  static Scene::SceneId recordScene(ecs::SceneManager *smgr, uint32_t load_type, const char *path, int import_depth,
    eastl::vector<Scene::SceneId> &loadRecordIndices, const TMatrix *rootTransform);
  static Scene::SceneId loadSceneInternal(SceneManager *smgr, const DataBlock &scene_blk, const char *scene_path,
    unsigned *ent_counter_ptr, int import_depth, uint32_t load_type, eastl::vector<Scene::SceneId> &loadRecordIndices);
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
    bool childScene = false;
    if (Scene::SceneRecord *record = getSceneRecordById(erec->sceneId))
    {
      record->entities.erase(eid);
      childScene = record->hasParent();
      if (childScene)
      {
        SceneId parentId = record->parent;
        while (Scene::SceneRecord *parentRecord = getSceneRecordById(parentId))
        {
          parentId = parentRecord->parent;
        }
      }

      eastl::erase_if(record->orderedEntries, [eid](auto &&val) { return val.eid == eid && val.isEntity == true; });
      setNewChangesApplied(record->id);
    }

    entities.erase(eid);
  }
}

inline void Scene::insertEmptyEntityRecord(ecs::EntityId eid, const char *tname)
{
  uint32_t loadType;
  SceneId sceneId;
  if (insertEntityToTargetSceneRecord(eid, loadType, sceneId))
  {
    entities.emplace(eid, Scene::EntityRecord(ComponentsList{}, tname, sceneId, loadType));
    setNewChangesApplied(sceneId);
  }
}

inline void Scene::insertEntityRecord(ecs::EntityId eid, const char *tname, ComponentsList const &comps)
{
  uint32_t loadType;
  SceneId sceneId;
  if (insertEntityToTargetSceneRecord(eid, loadType, sceneId))
  {
    entities.emplace(eid, Scene::EntityRecord(ComponentsList(comps), tname, sceneId, loadType));
    setNewChangesApplied(sceneId);
  }
}

inline void Scene::cloneEntityRecord(ecs::EntityId source_eid, ecs::EntityId dest_eid, const char *template_name)
{
  auto it = entities.find(source_eid);
  if (it != entities.end())
  {
    if (SceneRecord *record = getSceneRecordById(it->second.sceneId))
    {
      if (auto result = record->entities.insert(dest_eid); result.second)
      {
        record->orderedEntries.push_back(Scene::SceneRecord::OrderedEntry{.eid = dest_eid, .isEntity = true});
      }
      setNewChangesApplied(record->id);
    }

    EntityRecord rec(ComponentsList(it->second.clist), template_name ? template_name : it->second.templateName.c_str(),
      it->second.sceneId, it->second.loadType);
    entities.insert_or_assign(dest_eid, eastl::move(rec));
  }
}

inline bool Scene::isChildScene(SceneId id)
{
  if (const SceneRecord *record = getSceneRecordById(id))
  {
    return record->hasParent();
  }
  return false;
}

inline void Scene::setTargetSceneId(SceneId id)
{
  if (const Scene::SceneRecord *record = getSceneRecordById(id))
  {
    targetId = id;
  }
  else
  {
    if (id == C_INVALID_SCENE_ID)
    {
      targetId = id;
    }
    else
    {
      G_ASSERT_FAIL("Trying to set invalid target scene id %u", id);
    }
  }
}

// Sent when all entities that were requested for creation from within (one or more) `loadScene` calls are actually created
ECS_BROADCAST_EVENT_TYPE(EventOnLocalSceneEntitiesCreated)
ECS_BROADCAST_EVENT_TYPE(EventOnSceneCreated, Scene::SceneId /*sceneId*/)
ECS_BROADCAST_EVENT_TYPE(EventOnSceneDestroyed, Scene::SceneId /*sceneId*/)
ECS_BROADCAST_EVENT_TYPE(EventOnEntitySceneDataChanged, EntityId /*eid*/)
} // namespace ecs
