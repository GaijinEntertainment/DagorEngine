// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

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
  }

  void fillTree()
  {
    dag::ConstSpan<int> allowedTypes = selectAssetDlg.getAllowedTypes();
    const DagorAsset *selectedAsset = getSelectedAsset();
    PropPanel::TLeafHandle itemToSelect = nullptr;

    recentlyUsedTree.clear();

    String imageName;
    const dag::Vector<String> &recentlyUsedList = AssetSelectorGlobalState::getRecentlyUsed();
    for (int i = recentlyUsedList.size() - 1; i >= 0; --i)
    {
      const String &assetName = recentlyUsedList[i];
      const DagorAsset *asset = selectAssetDlg.getAssetByName(assetName, allowedTypes);

      if (!asset || eastl::find(allowedTypes.begin(), allowedTypes.end(), asset->getType()) == allowedTypes.end())
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
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (PropPanel::ImguiHelper::searchInput(&searchInputFocusId, "##searchInput", "Search", textToSearch, searchIcon, closeIcon))
      fillTree();

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_F))
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

  SelectAssetDlg &selectAssetDlg;
  PropPanel::TreeBaseWindow recentlyUsedTree;
  String textToSearch;
  ImTextureID closeIcon = nullptr;
  ImTextureID searchIcon = nullptr;
  const bool searchInputFocusId = false; // Only the address of this member is used.
};