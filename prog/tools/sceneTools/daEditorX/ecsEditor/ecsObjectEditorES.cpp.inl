// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ecsObjectEditor.h"

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <ecs/rendInst/riExtra.h>

class ECSVisualEntity
{
public:
  bool init(ecs::EntityId eid)
  {
    G_ASSERT(objEntities.empty());

    tryCreateObjEntity(eid, "animchar__res", "animChar");
    if (!tryCreateObjEntity(eid, "effect__name", "fx"))
      if (!tryCreateObjEntity(eid, "ecs_fx__res", "fx"))
        if (!tryCreateObjEntity(eid, "area_fx__res", "fx"))
          tryCreateObjEntity(eid, "camera_fx__res", "fx");
    if (!tryCreateObjEntity(eid, "ri_extra__name", "rendInst"))
      if (!tryCreateObjEntity(eid, "ri_preview__name", "rendInst"))
        tryCreateObjEntity(eid, "ri_gpu_object__name", "rendInst");

    return objEntities.size() != 0;
  }

  void destroy()
  {
    for (IObjEntity *objEntity : objEntities)
      objEntity->destroy();

    objEntities.clear();
  }

  void setTm(const TMatrix &tm)
  {
    for (IObjEntity *objEntity : objEntities)
      objEntity->setTm(tm);
  }

  void swap(ECSVisualEntity &other) { objEntities.swap(other.objEntities); }

private:
  bool tryCreateObjEntity(ecs::EntityId eid, const char *comp_name, const char *asset_type)
  {
    const ecs::string *compValue = g_entity_mgr->getNullable<ecs::string>(eid, ECS_HASH_SLOW(comp_name));
    if (!compValue || compValue->empty())
      return false;

    const DagorAsset *asset = getAssetByName(compValue->c_str(), asset_type);
    if (!asset)
    {
      DAEDITOR3.conError("ECS editor: asset %s:%s cannot be found. Not setting it for %s.", compValue->c_str(), asset_type, comp_name);
      return false;
    }

    IObjEntity *objEntity = DAEDITOR3.createEntity(*asset);
    if (!objEntity)
      return false;
    objEntity->setSubtype(DAEDITOR3.registerEntitySubTypeId("single_ent"));
    objEntities.push_back(objEntity);
    return true;
  }

  static const DagorAsset *getAssetByName(const char *asset_name, const char *asset_type)
  {
    const DagorAsset *asset = DAEDITOR3.getAssetByName(asset_name, DAEDITOR3.getAssetTypeId(asset_type));
    if (!asset && strcmp(asset_type, "fx") == 0)
      asset = DAEDITOR3.getAssetByName(asset_name, DAEDITOR3.getAssetTypeId("efx"));
    return asset;
  }

  dag::Vector<IObjEntity *> objEntities;
};

template <typename Callable>
static void editable_entities_ecs_query(Callable c);

template <typename Callable>
inline void singleton_mutex_ecs_query(Callable c);

// The riExtra substitute IObjEntity objects are intentionally created from ECS events to make them work as close to
// gameLibs/ecs/rendInst as possible. This will make switching to gameLibs/ecs/rendInst eaiser in the future.
static ska::flat_hash_map<ecs::EntityId, ECSVisualEntity, ecs::EidHash> eid_to_obj_entity_map;

void ecs_editor_release_ri_extra_substitutes()
{
  for (auto it = eid_to_obj_entity_map.begin(); it != eid_to_obj_entity_map.end(); ++it)
    it->second.destroy();
  eid_to_obj_entity_map.clear();
}

void ecs_editor_update_visual_entity_tm(ecs::EntityId eid, const TMatrix &tm)
{
  auto it = eid_to_obj_entity_map.find(eid);
  if (it != eid_to_obj_entity_map.end())
    it->second.setTm(tm);
}

bool ecs_editor_ignore_entity_in_collision_test_begin(ecs::EntityId eid)
{
  auto it = eid_to_obj_entity_map.find(eid);
  if (it == eid_to_obj_entity_map.end())
    return false;

  TMatrix newTm = TMatrix::IDENT;
  newTm.m[3][0] = newTm.m[3][2] = 1e6; // Just like AcesRendInstEntity::initTm().
  it->second.setTm(newTm);

  return true;
}

void ecs_editor_ignore_entity_in_collision_test_end(ecs::EntityId eid, const TMatrix &original_tm)
{
  auto it = eid_to_obj_entity_map.find(eid);
  if (it != eid_to_obj_entity_map.end())
    it->second.setTm(original_tm);
}

void ecs_editor_ignore_entities_for_export_begin()
{
  for (auto it = eid_to_obj_entity_map.begin(); it != eid_to_obj_entity_map.end(); ++it)
    it->second.destroy();
}

void ecs_editor_ignore_entities_for_export_end()
{
  for (auto it = eid_to_obj_entity_map.begin(); it != eid_to_obj_entity_map.end(); ++it)
  {
    it->second.init(it->first);
    if (const TMatrix *ecsTm = g_entity_mgr->getNullable<TMatrix>(it->first, ECS_HASH("transform")))
      it->second.setTm(*ecsTm);
  }
}

void ECSObjectEditor::addEditableEntities()
{
  editable_entities_ecs_query([this](ecs::EntityId eid ECS_REQUIRE(eastl::true_type editableObj = true)) { this->addEntity(eid); });
}

void ECSObjectEditor::refreshEids()
{
  editable_entities_ecs_query([this](ecs::EntityId eid ECS_REQUIRE(eastl::true_type editableObj = true)) {
    if (!eidsCreated.count(eid))
      this->addEntity(eid);
  });
}

bool ECSObjectEditor::checkSingletonMutexExists(const char *mutex_name)
{
  bool found = false;
  singleton_mutex_ecs_query([&](ecs::EntityId eid, const ecs::string *singleton_mutex) {
    G_UNUSED(eid);
    if (singleton_mutex && strcmp(singleton_mutex->c_str(), mutex_name) == 0)
      found = true;
  });
  return found;
}

ECS_ON_EVENT(on_appear)
static void riextra_spawn_ri_es(const ecs::Event &, ecs::EntityManager &manager, ecs::EntityId eid, const TMatrix &transform)
{
  ECSVisualEntity visualEntity;
  if (visualEntity.init(eid))
  {
    visualEntity.setTm(transform);

    G_ASSERT(eid_to_obj_entity_map.find(eid) == eid_to_obj_entity_map.end());
    auto result = eid_to_obj_entity_map.insert({eid, ECSVisualEntity()});
    result.first->second.swap(visualEntity);
  }
}

ECS_ON_EVENT(on_disappear)
void riextra_destroyed_es_event_handler(const ecs::Event &, ecs::EntityId eid)
{
  auto it = eid_to_obj_entity_map.find(eid);
  if (it != eid_to_obj_entity_map.end())
  {
    it->second.destroy();
    eid_to_obj_entity_map.erase(eid);
  }
}
