// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_ObjectEditor.h>
#include "ecsSceneOutlinerDragDrop.h"
#include "ecsSceneOutlinerUndoRedo.h"
#include "../ecsToEditableObjectConverter.h"

namespace
{
constexpr char ECS_SCENE_TREE_DRAG_DROP_PAYLOAD[] = "ECS_SCENE_D&D_PAYLOAD";
}

ECSSceneOutlinerPanel::DragHandler::DragHandler(ECSSceneOutlinerPanel &owner) noexcept : owner(owner) {}

void ECSSceneOutlinerPanel::DragHandler::DragHandler::onBeginDrag(PropPanel::TLeafHandle leaf)
{
  if (!owner.tree)
  {
    return;
  }

  DragAndDropPayload payload = collectPayload(leaf);

  if (!verifyPayload(payload))
  {
    return;
  }

  ImGui::SetDragDropPayload(ECS_SCENE_TREE_DRAG_DROP_PAYLOAD, payload.items.data(),
    payload.items.size() * sizeof(PropPanel::TLeafHandle));
  G_ASSERT(!payload.items.empty());

  if (payload.items.size() == 1)
  {
    ImGui::Text("%s", owner.tree->getCaption(payload.items[0]).c_str());
  }
  else
  {
    ImGui::Text("%s and %d more items", owner.tree->getCaption(payload.items[0]).c_str(), payload.items.size() - 1);
  }
}

ECSSceneOutlinerPanel::DragAndDropPayload ECSSceneOutlinerPanel::DragHandler::collectPayload(PropPanel::TLeafHandle current_leaf)
{
  DragAndDropPayload payload;
  dag::Vector<PropPanel::TLeafHandle> selectedLeaves;
  owner.tree->getSelectedLeafs(selectedLeaves, true, true);
  if (eastl::find(selectedLeaves.cbegin(), selectedLeaves.cend(), current_leaf) == selectedLeaves.cend())
  {
    payload.items.push_back(current_leaf);
  }
  else
  {
    payload.items = std::move(selectedLeaves);
  }

  return payload;
}

bool ECSSceneOutlinerPanel::DragHandler::verifyPayload(const DragAndDropPayload &payload)
{
  for (const PropPanel::TLeafHandle leaf : payload.items)
  {
    const auto *leafUserData = reinterpret_cast<ECSSceneOutlinerPanel::LeafUserData *>(owner.tree->getUserData(leaf));
    if (leafUserData->type == LeafType::SCENE)
    {
      const ecs::Scene::SceneRecord *record = ecs::g_scenes->getActiveScene().getSceneRecordById(leafUserData->sid);
      if (!record || record->importDepth == 0 || record->loadType != ecs::Scene::LoadType::IMPORT)
      {
        return false;
      }
    }
  }
  return true;
}

ECSSceneOutlinerPanel::DropHandler::DropHandler(ECSSceneOutlinerPanel &owner) noexcept : owner(owner) {}

PropPanel::DragAndDropResult ECSSceneOutlinerPanel::DropHandler::onDropTargetDirect(PropPanel::TLeafHandle leaf)
{
  if (!owner.tree)
  {
    return PropPanel::DragAndDropResult::NONE;
  }

  const ImGuiPayload *dragAndDropPayload =
    PropPanel::acceptDragDropPayloadBeforeDelivery(ECS_SCENE_TREE_DRAG_DROP_PAYLOAD, canDropBetween());
  if (!dragAndDropPayload)
  {
    return PropPanel::DragAndDropResult::NONE;
  }

  const auto *destUserData = reinterpret_cast<LeafUserData *>(owner.tree->getUserData(leaf));
  if (destUserData->type == LeafType::ENTITY)
  {
    return PropPanel::DragAndDropResult::NOT_ALLOWED;
  }

  if (RenderableEditableObject *rObj = owner.getEditableObjectFromHandle(leaf);
      leaf != owner.noSceneLeaf && (!rObj || rObj->isLocked()))
  {
    return PropPanel::DragAndDropResult::NOT_ALLOWED;
  }

  DragAndDropPayload payload;
  payload.items.resize(dragAndDropPayload->DataSize / sizeof(PropPanel::TLeafHandle));
  memcpy(payload.items.data(), dragAndDropPayload->Data, dragAndDropPayload->DataSize);

  if (!verifyPayloadDirect(payload, leaf))
  {
    return PropPanel::DragAndDropResult::NOT_ALLOWED;
  }

  if (dragAndDropPayload->IsDelivery())
  {
    owner.objEditor.getUndoSystem()->begin();

    for (const PropPanel::TLeafHandle sourceLeaf : payload.items)
    {
      const auto *leafUserData = reinterpret_cast<ECSSceneOutlinerPanel::LeafUserData *>(owner.tree->getUserData(sourceLeaf));

      if (leafUserData->type == LeafType::ENTITY)
      {
        owner.objEditor.getUndoSystem()->put(
          new SetEntityParentUndoRedo(owner.getSceneObject(destUserData->sid), owner.getEntityObject(leafUserData->eid), owner));

        ecs::g_scenes->setEntityParent(destUserData->sid, {leafUserData->eid});
      }
      else
      {
        owner.objEditor.getUndoSystem()->put(
          new SetSceneParentUndoRedo(owner.getSceneObject(destUserData->sid), owner.getSceneObject(leafUserData->sid), owner));

        ecs::g_scenes->setNewParent(leafUserData->sid, destUserData->sid);
        owner.tree->setNewParent(sourceLeaf, leaf);
      }
    }

    owner.objEditor.getUndoSystem()->accept("Entity/Scene reparent");

    return PropPanel::DragAndDropResult::ACCEPTED;
  }

  return PropPanel::DragAndDropResult::NONE;
}

