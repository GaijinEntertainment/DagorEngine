// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assets/asset.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <assetsGui/av_assetTypeFilterButtonGroupControl.h>
#include <assetsGui/av_assetTypeFilterControl.h>
#include <assetsGui/av_client.h>
#include <assetsGui/av_globalState.h>
#include <assetsGui/av_ids.h>
#include <assetsGui/av_textFilter.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/treeInterface.h>
#include <propPanel/focusHelper.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/propPanel.h>
#include <imgui/imgui.h>

class RecentlyUsedTab : public IMenuEventHandler, public PropPanel::ITreeViewEventHandler
{
public:
  explicit RecentlyUsedTab(IAssetSelectorFavoritesRecentlyUsedHost &tab_host, const DagorAssetMgr &asset_mgr) :
    tabHost(tab_host),
    assetMgr(asset_mgr),
    recentlyUsedTree(this, nullptr, 0, 0, hdpi::_pxActual(0), hdpi::_pxActual(0), "", /*icons_show = */ true)
  {
    closeIcon = (ImTextureID)((unsigned)PropPanel::load_icon("close_editor"));
    searchIcon = (ImTextureID)((unsigned)PropPanel::load_icon("search"));
    settingsIcon = (ImTextureID)((unsigned)PropPanel::load_icon("filter_default"));
    settingsOpenIcon = (ImTextureID)((unsigned)PropPanel::load_icon("filter_active"));
  }

  void onAllowedTypesChanged(dag::ConstSpan<int> allowed_type_indexes)
  {
    const int assetTypeCount = assetMgr.getAssetTypesCount();

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
    const dag::ConstSpan<int> allowedTypeIndexes = tabHost.getAllowedTypes();
    const DagorAsset *selectedAsset = getSelectedAsset();
    PropPanel::TLeafHandle itemToSelect = nullptr;
    int assetsMatchingAllowedTypes = 0;

    recentlyUsedTree.clear();

    String imageName;
    const dag::Vector<String> &recentlyUsedList = AssetSelectorGlobalState::getRecentlyUsed();
    for (int i = recentlyUsedList.size() - 1; i >= 0; --i)
    {
      const String &assetName = recentlyUsedList[i];
      const DagorAsset *asset = AssetSelectorCommon::getAssetByName(assetMgr, assetName, allowedTypeIndexes);
      if (!asset)
        continue;

      ++assetsMatchingAllowedTypes;

      if (!shownTypes[asset->getType()])
        continue;

      if (!textFilter.matchesFilter(asset->getName()))
        continue;

      const TEXTUREID icon = AssetSelectorCommon::getAssetTypeIcon(asset->getType());
      PropPanel::TLeafHandle newItem = recentlyUsedTree.addItem(assetName, icon, nullptr, const_cast<DagorAsset *>(asset));
      if (asset == selectedAsset)
        itemToSelect = newItem;
    }

    if (assetsMatchingAllowedTypes == 0)
      recentlyUsedTree.setMessage("No recently used");
    else if (!recentlyUsedTree.getRoot())
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
    ImGui::PushID(this);

    const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
    const ImVec2 settingsButtonSize = PropPanel::ImguiHelper::getImageButtonWithDownArrowSize(fontSizedIconSize);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (ImGui::GetStyle().ItemSpacing.x + settingsButtonSize.x));

    bool inputFocused;
    ImGuiID inputId;
    const bool searchInputChanged = PropPanel::ImguiHelper::searchInput(&searchInputFocusId, "##searchInput", "Filter and search",
      textToSearch, searchIcon, closeIcon, &inputFocused, &inputId);

    PropPanel::set_previous_imgui_control_tooltip((const void *)inputId, AssetSelectorCommon::searchTooltip);

    if (inputFocused)
    {
      if (ImGui::Shortcut(ImGuiKey_Enter, ImGuiInputFlags_None, inputId) ||
          ImGui::Shortcut(ImGuiKey_DownArrow, ImGuiInputFlags_None, inputId))
      {
        // The ImGui text input control reacts to Enter by making it inactive. Keep it active.
        ImGui::SetActiveID(inputId, ImGui::GetCurrentWindow());

        selectNextItem();
      }
      else if (ImGui::Shortcut(ImGuiMod_Shift | ImGuiKey_Enter, ImGuiInputFlags_None, inputId) ||
               ImGui::Shortcut(ImGuiKey_UpArrow, ImGuiInputFlags_None, inputId))
      {
        ImGui::SetActiveID(inputId, ImGui::GetCurrentWindow());
        selectNextItem(/*forward = */ false);
      }
    }

