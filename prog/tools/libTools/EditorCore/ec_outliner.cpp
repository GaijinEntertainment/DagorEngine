// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS
#include <EditorCore/ec_outliner.h>
#include "ec_outlinerModel.h"
#include <gui/dag_imguiUtil.h>
#include <ioSys/dag_dataBlock.h>
#include <propPanel/constants.h>
#include <propPanel/focusHelper.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/propPanel.h>
#include <propPanel/control/menu.h>
#include <imgui/imgui.h>

namespace Outliner
{

static const ImU32 OUTLINER_INPLACE_EDIT_NORMAL_BACKGROUND = IM_COL32(71, 114, 179, 255);
static const ImU32 OUTLINER_INPLACE_EDIT_ERROR_BACKGROUND = IM_COL32(240, 91, 91, 255);
static const char *OUTLINER_DRAG_AND_DROP_TYPE = "OutlinerObject";

class TreeItemInlineRenamerControl
{
public:
  explicit TreeItemInlineRenamerControl(const char *initial_name, ImTextureID alert_icon) :
    newName(initial_name), alertIcon(alert_icon)
  {
    PropPanel::focus_helper.requestFocus(&inputFocusId);
  }

  virtual ~TreeItemInlineRenamerControl() {}

  // Returns with true while renaming is in progress.
  bool updateImgui()
  {
    // Draw the controls into a separate draw channel. This way we are able to measure their size and can easily draw a
    // background.
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    G_ASSERT(drawList->_Splitter._Current == 0);
    drawList->ChannelsSplit(2);
    drawList->ChannelsSetCurrent(1);

    // Position the input field that way that the text inside it starts at the same X where the tree label was.
    const ImVec2 backgroundPadding(1.0f, 1.0f);
    const float textDisplayStartInInput = ImGui::GetStyle().FramePadding.x;
    const ImVec2 cursorStartPos =
      ImGui::GetCursorScreenPos() + ImVec2(ImGui::GetTreeNodeToLabelSpacing() - backgroundPadding.x - textDisplayStartInInput, 0.0f);
    ImGui::SetCursorScreenPos(cursorStartPos + backgroundPadding);

    ImGui::BeginGroup();

    const float inputWidth = floorf(ImGui::GetContentRegionAvail().x + ImGui::GetScrollX() - backgroundPadding.x);
    if (inputWidth > 0.0f)
      ImGui::SetNextItemWidth(inputWidth);

    PropPanel::focus_helper.setFocusToNextImGuiControlIfRequested(&inputFocusId);

    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
    ImGuiDagor::InputText("##renameInput", &newName);
    ImGui::PopStyleColor();

    const bool renameInputCanceled =
      ImGui::IsItemFocused() && ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_None, ImGui::GetItemID());
    const bool renameInputAccepted = ImGui::IsItemDeactivated() && !renameInputCanceled;

    ImGui::SameLine(0.0f, 0.0f);
    const float cursorInputEndScreenPosX = ImGui::GetCursorScreenPos().x;
    const float cursorInputEndPosX = ImGui::GetCursorPos().x;
    ImGui::NewLine();

    const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();

    // Draw the alert icon and the error message.
    errorMessage.clear();
    const bool canRename = canRenameTreeItem(newName, errorMessage);
    if (!canRename)
    {
      ImVec2 alertIconPos = ImGui::GetCursorScreenPos();
      const float alertIconLeftPadding = ImGui::GetStyle().ItemInnerSpacing.x;

      ImGui::Dummy(fontSizedIconSize + ImVec2(alertIconLeftPadding, 0.0f)); // Just reserve the horizontal space for the alert icon.
      ImGui::SameLine();

      ImGui::PushTextWrapPos(cursorInputEndPosX);
      ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
      ImGui::TextUnformatted(errorMessage.c_str());
      ImGui::PopStyleColor();
      ImGui::PopTextWrapPos();

      const ImVec2 textSize = ImGui::GetItemRectSize();
      alertIconPos.x += alertIconLeftPadding;
      alertIconPos.y += (textSize.y - fontSizedIconSize.y) * 0.5f; // Center the icon vertically.
      drawList->AddImage(alertIcon, alertIconPos, alertIconPos + fontSizedIconSize);
    }

    ImGui::EndGroup();

    // Draw the background.
    const ImVec2 cursorEndPos(cursorInputEndScreenPosX,
      ImGui::GetCursorScreenPos().y - (canRename ? ImGui::GetStyle().ItemSpacing.y : 0.0f));
    const ImVec2 rectTopLeft = cursorStartPos;
    const ImVec2 rectBottomRight(cursorEndPos + backgroundPadding);
    drawList->ChannelsSetCurrent(0);
    drawList->AddRectFilled(rectTopLeft, rectBottomRight,
      canRename ? OUTLINER_INPLACE_EDIT_NORMAL_BACKGROUND : OUTLINER_INPLACE_EDIT_ERROR_BACKGROUND);

    drawList->ChannelsMerge();

    // Add some vertical space after the error.
    if (!canRename)
      ImGui::Dummy(ImVec2(0.0f, 0.0f));

    if (renameInputAccepted && canRename)
    {
      renameTreeItem(newName);
      return false;
    }
    else if (renameInputCanceled)
    {
      return false;
    }

    return true;
  }

  virtual bool canRenameTreeItem(const String &new_name, String &error_message) = 0;
  virtual void renameTreeItem(const String &new_name) = 0;

private:
  const bool inputFocusId = false; // Only the address of this member is used.
  const ImTextureID alertIcon;
  String newName;
  String errorMessage;
};

class LayerTreeItemInlinerRenamerControl : public TreeItemInlineRenamerControl
{
public:
  LayerTreeItemInlinerRenamerControl(const char *initial_name, ImTextureID alert_icon, IOutliner &outliner_interface,
    OutlinerModel &outliner_model, int in_type, int per_type_layer_index) :
    TreeItemInlineRenamerControl(initial_name, alert_icon),
    outlinerInterface(outliner_interface),
    outlinerModel(outliner_model),
    type(in_type),
    perTypeLayerIndex(per_type_layer_index)
  {}

private:
  virtual bool canRenameTreeItem(const String &new_name, String &error_message) override
  {
    return outlinerInterface.canRenameLayerTo(type, perTypeLayerIndex, new_name, error_message);
  }

  virtual void renameTreeItem(const String &new_name) override
  {
    outlinerInterface.renameLayer(type, perTypeLayerIndex, new_name);
    outlinerModel.onLayerNameChanged(outlinerInterface);
  }

  IOutliner &outlinerInterface;
  OutlinerModel &outlinerModel;
  const int type;
  const int perTypeLayerIndex;
};

class ObjectTreeItemInlinerRenamerControl : public TreeItemInlineRenamerControl
{
public:
  ObjectTreeItemInlinerRenamerControl(RenderableEditableObject &in_object, const char *initial_name, ImTextureID alert_icon,
    IOutliner &outliner_interface) :
    TreeItemInlineRenamerControl(initial_name, alert_icon), object(in_object), outlinerInterface(outliner_interface)
  {}

  const RenderableEditableObject &getObject() const { return object; }

private:
  virtual bool canRenameTreeItem(const String &new_name, String &error_message) override
  {
    return outlinerInterface.canRenameObject(object, new_name, error_message);
  }

  virtual void renameTreeItem(const String &new_name) override { outlinerInterface.renameObject(object, new_name); }

  IOutliner &outlinerInterface;
  RenderableEditableObject &object;
};

ImTextureID OutlinerWindow::Icons::getForType(int type) const
{
  if (type == 0)
    return typeEntity;
  else if (type == 1)
    return typeSpline;
  else if (type == 2)
    return typePolygon;

  G_ASSERT(false);
  return typeEntity;
}

ImTextureID OutlinerWindow::Icons::getSelectionIcon(OutlinerSelectionState selection_state) const
{
  if (selection_state == OutlinerSelectionState::All)
    return selectAll;
  else if (selection_state == OutlinerSelectionState::Partial)
    return selectPartial;

  G_ASSERT(selection_state == OutlinerSelectionState::None);
  return selectNone;
}

ImTextureID OutlinerWindow::Icons::getVisibilityIcon(bool visible) const { return visible ? visibilityVisible : visibilityHidden; }

ImTextureID OutlinerWindow::Icons::getLockIcon(bool locked) const { return locked ? lockClosed : lockOpen; }

