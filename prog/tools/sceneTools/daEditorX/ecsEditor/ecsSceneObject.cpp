// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ecsSceneObject.h"
#include "ecsObjectEditor.h"

#include <daECS/core/entityManager.h>
#include <debug/dag_debug3d.h>

void ecs_editor_update_visual_entity_tm(ecs::EntityId eid, const TMatrix &tm);

namespace
{
TMatrix getRootTransformWithPivot(const ecs::Scene::SceneRecord &scene_record)
{
  TMatrix pivotTm = TMatrix::IDENT;
  pivotTm.setcol(3, scene_record.pivot);
  return scene_record.rootTransform.value_or(TMatrix::IDENT) * pivotTm;
}

void renderBox(const BBox3 &box, bool isSelected, const TMatrix &wtm)
{
#define BOUND_BOX_LEN_DIV    4
#define BOUND_BOX_INDENT_MUL 0.03

  const real deltaX = box[1].x - box[0].x;
  const real deltaY = box[1].y - box[0].y;
  const real deltaZ = box[1].z - box[0].z;

  const real dx = deltaX / BOUND_BOX_LEN_DIV;
  const real dy = deltaY / BOUND_BOX_LEN_DIV;
  const real dz = deltaZ / BOUND_BOX_LEN_DIV;

  const E3DCOLOR c = isSelected ? E3DCOLOR(0xff, 0, 0) : E3DCOLOR(0xff, 0xff, 0xff);

  ::set_cached_debug_lines_wtm(wtm);

  ::draw_cached_debug_line(box[0], box[0] + Point3(dx, 0, 0), c);
  ::draw_cached_debug_line(box[0], box[0] + Point3(0, dy, 0), c);
  ::draw_cached_debug_line(box[0], box[0] + Point3(0, 0, dz), c);

  ::draw_cached_debug_line(box[0] + Point3(deltaX, 0, 0), box[0] + Point3(deltaX - dx, 0, 0), c);
  ::draw_cached_debug_line(box[0] + Point3(deltaX, 0, 0), box[0] + Point3(deltaX, dy, 0), c);
  ::draw_cached_debug_line(box[0] + Point3(deltaX, 0, 0), box[0] + Point3(deltaX, 0, dz), c);

  ::draw_cached_debug_line(box[0] + Point3(deltaX, 0, deltaZ), box[0] + Point3(deltaX - dx, 0, deltaZ), c);
  ::draw_cached_debug_line(box[0] + Point3(deltaX, 0, deltaZ), box[0] + Point3(deltaX, dy, deltaZ), c);
  ::draw_cached_debug_line(box[0] + Point3(deltaX, 0, deltaZ), box[0] + Point3(deltaX, 0, deltaZ - dz), c);

  ::draw_cached_debug_line(box[0] + Point3(0, 0, deltaZ), box[0] + Point3(dx, 0, deltaZ), c);
  ::draw_cached_debug_line(box[0] + Point3(0, 0, deltaZ), box[0] + Point3(0, dy, deltaZ), c);
  ::draw_cached_debug_line(box[0] + Point3(0, 0, deltaZ), box[0] + Point3(0, 0, deltaZ - dz), c);


  ::draw_cached_debug_line(box[1], box[1] - Point3(dx, 0, 0), c);
  ::draw_cached_debug_line(box[1], box[1] - Point3(0, dy, 0), c);
  ::draw_cached_debug_line(box[1], box[1] - Point3(0, 0, dz), c);

  ::draw_cached_debug_line(box[1] - Point3(deltaX, 0, 0), box[1] - Point3(deltaX - dx, 0, 0), c);
  ::draw_cached_debug_line(box[1] - Point3(deltaX, 0, 0), box[1] - Point3(deltaX, dy, 0), c);
  ::draw_cached_debug_line(box[1] - Point3(deltaX, 0, 0), box[1] - Point3(deltaX, 0, dz), c);

  ::draw_cached_debug_line(box[1] - Point3(deltaX, 0, deltaZ), box[1] - Point3(deltaX - dx, 0, deltaZ), c);
  ::draw_cached_debug_line(box[1] - Point3(deltaX, 0, deltaZ), box[1] - Point3(deltaX, dy, deltaZ), c);
  ::draw_cached_debug_line(box[1] - Point3(deltaX, 0, deltaZ), box[1] - Point3(deltaX, 0, deltaZ - dz), c);

  ::draw_cached_debug_line(box[1] - Point3(0, 0, deltaZ), box[1] - Point3(dx, 0, deltaZ), c);
  ::draw_cached_debug_line(box[1] - Point3(0, 0, deltaZ), box[1] - Point3(0, dy, deltaZ), c);
  ::draw_cached_debug_line(box[1] - Point3(0, 0, deltaZ), box[1] - Point3(0, 0, deltaZ - dz), c);

#undef BOUND_BOX_LEN_DIV
#undef BOUND_BOX_INDENT_MUL
}
} // namespace

ECSSceneObject::ECSSceneObject(ecs::Scene::SceneId scene_id) : RenderableEditableObject(), sceneId(scene_id) { init(); }

bool ECSSceneObject::setPos(const Point3 &p)
{
  if (!canBeMoved())
  {
    return false;
  }

  if (!Base::setPos(p))
  {
    return false;
  }

  updateSceneTransform();

  return true;
}

bool ECSSceneObject::isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const { return false; }

bool ECSSceneObject::isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const { return false; }

void ECSSceneObject::setWtm(const TMatrix &wtm)
{
  if (!canBeMoved())
  {
    return;
  }

  Base::setWtm(wtm);
  updateSceneTransform();
}