    if (searchInputChanged)
    {
      textFilter.setSearchText(textToSearch);
      fillTree();
    }

    ImGui::SameLine();
    const char *popupId = "settingsPopup";
    const ImTextureID settingsButtonIcon = settingsPanelOpen ? settingsOpenIcon : settingsIcon;
    bool settingsButtonPressed =
      PropPanel::ImguiHelper::imageButtonWithArrow("settingsButton", settingsButtonIcon, fontSizedIconSize, settingsPanelOpen);
    PropPanel::set_previous_imgui_control_tooltip((const void *)ImGui::GetItemID(), "Settings");

    if (settingsPanelOpen)
      showSettingsPanel(popupId);

    if (AssetTypeFilterButtonGroupControl::updateImgui(assetMgr, allowedTypes, make_span(shownTypes), settingsButtonPressed))
      onShownTypeFilterChanged();

    if (settingsButtonPressed)
    {
      ImGui::OpenPopup(popupId);
      settingsPanelOpen = true;
    }

    if (!settingsPanelOpen && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_F))
      PropPanel::focus_helper.requestFocus(&searchInputFocusId);

    recentlyUsedTree.updateImgui();

    ImGui::PopID();
  }

private:
  virtual int onMenuItemClick(unsigned id) override
  {
    switch (id)
    {
      case AssetsGuiIds::AddToFavoritesMenuItem:
        if (DagorAsset *asset = getSelectedAsset())
          tabHost.addAssetToFavorites(*asset);
        return 1;

      case AssetsGuiIds::GoToAssetInSelectorMenuItem:
        if (DagorAsset *asset = getSelectedAsset())
          tabHost.goToAsset(*asset);
        return 1;

      case AssetsGuiIds::CopyAssetFilePathMenuItem:
        if (const DagorAsset *asset = getSelectedAsset())
          AssetSelectorCommon::copyAssetFilePathToClipboard(*asset);
        return 1;

      case AssetsGuiIds::CopyAssetFolderPathMenuItem:
        if (const DagorAsset *asset = getSelectedAsset())
          AssetSelectorCommon::copyAssetFolderPathToClipboard(*asset);
        return 1;

      case AssetsGuiIds::CopyAssetNameMenuItem:
        if (const DagorAsset *asset = getSelectedAsset())
          AssetSelectorCommon::copyAssetNameToClipboard(*asset);
        return 1;

      case AssetsGuiIds::RevealInExplorerMenuItem:
        if (const DagorAsset *asset = getSelectedAsset())
          AssetSelectorCommon::revealInExplorer(*asset);
        return 1;
    }

    return 0;
  }

  virtual void onTvSelectionChange(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::TLeafHandle new_sel) override
  {
    tabHost.onSelectionChanged(getSelectedAsset());
  }

  virtual void onTvDClick(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::TLeafHandle node) override
  {
    tabHost.onSelectionDoubleClicked(getSelectedAsset());
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

  void selectNextItem(bool forward = true)
  {
    PropPanel::TLeafHandle next = recentlyUsedTree.getSelectedItem();
    if (!next)
    {
      next = forward ? recentlyUsedTree.getRoot() : recentlyUsedTree.getNextNode(recentlyUsedTree.getRoot(), false);
      if (next)
        recentlyUsedTree.setSelectedItem(next);

      return;
    }

    next = recentlyUsedTree.getNextNode(next, forward);
    if (next)
      recentlyUsedTree.setSelectedItem(next);
  }

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

    if (assetTypeFilterControl.updateImgui(assetMgr, make_span(allowedTypes), make_span(shownTypes)))
      onShownTypeFilterChanged();

    ImGui::EndPopup();
  }

  IAssetSelectorFavoritesRecentlyUsedHost &tabHost;
  const DagorAssetMgr &assetMgr;
  PropPanel::TreeBaseWindow recentlyUsedTree;
  AssetTypeFilterControl assetTypeFilterControl;
  dag::Vector<bool> allowedTypes;
  dag::Vector<bool> shownTypes;
  AssetsGuiTextFilter textFilter;
  String textToSearch;
  ImTextureID closeIcon = nullptr;
  ImTextureID searchIcon = nullptr;
  ImTextureID settingsIcon = nullptr;
  ImTextureID settingsOpenIcon = nullptr;
  const bool searchInputFocusId = false; // Only the address of this member is used.
  bool settingsPanelOpen = false;
};