OutlinerWindow::OutlinerWindow()
{
  icons.settings = (ImTextureID)((unsigned)PropPanel::load_icon("filter_default"));
  icons.settingsOpen = (ImTextureID)((unsigned)PropPanel::load_icon("filter_active"));
  icons.search = (ImTextureID)((unsigned)PropPanel::load_icon("search"));
  icons.typeEntity = (ImTextureID)((unsigned)PropPanel::load_icon("layer_entity"));
  icons.typeSpline = (ImTextureID)((unsigned)PropPanel::load_icon("layer_spline"));
  icons.typePolygon = (ImTextureID)((unsigned)PropPanel::load_icon("layer_polygon"));
  icons.layerDefault = (ImTextureID)((unsigned)PropPanel::load_icon("layer_default"));
  icons.layerActive = (ImTextureID)((unsigned)PropPanel::load_icon("layer_active"));
  icons.layerLocked = (ImTextureID)((unsigned)PropPanel::load_icon("layer_locked"));
  icons.layerAdd = (ImTextureID)((unsigned)PropPanel::load_icon("layer_add"));
  icons.layerCreate = (ImTextureID)((unsigned)PropPanel::load_icon("layer_create"));
  icons.selectAll = (ImTextureID)((unsigned)PropPanel::load_icon("select_all"));
  icons.selectPartial = (ImTextureID)((unsigned)PropPanel::load_icon("select_partly"));
  icons.selectNone = (ImTextureID)((unsigned)PropPanel::load_icon("select_none"));
  icons.visibilityVisible = (ImTextureID)((unsigned)PropPanel::load_icon("eye_show"));
  icons.visibilityHidden = (ImTextureID)((unsigned)PropPanel::load_icon("eye_hide"));
  icons.lockOpen = (ImTextureID)((unsigned)PropPanel::load_icon("lock_open"));
  icons.lockClosed = (ImTextureID)((unsigned)PropPanel::load_icon("lock_close"));
  icons.applyToMask = (ImTextureID)((unsigned)PropPanel::load_icon("asset_tifMask"));
  icons.applyToMaskDisabled = (ImTextureID)((unsigned)PropPanel::load_icon("mask_disabled"));
  icons.exportLayer = (ImTextureID)((unsigned)PropPanel::load_icon("export"));
  icons.exportLayerDisabled = (ImTextureID)((unsigned)PropPanel::load_icon("export_disabled"));
  icons.alert = (ImTextureID)((unsigned)PropPanel::load_icon("alert"));
  icons.close = (ImTextureID)((unsigned)PropPanel::load_icon("close_editor"));
}

OutlinerWindow::~OutlinerWindow() { PropPanel::remove_delayed_callback(*this); }

int OutlinerWindow::onMenuItemClick(unsigned id)
{
  if (id == (unsigned)MenuItemId::AddNewLayer)
  {
    if (!treeInterface)
      return 0;

    const int selectedType = outlinerModel->getExclusiveSelectedType(*treeInterface, showTypes);
    if (selectedType >= 0)
    {
      addingLayerToType = selectedType;
      PropPanel::focus_helper.requestFocus(&addingLayerToType);
    }
  }
  else if (id == (unsigned)MenuItemId::SelectAllTypeObjects || id == (unsigned)MenuItemId::DeselectAllTypeObjects)
  {
    const bool select = id == (unsigned)MenuItemId::SelectAllTypeObjects;
    const int typeCount = treeInterface->getTypeCount();
    for (int typeIndex = 0; typeIndex < typeCount; ++typeIndex)
      if (outlinerModel->objectTypes[typeIndex]->isSelected())
        treeInterface->selectAllTypeObjects(typeIndex, select);
  }
  else if (id == (unsigned)MenuItemId::SelectAllLayerObjects || id == (unsigned)MenuItemId::DeselectAllLayerObjects)
  {
    const bool select = id == (unsigned)MenuItemId::SelectAllLayerObjects;
    const int typeCount = treeInterface->getTypeCount();
    for (int typeIndex = 0; typeIndex < typeCount; ++typeIndex)
    {
      const int layerCount = treeInterface->getLayerCount(typeIndex);
      for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
        if (outlinerModel->objectTypes[typeIndex]->layers[layerIndex]->isSelected())
          treeInterface->selectAllLayerObjects(typeIndex, layerIndex, select);
    }
  }
  else if (id == (unsigned)MenuItemId::ExpandLayerChildren || id == (unsigned)MenuItemId::CollapseLayerChildren)
  {
    const bool expand = id == (unsigned)MenuItemId::ExpandLayerChildren;
    for (ObjectTypeTreeItem *objectTypeTreeItem : outlinerModel->objectTypes)
      for (LayerTreeItem *layerTreeItem : objectTypeTreeItem->layers)
        if (layerTreeItem->isSelected())
        {
          const int objectCount = layerTreeItem->sortedObjects.size();
          for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
            layerTreeItem->sortedObjects[objectIndex]->setExpandedRecursive(expand);
        }
  }
  else if (id == (unsigned)MenuItemId::RenameLayer)
  {
    // Just use a basic check here, so the rename panel can show the error message.
    int type;
    int perTypeLayerIndex;
    if (outlinerModel->getExclusiveSelectedLayer(type, perTypeLayerIndex))
    {
      const String layerName(treeInterface->getLayerName(type, perTypeLayerIndex));
      layerRenamer.reset(
        new LayerTreeItemInlinerRenamerControl(layerName, icons.alert, *treeInterface, *outlinerModel, type, perTypeLayerIndex));
    }
  }
  else if (id >= (unsigned)MenuItemId::MoveObjectToLayerStart && id < (unsigned)MenuItemId::MoveObjectToLayerEnd)
  {
    int selectedType;
    int layersAffectedCount;
    const bool isExclusiveType = outlinerModel->getExclusiveTypeAndLayerInfoForSelectedObjects(selectedType, layersAffectedCount);
    if (isExclusiveType)
    {
      const int destinationPerTypeLayerIndex = id - ((unsigned)MenuItemId::MoveObjectToLayerStart);
      dag::Vector<RenderableEditableObject *> selectedObjects = outlinerModel->getSelectedObjects();
      treeInterface->moveObjectsToLayer(make_span(selectedObjects), selectedType, destinationPerTypeLayerIndex);
    }

    return 1;
  }
  else if (id == (unsigned)MenuItemId::RenameObject)
  {
    // Just use a basic check here (not canRenameObject), so the rename panel can show the error message.
    dag::Vector<RenderableEditableObject *> selectedObjects = outlinerModel->getSelectedObjects();
    if (selectedObjects.size() == 1)
    {
      RenderableEditableObject *object = selectedObjects[0];
      const String objectName(object->getName());
      objectRenamer.reset(new ObjectTreeItemInlinerRenamerControl(*object, objectName, icons.alert, *treeInterface));
    }
  }
  else if (id == (unsigned)MenuItemId::ChangeObjectAsset)
  {
    if (outlinerModel->getExclusiveObjectsSelectionCountIgnoringAssetNames() > 0)
    {
      dag::Vector<RenderableEditableObject *> selectedObjects = outlinerModel->getSelectedObjects();
      treeInterface->changeObjectAsset(make_span(selectedObjects));
    }
  }
  else if (id == (unsigned)MenuItemId::DeleteObject)
  {
    if (outlinerModel->getExclusiveObjectsSelectionCountIgnoringAssetNames() > 0)
    {
      dag::Vector<RenderableEditableObject *> selectedObjects = outlinerModel->getSelectedObjects();
      treeInterface->deleteObjects(make_span(selectedObjects));
    }
  }

  return 0;
}

void OutlinerWindow::onImguiDelayedCallback(void *user_data)
{
  if (changeAssetRequested)
  {
    treeInterface->changeObjectAsset(make_span(&changeAssetRequested, 1));
    changeAssetRequested = nullptr;
  }
}

ImTextureID OutlinerWindow::getObjectAssetTypeIcon(RenderableEditableObject &object)
{
  const char *assetTypeName;
  const int assetType = treeInterface->getObjectAssetType(object, assetTypeName);
  if (assetType < 0)
    return 0;

  if (assetType >= (int)assetTypeIcons.size())
    assetTypeIcons.resize(assetType + 1, eastl::optional<ImTextureID>());

  eastl::optional<ImTextureID> &assetTypeIcon = assetTypeIcons[assetType];
  if (!assetTypeIcon.has_value())
  {
    const String iconName(0, "asset_%s", assetTypeName);
    assetTypeIcons[assetType] = (ImTextureID)((unsigned)PropPanel::load_icon(iconName));
  }

  return assetTypeIcon.value();
}

