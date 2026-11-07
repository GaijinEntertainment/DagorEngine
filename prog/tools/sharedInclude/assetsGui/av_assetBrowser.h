// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <drv/3d/dag_resId.h>
#include <generic/dag_span.h>
#include <propPanel/propPanel.h>
#include <util/dag_string.h>

#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>
#include <imgui/imgui.h>

namespace PropPanel
{
class ContextMenu;
class IMenu;
} // namespace PropPanel

class DagorAsset;
class DagorAssetFolder;
class DagorAssetMgr;
class IAssetBrowserHost;

class AssetBrowser
{
public:
  explicit AssetBrowser(IAssetBrowserHost &host);
  ~AssetBrowser();

  // folder: the folder that contains the assets. It can be nullptr.
  // sub_folders: the subfolders of the current folder. They are shown only while getShowFolders() is true.
  // asset_mgr: used for finding the parent folders. When it is nullptr, no path is shown.
  void setAssets(dag::Span<DagorAsset *> assets, dag::Span<DagorAssetFolder *> sub_folders, DagorAsset *selected_asset,
    const DagorAssetMgr *asset_mgr, const DagorAssetFolder *folder);

  int getThumbnailImageSize() const;
  void setThumbnailImageSize(int size);
  bool getShowTypesWithoutThumbnailSupport() const { return showTypesWithoutThumbnailSupport; }
  void setShowTypesWithoutThumbnailSupport(bool show);
  bool getShowFolders() const { return showFolders; }
  void setShowFolders(bool show);

  // can_submit_contents: whether the contents of the windows should be drawn. Basically the result of ImGui::Begin().
  // Call updateImgui() even when ImGui::Begin() returns false.
  // This also handles the hotkey of toggling the Asset Browser window, but only while that window is in the focus stack.
  void updateImgui(bool can_submit_contents);
  bool isContextMenuOpen() const;

  PropPanel::IMenu &createContextMenu();

  static constexpr ImGuiKeyChord WINDOW_TOGGLE_HOTKEY = ImGuiMod_Ctrl | ImGuiKey_B;

private:
  struct BrowserItem
  {
    void release();
    TEXTUREID getThumbnailTexture();
    bool isThumbnailFailed() const { return assetThumbnail.has_value() && assetThumbnail.value() == BAD_TEXTUREID; }
    const void *getId() const { return asset ? (const void *)asset : (const void *)folder; }
    const char *getName() const;

    // Either asset or folder is set.
    DagorAsset *asset = nullptr;
    const DagorAssetFolder *folder = nullptr;
    eastl::optional<TEXTUREID> assetThumbnail;
  };

  // Calculated sizes for layout.
  struct Layout
  {
    void update(int item_count, int image_size, float available_width, int label_line_count);

    static int getLabelPadding();
    static float getLabelWidth(int image_size);

    static constexpr int IMAGE_SIZE_MINIMUM = 16;  // just the image part of the browser item widget
    static constexpr int IMAGE_SIZE_MAXIMUM = 256; // just the image part of the browser item widget
    static constexpr int VERTICAL_SPACE_BETWEEN_IMAGE_AND_LABEL = 3;
    static constexpr int LABEL_LINE_COUNT_MAXIMUM = 3;
    static constexpr int SELECTION_BORDER_SIZE = 12;
    static constexpr int ITEM_SPACING = 6;
    static constexpr int WINDOW_PADDING = 3;

    ImVec2 itemImageSize;
    ImVec2 itemSize;                           // itemImageSize + (SELECTION_BORDER_SIZE * 2)
    ImVec2 itemStep;                           // itemSize + ITEM_SPACING
    int verticalSpaceBetweenImageAndLabel = 0; // VERTICAL_SPACE_BETWEEN_IMAGE_AND_LABEL
    int selectionBorderSize = 0;               // SELECTION_BORDER_SIZE
    int windowPadding = 0;                     // WINDOW_PADDING
    int labelPadding = 0;                      // getLabelPadding()
    float labelWidth = 0.0f;                   // getLabelWidth()
    int labelLineCount = 1;
    int columnCount = 0;
    int lineCount = 0;
  };

  void clearItems();
  void clearTexturesDrawnThisFrame();
  void applySelectionRequests(ImGuiMultiSelectIO &multi_select_io);
  void drawItemTexture(ImDrawList &draw_list, TEXTUREID texture_id, const ImVec2 &pos);
  void drawItemIcon(ImDrawList &draw_list, PropPanel::IconId icon_id, const ImVec2 &pos);
  void drawLoadingIndicator(ImDrawList &draw_list, const ImVec2 &pos);

  // Returns the folder that the user activated, or nullptr.
  const DagorAssetFolder *drawItem(BrowserItem &browser_item, const ImVec2 &pos, int item_index);

  void drawItems();
  void drawFolderPath(float available_width);
  void updateLabelLineCount(float label_width);
  void showSettingsPanel(const char *popup_id);
  bool isTheSameItemList(dag::ConstSpan<BrowserItem> items) const;
  bool isSelected(const BrowserItem &browser_item) const;
  bool canShowAsset(const DagorAsset &asset) const;
  void updateShownItems();
  void setShownItemsOption(bool &option, bool value);

  // A combination of ImGui::InvisibleButton and ImGui::Selectable.
  static bool invisibleSelectable(bool selected, ImGuiButtonFlags button_flags, const ImVec2 &size_arg);

  IAssetBrowserHost &assetBrowserHost;
  dag::Vector<DagorAsset *> allAssets;                 // the assets from setAssets(), also the ones that are not shown
  dag::Vector<const DagorAssetFolder *> allSubFolders; // the subfolders from setAssets(), also when they are not shown
  dag::Vector<BrowserItem> browserItems;
  dag::Vector<const DagorAssetFolder *> folderPath; // from the root folder to the shown folder
  dag::Vector<TEXTUREID> texturesDrawnThisFrame;
  DagorAsset *selectedAsset = nullptr;
  const DagorAssetFolder *selectedFolder = nullptr; // only selected in the browser, not in the host
  Layout layout;
  String tempLabel;
  eastl::unique_ptr<PropPanel::ContextMenu> contextMenu;
  float requestedImageSize = 128.0f;
  float zoomWheelAccumulator = 0.0f;

  // The label line count of the item that needs the most lines, cached for these label width and font size values.
  int labelLineCount = 1;
  float labelLineCountLabelWidth = -1.0f;
  float labelLineCountFontSize = -1.0f;

  unsigned d3dResetCounter = 0;
  bool ensureVisibleRequested = false;
  bool ignoreEnsureVisibleRequest = false;
  bool focusItemRequested = false;
  bool showTypesWithoutThumbnailSupport = false;
  bool showFolders = false;

  PropPanel::IconId settingsIcon = PropPanel::IconId::Invalid;
  PropPanel::IconId folderIcon = PropPanel::IconId::Invalid;
  bool settingsPanelOpen = false;
};
