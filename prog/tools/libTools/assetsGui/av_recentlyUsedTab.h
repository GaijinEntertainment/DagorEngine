// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "av_assetTypeFilterControl.h"
#include <propPanel/commonWindow/treeviewPanel.h>
#include <propPanel/focusHelper.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/propPanel.h>
#include <imgui/imgui.h>

class RecentlyUsedTab : public IMenuEventHandler, public PropPanel::ITreeViewEventHandler
{
public:
  explicit RecentlyUsedTab(SelectAssetDlg &select_asset_dlg) :
    selectAssetDlg(select_asset_dlg),
    recentlyUsedTree(this, nullptr, 0, 0, hdpi::_pxActual(0), hdpi::_pxActual(0), "", /*icons_show = */ true)
  {
    closeIcon = (ImTextureID)((unsigned)PropPanel::load_icon("close_editor"));
    searchIcon = (ImTextureID)((unsigned)PropPanel::load_icon("search"));
    settingsIcon = (ImTextureID)((unsigned)PropPanel::load_icon("filter_default"));
    settingsOpenIcon = (ImTextureID)((unsigned)PropPanel::load_icon("filter_active"));
  }

  void onAllowedTypesChanged(dag::ConstSpan<int> allowed_type_indexes)
  {
    const int assetTypeCount = selectAssetDlg.getAssetMgr()->getAssetTypesCount();

    allowedTypes.clear();
    shownTypes.clear();
    allowedTypes.resize(assetTypeCount);
    shownTypes.resize(assetTypeCount);

    for (int i = 0; i < assetTypeCount; ++i)
    {
      const bool allowed = eastl::find(allowed_type_indexes.begin(), allowed_type_indexes.end(), i) != allowed_type_indexes.end();
      allowedTypes[i] = shownTypes[i] = allowed;
    }

    onShownTypeFilterChanged();
  }

  void fillTree()
  {
    const dag::ConstSpan<int> allowedTypeIndexes = selectAssetDlg.getAllowedTypes();
    const DagorAsset *selectedAsset = getSelectedAsset();
    PropPanel::TLeafHandle itemToSelect = nullptr;

    recentlyUsedTree.clear();

    String imageName;
    const dag::Vector<String> &recentlyUsedList = AssetSelectorGlobalState::getRecentlyUsed();
    for (int i = recentlyUsedList.size() - 1; i >= 0; --i)
    {
      const String &assetName = recentlyUsedList[i];
      const DagorAsset *asset = selectAssetDlg.getAssetByName(assetName, allowedTypeIndexes);

      if (!asset || !shownTypes[asset->getType()])
        continue;

      if (!SelectAssetDlg::matchesSearchText(asset->getName(), textToSearch))
        continue;

      selectAssetDlg.getAssetImageName(imageName, asset);
      PropPanel::TLeafHandle newItem = recentlyUsedTree.addItem(assetName, imageName, nullptr, const_cast<DagorAsset *>(asset));
      if (asset == selectedAsset)
        itemToSelect = newItem;
    }

    if (recentlyUsedList.empty())
      recentlyUsedTree.setMessage("No recently used");
    else if (!textToSearch.empty() && !recentlyUsedTree.getRoot())
      recentlyUsedTree.setMessage("No results found");
    else
      recentlyUsedTree.setMessage("");

    if (itemToSelect)
      recentlyUsedTree.setSelectedItem(itemToSelect);
  }

  DagorAsset *getSelectedAsset()
  {
    const PropPanel::TLeafHandle selectedNode = recentlyUsedTree.getSelectedItem();
    return reinterpret_cast<DagorAsset *>(recentlyUsedTree.getItemData(selectedNode));
  }

  void updateImgui()
  {
    const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
    const ImVec2 settingsButtonSize = PropPanel::ImguiHelper::getImageButtonWithDownArrowSize(fontSizedIconSize);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (ImGui::GetStyle().ItemSpacing.x + settingsButtonSize.x));
    if (PropPanel::ImguiHelper::searchInput(&searchInputFocusId, "##searchInput", "Search", textToSearch, searchIcon, closeIcon))
      fillTree();
    ImGui::SameLine();

