// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <de3_interface.h>
#include <EASTL/stack.h>
#include <ecsPropPanel/ecsEditorTemplates.h>
#include <EditorCore/ec_ObjectEditor.h>
#include <EditorCore/ec_rendEdObject.h>
#include <libTools/util/hdpiUtil.h>
#include <osApiWrappers/dag_localConv.h>
#include <propPanel/control/treeNode.h>
#include <propPanel/propPanel.h>
#include <propPanel/toastManager.h>
#include <winGuiWrapper/wgw_input.h>
#include "ecsSceneOutlinerDragDrop.h"
#include "ecsSceneOutlinerEventHandler.h"
#include "ecsSceneOutlinerPanel.h"
#include "ecsSceneOutlinerUndoRedo.h"
#include "../ecsToEditableObjectConverter.h"
#include "../ecsEntityObject.h"
#include "../ecsSceneObject.h"

namespace
{
template <typename TFunc>
void iterateTreeFiltered(PropPanel::ContainerPropertyControl &tree, PropPanel::TLeafHandle leaf, TFunc &&cb)
{
  for (int i = 0; i < tree.getChildCount(leaf); ++i)
  {
    const PropPanel::TLeafHandle childLeaf = tree.getChildLeaf(leaf, i);
    if (tree.isFilteredIn(childLeaf))
    {
      cb(childLeaf);
      iterateTreeFiltered(tree, childLeaf, cb);
    }
  }
}
} // namespace

ECSSceneOutlinerPanel::ECSSceneOutlinerPanel(ObjectEditor &obj_editor, const IECSToEditableObjectConverter &obj_converter, void *hwnd,
  const char *caption) noexcept :
  dragHandler(eastl::make_unique<DragHandler>(*this)),
  dropHandler(eastl::make_unique<DropHandler>(*this)),
  eventHandler(eastl::make_unique<EventHandler>(*this)),
  objEditor(obj_editor),
  objConverter(obj_converter)
{
  panelWindow = IEditorCoreEngine::get()->createPropPanel(this, caption);

  locateIconId = PropPanel::load_icon("zoom_and_center");
  selectionIconId = PropPanel::load_icon("layer_entity");
  eyeShownIconId = PropPanel::load_icon("eye_show");
  eyeHiddenIconId = PropPanel::load_icon("eye_hide");
  lockOpenIconId = PropPanel::load_icon("lock_open");
  lockClosedIconId = PropPanel::load_icon("lock_close");
  deleteIconId = PropPanel::load_icon("delete");
  searchIconId = PropPanel::load_icon("search");
  closeIconId = PropPanel::load_icon("close_editor");
  filterDefaultIconId = PropPanel::load_icon("filter_default");
  filterActiveIconId = PropPanel::load_icon("filter_active");
  addToSelectionIconId = PropPanel::load_icon("filter_selected");
  removeFromSelectionIconId = PropPanel::load_icon("filter_unselected");
}

ECSSceneOutlinerPanel::~ECSSceneOutlinerPanel() noexcept { IEditorCoreEngine::get()->deleteCustomPanel(panelWindow); }

void ECSSceneOutlinerPanel::resetPanel(const eastl::function<void(const eastl::function<void(ecs::EntityId)> &)> &cb)
{
  panelWindow->clear();
  userDataHolder.clear();
  edObjToHandle.clear();
  noSceneLeaf = nullptr;

  tree = panelWindow->createMultiSelectTree(PID_SCENE_TREE, "Scene:", hdpi::_pxScaled(0));
  tree->setTreeRenderEx(this);

  tree->setTreeDragHandler(dragHandler.get());
  tree->setTreeDropHandler(dropHandler.get());
  tree->setTreeEventHandler(eventHandler.get());
  tree->setTreeFilter(this);

  addNoSceneLeaf();

  cb([this](ecs::EntityId eid) { addEntityLeaf(noSceneLeaf, eid); });
}

