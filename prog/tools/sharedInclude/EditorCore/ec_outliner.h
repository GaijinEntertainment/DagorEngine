// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_outlinerInterface.h>
#include <propPanel/control/menu.h>
#include <propPanel/messageQueue.h>
#include <propPanel/propPanel.h>
#include <util/dag_string.h>
#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>

namespace PropPanel
{
class IMenu;
}

class DataBlock;
class RenderableEditableObject;
struct ImGuiMultiSelectIO;
struct ImGuiSelectionRequest;
struct ImVec4;

#ifndef ImTextureID
typedef void *ImTextureID;
#endif

namespace Outliner
{

class LayerTreeItem;
class ObjectAssetNameTreeItem;
class ObjectTreeItem;
class ObjectTypeTreeItem;
class OutlinerModel;
class OutlinerTreeItem;
class TreeItemInlineRenamerControl;
enum class OutlinerSelectionState;

class OutlinerWindow : public PropPanel::IMenuEventHandler, public PropPanel::IDelayedCallbackHandler
{
public:
  OutlinerWindow();
  virtual ~OutlinerWindow();

  void setTreeInterface(IOutliner *tree_interface) { treeInterface = tree_interface; }
  virtual void fillTypesAndLayers();

  // loadOutlinerSettings must be after fillTypesAndLayers because it depends on the number of object types being
  // available.
  virtual void loadOutlinerSettings(const DataBlock &settings);
  virtual void saveOutlinerSettings(DataBlock &settings) const;

  // loadOutlinerState must be called after the objects have been added to properly restore their expansion state, and
  // saveOutlinerState must be called before removing the objects to be able to get their expansion state.
  virtual void loadOutlinerState(const DataBlock &state);
  virtual void saveOutlinerState(DataBlock &state) const;

  virtual void onAddObject(RenderableEditableObject &object);
  virtual void onRemoveObject(RenderableEditableObject &object);
  virtual void onRenameObject(RenderableEditableObject &object);
  virtual void onObjectSelectionChanged(RenderableEditableObject &object);
  virtual void onObjectAssetNameChanged(RenderableEditableObject &object);
  virtual void onObjectEditLayerChanged(RenderableEditableObject &object);

  virtual void updateImgui();

private:
  struct DragData
  {
    int objectType;
    int perTypeLayerIndex;
  };

  struct Icons
  {
    PropPanel::IconId getForType(int type) const;
    PropPanel::IconId getSelectionIcon(OutlinerSelectionState selection_state) const;
    PropPanel::IconId getVisibilityIcon(bool visible) const;
    PropPanel::IconId getLockIcon(bool locked) const;

    PropPanel::IconId settings = PropPanel::IconId::Invalid;
    PropPanel::IconId settingsOpen = PropPanel::IconId::Invalid;
    PropPanel::IconId search = PropPanel::IconId::Invalid;
    PropPanel::IconId typeEntity = PropPanel::IconId::Invalid;
    PropPanel::IconId typeSpline = PropPanel::IconId::Invalid;
    PropPanel::IconId typePolygon = PropPanel::IconId::Invalid;
    PropPanel::IconId layerDefault = PropPanel::IconId::Invalid;
    PropPanel::IconId layerActive = PropPanel::IconId::Invalid;
    PropPanel::IconId layerActiveLocked = PropPanel::IconId::Invalid;
    PropPanel::IconId layerLocked = PropPanel::IconId::Invalid;
    PropPanel::IconId layerAdd = PropPanel::IconId::Invalid;
    PropPanel::IconId layerCreate = PropPanel::IconId::Invalid;
    PropPanel::IconId selectAll = PropPanel::IconId::Invalid;
    PropPanel::IconId selectPartial = PropPanel::IconId::Invalid;
    PropPanel::IconId selectNone = PropPanel::IconId::Invalid;
    PropPanel::IconId visibilityVisible = PropPanel::IconId::Invalid;
    PropPanel::IconId visibilityHidden = PropPanel::IconId::Invalid;
    PropPanel::IconId lockOpen = PropPanel::IconId::Invalid;
    PropPanel::IconId lockClosed = PropPanel::IconId::Invalid;
    PropPanel::IconId applyToMask = PropPanel::IconId::Invalid;
    PropPanel::IconId applyToMaskDisabled = PropPanel::IconId::Invalid;
    PropPanel::IconId exportLayer = PropPanel::IconId::Invalid;
    PropPanel::IconId exportLayerDisabled = PropPanel::IconId::Invalid;
    PropPanel::IconId alert = PropPanel::IconId::Invalid;
    PropPanel::IconId close = PropPanel::IconId::Invalid;
  };