    const char *popupId = "settingsPopup";
    const ImTextureID settingsButtonIcon = settingsPanelOpen ? settingsOpenIcon : settingsIcon;
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

    if (!settingsPanelOpen && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_F))
      PropPanel::focus_helper.requestFocus(&searchInputFocusId);

    recentlyUsedTree.updateImgui();
  }

private:
  virtual int onMenuItemClick(unsigned id) override
  {
    switch (id)
    {
      case AssetsGuiIds::AddToFavoritesMenuItem:
        if (DagorAsset *asset = getSelectedAsset())
          selectAssetDlg.addAssetToFavorites(*asset);
        return 1;

      case AssetsGuiIds::GoToAssetInSelectorMenuItem:
        if (DagorAsset *asset = getSelectedAsset())
          selectAssetDlg.goToAsset(*asset);
        return 1;

      case AssetsGuiIds::CopyAssetFilePathMenuItem:
        if (const DagorAsset *asset = getSelectedAsset())
          SelectAssetDlg::copyAssetFilePathToClipboard(*asset);
        return 1;

      case AssetsGuiIds::CopyAssetFolderPathMenuItem:
        if (const DagorAsset *asset = getSelectedAsset())
          SelectAssetDlg::copyAssetFolderPathToClipboard(*asset);
        return 1;

      case AssetsGuiIds::CopyAssetNameMenuItem:
        if (const DagorAsset *asset = getSelectedAsset())
          SelectAssetDlg::copyAssetNameToClipboard(*asset);
        return 1;

      case AssetsGuiIds::RevealInExplorerMenuItem:
        if (const DagorAsset *asset = getSelectedAsset())
          SelectAssetDlg::revealInExplorer(*asset);
        return 1;
    }

    return 0;
  }

  virtual void onTvSelectionChange(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::TLeafHandle new_sel) override
  {
    selectAssetDlg.onRecentlyUsedSelectionChanged(getSelectedAsset());
  }

  virtual void onTvDClick(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::TLeafHandle node) override
  {
    selectAssetDlg.onRecentlyUsedSelectionDoubleClicked(getSelectedAsset());
  }

  virtual bool onTvContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &in_tree) override
  {
    if (getSelectedAsset())
    {
      PropPanel::IMenu &menu = in_tree.createContextMenu();
      menu.setEventHandler(this);
      menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::AddToFavoritesMenuItem, "Add to favorites");
      menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::GoToAssetInSelectorMenuItem, "Go to asset");
      AssetBaseView::addCommonMenuItems(menu);
      return true;
    }

    return false;
  }

  void onShownTypeFilterChanged() { fillTree(); }

  void showSettingsPanel(const char *popup_id)
  {
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
    const bool popupIsOpen = ImGui::BeginPopup(popup_id, ImGuiWindowFlags_NoMove);
    ImGui::PopStyleColor();

    if (!popupIsOpen)
    {
      settingsPanelOpen = false;
      return;
    }

    ImGui::TextUnformatted("Select filter");

    if (assetTypeFilterControl.updateImgui(*selectAssetDlg.getAssetMgr(), make_span(allowedTypes), make_span(shownTypes)))
      onShownTypeFilterChanged();

    ImGui::EndPopup();
  }

  SelectAssetDlg &selectAssetDlg;
  PropPanel::TreeBaseWindow recentlyUsedTree;
  AssetTypeFilterControl assetTypeFilterControl;
  dag::Vector<bool> allowedTypes;
  dag::Vector<bool> shownTypes;
  String textToSearch;
  ImTextureID closeIcon = nullptr;
  ImTextureID searchIcon = nullptr;
  ImTextureID settingsIcon = nullptr;
  ImTextureID settingsOpenIcon = nullptr;
  const bool searchInputFocusId = false; // Only the address of this member is used.
  bool settingsPanelOpen = false;
};