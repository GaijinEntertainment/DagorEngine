// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_ObjectEditor.h>
#include <de3_interface.h>
#include <ioSys/dag_dataBlock.h>
#include "ecsSceneOutlinerUndoRedo.h"
#include "../ecsToEditableObjectConverter.h"
#include "../ecsEntityObject.h"
#include "../ecsSceneObject.h"

AddImportUndoRedo::AddImportUndoRedo(ECSSceneObject *sobj, ObjectEditor &editor) noexcept : sobj(sobj), editor(editor) {}

void AddImportUndoRedo::restore([[maybe_unused]] bool save_redo) { editor.removeObject(sobj, false); }

void AddImportUndoRedo::redo() { editor.addObject(sobj, false); }

ECSSceneOutlinerPanel::SetEntityParentUndoRedo::SetEntityParentUndoRedo(ECSSceneObject *new_parent, ECSEntityObject *eobj,
  ECSSceneOutlinerPanel &owner) noexcept :
  prevParent(nullptr), newParent(new_parent), eobj(eobj), owner(owner)
{
  const ecs::EntityId eid = this->eobj->getEid();
  if (const ecs::Scene::EntityRecord *erecord = ecs::g_scenes->getActiveScene().findEntityRecord(eid))
  {
    prevParent = owner.getSceneObject(erecord->sceneId);
    prevOrder = ecs::g_scenes->getEntityOrder(eid);
  }
}

void ECSSceneOutlinerPanel::SetEntityParentUndoRedo::restore([[maybe_unused]] bool save_redo)
{
  const ecs::EntityId eid = eobj->getEid();
  if (!prevParent)
  {
    ecs::g_scenes->setEntityParent(ecs::Scene::C_INVALID_SCENE_ID, {eid});
  }
  else
  {
    ecs::g_scenes->setEntityParent(prevParent->getSceneId(), {eid});
    ecs::g_scenes->setEntityOrder(eid, prevOrder);
  }
}

void ECSSceneOutlinerPanel::SetEntityParentUndoRedo::redo()
{
  const ecs::EntityId eid = eobj->getEid();
  if (!newParent)
  {
    ecs::g_scenes->setEntityParent(ecs::Scene::C_INVALID_SCENE_ID, {eid});
  }
  else
  {
    ecs::g_scenes->setEntityParent(newParent->getSceneId(), {eid});
  }
}

ECSSceneOutlinerPanel::SetSceneParentUndoRedo::SetSceneParentUndoRedo(ECSSceneObject *new_parent, ECSSceneObject *sobj,
  ECSSceneOutlinerPanel &owner) noexcept :
  prevParent(nullptr), newParent(new_parent), sobj(sobj), owner(owner)
{
  const ecs::Scene::SceneId sid = sobj->getSceneId();
  if (const ecs::Scene::SceneRecord *srecord = ecs::g_scenes->getActiveScene().getSceneRecordById(sid))
  {
    prevParent = owner.getSceneObject(srecord->parent);
    prevOrder = ecs::g_scenes->getSceneOrder(sid);
  }
}

void ECSSceneOutlinerPanel::SetSceneParentUndoRedo::restore([[maybe_unused]] bool save_redo)
{
  const ecs::Scene::SceneId sid = sobj->getSceneId();
  const ecs::Scene::SceneId prevParentId = prevParent->getSceneId();
  ecs::g_scenes->setNewParent(sid, prevParentId);
  ecs::g_scenes->setSceneOrder(sid, prevOrder);
  if (sobj && prevParent)
  {
    const auto sIt = owner.edObjToHandle.find(sobj);
    const auto pIt = owner.edObjToHandle.find(prevParent);
    if (sIt != owner.edObjToHandle.cend() && pIt != owner.edObjToHandle.cend())
    {
      owner.tree->setNewParent(sIt->second, pIt->second);
      owner.tree->setChildIndex(sIt->second, prevOrder);
    }
  }
}

void ECSSceneOutlinerPanel::SetSceneParentUndoRedo::redo()
{
  const ecs::Scene::SceneId sid = sobj->getSceneId();
  const ecs::Scene::SceneId newParentId = newParent->getSceneId();
  ecs::g_scenes->setNewParent(sid, newParentId);
  if (sobj && newParent)
  {
    const auto eIt = owner.edObjToHandle.find(sobj);
    const auto pIt = owner.edObjToHandle.find(newParent);
    if (eIt != owner.edObjToHandle.cend() && pIt != owner.edObjToHandle.cend())
    {
      owner.tree->setNewParent(eIt->second, pIt->second);
    }
  }
}

ECSSceneOutlinerPanel::SetEntityOrderUndoRedo::SetEntityOrderUndoRedo(uint32_t prev_order, uint32_t new_order, ECSEntityObject *eobj,
  ECSSceneOutlinerPanel &owner) noexcept :
  prevOrder(prev_order), newOrder(new_order), eobj(eobj), owner(owner)
{
  if (newOrder < prevOrder)
  {
    prevOrder += 1;
  }
}

void ECSSceneOutlinerPanel::SetEntityOrderUndoRedo::restore([[maybe_unused]] bool save_redo)
{
  ecs::g_scenes->setEntityOrder(eobj->getEid(), prevOrder);
}

void ECSSceneOutlinerPanel::SetEntityOrderUndoRedo::redo() { ecs::g_scenes->setEntityOrder(eobj->getEid(), newOrder); }

ECSSceneOutlinerPanel::SetSceneOrderUndoRedo::SetSceneOrderUndoRedo(uint32_t prev_order, uint32_t new_order, ECSSceneObject *sobj,
  ECSSceneOutlinerPanel &owner) noexcept :
  prevOrder(prev_order), newOrder(new_order), sobj(sobj), owner(owner)
{
  if (newOrder < prevOrder)
  {
    prevOrder += 1;
  }
}

void ECSSceneOutlinerPanel::SetSceneOrderUndoRedo::restore([[maybe_unused]] bool save_redo)
{
  ecs::g_scenes->setSceneOrder(sobj->getSceneId(), prevOrder);
  if (sobj)
  {
    const auto sIt = owner.edObjToHandle.find(sobj);
    if (sIt != owner.edObjToHandle.cend())
    {
      owner.tree->setChildIndex(sIt->second, prevOrder);
    }
  }
}

void ECSSceneOutlinerPanel::SetSceneOrderUndoRedo::redo()
{
  ecs::g_scenes->setSceneOrder(sobj->getSceneId(), newOrder);
  if (sobj)
  {
    const auto sIt = owner.edObjToHandle.find(sobj);
    if (sIt != owner.edObjToHandle.cend())
    {
      owner.tree->setChildIndex(sIt->second, newOrder);
    }
  }
}