  enum class MenuItemId
  {
    SelectAllTypeObjects = 1,
    DeselectAllTypeObjects,
    MoveToThisLayer,
    SelectAllLayerObjects,
    DeselectAllLayerObjects,
    ExpandLayerChildren,
    CollapseLayerChildren,
    AddNewLayer,
    RenameLayer,
    MoveObjectToLayerSubMenu,
    MoveObjectToLayerStart = MoveObjectToLayerSubMenu + 1, // Inclusive
    MoveObjectToLayerEnd = MoveObjectToLayerStart + 64,    // Not inclusive.
    RenameObject,
    ChangeObjectAsset,
    DeleteObject,
  };

  enum class EnsureVisibleRequestState
  {
    NoRequest,
    FoundSelection,
    Requested,
  };

  int onMenuItemClick(unsigned id) override;
  void onImguiDelayedCallback(void *user_data) override;

  PropPanel::IconId getObjectAssetTypeIcon(RenderableEditableObject &object);
  void handleDragAndDropDropping(int type, int per_type_layer_index);
  bool showTypeControls(ObjectTypeTreeItem &tree_item, int type, bool type_visible, bool type_locked, bool dim_type_color,
    const ImVec4 &dimmed_text_color, float action_buttons_total_width);
  bool showLayerControls(LayerTreeItem &tree_item, int type, int per_type_layer_index, bool layer_visible, bool layer_locked,
    bool dim_layer_color, const ImVec4 &dimmed_text_color, float action_buttons_total_width, ImGuiMultiSelectIO *multiSelectIo);
  const char *getObjectNoun(int type, int count) const;
  bool showObjectControls(ObjectTreeItem &tree_item, int type, int per_type_layer_index, bool has_child);
  bool showObjectAssetNameControls(ObjectAssetNameTreeItem &tree_item, RenderableEditableObject &object);

  void fillTypeContextMenu(int type);
  void fillLayerContextMenu(int type, int per_type_layer_index);
  void fillObjectContextMenu(int type, int per_type_layer_index);
  void createContextMenu(int type, int per_type_layer_index = -1, bool object_menu = false);
  void createContextMenuByKeyboard();
  LayerTreeItem *getContextMenuTargetLayer() const;
  void resetContextMenu(PropPanel::IMenu *context_menu = nullptr, int type = -1, int per_type_layer_index = -1);
  void updateSelectionHead(OutlinerTreeItem &tree_item);
  void fillTree(ImGuiMultiSelectIO *multiSelectIo);
  bool applyRangeSelectionRequestInternal(const ImGuiSelectionRequest &request, OutlinerTreeItem &tree_item, bool &found_first);
  void applyRangeSelectionRequest(const ImGuiSelectionRequest &request);
  void applySelectionRequests(const ImGuiMultiSelectIO &multi_select_io, bool &started_selecting);
  void showAddLayerControls();
  void showSettingsPanel(const char *popup_id);

  IOutliner *treeInterface = nullptr;
  Icons icons;

  // Add layer.
  int addingLayerToType = -1;
  String addLayerName;
  String addLayerErrorMessage;

  // Settings panel.
  bool settingsPanelOpen = false;
  bool showActionButtonVisibility = true;
  bool showActionButtonLock = true;
  bool showActionButtonApplyToMask = true;
  bool showActionButtonExportLayer = true;

  // Context menu.
  eastl::unique_ptr<PropPanel::IMenu> contextMenu;
  int contextMenuType = -1;
  int contextMenuPerTypeLayerIndex = -1;

  eastl::unique_ptr<OutlinerModel> outlinerModel;
  eastl::unique_ptr<TreeItemInlineRenamerControl> layerRenamer;
  eastl::unique_ptr<TreeItemInlineRenamerControl> objectRenamer;
  dag::Vector<eastl::optional<PropPanel::IconId>> assetTypeIcons;
  RenderableEditableObject *changeAssetRequested = nullptr;
  EnsureVisibleRequestState ensureVisibleRequested = EnsureVisibleRequestState::NoRequest;
  const bool searchInputFocusId = false; // Only the address of this member is used.
};

} // namespace Outliner