void OutlinerWindow::handleDragAndDropDropping(int type, int per_type_layer_index)
{
  if (!ImGui::BeginDragDropTarget())
    return;

  const ImGuiPayload *dragAndDropPayload =
    ImGui::AcceptDragDropPayload(OUTLINER_DRAG_AND_DROP_TYPE, ImGuiDragDropFlags_AcceptBeforeDelivery);
  if (!dragAndDropPayload)
    return;

  DragData dragData;
  G_ASSERT(dragAndDropPayload->DataSize == sizeof(dragData));
  memcpy(&dragData, dragAndDropPayload->Data, dragAndDropPayload->DataSize);

  // Only allow dropping into the same object type.
  if (type == dragData.objectType && per_type_layer_index >= 0)
  {
    // Dropping into the same layer is a no operation (except when moving objects from multiple layers into one)
    // In this case there's no need to show the not allowed cursor.
    if (dragAndDropPayload->IsDelivery())
    {
      int selectedType;
      int layersAffectedCount;
      const bool isExclusiveType = outlinerModel->getExclusiveTypeAndLayerInfoForSelectedObjects(selectedType, layersAffectedCount);
      if (isExclusiveType && selectedType == dragData.objectType &&
          (per_type_layer_index != dragData.perTypeLayerIndex || layersAffectedCount > 1))
      {
        dag::Vector<RenderableEditableObject *> selectedObjects = outlinerModel->getSelectedObjects();
        treeInterface->moveObjectsToLayer(make_span(selectedObjects), dragData.objectType, per_type_layer_index);
      }
    }
  }
  else
  {
    ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);
  }

  ImGui::EndDragDropTarget();
}

bool OutlinerWindow::showTypeControls(ObjectTypeTreeItem &tree_item, int type, bool type_visible, bool type_locked,
  bool dim_type_color, int layer_count, const ImVec4 &dimmed_text_color, float action_buttons_total_width)
{
  const bool wasExpanded = tree_item.isExpanded();
  ImGui::SetNextItemOpen(wasExpanded);

  G_STATIC_ASSERT(sizeof(ImGuiSelectionUserData) == sizeof(&tree_item));
  ImGui::SetNextItemSelectionUserData(reinterpret_cast<ImGuiSelectionUserData>(&tree_item));

  ImGuiTreeNodeFlags typeTreeNodeFlags = ImGuiTreeNodeFlags_NavLeftJumpsBackHere | ImGuiTreeNodeFlags_SpanAvailWidth |
                                         ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_OpenOnDoubleClick;
  if (layer_count == 0)
    typeTreeNodeFlags |= ImGuiTreeNodeFlags_Leaf;
  if (tree_item.isSelected())
    typeTreeNodeFlags |= ImGuiTreeNodeFlags_Selected;

  PropPanel::ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData endData;
  const bool isExpanded =
    PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorStart(tree_item.getLabel(), typeTreeNodeFlags, endData);

  updateSelectionHead(tree_item);

  if (endData.draw)
  {
    const ImVec2 leftIconPos = endData.textPos;
    const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::ImguiHelper::getFontSizedIconSize();
    const float iconWidthWithSpacing = fontSizedIconSize.x + ImGui::GetStyle().ItemSpacing.x;
    const bool hasActionButton =
      endData.hovered || (showActionButtonVisibility && !type_visible) || (showActionButtonLock && type_locked);
    const float maxX = PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorGetLabelClipMaxX();
    const float actionButtonsStartX = maxX - action_buttons_total_width;
    const float treeNodeLabelClipMaxX = hasActionButton ? (actionButtonsStartX - ImGui::GetStyle().ItemSpacing.x) : maxX;

    handleDragAndDropDropping(type, -1);

    if (dim_type_color)
      ImGui::PushStyleColor(ImGuiCol_Text, dimmed_text_color);

    endData.textPos.x += iconWidthWithSpacing;
    PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorEnd(endData, &treeNodeLabelClipMaxX);

    if (dim_type_color)
      ImGui::PopStyleColor();

    const ImVec2 originaCursorPos = ImGui::GetCursorScreenPos();

    ImGui::SetCursorScreenPos(leftIconPos);
    ImGui::Image(icons.getForType(type), fontSizedIconSize);

    ImGui::SetCursorScreenPos(ImVec2(actionButtonsStartX, endData.textPos.y));

    const OutlinerSelectionState selectionState = tree_item.getSelectionState();
    const ImTextureID typeSelectionIcon = icons.getSelectionIcon(selectionState);
    if (PropPanel::ImguiHelper::imageButtonFramelessOrPlaceholder("type_select", typeSelectionIcon, fontSizedIconSize,
          "Toggle selection", endData.hovered))
    {
      const bool select = selectionState == OutlinerSelectionState::None || selectionState == OutlinerSelectionState::Partial;
      treeInterface->selectAllTypeObjects(type, select);
    }

    if (showActionButtonVisibility)
    {
      ImGui::SameLine();
      const ImTextureID typeVisibilityIcon = icons.getVisibilityIcon(type_visible);
      if (PropPanel::ImguiHelper::imageButtonFramelessOrPlaceholder("type_visibility", typeVisibilityIcon, fontSizedIconSize,
            "Toggle visibility", endData.hovered || !type_visible))
        treeInterface->toggleTypeVisibility(type);
    }

    if (showActionButtonLock)
    {
      ImGui::SameLine();
      const ImTextureID typeLockIcon = icons.getLockIcon(type_locked);
      if (PropPanel::ImguiHelper::imageButtonFramelessOrPlaceholder("type_lock", typeLockIcon, fontSizedIconSize, "Toggle locking",
            endData.hovered || type_locked))
        treeInterface->toggleTypeLock(type);
    }

    ImGui::SetCursorScreenPos(originaCursorPos);
  }

  if (isExpanded != wasExpanded)
    tree_item.setExpanded(isExpanded);

  // Imitate Windows where the context menu comes up on mouse button release. (BeginPopupContextItem also does this.)
  if (endData.hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    createContextMenu(type);

  return isExpanded;
}

bool OutlinerWindow::showLayerControls(LayerTreeItem &tree_item, int type, int per_type_layer_index, bool layer_visible,
  bool layer_locked, bool dim_layer_color, int object_count, const ImVec4 &dimmed_text_color, float action_buttons_total_width)
{
  const bool selected = tree_item.isSelected();
  if (selected && layerRenamer)
  {
    if (!layerRenamer->updateImgui())
      layerRenamer.reset();
    return false;
  }

  const bool wasExpanded = tree_item.isExpanded();
  ImGui::SetNextItemOpen(wasExpanded);

  G_STATIC_ASSERT(sizeof(ImGuiSelectionUserData) == sizeof(&tree_item));
  ImGui::SetNextItemSelectionUserData(reinterpret_cast<ImGuiSelectionUserData>(&tree_item));

  ImGuiTreeNodeFlags layerTreeNodeFlags = ImGuiTreeNodeFlags_NavLeftJumpsBackHere | ImGuiTreeNodeFlags_SpanAvailWidth |
                                          ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_OpenOnDoubleClick;
  if (object_count == 0)
    layerTreeNodeFlags |= ImGuiTreeNodeFlags_Leaf;
  if (selected)
    layerTreeNodeFlags |= ImGuiTreeNodeFlags_Selected;

  PropPanel::ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData endData;
  const bool isExpanded =
    PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorStart(tree_item.getLabel(), layerTreeNodeFlags, endData);

  updateSelectionHead(tree_item);

  if (endData.draw)
  {
    const ImVec2 leftIconPos = endData.textPos;
    const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
    const float iconWidthWithSpacing = fontSizedIconSize.x + ImGui::GetStyle().ItemSpacing.x;
    const bool layerApplyToMask = treeInterface->isLayerAppliedToMask(type, per_type_layer_index);
    const bool layerExported = treeInterface->isLayerExported(type, per_type_layer_index);
    const bool hasActionButton = endData.hovered || (showActionButtonVisibility && !layer_visible) ||
                                 (showActionButtonLock && layer_locked) || (showActionButtonApplyToMask && !layerApplyToMask) ||
                                 (showActionButtonExportLayer && !layerExported);
    const float maxX = PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorGetLabelClipMaxX();
    const float actionButtonsStartX = maxX - action_buttons_total_width;
    const float treeNodeLabelClipMaxX = hasActionButton ? (actionButtonsStartX - ImGui::GetStyle().ItemSpacing.x) : maxX;

    handleDragAndDropDropping(type, per_type_layer_index);

    if (dim_layer_color)
      ImGui::PushStyleColor(ImGuiCol_Text, dimmed_text_color);

    endData.textPos.x += iconWidthWithSpacing;
    PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorEnd(endData, &treeNodeLabelClipMaxX);

    if (dim_layer_color)
      ImGui::PopStyleColor();

    const ImVec2 originaCursorPos = ImGui::GetCursorScreenPos();

    ImGui::SetCursorScreenPos(leftIconPos);
    if (layer_locked)
    {
      const ImGuiID layerButtonId = ImGui::GetCurrentWindow()->GetID("layer");
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true); // Using this instead of BeginDisabled to avoid dimming.
      PropPanel::ImguiHelper::imageButtonFrameless(layerButtonId, icons.layerLocked, fontSizedIconSize);
      ImGui::PopItemFlag();
      PropPanel::set_previous_imgui_control_tooltip((const void *)layerButtonId, "This layer is locked.");
    }
    else
    {
      const ImTextureID layerIcon = treeInterface->isLayerActive(type, per_type_layer_index) ? icons.layerActive : icons.layerDefault;
      if (PropPanel::ImguiHelper::imageButtonFrameless("layer", layerIcon, fontSizedIconSize, "Set active layer"))
        treeInterface->setLayerActive(type, per_type_layer_index);
    }

    ImGui::SetCursorScreenPos(ImVec2(actionButtonsStartX, endData.textPos.y));

    const OutlinerSelectionState selectionState = tree_item.getSelectionState();
    const ImTextureID layerSelectionIcon = icons.getSelectionIcon(selectionState);
    if (PropPanel::ImguiHelper::imageButtonFramelessOrPlaceholder("layer_select", layerSelectionIcon, fontSizedIconSize,
          "Toggle selection", endData.hovered))
    {
      const bool select = selectionState == OutlinerSelectionState::None || selectionState == OutlinerSelectionState::Partial;
      treeInterface->selectAllLayerObjects(type, per_type_layer_index, select);
    }

    if (showActionButtonVisibility)
    {
      const bool canChangeVisibility = treeInterface->canChangeLayerVisibility(type, per_type_layer_index);
      if (!canChangeVisibility)
        ImGui::BeginDisabled();

      ImGui::SameLine();
      const ImTextureID layerVisibilityIcon = icons.getVisibilityIcon(layer_visible);
      if (PropPanel::ImguiHelper::imageButtonFramelessOrPlaceholder("layer_visibility", layerVisibilityIcon, fontSizedIconSize,
            "Toggle visibility", endData.hovered || !layer_visible))
        treeInterface->toggleLayerVisibility(type, per_type_layer_index);

      if (!canChangeVisibility)
        ImGui::EndDisabled();
    }

    if (showActionButtonLock)
    {
      const bool canChangeLock = treeInterface->canChangeLayerLock(type, per_type_layer_index);
      if (!canChangeLock)
        ImGui::BeginDisabled();

      ImGui::SameLine();
      const ImTextureID layerLockIcon = icons.getLockIcon(layer_locked);
      if (PropPanel::ImguiHelper::imageButtonFramelessOrPlaceholder("layer_lock", layerLockIcon, fontSizedIconSize, "Toggle locking",
            endData.hovered || layer_locked))
        treeInterface->toggleLayerLock(type, per_type_layer_index);

      if (!canChangeLock)
        ImGui::EndDisabled();
    }

    if (showActionButtonApplyToMask)
    {
      ImGui::SameLine();
      const ImTextureID layerApplyToMaskIcon = layerApplyToMask ? icons.applyToMask : icons.applyToMaskDisabled;
      if (PropPanel::ImguiHelper::imageButtonFramelessOrPlaceholder("layer_apply_to_mask", layerApplyToMaskIcon, fontSizedIconSize,
            "Toggle apply to mask", endData.hovered || !layerApplyToMask))
        treeInterface->toggleLayerApplyToMask(type, per_type_layer_index);
    }

    if (showActionButtonExportLayer)
    {
      ImGui::SameLine();
      const ImTextureID layerExportedIcon = layerExported ? icons.exportLayer : icons.exportLayerDisabled;
      if (PropPanel::ImguiHelper::imageButtonFramelessOrPlaceholder("layer_exported", layerExportedIcon, fontSizedIconSize,
            "Toggle export", endData.hovered || !layerExported))
        treeInterface->toggleLayerExport(type, per_type_layer_index);
    }

    ImGui::SetCursorScreenPos(originaCursorPos);
  }

  if (isExpanded != wasExpanded)
    tree_item.setExpanded(isExpanded);

  // Imitate Windows where the context menu comes up on mouse button release. (BeginPopupContextItem also does this.)
  if (endData.hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    createContextMenu(type, per_type_layer_index);

  return isExpanded;
}

const char *OutlinerWindow::getObjectNoun(int type, int count) const
{
  if (type == -1)
    return count > 1 ? "Objects" : "Object";
  else
    return treeInterface->getTypeName(type, count > 1);
}

bool OutlinerWindow::showObjectControls(ObjectTreeItem &tree_item, int type, int per_type_layer_index, int object_index,
  bool has_child)
{
  const bool selected = tree_item.isSelected();
  if (selected && objectRenamer)
  {
    if (!objectRenamer->updateImgui())
      objectRenamer.reset();
    return false;
  }

  ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_NavLeftJumpsBackHere | ImGuiTreeNodeFlags_SpanAvailWidth;
  if (!has_child)
    treeFlags |= ImGuiTreeNodeFlags_Leaf;
  if (selected)
    treeFlags |= ImGuiTreeNodeFlags_Selected;

  const bool wasExpanded = tree_item.isExpanded();
  ImGui::SetNextItemOpen(wasExpanded);

  G_STATIC_ASSERT(sizeof(ImGuiSelectionUserData) == sizeof(&tree_item));
  ImGui::SetNextItemSelectionUserData(reinterpret_cast<ImGuiSelectionUserData>(&tree_item));

  PropPanel::ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData endData;
  const ImGuiID nodeId = ImGui::GetCurrentWindow()->GetID(&tree_item);
  const char *label = tree_item.getLabel();
  if (label == nullptr || *label == 0)
    label = "<empty>";
  const bool isExpanded = PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorStart(nodeId, treeFlags, label, nullptr, endData);

  updateSelectionHead(tree_item);

  if (endData.draw)
  {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_PayloadNoCrossProcess))
    {
      DragData dragData;
      dragData.objectType = type;
      dragData.perTypeLayerIndex = per_type_layer_index;
      ImGui::SetDragDropPayload(OUTLINER_DRAG_AND_DROP_TYPE, &dragData, sizeof(dragData));

      int selectionType;
      int affectedLayerCount;
      int firstAffectedLayerIndex;
      const int selectedObjectCount = outlinerModel->getExclusiveObjectsSelectionDetailsIgnoringAssetNames(selectionType,
        affectedLayerCount, firstAffectedLayerIndex);
      const char *objectNoun = getObjectNoun(selectionType, selectedObjectCount);
      if (affectedLayerCount == 1)
      {
        const char *layerName = treeInterface->getLayerName(type, per_type_layer_index);
        ImGui::Text("Move %d %s from \"%s\" layer", selectedObjectCount, objectNoun, layerName);
      }
      else
      {
        ImGui::Text("Move %d %s from multiple layers", selectedObjectCount, objectNoun);
      }

      ImGui::EndDragDropSource();
    }

    handleDragAndDropDropping(type, per_type_layer_index);

    PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorEnd(endData);

    if (endData.hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
      G_ASSERT(tree_item.getType() == OutlinerTreeItem::ItemType::Object);
      ObjectTreeItem &objectTreeItem = static_cast<ObjectTreeItem &>(tree_item);
      treeInterface->zoomAndCenterObject(*objectTreeItem.renderableEditableObject);
    }
    // Imitate Windows where the context menu comes up on mouse button release. (BeginPopupContextItem also does this.)
    else if (endData.hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
      createContextMenu(type, per_type_layer_index, true);
    }
  }

  if (isExpanded != wasExpanded && has_child)
    tree_item.setExpanded(isExpanded);

  return isExpanded;
}