PropPanel::DragAndDropResult ECSSceneOutlinerPanel::DropHandler::onDropTargetBetween(PropPanel::TLeafHandle leaf, int idx)
{
  if (!owner.tree)
  {
    return PropPanel::DragAndDropResult::NONE;
  }

  // since we load only one import scene(without common, client and etc) compared to daEditorE,
  // reordering against root nodes is disabled
  if (!leaf)
  {
    return PropPanel::DragAndDropResult::NOT_ALLOWED;
  }

  const ImGuiPayload *dragAndDropPayload =
    PropPanel::acceptDragDropPayloadBeforeDelivery(ECS_SCENE_TREE_DRAG_DROP_PAYLOAD, canDropBetween());
  if (!dragAndDropPayload)
  {
    return PropPanel::DragAndDropResult::NONE;
  }

  const auto *destUserData = reinterpret_cast<LeafUserData *>(owner.tree->getUserData(leaf));
  G_ASSERT(destUserData->type != LeafType::ENTITY);

  const ecs::Scene::SceneRecord *destRecord = ecs::g_scenes->getActiveScene().getSceneRecordById(destUserData->sid);
  if (!destRecord || destRecord->loadType != ecs::Scene::LoadType::IMPORT)
  {
    return PropPanel::DragAndDropResult::NOT_ALLOWED;
  }

  if (RenderableEditableObject *rObj = owner.getEditableObjectFromHandle(leaf); !rObj || rObj->isLocked())
  {
    return PropPanel::DragAndDropResult::NOT_ALLOWED;
  }

  DragAndDropPayload payload;
  payload.items.resize(dragAndDropPayload->DataSize / sizeof(PropPanel::TLeafHandle));
  memcpy(payload.items.data(), dragAndDropPayload->Data, dragAndDropPayload->DataSize);

  if (!verifyPayloadBetween(payload, leaf))
  {
    return PropPanel::DragAndDropResult::NOT_ALLOWED;
  }

  if (dragAndDropPayload->IsDelivery())
  {
    owner.objEditor.getUndoSystem()->begin();

    for (const PropPanel::TLeafHandle sourceLeaf : payload.items)
    {
      const auto *sourceUserData = reinterpret_cast<LeafUserData *>(owner.tree->getUserData(sourceLeaf));
      if (sourceUserData->type == LeafType::ENTITY)
      {
        if (owner.tree->getParentLeaf(sourceLeaf) != leaf)
        {
          owner.objEditor.getUndoSystem()->put(
            new SetEntityParentUndoRedo(owner.getSceneObject(destUserData->sid), owner.getEntityObject(sourceUserData->eid), owner));
          ecs::g_scenes->setEntityParent(destUserData->sid, {sourceUserData->eid});
        }

        uint32_t prevOrder = ecs::g_scenes->getEntityOrder(sourceUserData->eid);
        owner.objEditor.getUndoSystem()->put(
          new SetEntityOrderUndoRedo(prevOrder, idx, owner.getEntityObject(sourceUserData->eid), owner));
        ecs::g_scenes->setEntityOrder(sourceUserData->eid, idx);
        if (prevOrder >= idx)
        {
          ++idx;
        }
      }
      else
      {
        if (owner.tree->getParentLeaf(sourceLeaf) != leaf)
        {
          owner.objEditor.getUndoSystem()->put(
            new SetSceneParentUndoRedo(owner.getSceneObject(destUserData->sid), owner.getSceneObject(sourceUserData->sid), owner));
          ecs::g_scenes->setNewParent(sourceUserData->sid, destUserData->sid);
          owner.tree->setNewParent(sourceLeaf, leaf);
        }

        uint32_t prevOrder = ecs::g_scenes->getSceneOrder(sourceUserData->sid);
        owner.objEditor.getUndoSystem()->put(
          new SetSceneOrderUndoRedo(prevOrder, idx, owner.getSceneObject(sourceUserData->sid), owner));
        ecs::g_scenes->setSceneOrder(sourceUserData->sid, idx);
        owner.tree->setChildIndex(sourceLeaf, idx);
        if (prevOrder >= idx)
        {
          ++idx;
        }
      }
    }

    owner.objEditor.getUndoSystem()->accept("Entity/Scene reorder");

    return PropPanel::DragAndDropResult::ACCEPTED;
  }

  return PropPanel::DragAndDropResult::NONE;
}