void ECSSceneObject::update([[maybe_unused]] float dt)
{
  matrix = getRootTransformWithPivot(*ecs::g_scenes->getActiveScene().getSceneRecordById(sceneId));

  if (addEntities)
  {
    createEntityObjects(getObjEditor());
    addEntities = false;
  }
}

void ECSSceneObject::beforeRender() {}

void ECSSceneObject::render()
{
  const BBox3 wbb = ecs::g_scenes->getSceneWbb(sceneId);
  if (!wbb.isempty())
  {
    TMatrix tm = Base::getWtm();
    setWtm(TMatrix::IDENT);
    renderBox(wbb, isSelected(), getWtm());
    setWtm(tm);
  }
}

void ECSSceneObject::renderTrans() {}

bool ECSSceneObject::getWorldBox([[maybe_unused]] BBox3 &box) const
{
  box = ecs::g_scenes->getSceneWbb(sceneId);
  return true;
}

void ECSSceneObject::updateSceneTransform()
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
          ecs_editor_update_visual_entity_tm(entityId, final);
        }
      }
    }

    ++it;
  } while (it != imports.end() && rootImportDepth < (*it)->importDepth);
}

bool ECSSceneObject::mayDelete()
{
  ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  if (const ecs::Scene::SceneRecord *srecord = scene.getSceneRecordById(sceneId);
      srecord && (srecord->importDepth == 0 || srecord->loadType != ecs::Scene::LoadType::IMPORT))
  {
    return false;
  }

  return !isLocked();
}

void ECSSceneObject::onRemove(ObjectEditor *editor)
{
  if (const ecs::Scene::SceneRecord *record = ecs::g_scenes->getActiveScene().getSceneRecordById(sceneId))
  {
    auto &ed = *static_cast<ECSObjectEditor *>(editor);

    undoData = eastl::make_unique<UndoData>();
    undoData->parentId = record->parent;
    undoData->path = record->path;
    undoData->order = ecs::g_scenes->getSceneOrder(sceneId);

    ECSObjectEditor::writeSceneToBlk(undoData->sceneData, *record, ed);

    Tab<RenderableEditableObject *> entities;
    ecs::g_scenes->removeScene(sceneId, [&](ecs::EntityId id) {
      if (RenderableEditableObject *eo = ed.getObjectFromEntityId(id))
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

void ECSSceneObject::onAdd(ObjectEditor *editor)
{
  if (undoData)
  {
    sceneId = ecs::g_scenes->addImportScene(undoData->parentId, undoData->sceneData, undoData->path.c_str());

    if (sceneId != ecs::Scene::C_INVALID_SCENE_ID)
    {
      addEntities = true;
    }

    if (undoData->order != ecs::Scene::C_INVALID_SCENE_ID)
    {
      ecs::g_scenes->setSceneOrder(sceneId, undoData->order);
    }

    undoData.reset();
  }
}

void ECSSceneObject::hideObject(bool hide)
{
  if (const ecs::Scene::SceneRecord *record = ecs::g_scenes->getActiveScene().getSceneRecordById(sceneId))
  {
    const auto &ed = *static_cast<ECSObjectEditor *>(getObjEditor());
    for (const ecs::Scene::SceneRecord::OrderedEntry &entry : record->orderedEntries)
    {
      if (RenderableEditableObject *obj = entry.isEntity ? ed.getObjectFromEntityId(entry.eid) : ed.getObjectFromSceneId(entry.sid))
      {
        obj->hideObject(hide);
      }
    }
  }

  Base::hideObject(hide);
}

void ECSSceneObject::lockObject(bool lock)
{
  if (const ecs::Scene::SceneRecord *record = ecs::g_scenes->getActiveScene().getSceneRecordById(sceneId))
  {
    const auto &ed = *static_cast<ECSObjectEditor *>(getObjEditor());
    for (const ecs::Scene::SceneRecord::OrderedEntry &entry : record->orderedEntries)
    {
      if (RenderableEditableObject *obj = entry.isEntity ? ed.getObjectFromEntityId(entry.eid) : ed.getObjectFromSceneId(entry.sid))
      {
        obj->lockObject(lock);
      }
    }
  }

  Base::lockObject(lock);
}

void ECSSceneObject::createEntityObjects(ObjectEditor *editor)
{
  auto &ed = *static_cast<ECSObjectEditor *>(editor);
  ecs::g_scenes->iterateHierarchy(sceneId, [&](const ecs::Scene::SceneRecord &record) {
    for (const ecs::EntityId eid : record.entities)
    {
      ed.addEntity(eid);
    }
  });
}

bool ECSSceneObject::canBeMoved() const
{
  const ecs::Scene::SceneRecord *record = ecs::g_scenes->getActiveScene().getSceneRecordById(sceneId);
  return record && ecs::g_scenes->isSceneInTransformableHierarchy(sceneId) && record->hasParent() && !isLocked() &&
         record->loadType == ecs::Scene::LoadType::IMPORT;
}

void ECSSceneObject::init()
{
  const ecs::Scene &scene = ecs::g_scenes->getActiveScene();
  const ecs::Scene::SceneRecord &sceneRecord = *scene.getSceneRecordById(sceneId);
  matrix = getRootTransformWithPivot(sceneRecord);
}

void ECSSceneObject::onPPChange([[maybe_unused]] int pid, [[maybe_unused]] bool edit_finished,
  [[maybe_unused]] PropPanel::ContainerPropertyControl &panel, [[maybe_unused]] dag::ConstSpan<RenderableEditableObject *> objects)
{}