bool OutlinerWindow::showObjectAssetNameControls(ObjectAssetNameTreeItem &tree_item, RenderableEditableObject &object)
{
  const bool selected = tree_item.isSelected();

  ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf;
  if (selected)
    treeFlags |= ImGuiTreeNodeFlags_Selected;

  G_STATIC_ASSERT(sizeof(ImGuiSelectionUserData) == sizeof(&tree_item));
  ImGui::SetNextItemSelectionUserData(reinterpret_cast<ImGuiSelectionUserData>(&tree_item));

  PropPanel::ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData endData;
  const bool isExpanded = PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorStart(tree_item.getLabel(), treeFlags, endData);

  updateSelectionHead(tree_item);

  if (endData.draw)
  {
    const ImTextureID assetTypeIcon = getObjectAssetTypeIcon(object);
    if (assetTypeIcon == 0)
    {
      PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorEnd(endData);
    }
    else
    {
      const ImVec2 leftIconPos = endData.textPos;
      const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
      const float iconWidthWithSpacing = fontSizedIconSize.x + ImGui::GetStyle().ItemSpacing.x;

      endData.textPos.x += iconWidthWithSpacing;
      PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorEnd(endData);

      const ImVec2 originaCursorPos = ImGui::GetCursorScreenPos();
      ImGui::SetCursorScreenPos(leftIconPos);
      ImGui::Image(assetTypeIcon, fontSizedIconSize);
      ImGui::SetCursorScreenPos(originaCursorPos);
    }

    if (endData.hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
      changeAssetRequested = &object;
      PropPanel::request_delayed_callback(*this);
    }
  }

  ImGui::TreePop();
  return isExpanded;
}

