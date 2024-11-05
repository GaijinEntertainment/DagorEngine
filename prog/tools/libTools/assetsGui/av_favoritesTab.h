// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "av_favoritesTree.h"
#include <propPanel/focusHelper.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/propPanel.h>
#include <imgui/imgui.h>

class FavoritesTab
{
public:
  explicit FavoritesTab(SelectAssetDlg &select_asset_dlg) : favoritesTree(select_asset_dlg)
  {
    closeIcon = (ImTextureID)((unsigned)PropPanel::load_icon("close_editor"));
    searchIcon = (ImTextureID)((unsigned)PropPanel::load_icon("search"));
    settingsIcon = (ImTextureID)((unsigned)PropPanel::load_icon("filter_default"));
    settingsOpenIcon = (ImTextureID)((unsigned)PropPanel::load_icon("filter_active"));
  }

  void fillTree() { favoritesTree.fillTree(); }

  DagorAsset *getSelectedAsset() { return favoritesTree.getSelectedAsset(); }

  void updateImgui()
  {
    const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
    const ImVec2 settingsButtonSize = PropPanel::ImguiHelper::getImageButtonWithDownArrowSize(fontSizedIconSize);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (ImGui::GetStyle().ItemSpacing.x + settingsButtonSize.x));
    if (PropPanel::ImguiHelper::searchInput(&searchInputFocusId, "##searchInput", "Search", favoritesTree.getTextToSearch(),
          searchIcon, closeIcon))
      favoritesTree.fillTree();
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

    if (favoritesTree.getShowHierarchy())
    {
      const char *expandAllTitle = "Expand all";
      const char *collapseAllTitle = "Collapse all";
      const ImVec2 expandAllSize = PropPanel::ImguiHelper::getButtonSize(expandAllTitle);
      const ImVec2 collapseAllSize = PropPanel::ImguiHelper::getButtonSize(collapseAllTitle);
      const ImVec2 expandButtonRegionAvailable = ImGui::GetContentRegionAvail();
      const ImVec2 expandButtonCursorPos = ImGui::GetCursorPos();

      ImGui::SetCursorPosX(expandButtonCursorPos.x + expandButtonRegionAvailable.x -
                           (expandAllSize.x + ImGui::GetStyle().ItemSpacing.x + collapseAllSize.x));
      if (ImGui::Button(expandAllTitle))
        favoritesTree.expandAll(true);

      ImGui::SameLine();
      ImGui::SetCursorPosX(expandButtonCursorPos.x + expandButtonRegionAvailable.x - collapseAllSize.x);
      if (ImGui::Button(collapseAllTitle))
        favoritesTree.expandAll(false);
    }

    if (!settingsPanelOpen && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_F))
      PropPanel::focus_helper.requestFocus(&searchInputFocusId);

    favoritesTree.updateImgui();
  }

private:
  void showSettingsPanel(const char *popup_id)
  {
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
    const bool popupIsOpen = ImGui::BeginPopup(popup_id);
    ImGui::PopStyleColor();

    if (!popupIsOpen)
    {
      settingsPanelOpen = false;
      return;
    }

    bool showHierarchy = favoritesTree.getShowHierarchy();
    const bool showHierarchyPressed = ImGui::Checkbox("Show hierarchy", &showHierarchy);
    PropPanel::set_previous_imgui_control_tooltip((const void *)ImGui::GetItemID(),
      "Show the favorite assets including their hierarchy.");

    if (showHierarchyPressed)
      favoritesTree.setShowHierarchy(showHierarchy);

    ImGui::EndPopup();
  }

  FavoritesTree favoritesTree;
  String textToSearch;
  ImTextureID closeIcon = nullptr;
  ImTextureID searchIcon = nullptr;
  ImTextureID settingsIcon = nullptr;
  ImTextureID settingsOpenIcon = nullptr;
  const bool searchInputFocusId = false; // Only the address of this member is used.
  bool settingsPanelOpen = false;
};