void ECSSceneOutlinerPanel::fillPanel(const eastl::function<void(const eastl::function<void(ecs::EntityId)> &)> &cb)
{
  resetPanel(cb);

  if (!ecs::g_scenes)
  {
    return;
  }

  const ecs::Scene::ScenesList &scenes = ecs::g_scenes->getActiveScene().getAllScenes();
  if (scenes.empty())
  {
    return;
  }

  for (const ecs::Scene::SceneRecord &record : scenes)
  {
    if (record.importDepth == 0)
    {
      addSceneLeaf(nullptr, record);
    }
  }
}

void ECSSceneOutlinerPanel::updateImgui()
{
  if (requestToastOnAdd)
  {
    PropPanel::ToastMessage message;
    message.type = PropPanel::ToastType::Success;
    message.text = "Scene added successfully.\nIt now appears in the Scene Outliner.";
    message.fadeoutOnlyOnMouseLeave = true;
    message.fadeoutIntervalMs = 3000;
    message.setToMousePos(Point2(0.5f, 0.5f));
    PropPanel::set_toast_message(message);
    requestToastOnAdd = false;
  }

  if (panelWindow)
  {
    if (drawFilterControls())
    {
      tree->filterTree();
    }

    const char *expandAllTitle = "Expand all";
    const char *collapseAllTitle = "Collapse all";
    const ImVec2 expandAllSize = PropPanel::ImguiHelper::getButtonSize(expandAllTitle);
    const ImVec2 collapseAllSize = PropPanel::ImguiHelper::getButtonSize(collapseAllTitle);
    const ImVec2 expandButtonRegionAvailable = ImGui::GetContentRegionAvail();
    const ImVec2 expandButtonCursorPos = ImGui::GetCursorPos();
    ImGui::SetCursorPosX(expandButtonCursorPos.x + expandButtonRegionAvailable.x -
                         (expandAllSize.x + ImGui::GetStyle().ItemSpacing.x + collapseAllSize.x));
    if (ImGui::Button(expandAllTitle))
    {
      tree->setExpandedRecursively(nullptr, true);
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(expandButtonCursorPos.x + expandButtonRegionAvailable.x - collapseAllSize.x);
    if (ImGui::Button(collapseAllTitle))
    {
      tree->setExpandedRecursively(nullptr, false);
    }


    panelWindow->updateImgui();
  }

  for (PropPanel::TLeafHandle handle : deferredDeletions)
  {
    deleteLeaf(handle);
  }

  deferredDeletions.clear();
}

long ECSSceneOutlinerPanel::onKeyDown(int pcb_id, PropPanel::ContainerPropertyControl *panel, unsigned v_key)
{
  if (pcb_id == PID_SCENE_TREE && tree)
  {
    if (v_key == wingw::V_DELETE)
    {
      if (canSelectedLeafsBeDeleted())
      {
        deleteSelectedLeafs();
      }
      return 1;
    }
  }

  return 0;
}

void ECSSceneOutlinerPanel::syncSelection()
{
  if (!isUpdatingSelection)
  {
    tree->setSelLeaf(nullptr, false);

    for (int i = 0; i < objEditor.selectedCount(); ++i)
    {
      RenderableEditableObject *obj = objEditor.getSelected(i);
      if (!obj)
      {
        DAEDITOR3.conError("ECSSceneOutlinerPanel :: Failed to select an object, object editor returns nullptr");
        continue;
      }

      if (const auto it = edObjToHandle.find(obj); it != edObjToHandle.cend())
      {
        tree->setSelLeaf(it->second, true);
      }
      else
      {
        DAEDITOR3.conError("ECSSceneOutlinerPanel :: Failed to select %s", obj->getName());
      }
    }
  }
}

void ECSSceneOutlinerPanel::onEntityAdded(const ecs::EntityId eid)
{
  if (eid == ecs::INVALID_ENTITY_ID)
  {
    return;
  }

  if (const ecs::Scene::EntityRecord *erecord = ecs::g_scenes->getActiveScene().findEntityRecord(eid))
  {
    const auto it = edObjToHandle.find(objConverter.getObjectFromSceneId(erecord->sceneId));
    if (it != edObjToHandle.cend())
    {
      addEntityLeaf(it->second, eid);
    }
  }
  else
  {
    addEntityLeaf(noSceneLeaf, eid);
  }
}

void ECSSceneOutlinerPanel::onSceneAdded(const ecs::Scene::SceneId sid)
{
  if (const ecs::Scene::SceneRecord *srecord = ecs::g_scenes->getActiveScene().getSceneRecordById(sid))
  {
    const auto it = edObjToHandle.find(objConverter.getObjectFromSceneId(srecord->parent));
    addSceneLeaf(it != edObjToHandle.cend() ? it->second : nullptr, *srecord, false);
  }
}

void ECSSceneOutlinerPanel::onEntityRemoved(const ecs::EntityId eid)
{
  if (const auto it = edObjToHandle.find(objConverter.getObjectFromEntityId(eid)); it != edObjToHandle.cend())
  {
    deleteLeaf(it->second, true);
  }
}

void ECSSceneOutlinerPanel::onSceneRemoved(const ecs::Scene::SceneId sid)
{
  if (const auto it = edObjToHandle.find(objConverter.getObjectFromSceneId(sid)); it != edObjToHandle.cend())
  {
    deleteLeaf(it->second, true);
  }
}

void ECSSceneOutlinerPanel::onEntitySceneDataChanged(const ecs::EntityId eid)
{
  if (const auto it = edObjToHandle.find(objConverter.getObjectFromEntityId(eid)); it != edObjToHandle.cend())
  {
    if (const ecs::Scene::EntityRecord *erecord = ecs::g_scenes->getActiveScene().findEntityRecord(eid))
    {
      if (const auto sit = edObjToHandle.find(objConverter.getObjectFromSceneId(erecord->sceneId)); sit != edObjToHandle.cend())
      {
        tree->setNewParent(it->second, sit->second);
        reinsertLeafAtOrderedPosition(sit->second, it->second);
      }
    }
    else
    {
      tree->setNewParent(it->second, noSceneLeaf);
    }
  }
}

void ECSSceneOutlinerPanel::addNoSceneLeaf()
{
  const PropPanel::TLeafHandle leafHandle = tree->createTreeLeaf(nullptr, "No Scene", "");
  auto userDataPtr = eastl::make_unique<LeafUserData>(LeafUserData{.type = LeafType::SCENE, .sid = ecs::Scene::C_INVALID_SCENE_ID});
  tree->setUserData(leafHandle, userDataPtr.get());
  userDataHolder.emplace(leafHandle, std::move(userDataPtr));

  noSceneLeaf = leafHandle;
}

void ECSSceneOutlinerPanel::addSceneLeaf(PropPanel::TLeafHandle parent, const ecs::Scene::SceneRecord &record, bool add_children)
{
  const PropPanel::TLeafHandle leafHandle = tree->createTreeLeaf(parent, record.path.c_str(), "");

  auto userDataPtr = eastl::make_unique<LeafUserData>(LeafUserData{.type = LeafType::SCENE, .sid = record.id});
  tree->setUserData(leafHandle, userDataPtr.get());
  G_ASSERT(userDataHolder.find(leafHandle) == userDataHolder.cend());
  userDataHolder.emplace(leafHandle, std::move(userDataPtr));
  edObjToHandle.emplace(objConverter.getObjectFromSceneId(record.id), leafHandle);

  if (record.importDepth != 0)
  {
    reinsertLeafAtOrderedPosition(parent, leafHandle);
  }
  else if (noSceneLeaf != nullptr)
  {
    tree->setChildIndex(noSceneLeaf, tree->getChildCount(nullptr));
  }

  if (add_children)
  {
    for (const ecs::Scene::SceneRecord::OrderedEntry &sceneEntry : record.orderedEntries)
    {
      if (!sceneEntry.isEntity)
      {
        if (const ecs::Scene::SceneRecord *childRecord = ecs::g_scenes->getActiveScene().getSceneRecordById(sceneEntry.sid))
        {
          addSceneLeaf(leafHandle, *childRecord);
        }
      }
      else
      {
        addEntityLeaf(leafHandle, sceneEntry.eid);
      }
    }
  }

  if (record.id == importRequested)
  {
    objEditor.getUndoSystem()->begin();
    objEditor.getUndoSystem()->put(new AddImportUndoRedo(getSceneObject(record.id), objEditor));
    objEditor.getUndoSystem()->accept("Add import");

    requestToastOnAdd = true;
    importRequested = ecs::Scene::C_INVALID_SCENE_ID;
  }
}

void ECSSceneOutlinerPanel::addEntityLeaf(PropPanel::TLeafHandle parent, const ecs::EntityId eid)
{
  eastl::string templateName = g_entity_mgr->getEntityTemplateName(eid);
  removeEditorTemplateNames(templateName);
  const PropPanel::TLeafHandle leafHandle = tree->createTreeLeaf(parent, String(0, "%d - %s", eid, templateName), "");

  auto userDataPtr = eastl::make_unique<LeafUserData>(LeafUserData{.type = LeafType::ENTITY, .eid = eid});
  tree->setUserData(leafHandle, userDataPtr.get());
  G_ASSERT(userDataHolder.find(leafHandle) == userDataHolder.cend());
  userDataHolder.emplace(leafHandle, std::move(userDataPtr));
  edObjToHandle.emplace(objConverter.getObjectFromEntityId(eid), leafHandle);

  if (ecs::g_scenes->getActiveScene().findEntityRecord(eid) && parent != noSceneLeaf)
  {
    reinsertLeafAtOrderedPosition(parent, leafHandle);
  }
}

void ECSSceneOutlinerPanel::reinsertLeafAtOrderedPosition(PropPanel::TLeafHandle parent, PropPanel::TLeafHandle leaf)
{
  const uint32_t leafOrder = getLeafOrder(leaf);
  for (int i = 0; i < tree->getChildCount(parent); ++i)
  {
    const PropPanel::TLeafHandle childLeaf = tree->getChildLeaf(parent, i);
    if (leaf != childLeaf && leafOrder < getLeafOrder(tree->getChildLeaf(parent, i)))
    {
      tree->setChildIndex(leaf, i);
      return;
    }
  }
}

uint32_t ECSSceneOutlinerPanel::getLeafOrder(PropPanel::TLeafHandle leaf) const
{
  const auto *userData = reinterpret_cast<LeafUserData *>(tree->getUserData(leaf));
  if (userData->type == LeafType::SCENE)
  {
    return ecs::g_scenes->getSceneOrder(userData->sid);
  }
  else if (userData->type == LeafType::ENTITY)
  {
    return ecs::g_scenes->getEntityOrder(userData->eid);
  }

  return tree->getChildCount(tree->getParentLeaf(leaf));
}

void ECSSceneOutlinerPanel::deleteSelectedLeafs()
{
  objEditor.getUndoSystem()->begin();
  dag::Vector<PropPanel::TLeafHandle> selected;
  tree->getSelectedLeafs(selected, true, true);

  for (const PropPanel::TLeafHandle handle : selected)
  {
    deleteLeaf(handle);
  }
  objEditor.getUndoSystem()->accept("Delete object");
}

void ECSSceneOutlinerPanel::deleteLeaf(PropPanel::TLeafHandle handle, bool editor_event)
{
  if (!editor_event)
  {
    objEditor.getUndoSystem()->begin();
  }

  const auto *userData = reinterpret_cast<LeafUserData *>(tree->getUserData(handle));
  if (userData->type == LeafType::SCENE)
  {
    if (const ecs::Scene::SceneRecord *record = ecs::g_scenes->getActiveScene().getSceneRecordById(userData->sid);
        (record && record->importDepth != 0 && record->loadType == ecs::Scene::LoadType::IMPORT) || editor_event)
    {
      {
        eastl::stack<PropPanel::TLeafHandle> children{handle};
        while (!children.empty())
        {
          const PropPanel::TLeafHandle currentChild = children.top();
          children.pop();

          if (RenderableEditableObject *obj = getEditableObjectFromHandle(currentChild))
          {
            const auto *childUserData = reinterpret_cast<LeafUserData *>(tree->getUserData(currentChild));
            edObjToHandle.erase(obj);
            // remove only root scene or entity
            // children scenes will be handled by root ECSSceneObject
            if (!editor_event && !(childUserData->type == LeafType::SCENE && currentChild != handle))
            {
              objEditor.removeObject(obj);
            }
          }

          userDataHolder.erase(currentChild);
          for (int i = 0; i < tree->getChildCount(currentChild); ++i)
          {
            const PropPanel::TLeafHandle grandChild = tree->getChildLeaf(currentChild, i);
            children.push(grandChild);
          }
        }
      }

      tree->removeLeaf(handle);
    }
  }
  else if (userData->type == LeafType::ENTITY)
  {
    tree->removeLeaf(handle);
    userDataHolder.erase(handle);
    if (RenderableEditableObject *obj = getEditableObjectFromHandle(handle))
    {
      edObjToHandle.erase(obj);
      if (!editor_event)
      {
        objEditor.removeObject(obj);
      }
    }
  }

  if (!editor_event)
  {
    objEditor.getUndoSystem()->accept("Delete object");
  }
}

bool ECSSceneOutlinerPanel::treeNodeStart(const NodeStartData &data)
{
  const auto flags = data.flags | ImGuiTreeNodeFlags_AllowOverlap;
  return PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorStart(data.id, flags, data.label, data.labelEnd, data.endData, true);
}

void ECSSceneOutlinerPanel::treeNodeRender(PropPanel::ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data, bool show_arrow)
{
  PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorRender(end_data, false);
}

void ECSSceneOutlinerPanel::treeNodeEnd(PropPanel::ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data)
{
  PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorEnd(end_data);
}

void ECSSceneOutlinerPanel::treeItemRenderEx(const ItemRenderData &data)
{
  const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
  const float actionButtonsTotalWidth = (fontSizedIconSize.x + ImGui::GetStyle().ItemSpacing.x) * 5;
  const float maxX = PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorGetLabelClipMaxX();
  const float actionButtonsStartX = maxX - actionButtonsTotalWidth;

  RenderableEditableObject *obj = getEditableObjectFromHandle(data.handle);
  if (!obj)
  {
    return;
  }

  const ImVec2 prevCursorPos = ImGui::GetCursorScreenPos();
  ImGui::SetCursorScreenPos(ImVec2(actionButtonsStartX, data.textPos.y));
  ImGui::PushID(data.handle);

  if (PropPanel::ImguiHelper::imageButtonFramelessOrPlaceholder("locate_object", locateIconId, fontSizedIconSize, "Locate object",
        data.isHovered))
  {
    tree->setSelLeaf(nullptr, false);
    objEditor.getUndoSystem()->begin();
    objEditor.unselectAll();
    obj->selectObject();
    objEditor.zoomAndCenter();
    objEditor.getUndoSystem()->accept("Select objects");
  }

  ImGui::SameLine();
  if (PropPanel::ImguiHelper::imageButtonFramelessOrPlaceholder("toggle_selection", selectionIconId, fontSizedIconSize,
        "Toggle selection", data.isHovered))
  {
    const bool wasSelected = obj->isSelected();
    objEditor.getUndoSystem()->begin();

    obj->selectObject(!wasSelected);

    const auto *userData = reinterpret_cast<LeafUserData *>(tree->getUserData(data.handle));
    if (userData->type == LeafType::SCENE)
    {
      ecs::g_scenes->iterateHierarchy(userData->sid, [&](const ecs::Scene::SceneRecord &record) {
        for (const ecs::Scene::SceneRecord::OrderedEntry &entry : record.orderedEntries)
        {
          RenderableEditableObject *entryObj =
            entry.isEntity ? objConverter.getObjectFromEntityId(entry.eid) : objConverter.getObjectFromSceneId(entry.sid);
          if (entryObj)
          {
            entryObj->selectObject(!wasSelected);
          }
        }
      });
    }

    objEditor.updateSelection();
    objEditor.getUndoSystem()->accept("Toggle selection");
  }

  ImGui::SameLine();
  if (const bool isHidden = obj->isHidden();
      PropPanel::ImguiHelper::imageButtonFramelessOrPlaceholder("toggle_visibility", isHidden ? eyeHiddenIconId : eyeShownIconId,
        fontSizedIconSize, isHidden ? "Show object" : "Hide object", data.isHovered || isHidden))
  {
    objEditor.getUndoSystem()->begin();
    obj->hideObject(!isHidden);
    objEditor.getUndoSystem()->accept("Hide/show object");
  }

  ImGui::SameLine();
  if (const bool isLocked = obj->isLocked();
      PropPanel::ImguiHelper::imageButtonFramelessOrPlaceholder("lock_object", isLocked ? lockClosedIconId : lockOpenIconId,
        fontSizedIconSize, isLocked ? "Unlock object" : "Lock object", data.isHovered || isLocked))
  {
    objEditor.getUndoSystem()->begin();
    obj->lockObject(!isLocked);
    objEditor.getUndoSystem()->accept("Lock/unlock object");
  }

  ImGui::SameLine();
  if (PropPanel::ImguiHelper::imageButtonFramelessOrPlaceholder("delete_item", deleteIconId, fontSizedIconSize, "Delete object",
        data.isHovered && obj->mayDelete()))
  {
    deferredDeletions.emplace(data.handle);
  }

  ImGui::PopID();
  ImGui::SetCursorScreenPos(prevCursorPos);
}

RenderableEditableObject *ECSSceneOutlinerPanel::getEditableObjectFromHandle(PropPanel::TLeafHandle handle) const
{
  const auto *userData = reinterpret_cast<ECSSceneOutlinerPanel::LeafUserData *>(tree->getUserData(handle));
  if (userData->type == ECSSceneOutlinerPanel::LeafType::ENTITY)
  {
    return objConverter.getObjectFromEntityId(userData->eid);
  }
  else if (userData->type == ECSSceneOutlinerPanel::LeafType::SCENE)
  {
    return objConverter.getObjectFromSceneId(userData->sid);
  }

  return nullptr;
}

bool ECSSceneOutlinerPanel::canSelectedLeafsBeDeleted()
{
  dag::Vector<PropPanel::TLeafHandle> selected;
  tree->getSelectedLeafs(selected, true, true);
  for (const PropPanel::TLeafHandle handle : selected)
  {
    if (RenderableEditableObject *rObj = getEditableObjectFromHandle(handle); !rObj || !rObj->mayDelete())
    {
      return false;
    }
  }

  return true;
}

void ECSSceneOutlinerPanel::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == PID_SCENE_TREE)
  {
    isUpdatingSelection = true;

    dag::Vector<PropPanel::TLeafHandle> selected;
    tree->getSelectedLeafs(selected, true, true);
    objEditor.unselectAll();
    for (const PropPanel::TLeafHandle leaf : selected)
    {
      if (RenderableEditableObject *obj = getEditableObjectFromHandle(leaf))
      {
        obj->selectObject();
      }
    }
    objEditor.updateSelection();

    isUpdatingSelection = false;
  }
}