void OutlinerWindow::fillTypeContextMenu(int type)
{
  G_ASSERT(treeInterface);
  G_ASSERT(contextMenu);
  if (!treeInterface || !contextMenu)
    return;

  if (!outlinerModel->isAnyTypeSelected())
    return;

  const int selectedType = outlinerModel->getExclusiveSelectedType(*treeInterface, showTypes);
  if (selectedType >= 0)
  {
    contextMenu->addItem(PropPanel::ROOT_MENU_ITEM, (unsigned)MenuItemId::AddNewLayer, "Add new layer");
    contextMenu->addSeparator(PropPanel::ROOT_MENU_ITEM);
  }

  const bool canChangeSelection = !treeInterface->isTypeLocked(type);

  contextMenu->addItem(PropPanel::ROOT_MENU_ITEM, (unsigned)MenuItemId::SelectAllTypeObjects, "Select all objects");
  contextMenu->setEnabledById((unsigned)MenuItemId::SelectAllTypeObjects, canChangeSelection);
  contextMenu->addItem(PropPanel::ROOT_MENU_ITEM, (unsigned)MenuItemId::DeselectAllTypeObjects, "Deselect all objects");
  contextMenu->setEnabledById((unsigned)MenuItemId::DeselectAllTypeObjects, canChangeSelection);
}

void OutlinerWindow::fillLayerContextMenu(int type, int per_type_layer_index)
{
  G_ASSERT(treeInterface);
  G_ASSERT(contextMenu);
  if (!treeInterface || !contextMenu)
    return;

  if (!outlinerModel->isAnyLayerSelected())
    return;

  const bool canChangeSelection = !treeInterface->isLayerLocked(type, per_type_layer_index);

  contextMenu->addItem(PropPanel::ROOT_MENU_ITEM, (unsigned)MenuItemId::SelectAllLayerObjects, "Select all objects");
  contextMenu->setEnabledById((unsigned)MenuItemId::SelectAllLayerObjects, canChangeSelection);
  contextMenu->addItem(PropPanel::ROOT_MENU_ITEM, (unsigned)MenuItemId::DeselectAllLayerObjects, "Deselect all objects");
  contextMenu->setEnabledById((unsigned)MenuItemId::DeselectAllLayerObjects, canChangeSelection);
  contextMenu->addSeparator(PropPanel::ROOT_MENU_ITEM);
  contextMenu->addItem(PropPanel::ROOT_MENU_ITEM, (unsigned)MenuItemId::ExpandLayerChildren, "Expand children");
  contextMenu->addItem(PropPanel::ROOT_MENU_ITEM, (unsigned)MenuItemId::CollapseLayerChildren, "Collapse children");

  if (treeInterface->isLayerRenameable(type, per_type_layer_index) &&
      outlinerModel->getExclusiveSelectionCount(OutlinerTreeItem::ItemType::Layer) == 1)
  {
    contextMenu->addSeparator(PropPanel::ROOT_MENU_ITEM);
    contextMenu->addItem(PropPanel::ROOT_MENU_ITEM, (unsigned)MenuItemId::RenameLayer, "Rename\tF2");
  }
}

void OutlinerWindow::fillObjectContextMenu(int type, int per_type_layer_index)
{
  G_ASSERT(treeInterface);
  G_ASSERT(contextMenu);
  if (!treeInterface || !contextMenu)
    return;

  bool addSeparator = false;

  { // Move to layer
    int selectedType;
    int layersAffectedCount;
    const bool isExclusiveType = outlinerModel->getExclusiveTypeAndLayerInfoForSelectedObjects(selectedType, layersAffectedCount);
    if (isExclusiveType && selectedType == type)
    {
      const int layerCount = treeInterface->getLayerCount(selectedType);
      if (layerCount > 1)
      {
        contextMenu->addSubMenu(PropPanel::ROOT_MENU_ITEM, (unsigned)MenuItemId::MoveObjectToLayerSubMenu, "Move to layer");

        int skippedTargetLayer = layersAffectedCount > 1 ? -1 : per_type_layer_index;
        for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
        {
          if (layerIndex == skippedTargetLayer)
            continue;

          const unsigned menuItemId = ((unsigned)MenuItemId::MoveObjectToLayerStart) + layerIndex;
          G_ASSERT(menuItemId < (unsigned)MenuItemId::MoveObjectToLayerEnd);
          const char *layerName = treeInterface->getLayerName(type, layerIndex);

          contextMenu->addItem((unsigned)MenuItemId::MoveObjectToLayerSubMenu, menuItemId, layerName);
          addSeparator = true;
        }
      }
    }
  }

  if (outlinerModel->getExclusiveObjectsSelectionCountIgnoringAssetNames() > 0)
  {
    if (addSeparator)
    {
      addSeparator = false;
      contextMenu->addSeparator(PropPanel::ROOT_MENU_ITEM);
    }

    contextMenu->addItem(PropPanel::ROOT_MENU_ITEM, (unsigned)MenuItemId::ChangeObjectAsset, "Change asset");

    // Just use a basic check here (not canRenameObject), so the rename panel can show the error message.
    const bool allowRenaming = outlinerModel->isOnlyASingleObjectIsSelected();
    contextMenu->addItem(PropPanel::ROOT_MENU_ITEM, (unsigned)MenuItemId::RenameObject, "Rename\tF2");
    contextMenu->setEnabledById((unsigned)MenuItemId::RenameObject, allowRenaming);

    if (contextMenu->getItemCount(PropPanel::ROOT_MENU_ITEM) > 0)
      contextMenu->addSeparator(PropPanel::ROOT_MENU_ITEM);

    contextMenu->addItem(PropPanel::ROOT_MENU_ITEM, (unsigned)MenuItemId::DeleteObject, "Delete\tDelete");
  }
}

void OutlinerWindow::createContextMenu(int type, int per_type_layer_index, bool object_menu)
{
  if (contextMenu)
    return;

  contextMenu.reset(PropPanel::create_context_menu());
  contextMenu->setEventHandler(this);

  if (type >= 0 && per_type_layer_index >= 0 && object_menu)
    fillObjectContextMenu(type, per_type_layer_index);
  else if (type >= 0 && per_type_layer_index >= 0)
    fillLayerContextMenu(type, per_type_layer_index);
  else if (type >= 0)
    fillTypeContextMenu(type);
}

void OutlinerWindow::createContextMenuByKeyboard()
{
  int type;
  int perTypeLayerIndex;

  dag::Vector<RenderableEditableObject *> selectedObjects = outlinerModel->getSelectedObjects();
  if (selectedObjects.size() > 0)
  {
    if (treeInterface->getObjectTypeAndPerTypeLayerIndex(*selectedObjects[0], type, perTypeLayerIndex))
      createContextMenu(type, perTypeLayerIndex, true);
  }
  else
  {
    if (outlinerModel->getExclusiveSelectedLayer(type, perTypeLayerIndex))
    {
      createContextMenu(type, perTypeLayerIndex);
    }
    else if (outlinerModel->isAnyTypeSelected())
    {
      type = outlinerModel->getExclusiveSelectedType(*treeInterface, showTypes);
      if (type >= 0)
        createContextMenu(type);
    }
  }
}

void OutlinerWindow::updateSelectionHead(OutlinerTreeItem &tree_item)
{
  const bool focused = ImGui::IsItemFocused();
  if (focused)
    outlinerModel->setSelectionHead(&tree_item);

  if (ensureVisibleRequested != EnsureVisibleRequestState::NoRequest && tree_item.isSelected() &&
      tree_item.getType() == OutlinerTreeItem::ItemType::Object)
  {
    // Prefer the focused selection, then the first selection.
    if (focused)
    {
      ensureVisibleRequested = EnsureVisibleRequestState::NoRequest;
      ImGui::SetScrollHereY();
    }
    else if (ensureVisibleRequested == EnsureVisibleRequestState::Requested)
    {
      ensureVisibleRequested = EnsureVisibleRequestState::FoundSelection;
      ImGui::SetScrollHereY();
    }
  }
}

