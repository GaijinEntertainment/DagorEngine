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
class IAssetBrowserHost;

class AssetBrowser
{
public:
  explicit AssetBrowser(IAssetBrowserHost &host);
  ~AssetBrowser();

  void setAssets(dag::Span<DagorAsset *> assets, DagorAsset *selected_asset);
  int getThumbnailImageSize() const;
  void setThumbnailImageSize(int size);
  void updateImgui();

  PropPanel::IMenu &createContextMenu();

  static constexpr ImGuiKeyChord WINDOW_TOGGLE_HOTKEY = ImGuiMod_Ctrl | ImGuiKey_B;

private:
  struct BrowserItem
  {
    void release();
    void setAsset(DagorAsset &in_asset);
    TEXTUREID getThumbnailTexture();

    DagorAsset *asset = nullptr;
    eastl::optional<TEXTUREID> assetThumbnail;
  };

  // Calculated sizes for layout.
  struct Layout
  {
    void update(int item_count, int image_size, float available_width);

    static constexpr int IMAGE_SIZE_MINIMUM = 16;  // just the image part of the browser item widget
    static constexpr int IMAGE_SIZE_MAXIMUM = 256; // just the image part of the browser item widget
    static constexpr int VERTICAL_SPACE_BETWEEN_IMAGE_AND_LABEL = 3;
    static constexpr int SELECTION_BORDER_SIZE = 12;
    static constexpr int ITEM_SPACING = 6;
    static constexpr int WINDOW_PADDING = 3;

    ImVec2 itemImageSize;
    ImVec2 itemSize;                           // itemImageSize + (SELECTION_BORDER_SIZE * 2)
    ImVec2 itemStep;                           // itemSize + ITEM_SPACING
    int verticalSpaceBetweenImageAndLabel = 0; // VERTICAL_SPACE_BETWEEN_IMAGE_AND_LABEL
    int selectionBorderSize = 0;               // SELECTION_BORDER_SIZE
    int windowPadding = 0;                     // WINDOW_PADDING
    int columnCount = 0;
    int lineCount = 0;
  };

  void clearItems();
  void clearTexturesDrawnThisFrame();
  void applySelectionRequests(ImGuiMultiSelectIO &multi_select_io);
  void drawItemTexture(ImDrawList &draw_list, TEXTUREID texture_id, const ImVec2 &pos);
  void drawItem(BrowserItem &browser_item, const ImVec2 &pos, int item_index);
  void drawItems();
  void showSettingsPanel(const char *popup_id);
  bool isTheSameAssetList(dag::Span<DagorAsset *> assets) const;

  // A combination of ImGui::InvisibleButton and ImGui::Selectable.
  static bool invisibleSelectable(bool selected, ImGuiButtonFlags button_flags, const ImVec2 &size_arg);

  IAssetBrowserHost &assetBrowserHost;
  dag::Vector<BrowserItem> browserItems;
  dag::Vector<TEXTUREID> texturesDrawnThisFrame;
  DagorAsset *selectedAsset = nullptr;
  Layout layout;
  String tempLabel;
  eastl::unique_ptr<PropPanel::ContextMenu> contextMenu;
  float requestedImageSize = 128.0f;
  float zoomWheelAccumulator = 0.0f;
  bool ensureVisibleRequested = false;
  bool ignoreEnsureVisibleRequest = false;
  bool focusItemRequested = false;

  PropPanel::IconId settingsIcon = PropPanel::IconId::Invalid;
  bool settingsPanelOpen = false;
};