bool ECSSceneOutlinerPanel::drawFilterControls()
{
  bool anyFilterChanged = false;

  const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
  const ImVec2 settingsButtonSize = PropPanel::ImguiHelper::getImageButtonWithDownArrowSize(fontSizedIconSize);
  const ImVec2 buttonSize = PropPanel::ImguiHelper::getImageButtonSize(fontSizedIconSize);

  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (ImGui::GetStyle().ItemSpacing.x + settingsButtonSize.x) -
                          2 * (ImGui::GetStyle().ItemSpacing.x + buttonSize.x));
  if (PropPanel::ImguiHelper::searchInput(&searchInputFocusId, "##searchInput", "Search", filters.filterString, searchIconId,
        closeIconId))
  {
    anyFilterChanged = true;
  }

  const char *popupId = "filterPopup";
  const PropPanel::IconId filterButtonIcon = filterSettingsOpen ? filterActiveIconId : filterDefaultIconId;
  ImGui::SameLine();
  if (PropPanel::ImguiHelper::imageButtonWithArrow("filterButton", filterButtonIcon, fontSizedIconSize, filterSettingsOpen))
  {
    ImGui::OpenPopup(popupId);
    filterSettingsOpen = true;
  }

  {
    const bool disabled = Filters{} == filters;
    ImGui::BeginDisabled(disabled);

    ImGui::SameLine();
    if (ImGui::ImageButton("add_to_selection", PropPanel::get_im_texture_id_from_icon_id(addToSelectionIconId), fontSizedIconSize))
    {
      objEditor.getUndoSystem()->begin();

      isUpdatingSelection = true;
      iterateTreeFiltered(*tree, nullptr, [this](PropPanel::TLeafHandle leaf) {
        if (RenderableEditableObject *obj = getEditableObjectFromHandle(leaf))
        {
          obj->selectObject();
        }
        tree->setSelLeaf(leaf, true);
      });

      objEditor.updateSelection();
      isUpdatingSelection = false;
      objEditor.getUndoSystem()->accept("Toggle selection");
    }

    PropPanel::set_previous_imgui_control_tooltip(ImGui::GetItemID(), "Append filtered items to selection");

    ImGui::SameLine();
    if (ImGui::ImageButton("remove_from_selection", PropPanel::get_im_texture_id_from_icon_id(removeFromSelectionIconId),
          fontSizedIconSize))
    {
      objEditor.getUndoSystem()->begin();

      dag::Vector<PropPanel::TLeafHandle> currentSelection;
      tree->getSelectedLeafs(currentSelection, true, true);
      eastl::unordered_set<PropPanel::TLeafHandle> selectionSet{currentSelection.cbegin(), currentSelection.cend()};
      iterateTreeFiltered(*tree, nullptr, [&selectionSet](PropPanel::TLeafHandle leaf) { selectionSet.erase(leaf); });

      isUpdatingSelection = true;
      objEditor.unselectAll();

      tree->setSelLeaf(nullptr);

      for (const PropPanel::TLeafHandle leaf : selectionSet)
      {
        if (RenderableEditableObject *obj = getEditableObjectFromHandle(leaf))
        {
          obj->selectObject();
        }
        tree->setSelLeaf(leaf, true);
      }

      objEditor.updateSelection();
      isUpdatingSelection = false;
      objEditor.getUndoSystem()->accept("Toggle selection");
    }

    PropPanel::set_previous_imgui_control_tooltip(ImGui::GetItemID(), "Remove filtered items from selection");

    ImGui::EndDisabled();
  }

  if (filterSettingsOpen)
  {
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
    const bool popupIsOpen = ImGui::BeginPopup(popupId);
    ImGui::PopStyleColor();

    if (!popupIsOpen)
    {
      filterSettingsOpen = false;
      return anyFilterChanged;
    }

    ImGui::TextUnformatted("Show:");

    if (PropPanel::ImguiHelper::checkboxWithDragSelection("Entities", &filters.showEntities))
    {
      anyFilterChanged = true;
    }

    ImGui::EndPopup();
  }

  return anyFilterChanged;
}

bool ECSSceneOutlinerPanel::filterNode(const PropPanel::TreeNode &node)
{
  if (!filters.filterString.empty() && !dd_stristr(node.getTitle(), filters.filterString))
  {
    return false;
  }

  if (!filters.showEntities)
  {
    const auto *userData = reinterpret_cast<LeafUserData *>(node.getUserData());
    if (userData->type == LeafType::ENTITY)
    {
      return false;
    }
  }

  return true;
}

ECSSceneObject *ECSSceneOutlinerPanel::getSceneObject(ecs::Scene::SceneId sid) const
{
  return static_cast<ECSSceneObject *>(objConverter.getObjectFromSceneId(sid));
}

ECSEntityObject *ECSSceneOutlinerPanel::getEntityObject(ecs::EntityId eid) const
{
  return static_cast<ECSEntityObject *>(objConverter.getObjectFromEntityId(eid));
}
