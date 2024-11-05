// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_outlinerInterface.h>
#include <drv/3d/dag_resId.h>
#include <propPanel/messageQueue.h>
#include <sepGui/wndMenuInterface.h>
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

class OutlinerWindow : public IMenuEventHandler, public PropPanel::IDelayedCallbackHandler
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
    ImTextureID getForType(int type) const;
    ImTextureID getSelectionIcon(OutlinerSelectionState selection_state) const;
    ImTextureID getVisibilityIcon(bool visible) const;
    ImTextureID getLockIcon(bool locked) const;

    ImTextureID settings = 0;
    ImTextureID settingsOpen = 0;
    ImTextureID search = 0;
    ImTextureID typeEntity = 0;
    ImTextureID typeSpline = 0;
    ImTextureID typePolygon = 0;
    ImTextureID layerDefault = 0;
    ImTextureID layerActive = 0;
    ImTextureID layerLocked = 0;
    ImTextureID layerAdd = 0;
    ImTextureID layerCreate = 0;
    ImTextureID selectAll = 0;
    ImTextureID selectPartial = 0;
    ImTextureID selectNone = 0;
    ImTextureID visibilityVisible = 0;
    ImTextureID visibilityHidden = 0;
    ImTextureID lockOpen = 0;
    ImTextureID lockClosed = 0;
    ImTextureID applyToMask = 0;
    ImTextureID applyToMaskDisabled = 0;
    ImTextureID exportLayer = 0;
    ImTextureID exportLayerDisabled = 0;
    ImTextureID alert = 0;
    ImTextureID close = 0;
  };

  enum class MenuItemId
  {
    SelectAllTypeObjects = 1,
    DeselectAllTypeObjects,
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

  virtual int onMenuItemClick(unsigned id) override;
  virtual void onImguiDelayedCallback(void *user_data) override;

  ImTextureID getObjectAssetTypeIcon(RenderableEditableObject &object);
  void handleDragAndDropDropping(int type, int per_type_layer_index);
  bool showTypeControls(ObjectTypeTreeItem &tree_item, int type, bool type_visible, bool type_locked, bool dim_type_color,
    int layer_count, const ImVec4 &dimmed_text_color, float action_buttons_total_width);
  bool showLayerControls(LayerTreeItem &tree_item, int type, int per_type_layer_index, bool layer_visible, bool layer_locked,
    bool dim_layer_color, int object_count, const ImVec4 &dimmed_text_color, float action_buttons_total_width);
  const char *getObjectNoun(int type, int count) const;
  bool showObjectControls(ObjectTreeItem &tree_item, int type, int per_type_layer_index, int object_index, bool has_child);
  bool showObjectAssetNameControls(ObjectAssetNameTreeItem &tree_item, RenderableEditableObject &object);

  void fillTypeContextMenu(int type);
  void fillLayerContextMenu(int type, int per_type_layer_index);
  void fillObjectContextMenu(int type, int per_type_layer_index);
  void createContextMenu(int type, int per_type_layer_index = -1, bool object_menu = false);
  void createContextMenuByKeyboard();
  void updateSelectionHead(OutlinerTreeItem &tree_item);
  void fillTree();
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
  dag::Vector<bool> showTypes;

  eastl::unique_ptr<PropPanel::IMenu> contextMenu;
  eastl::unique_ptr<OutlinerModel> outlinerModel;
  eastl::unique_ptr<TreeItemInlineRenamerControl> layerRenamer;
  eastl::unique_ptr<TreeItemInlineRenamerControl> objectRenamer;
  dag::Vector<eastl::optional<ImTextureID>> assetTypeIcons;
  RenderableEditableObject *changeAssetRequested = nullptr;
  EnsureVisibleRequestState ensureVisibleRequested = EnsureVisibleRequestState::NoRequest;
  const bool searchInputFocusId = false; // Only the address of this member is used.
};

} // namespace Outliner