void OutlinerWindow::fillTree()
{
  G_ASSERT(treeInterface);

  const ImVec4 textColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
  const ImVec4 dimmedTextColor(textColor.x, textColor.y, textColor.z, textColor.w * 0.7f);
  const ImGuiID lastItemIdAtStart = ImGui::GetItemID();

  int actionButtonCount = 1; // One button for selection state.
  actionButtonCount += showActionButtonVisibility ? 1 : 0;
  actionButtonCount += showActionButtonLock ? 1 : 0;
  actionButtonCount += showActionButtonApplyToMask ? 1 : 0;
  actionButtonCount += showActionButtonExportLayer ? 1 : 0;

  const float actionButtonsTotalWidth =
    (PropPanel::ImguiHelper::getFontSizedIconSize().x + ImGui::GetStyle().ItemSpacing.x) * actionButtonCount;

  outlinerModel->setSelectionHead(nullptr);

  for (int typeIndex = 0; typeIndex < outlinerModel->objectTypes.size(); ++typeIndex)
  {
    ObjectTypeTreeItem *objectTypeTreeItem = outlinerModel->objectTypes[typeIndex];
    if (!showTypes[typeIndex] || !objectTypeTreeItem->matchesSearchTextRecursive())
      continue;

    ImGui::PushID(typeIndex);

    const bool typeVisible = treeInterface->isTypeVisible(typeIndex);
    const bool typeLocked = treeInterface->isTypeLocked(typeIndex);
    const bool dimTypeColor = !typeVisible;
    const bool dimTypeChildColor = dimTypeColor || typeLocked;
    const int layerCount = objectTypeTreeItem->layers.size();

    const bool typeOpen = showTypeControls(*objectTypeTreeItem, typeIndex, typeVisible, typeLocked, dimTypeColor, layerCount,
      dimmedTextColor, actionButtonsTotalWidth);

    if (typeIndex == addingLayerToType)
      showAddLayerControls();

    if (!typeOpen)
    {
      ImGui::PopID();
      continue;
    }

    for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
      LayerTreeItem *layerTreeItem = objectTypeTreeItem->layers[layerIndex];
      if (!layerTreeItem->matchesSearchTextRecursive())
        continue;

      ImGui::PushID(layerIndex);

      const bool layerVisible = treeInterface->isLayerVisible(typeIndex, layerIndex);
      const bool layerLocked = treeInterface->isLayerLocked(typeIndex, layerIndex);
      const bool dimLayerColor = dimTypeChildColor || !layerVisible;
      const bool dimLayerChildColor = dimLayerColor || layerLocked;
      const int objectCount = layerTreeItem->sortedObjects.size();

      const bool layerOpen = showLayerControls(*layerTreeItem, typeIndex, layerIndex, layerVisible, layerLocked, dimLayerColor,
        objectCount, dimmedTextColor, actionButtonsTotalWidth);

      if (!layerOpen)
      {
        ImGui::PopID();
        continue;
      }

      if (dimLayerChildColor)
        ImGui::PushStyleColor(ImGuiCol_Text, dimmedTextColor);

      for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
      {
        ObjectTreeItem *objectTreeItem = layerTreeItem->sortedObjects[objectIndex];
        if (!objectTreeItem->matchesSearchTextRecursive())
          continue;

        ObjectAssetNameTreeItem *objectAssetNameTreeItem = objectTreeItem->objectAssetNameTreeItem.get();
        const bool hasAssetName = objectAssetNameTreeItem != nullptr;
        if (!showObjectControls(*objectTreeItem, typeIndex, layerIndex, objectIndex, hasAssetName))
          continue;

        if (hasAssetName)
          showObjectAssetNameControls(*objectAssetNameTreeItem, *objectTreeItem->renderableEditableObject);

        ImGui::TreePop();
      }

      if (dimLayerChildColor)
        ImGui::PopStyleColor();

      ImGui::TreePop();
      ImGui::PopID();
    }

    ImGui::TreePop();
    ImGui::PopID();
  }

  // If there are no search results then show the no results message at the center of the tree control.
  if (!outlinerModel->textToSearch.empty() && ImGui::GetItemID() == lastItemIdAtStart)
  {
    const char *message = "No results found";
    const ImVec2 textSize = ImGui::CalcTextSize(message, nullptr);
    ImVec2 posMin = ImGui::GetCursorScreenPos() + ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
    ImVec2 posMax = posMin + ImGui::GetContentRegionAvail();
    posMin.x = max(posMin.x, posMin.x + (((posMax.x - posMin.x) - textSize.x) * 0.5f));
    posMin.y = max(posMin.y, posMin.y + (((posMax.y - posMin.y) - textSize.y) * 0.5f));
    posMax.x = min(posMax.x, posMin.x + textSize.x);
    posMax.y = min(posMax.y, posMin.y + textSize.y);
    ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), posMin, posMax, posMax.x, posMax.x, message, nullptr, &textSize);
  }

  ImGui::Dummy(ImVec2(0.0f, 0.0f)); // This is here to prevent assert in ImGui::ErrorCheckUsingSetCursorPosToExtendParentBoundaries().

  ensureVisibleRequested = EnsureVisibleRequestState::NoRequest;

  if (addingLayerToType < 0 && !layerRenamer && !objectRenamer)
  {
    if (ImGui::Shortcut(ImGuiKey_F2))
    {
      if (outlinerModel->isOnlyASingleObjectIsSelected())
        onMenuItemClick((unsigned)MenuItemId::RenameObject);
      else
        onMenuItemClick((unsigned)MenuItemId::RenameLayer);
    }
    else if (ImGui::Shortcut(ImGuiKey_Delete))
    {
      onMenuItemClick((unsigned)MenuItemId::DeleteObject);
    }
    else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_F))
    {
      PropPanel::focus_helper.requestFocus(&searchInputFocusId);
    }
    else if (ImGui::Shortcut(ImGuiMod_Shift | ImGuiKey_LeftArrow))
    {
      if (OutlinerTreeItem *selectionHead = outlinerModel->getSelectionHead())
        selectionHead->setExpandedRecursive(false);
    }
    else if (ImGui::Shortcut(ImGuiMod_Shift | ImGuiKey_RightArrow))
    {
      if (OutlinerTreeItem *selectionHead = outlinerModel->getSelectionHead())
        selectionHead->setExpandedRecursive(true);
    }
    else if (ImGui::Shortcut(ImGuiKey_Z))
    {
      ensureVisibleRequested = EnsureVisibleRequestState::Requested;
    }
    else if (ImGui::Shortcut(ImGuiKey_Menu))
    {
      createContextMenuByKeyboard();
    }
    // Imitate Windows where the context menu comes up on mouse button release. (BeginPopupContextItem also does this.)
    else if (!contextMenu && ImGui::IsWindowHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
      createContextMenuByKeyboard();
    }
  }
}

bool OutlinerWindow::applyRangeSelectionRequestInternal(const ImGuiSelectionRequest &request, OutlinerTreeItem &tree_item,
  bool &found_first)
{
  if (&tree_item == reinterpret_cast<OutlinerTreeItem *>(request.RangeFirstItem))
  {
    G_ASSERT(!found_first);
    found_first = true;
  }

  if (found_first)
  {
    if (tree_item.getType() == OutlinerTreeItem::ItemType::Object)
    {
      ObjectTreeItem &objectTreeItem = static_cast<ObjectTreeItem &>(tree_item);
      const bool selectObject = request.Selected && treeInterface->canSelectObject(*objectTreeItem.renderableEditableObject);

      // This will call onObjectSelectionChanged.
      treeInterface->setObjectSelected(*objectTreeItem.renderableEditableObject, selectObject);
      G_ASSERT(objectTreeItem.isSelected() == selectObject);
    }
    else
    {
      tree_item.setSelected(request.Selected);
    }

    if (&tree_item == reinterpret_cast<OutlinerTreeItem *>(request.RangeLastItem))
      return false;
  }

  if (tree_item.isExpanded())
  {
    const int childCount = tree_item.getChildCount();
    for (int childIndex = 0; childIndex < childCount; ++childIndex)
      if (!applyRangeSelectionRequestInternal(request, *tree_item.getChild(childIndex), found_first))
        return false;
  }

  return true;
}

void OutlinerWindow::applyRangeSelectionRequest(const ImGuiSelectionRequest &request)
{
  G_ASSERT(request.Type == ImGuiSelectionRequestType_SetRange);

  bool foundFirst = false;
  for (ObjectTypeTreeItem *objectTypeTreeItem : outlinerModel->objectTypes)
    if (!applyRangeSelectionRequestInternal(request, *objectTypeTreeItem, foundFirst))
      break;
}