bool ECSSceneOutlinerPanel::DropHandler::canDropBetween()
{
  // disable reordering if filtering is enabled
  return ECSSceneOutlinerPanel::Filters{} == owner.filters;
}

bool ECSSceneOutlinerPanel::DropHandler::canSceneBeReparented(ecs::Scene::SceneId source_id, ecs::Scene::SceneId dest_id) const
{
  const ecs::Scene::SceneRecord *sourceRecord = ecs::g_scenes->getActiveScene().getSceneRecordById(source_id);
  const ecs::Scene::SceneRecord *destRecord = ecs::g_scenes->getActiveScene().getSceneRecordById(dest_id);
  if (!sourceRecord || !destRecord || source_id == dest_id || sourceRecord->loadType != ecs::Scene::LoadType::IMPORT ||
      destRecord->loadType != ecs::Scene::LoadType::IMPORT)
  {
    return false;
  }

  while (destRecord)
  {
    if (destRecord->parent == source_id)
    {
      return false;
    }

    destRecord = ecs::g_scenes->getActiveScene().getSceneRecordById(destRecord->parent);
  }

  return true;
}

bool ECSSceneOutlinerPanel::DropHandler::verifyPayloadDirect(const DragAndDropPayload &payload, PropPanel::TLeafHandle leaf)
{
  const auto *destUserData = reinterpret_cast<LeafUserData *>(owner.tree->getUserData(leaf));

  for (const PropPanel::TLeafHandle sourceLeaf : payload.items)
  {
    if (leaf == sourceLeaf)
    {
      return false;
    }

    if (RenderableEditableObject *rObj = owner.getEditableObjectFromHandle(sourceLeaf); !rObj || rObj->isLocked())
    {
      return false;
    }

    const auto *leafUserData = reinterpret_cast<ECSSceneOutlinerPanel::LeafUserData *>(owner.tree->getUserData(sourceLeaf));
    if (leafUserData->type == LeafType::ENTITY)
    {
      const ecs::Scene::SceneRecord *destRecord = ecs::g_scenes->getActiveScene().getSceneRecordById(destUserData->sid);
      if (destUserData->sid != ecs::Scene::C_INVALID_SCENE_ID && (!destRecord || destRecord->loadType != ecs::Scene::LoadType::IMPORT))
      {
        return false;
      }
    }
    else
    {
      if (!canSceneBeReparented(leafUserData->sid, destUserData->sid))
      {
        return false;
      }
    }
  }

  return true;
}

bool ECSSceneOutlinerPanel::DropHandler::verifyPayloadBetween(const DragAndDropPayload &payload, PropPanel::TLeafHandle leaf)
{
  const auto *destUserData = reinterpret_cast<LeafUserData *>(owner.tree->getUserData(leaf));

  for (const PropPanel::TLeafHandle sourceLeaf : payload.items)
  {
    if (RenderableEditableObject *rObj = owner.getEditableObjectFromHandle(sourceLeaf); !rObj || rObj->isLocked())
    {
      return false;
    }

    const auto *sourceUserData = reinterpret_cast<LeafUserData *>(owner.tree->getUserData(sourceLeaf));
    if (sourceUserData->type == LeafType::SCENE)
    {
      if (owner.tree->getParentLeaf(sourceLeaf) != leaf && !canSceneBeReparented(sourceUserData->sid, destUserData->sid))
      {
        return false;
      }
    }
  }

  return true;
}
