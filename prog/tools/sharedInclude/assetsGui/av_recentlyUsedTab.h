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

class RecentlyUsedTab : public PropPanel::IMenuEventHandler, public PropPanel::ITreeViewEventHandler
{
public:
  explicit RecentlyUsedTab(IAssetSelectorFavoritesRecentlyUsedHost &tab_host, const DagorAssetMgr &asset_mgr) :
    tabHost(tab_host),
    assetMgr(asset_mgr),
    recentlyUsedTree(this, nullptr, 0, 0, hdpi::_pxActual(0), hdpi::_pxActual(0), "", /*icons_show = */ true)
  {
    closeIcon = PropPanel::load_icon("close_editor");
    searchIcon = PropPanel::load_icon("search");
    settingsIcon = PropPanel::load_icon("filter_default");
    settingsOpenIcon = PropPanel::load_icon("filter_active");
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

  void fillTree(const DagorAsset *asset_to_select)
  {
    const dag::ConstSpan<int> allowedTypeIndexes = tabHost.getAllowedTypes();
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

      const PropPanel::IconId icon = AssetSelectorCommon::getAssetTypeIcon(asset->getType());
      PropPanel::TLeafHandle newItem = recentlyUsedTree.addItem(assetName, icon, nullptr, const_cast<DagorAsset *>(asset));
      if (asset == asset_to_select)
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

  void setSelectedAsset(const DagorAsset *asset)
  {
    PropPanel::TLeafHandle treeItem = getTreeItemByItemData(asset);
    recentlyUsedTree.setSelectedItem(treeItem);
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

    PropPanel::set_previous_imgui_control_tooltip((const void *)((uintptr_t)inputId), AssetSelectorCommon::searchTooltip);

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
      fillTree(getSelectedAsset());
    }

    ImGui::SameLine();
    const char *popupId = "settingsPopup";
    const PropPanel::IconId settingsButtonIcon = settingsPanelOpen ? settingsOpenIcon : settingsIcon;
    bool settingsButtonPressed =
      PropPanel::ImguiHelper::imageButtonWithArrow("settingsButton", settingsButtonIcon, fontSizedIconSize, settingsPanelOpen);
    PropPanel::set_previous_imgui_control_tooltip((const void *)((uintptr_t)ImGui::GetItemID()), "Settings");

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
  int onMenuItemClick(unsigned id) override
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

  void onTvSelectionChange(PropPanel::TreeBaseWindow &, PropPanel::TLeafHandle) override
  {
    tabHost.onSelectionChanged(getSelectedAsset());
  }

  void onTvDClick(PropPanel::TreeBaseWindow &, PropPanel::TLeafHandle) override
  {
    tabHost.onSelectionDoubleClicked(getSelectedAsset());
  }

  bool onTvContextMenu(PropPanel::TreeBaseWindow &, PropPanel::ITreeInterface &in_tree) override
  {
    using PropPanel::ROOT_MENU_ITEM;
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

  void onShownTypeFilterChanged() { fillTree(getSelectedAsset()); }

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

  PropPanel::TLeafHandle getTreeItemByItemData(const void *item_data)
  {
    const int childrenCount = recentlyUsedTree.getChildrenCount(nullptr);
    for (int i = 0; i < childrenCount; ++i)
    {
      PropPanel::TLeafHandle childItem = recentlyUsedTree.getChild(nullptr, i);
      G_ASSERT(recentlyUsedTree.getChildrenCount(childItem) == 0);
      if (recentlyUsedTree.getItemData(childItem) == item_data)
        return childItem;
    }

    return nullptr;
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
  PropPanel::IconId closeIcon = PropPanel::IconId::Invalid;
  PropPanel::IconId searchIcon = PropPanel::IconId::Invalid;
  PropPanel::IconId settingsIcon = PropPanel::IconId::Invalid;
  PropPanel::IconId settingsOpenIcon = PropPanel::IconId::Invalid;
  const bool searchInputFocusId = false; // Only the address of this member is used.
  bool settingsPanelOpen = false;
};