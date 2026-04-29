// required externals:
//   ECS_EVENT(CmdTeleportEntity, TMatrix /* newtm*/, bool /*hard teleport*/)

#include "sceneObj.h"

#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daEditorE/editorCommon/entityEditor.h>
#include <debug/dag_log.h>
#include <quirrel/sqEventBus/sqEventBus.h>

namespace
{
TMatrix getRootTransformWithPivot(const ecs::Scene::SceneRecord &scene_record)
{
  TMatrix pivotTm = TMatrix::IDENT;
  pivotTm.setcol(3, scene_record.pivot);
  return scene_record.rootTransform.value_or(TMatrix::IDENT) * pivotTm;
}
} // namespace

SceneObj::SceneObj(ecs::Scene::SceneId scene_id) : EditableObject(), sceneId(scene_id) { init(); }

bool SceneObj::isValid() const { return !!ecs::g_scenes->getActiveScene().getSceneRecordById(sceneId); }

bool SceneObj::setPos(const Point3 &p)
{
  if (!isValid() || !canBeMoved())
  {
    return false;
  }

  if (!EditableObject::setPos(p))
  {
    return false;
  }

  updateSceneTransform();

  return true;
}

void SceneObj::setWtm(const TMatrix &wtm)
{
  if (!isValid() || !canBeMoved())
  {
    return;
  }

  EditableObject::setWtm(wtm);
  updateSceneTransform();
}

void SceneObj::update([[maybe_unused]] float dt)
{
  if (!isValid())
  {
    return;
  }

  matrix = getRootTransformWithPivot(*ecs::g_scenes->getActiveScene().getSceneRecordById(sceneId));
}

void SceneObj::beforeRender() {}

void SceneObj::render()
{
  if (!isValid())
  {
    return;
  }

  const BBox3 wbb = ecs::g_scenes->getSceneWbb(sceneId);
  if (!wbb.isempty())
  {
    TMatrix tm = EditableObject::getWtm();
    EditableObject::setWtm(TMatrix::IDENT);
    renderBox(wbb);
    EditableObject::setWtm(tm);
  }
}

void SceneObj::renderTrans() {}

bool SceneObj::getWorldBox([[maybe_unused]] BBox3 &box) const
{
  if (!isValid())
  {
    return false;
  }

  box = ecs::g_scenes->getSceneWbb(sceneId);
  return true;
}

BBox3 SceneObj::getLbb() const { return {}; }

void SceneObj::updateSceneTransform()
{
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  const ecs::Scene::ScenesPtrList &imports = scene.getScenesForModify(ecs::Scene::LoadType::IMPORT);
  auto it = eastl::find_if(imports.begin(), imports.end(), [&](auto &&val) { return sceneId == val->id; });
  if (it == imports.end())
  {
    return;
  }

  TMatrix pivotTm = TMatrix::IDENT;
  pivotTm.setcol(3, (*it)->pivot);

  const TMatrix dtm = matrix * inverse(getRootTransformWithPivot(**it));
  (*it)->rootTransform = matrix * inverse(pivotTm);

  if (const ecs::Scene::SceneRecord *parentRecord = scene.getSceneRecordById((*it)->parent))
  {
    scene.setNewChangesApplied(parentRecord->id);
  }

  const int rootImportDepth = (*it)->importDepth;
  do
  {
    if (ecs::g_scenes->isSceneInTransformableHierarchy((*it)->id))
    {
      for (const ecs::EntityId entityId : (*it)->entities)
      {
        if (g_entity_mgr->has(entityId, ECS_HASH("transform")))
        {
          const TMatrix base = g_entity_mgr->get<TMatrix>(entityId, ECS_HASH("transform"));
          const TMatrix final = dtm * base;
          g_entity_mgr->set(entityId, ECS_HASH("transform"), final);
          g_entity_mgr->sendEvent(entityId, CmdTeleportEntity(final, true));
        }
      }
    }

    ++it;
  } while (it != imports.end() && rootImportDepth < (*it)->importDepth);
}

bool SceneObj::mayDelete()
{
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  if (const ecs::Scene::SceneRecord *srecord = scene.getSceneRecordById(sceneId);
      srecord && srecord->importDepth == 0 && srecord->loadType != ecs::Scene::LoadType::IMPORT)
  {
    return false;
  }

  auto &ed = *static_cast<EntityObjEditor *>(getObjEditor());
  return !ed.isSceneInLockedHierarchy(sceneId);
}

void SceneObj::onRemove(ObjectEditor *editor)
{
  if (const ecs::Scene::SceneRecord *record = ecs::g_scenes->getActiveScene().getSceneRecordById(sceneId))
  {
    auto &ed = *static_cast<EntityObjEditor *>(editor);

    undoData = eastl::make_unique<UndoData>();
    undoData->parentId = record->parent;
    undoData->path = record->path;
    undoData->order = ecs::g_scenes->getSceneOrder(sceneId);

    EntityObjEditor::writeSceneToBlk(undoData->sceneData, *record, ed);

    Tab<EditableObject *> entities;
    ecs::g_scenes->removeScene(sceneId, [&](ecs::EntityId id) {
      if (EditableObject *eo = ed.getObjectByEid(id))
      {
        entities.push_back(eo);
      }
    });

    if (!entities.empty())
    {
      ed.removeObjects(&entities[0], entities.size(), false);
    }

    g_entity_mgr->performDelayedCreation();
  }
}

void SceneObj::onAdd(ObjectEditor *editor)
{
  auto &ed = *static_cast<EntityObjEditor *>(editor);

  if (undoData)
  {
    sceneId = ecs::g_scenes->addImportScene(undoData->parentId, undoData->sceneData, undoData->path.c_str());

    if (sceneId != ecs::Scene::C_INVALID_SCENE_ID)
    {
      ecs::g_scenes->iterateHierarchy(sceneId, [&](const ecs::Scene::SceneRecord &record) {
        for (const ecs::EntityId eid : record.entities)
        {
          ed.addEntity(eid);
        }
      });
    }

    if (undoData->order != ecs::Scene::C_INVALID_SCENE_ID)
    {
      ecs::g_scenes->setSceneOrder(sceneId, undoData->order);
      sqeventbus::send_event("entity_editor.onEcsScenesStateChanged");
    }

    undoData.reset();
  }
}

bool SceneObj::canBeMoved() const
{
  const ecs::Scene::SceneRecord *record = ecs::g_scenes->getActiveScene().getSceneRecordById(sceneId);
  return record && ecs::g_scenes->isSceneInTransformableHierarchy(sceneId) && record->hasParent() &&
         record->loadType == ecs::Scene::LoadType::IMPORT;
}

void SceneObj::init()
{
  if (!SceneObj::isValid())
  {
    logerr("SceneObj is invalid, import scene index %i does not exist", sceneId);
    return;
  }

  const ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  const ecs::Scene::SceneRecord &sceneRecord = *scene.getSceneRecordById(sceneId);
  matrix = getRootTransformWithPivot(sceneRecord);
}