void OutlinerWindow::applySelectionRequests(const ImGuiMultiSelectIO &multi_select_io, bool &started_selecting)
{
  if (multi_select_io.Requests.empty())
    return;

  if (!started_selecting)
  {
    started_selecting = true;
    treeInterface->startObjectSelection();
  }

  for (const ImGuiSelectionRequest &request : multi_select_io.Requests)
  {
    // The design was that Ctrl+A should only selects objects. When filtering is enabled then it should only select
    // objects that matches the filter.
    if (request.Type == ImGuiSelectionRequestType_SetAll)
    {
      // If requested unselect everything. We cannot use our object list here for unselection because it might not
      // have all objects. (For example we have SplineObject but not SplinePointObject, so not everything would be
      // unselected.)
      if (!request.Selected)
        treeInterface->unselectAllObjects();

      const int typeCount = outlinerModel->objectTypes.size();
      for (int typeIndex = 0; typeIndex < typeCount; ++typeIndex)
      {
        ObjectTypeTreeItem *objectTypeTreeItem = outlinerModel->objectTypes[typeIndex];
        objectTypeTreeItem->setSelected(false);

        const int layerCount = objectTypeTreeItem->layers.size();
        for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
        {
          LayerTreeItem *layerTreeItem = objectTypeTreeItem->layers[layerIndex];
          layerTreeItem->setSelected(false);

          const int objectCount = layerTreeItem->sortedObjects.size();
          for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
          {
            ObjectTreeItem *objectTreeItem = layerTreeItem->sortedObjects[objectIndex];

            const bool selectObject = request.Selected && showTypes[typeIndex] &&
                                      treeInterface->canSelectObject(*objectTreeItem->renderableEditableObject) &&
                                      objectTreeItem->matchesSearchTextRecursive();

            // This will call onObjectSelectionChanged.
            treeInterface->setObjectSelected(*objectTreeItem->renderableEditableObject, selectObject);
            G_ASSERT(objectTreeItem->isSelected() == selectObject);

            if (objectTreeItem->getChildCount() == 1)
              objectTreeItem->getChild(0)->setSelected(false);
          }
        }
      }
    }
    else if (request.Type == ImGuiSelectionRequestType_SetRange)
    {
      applyRangeSelectionRequest(request);
    }
  }
}

void OutlinerWindow::showAddLayerControls()
{
  G_ASSERT(addingLayerToType >= 0);

  // Draw the controls into a separate draw channel. This way we are able to measure their size and can easily draw a
  // background.
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  G_ASSERT(drawList->_Splitter._Current == 0);
  drawList->ChannelsSplit(2);
  drawList->ChannelsSetCurrent(1);

  // Unlike the rename panel, the add panel has full width within the tree.
  const ImVec2 backgroundPadding(ImGui::GetStyle().FramePadding);
  const ImVec2 cursorStartPos(ImGui::GetCurrentWindow()->ContentRegionRect.Min.x + ImGui::GetScrollX(), ImGui::GetCursorScreenPos().y);
  ImGui::SetCursorScreenPos(cursorStartPos + backgroundPadding);

  ImGui::BeginGroup();

  ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
  ImGui::TextUnformatted("Enter layer name:");
  ImGui::PopStyleColor();

  const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
  const ImVec2 imageButtonSize = PropPanel::ImguiHelper::getImageButtonFramelessFullSize(fontSizedIconSize);
  const float imageButtonsWidth = imageButtonSize.x * 2; // We have two buttons.
  const float inputWidth = floorf(ImGui::GetContentRegionAvail().x + ImGui::GetScrollX() - ImGui::GetStyle().ItemSpacing.x -
                                  imageButtonsWidth - backgroundPadding.x - ImGui::GetStyle().FramePadding.x);
  if (inputWidth > 0.0f)
    ImGui::SetNextItemWidth(inputWidth);

  PropPanel::focus_helper.setFocusToNextImGuiControlIfRequested(&addingLayerToType);

  ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
  const bool addInputAccepted = ImGuiDagor::InputText("##addInput", &addLayerName, ImGuiInputTextFlags_EnterReturnsTrue);
  bool addInputCanceled = !addInputAccepted && ImGui::IsItemDeactivated();
  ImGui::PopStyleColor();

  addLayerErrorMessage.clear();
  const bool canAddLayer = treeInterface->canAddNewLayerWithName(addingLayerToType, addLayerName.c_str(), addLayerErrorMessage);

  // Center the buttons vertically to the text input.
  const float buttonOffsetY = (ImGui::GetItemRectSize().y - imageButtonSize.y) * 0.5f;

  ImGui::SameLine();
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + buttonOffsetY);
  if (!canAddLayer)
    ImGui::BeginDisabled();
  const bool okButtonPressed =
    PropPanel::ImguiHelper::imageButtonFrameless("addOk", icons.layerCreate, fontSizedIconSize, "Add layer");
  if (addInputCanceled && ImGui::IsItemActivated())
    addInputCanceled = false;
  if (!canAddLayer)
    ImGui::EndDisabled();

  ImGui::SameLine();
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + buttonOffsetY);
  // There is no need to check the result of this, clicking anywhere (beside the add button) will cancel the process.
  PropPanel::ImguiHelper::imageButtonFrameless("addCancel", icons.close, fontSizedIconSize, "Cancel");

  ImGui::SameLine(0.0f, 0.0f);
  const float cursorInputEndScreenPosX = ImGui::GetCursorScreenPos().x;
  const float cursorInputEndPosX = ImGui::GetCursorPos().x;
  ImGui::NewLine();

  // Draw the alert icon and the error message.
  if (!canAddLayer)
  {
    ImVec2 alertIconPos = ImGui::GetCursorScreenPos();
    const float alertIconLeftPadding = ImGui::GetStyle().ItemInnerSpacing.x;

    ImGui::Dummy(fontSizedIconSize + ImVec2(alertIconLeftPadding, 0.0f)); // Just reserve the horizontal space for the alert icon.
    ImGui::SameLine();

    ImGui::PushTextWrapPos(cursorInputEndPosX);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ImGui::TextUnformatted(addLayerErrorMessage.c_str());
    ImGui::PopStyleColor();
    ImGui::PopTextWrapPos();

    const ImVec2 textSize = ImGui::GetItemRectSize();
    alertIconPos.x += alertIconLeftPadding;
    alertIconPos.y += (textSize.y - fontSizedIconSize.y) * 0.5f; // Center the icon vertically.
    drawList->AddImage(icons.alert, alertIconPos, alertIconPos + fontSizedIconSize);
  }

  ImGui::EndGroup();

  // Draw the background.
  const ImVec2 cursorEndPos(cursorInputEndScreenPosX, ImGui::GetCursorScreenPos().y - ImGui::GetStyle().ItemSpacing.y);
  const ImVec2 rectTopLeft = cursorStartPos;
  const ImVec2 rectBottomRight(cursorEndPos + backgroundPadding);
  drawList->ChannelsSetCurrent(0);
  drawList->AddRectFilled(rectTopLeft, rectBottomRight,
    canAddLayer ? OUTLINER_INPLACE_EDIT_NORMAL_BACKGROUND : OUTLINER_INPLACE_EDIT_ERROR_BACKGROUND);

  drawList->ChannelsMerge();

  // Add some vertical space after the background.
  ImGui::Dummy(ImVec2(0.0f, 0.0f));

  if ((addInputAccepted || okButtonPressed) && canAddLayer)
  {
    outlinerModel->addNewLayer(*treeInterface, addingLayerToType, addLayerName);
    addingLayerToType = -1;
    addLayerName.clear();
  }
  else if (addInputCanceled)
  {
    addingLayerToType = -1;
    addLayerName.clear();
  }
}

void OutlinerWindow::showSettingsPanel(const char *popup_id)
{
  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
  const bool popupIsOpen = ImGui::BeginPopup(popup_id);
  ImGui::PopStyleColor();

  if (!popupIsOpen)
  {
    settingsPanelOpen = false;
    return;
  }

  ImGui::TextUnformatted("Shown buttons");

  const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();

  const ImTextureID visibilityIcon = icons.getVisibilityIcon(showActionButtonVisibility);
  if (PropPanel::ImguiHelper::imageCheckButtonWithBackground("settings_visibility", visibilityIcon, fontSizedIconSize,
        showActionButtonVisibility, "Show the visibility button"))
    showActionButtonVisibility = !showActionButtonVisibility;

  const ImTextureID lockIcon = icons.getLockIcon(!showActionButtonLock);
  ImGui::SameLine();
  if (PropPanel::ImguiHelper::imageCheckButtonWithBackground("settings_lock", lockIcon, fontSizedIconSize, showActionButtonLock,
        "Show the lock button"))
    showActionButtonLock = !showActionButtonLock;

  const ImTextureID applyToMaskIcon = showActionButtonApplyToMask ? icons.applyToMask : icons.applyToMaskDisabled;
  ImGui::SameLine();
  if (PropPanel::ImguiHelper::imageCheckButtonWithBackground("settings_mask", applyToMaskIcon, fontSizedIconSize,
        showActionButtonApplyToMask, "Show the apply to mask button"))
    showActionButtonApplyToMask = !showActionButtonApplyToMask;

  const ImTextureID exportLayerIcon = showActionButtonExportLayer ? icons.exportLayer : icons.exportLayerDisabled;
  ImGui::SameLine();
  if (PropPanel::ImguiHelper::imageCheckButtonWithBackground("settings_export", exportLayerIcon, fontSizedIconSize,
        showActionButtonExportLayer, "Show the export layer button"))
    showActionButtonExportLayer = !showActionButtonExportLayer;

  ImGui::NewLine();
  ImGui::TextUnformatted("Shown types");

  const int typeCount = treeInterface->getTypeCount();
  for (int typeIndex = 0; typeIndex < typeCount; ++typeIndex)
    ImGui::Checkbox(treeInterface->getTypeName(typeIndex), &showTypes[typeIndex]);

  ImGui::NewLine();
  bool showAssetNameUnderObjects = outlinerModel->getShowAssetNameUnderObjects();
  const bool showAssetNamePressed = ImGui::Checkbox("Show asset names", &showAssetNameUnderObjects);
  PropPanel::set_previous_imgui_control_tooltip((const void *)ImGui::GetItemID(),
    "Display the object's asset name below the object in the tree.");

  if (showAssetNamePressed)
    outlinerModel->setShowAssetNameUnderObjects(*treeInterface, showAssetNameUnderObjects);

  ImGui::EndPopup();
}

void OutlinerWindow::fillTypesAndLayers()
{
  G_ASSERT(!outlinerModel);
  outlinerModel.reset(new OutlinerModel());

  outlinerModel->fillTypesAndLayers(*treeInterface);

  const bool showByDefault = true;
  showTypes.resize(treeInterface->getTypeCount(), showByDefault);
}

void OutlinerWindow::loadOutlinerSettings(const DataBlock &settings)
{
  if (showTypes.empty())
    return;

  String tempString;
  for (int i = 0; i < showTypes.size(); ++i)
  {
    tempString.printf(12, "showType%d", i);
    showTypes[i] = settings.getBool(tempString, true);
  }

  showActionButtonVisibility = settings.getBool("showActionButtonVisibility", true);
  showActionButtonLock = settings.getBool("showActionButtonLock", true);
  showActionButtonApplyToMask = settings.getBool("showActionButtonApplyToMask", true);
  showActionButtonExportLayer = settings.getBool("showActionButtonExportLayer", true);

  const bool showAssetNameUnderObjects = settings.getBool("showAssetNameUnderObjects", true);
  outlinerModel->setShowAssetNameUnderObjects(*treeInterface, showAssetNameUnderObjects);
}

void OutlinerWindow::saveOutlinerSettings(DataBlock &settings) const
{
  if (showTypes.empty())
    return;

  String tempString;
  for (int i = 0; i < showTypes.size(); ++i)
  {
    tempString.printf(12, "showType%d", i);
    settings.setBool(tempString, showTypes[i]);
  }

  settings.setBool("showActionButtonVisibility", showActionButtonVisibility);
  settings.setBool("showActionButtonLock", showActionButtonLock);
  settings.setBool("showActionButtonApplyToMask", showActionButtonApplyToMask);
  settings.setBool("showActionButtonExportLayer", showActionButtonExportLayer);
  settings.setBool("showAssetNameUnderObjects", outlinerModel->getShowAssetNameUnderObjects());
}

void OutlinerWindow::onAddObject(RenderableEditableObject &object) { outlinerModel->onAddObject(*treeInterface, object); }

void OutlinerWindow::onRemoveObject(RenderableEditableObject &object)
{
  ObjectTreeItemInlinerRenamerControl *renamer = static_cast<ObjectTreeItemInlinerRenamerControl *>(objectRenamer.get());
  if (renamer && &renamer->getObject() == &object)
    objectRenamer.reset();

  if (changeAssetRequested == &object)
    changeAssetRequested = nullptr;

  outlinerModel->onRemoveObject(*treeInterface, object);
}

void OutlinerWindow::onRenameObject(RenderableEditableObject &object) { outlinerModel->onRenameObject(*treeInterface, object); }

void OutlinerWindow::onObjectSelectionChanged(RenderableEditableObject &object)
{
  outlinerModel->onObjectSelectionChanged(*treeInterface, object);
}

void OutlinerWindow::onObjectAssetNameChanged(RenderableEditableObject &object)
{
  outlinerModel->onObjectAssetNameChanged(*treeInterface, object);
}

void OutlinerWindow::onObjectEditLayerChanged(RenderableEditableObject &object)
{
  outlinerModel->onObjectEditLayerChanged(*treeInterface, object);
}

void OutlinerWindow::updateImgui()
{
  if (!treeInterface)
    return;

  ImGui::PushID(this);

  const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();

  const int selectedType = outlinerModel->getExclusiveSelectedType(*treeInterface, showTypes);
  if (selectedType < 0)
    ImGui::BeginDisabled();

  const bool pressedAddLayer = ImGui::ImageButton("addLayer", icons.layerAdd, fontSizedIconSize);
  PropPanel::set_previous_imgui_control_tooltip((const void *)ImGui::GetItemID(), "Add new layer");

  if (selectedType < 0)
    ImGui::EndDisabled();

  if (pressedAddLayer)
  {
    addingLayerToType = selectedType;
    PropPanel::focus_helper.requestFocus(&addingLayerToType);
  }

  const ImVec2 settingsButtonSize = PropPanel::ImguiHelper::getImageButtonWithDownArrowSize(fontSizedIconSize);

  ImGui::SameLine();
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (ImGui::GetStyle().ItemSpacing.x + settingsButtonSize.x));
  if (PropPanel::ImguiHelper::searchInput(&searchInputFocusId, "##searchInput", "Search", outlinerModel->textToSearch, icons.search,
        icons.close))
    outlinerModel->onFilterChanged(*treeInterface);
  ImGui::SameLine();

  const char *popupId = "settingsPopup";
  const ImTextureID settingsButtonIcon = settingsPanelOpen ? icons.settingsOpen : icons.settings;
  const bool settingsButtonPressed =
    PropPanel::ImguiHelper::imageButtonWithArrow("settingsButton", settingsButtonIcon, fontSizedIconSize, settingsPanelOpen);
  PropPanel::set_previous_imgui_control_tooltip((const void *)ImGui::GetItemID(), "Settings");

  if (settingsButtonPressed)
  {
    ImGui::OpenPopup(popupId);
    settingsPanelOpen = true;
  }

  if (settingsPanelOpen)
    showSettingsPanel(popupId);

  const char *expandAllTitle = "Expand all";
  const char *collapseAllTitle = "Collapse all";
  const ImVec2 expandAllSize = PropPanel::ImguiHelper::getButtonSize(expandAllTitle);
  const ImVec2 collapseAllSize = PropPanel::ImguiHelper::getButtonSize(collapseAllTitle);
  const ImVec2 expandButtonRegionAvailable = ImGui::GetContentRegionAvail();
  const ImVec2 expandButtonCursorPos = ImGui::GetCursorPos();

  ImGui::SetCursorPosX(
    expandButtonCursorPos.x + expandButtonRegionAvailable.x - (expandAllSize.x + ImGui::GetStyle().ItemSpacing.x + collapseAllSize.x));
  if (ImGui::Button(expandAllTitle))
    outlinerModel->expandAll(true);

  ImGui::SameLine();
  ImGui::SetCursorPosX(expandButtonCursorPos.x + expandButtonRegionAvailable.x - collapseAllSize.x);
  if (ImGui::Button(collapseAllTitle))
    outlinerModel->expandAll(false);

  const ImGuiID childWindowId = ImGui::GetCurrentWindow()->GetID("c");
  const ImVec2 childRegionAvailable = ImGui::GetContentRegionAvail();
  const ImVec2 childSize(childRegionAvailable.x, childRegionAvailable.y);
  if (ImGui::BeginChild(childWindowId, childSize, ImGuiChildFlags_FrameStyle,
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_HorizontalScrollbar))
  {
    bool startedSelecting = false;

    // When showing the rename text input and setting the initial focus to it we got a selection changed request if
    // ImGuiMultiSelectFlags_NoAutoSelect was not set.
    const ImGuiMultiSelectFlags multiSelectFlags =
      ImGuiMultiSelectFlags_ClearOnClickVoid |
      ((layerRenamer || objectRenamer) ? ImGuiMultiSelectFlags_NoAutoSelect : ImGuiMultiSelectFlags_BoxSelect1d);

    ImGuiMultiSelectIO *multiSelectIo = ImGui::BeginMultiSelect(multiSelectFlags, -1, -1);
    if (multiSelectIo)
      applySelectionRequests(*multiSelectIo, startedSelecting);

    ImGui::PushStyleColor(ImGuiCol_Header, PropPanel::Constants::TREE_SELECTION_BACKGROUND_COLOR);
    fillTree();
    ImGui::PopStyleColor();

    multiSelectIo = ImGui::EndMultiSelect();
    if (multiSelectIo)
      applySelectionRequests(*multiSelectIo, startedSelecting);

    if (startedSelecting)
      treeInterface->endObjectSelection();

    if (contextMenu && (contextMenu->getItemCount(PropPanel::ROOT_MENU_ITEM) == 0 || !PropPanel::render_context_menu(*contextMenu)))
      contextMenu.reset();
  }
  ImGui::EndChild();

  ImGui::PopID();
}

} // namespace Outliner