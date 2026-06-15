// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_assetSearchResultsWindow.h"

#include "av_appwnd.h"

#include <assets/asset.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <assetsGui/av_globalState.h>
#include <assetsGui/av_ids.h>
#include <assetsGui/av_view.h>
#include <de3_interface.h>
#include <osApiWrappers/dag_clipboard.h>
#include <propPanel/imguiHelper.h>
#include <winGuiWrapper/wgw_dialogs.h>

AssetSearchResultsWindow::AssetSearchResultsWindow(const char *caption) : DialogWindow(nullptr, hdpi::Px(200), hdpi::Px(200), caption)
{
  closeIcon = PropPanel::load_icon("close_editor");
  searchIcon = PropPanel::load_icon("search");
  pinIcon = PropPanel::load_icon("pin");
  unpinIcon = PropPanel::load_icon("unpin");

  propertiesPanel->createCustomControlHolder(CustomControlHolderId, this);

  showButtonPanel(false);

  if (!hasEverBeenShown())
    positionBesideWindow("Assets Tree", false);
}

void AssetSearchResultsWindow::fillContextMenu(PropPanel::IMenu &menu,
  const AssetSearchResultsListControl::SearchResult &search_result)
{
  using PropPanel::ROOT_MENU_ITEM;

  if (search_result.asset)
  {
    menu.setEventHandler(this);

    menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::AddToFavoritesMenuItem, "Add to favorites");
    menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::GoToAssetInSelectorMenuItem, "Go to asset");
    AssetBaseView::addCommonMenuItems(menu);
  }
  else
  {
    menu.setEventHandler(this);

    menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::CopyAssetNameMenuItem, "Copy name");
  }
}

int AssetSearchResultsWindow::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case AssetsGuiIds::GoToAssetInSelectorMenuItem:
      if (const DagorAsset *asset = getSelectedAsset())
        get_app().selectAsset(*asset);
      return 1;

    case AssetsGuiIds::AddToFavoritesMenuItem:
      if (const DagorAsset *asset = getSelectedAsset())
        AssetSelectorGlobalState::addFavorite(asset->getNameTypified());
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
      if (const AssetSearchResultsListControl::SearchResult *searchResult = searchResultsList.getSelectedSearchResult())
      {
        if (searchResult->asset)
          AssetSelectorCommon::copyAssetNameToClipboard(*searchResult->asset);
        else
          clipboard::set_clipboard_ansi_text(searchResult->assetText);
      }
      return 1;

    case AssetsGuiIds::RevealInExplorerMenuItem:
      if (const DagorAsset *asset = getSelectedAsset())
        AssetSelectorCommon::revealInExplorer(*asset);
      return 1;
  }

  return 0;
}

void AssetSearchResultsWindow::saveResultsToBlk(DataBlock &blk) const
{
  dag::ConstSpan<SimpleString> columnTitles = searchResultsList.getColumnTitles();
  DataBlock *headerBlk = blk.addNewBlock("header");
  for (const SimpleString &columnTitle : columnTitles)
    headerBlk->addStr("column", columnTitle);

  String tempBuffer;
  for (const AssetSearchResultsListControl::SearchResult &searchResult : searchResultsList.getSearchResults())
  {
    DataBlock *resultBlk = blk.addNewBlock("result");
    resultBlk->addStr("column", searchResult.asset ? searchResult.asset->getNameTypified() : searchResult.assetText.c_str());
    for (int i = 1; i < columnTitles.size(); ++i)
      resultBlk->addStr("column", searchResult.getColumnText(i));
  }
}

void AssetSearchResultsWindow::saveResultsToFile(const char *path) const
{
  DataBlock blk;
  saveResultsToBlk(blk);

  if (!blk.saveToTextFile(path))
    logerr("Saving search results to \"%s\" has failed.", path);
}

void AssetSearchResultsWindow::loadResult(const DataBlock &result_blk, int column_name_id)
{
  AssetSearchResultsListControl::SearchResult searchResult;

  bool firstColumn = true;
  for (int paramIndex = 0; paramIndex < result_blk.paramCount(); ++paramIndex)
  {
    if (result_blk.getParamType(paramIndex) != DataBlock::TYPE_STRING || result_blk.getParamNameId(paramIndex) != column_name_id)
      continue;

    const char *columnValue = result_blk.getStr(paramIndex);

    // The first column is always the asset.
    if (firstColumn)
    {
      firstColumn = false;

      searchResult.asset = DAEDITOR3.getAssetByName(columnValue);
      if (!searchResult.asset)
      {
        searchResult.assetText = columnValue;
        logwarn("Search result asset \"%s\" cannot be found.", columnValue);
      }
    }
    else
    {
      searchResult.additionalColumns.emplace_back(columnValue);
    }
  }

  addResult(eastl::move(searchResult));
}

bool AssetSearchResultsWindow::loadResultsFromBlk(const DataBlock &blk, const char *path)
{
  const DataBlock *headerBlk = blk.getBlockByName("header");
  if (!headerBlk)
  {
    logerr("Loading search results from \"%s\" has failed. The header block is missing.", path);
    return false;
  }

  const int columnNameId = blk.getNameId("column");
  if (columnNameId == -1)
  {
    logerr("Loading search results from \"%s\" has failed. It does not contain any columns.", path);
    return false;
  }

  dag::Vector<SimpleString> loadedColumnTitles;
  for (int i = 0; i < headerBlk->paramCount(); ++i)
    if (headerBlk->getParamType(i) == DataBlock::TYPE_STRING && headerBlk->getParamNameId(i) == columnNameId)
      loadedColumnTitles.emplace_back(headerBlk->getStr(i));

  const dag::ConstSpan<SimpleString> listColumnTitles = searchResultsList.getColumnTitles();
  if (loadedColumnTitles.size() != listColumnTitles.size())
  {
    logerr("Loading search results from \"%s\" has failed. The file contains %d columns. Expected %d columns.", path,
      loadedColumnTitles.size(), listColumnTitles.size());
    return false;
  }

  for (int i = 0; i < loadedColumnTitles.size(); ++i)
    if (loadedColumnTitles[i] != listColumnTitles[i])
    {
      logerr("Loading search results from \"%s\" has failed. Column %d is named \"%s\" in the file. Expected name \"%s\".", path,
        i + 1, loadedColumnTitles[i], listColumnTitles[i]);
      return false;
    }

  clearResults();

  const int resultNameId = blk.getNameId("result");
  if (resultNameId >= 0)
  {
    for (int blockIndex = 0; blockIndex < blk.blockCount(); ++blockIndex)
    {
      const DataBlock *resultBlk = blk.getBlock(blockIndex);
      if (resultBlk->getNameId() != resultNameId)
        continue;

      loadResult(*resultBlk, columnNameId);
    }
  }

  fillResultsList();

  return true;
}

void AssetSearchResultsWindow::loadResultsFromFile(const char *path)
{
  DataBlock blk;
  if (!blk.load(path))
  {
    logerr("Load search results from \"%s\" has failed.", path);
    return;
  }

  loadResultsFromBlk(blk, path);
}

void AssetSearchResultsWindow::updateImguiSubtitle(const ImVec2 &start_pos, float available_width)
{
  // horizontal layout: framePadding.x + optional(fontSizedIconSize.x + itemSpacingX) + text width + framePadding.x

  const float itemSpacingX = ImGui::GetStyle().ItemSpacing.x;
  const ImVec2 framePadding = ImGui::GetStyle().FramePadding;
  const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();

  float frameWidthWithoutText = framePadding.x * 2.0f;
  if (windowSubtitleIcon != PropPanel::IconId::Invalid)
  {
    frameWidthWithoutText += fontSizedIconSize.x + itemSpacingX;
    available_width -= frameWidthWithoutText;
  }

  const ImVec2 unclipppedTextSize = ImGui::CalcTextSize(windowSubtitle.begin(), windowSubtitle.end());
  const float clippedTextWidth = clamp(unclipppedTextSize.x, 0.0f, available_width);

  if (pinned)
  {
    const ImVec2 frameEndPos(start_pos.x + frameWidthWithoutText + clippedTextWidth,
      start_pos.y + unclipppedTextSize.y + (framePadding.y * 2.0f));
    ImGui::RenderFrame(start_pos, frameEndPos, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
  }

  if (windowSubtitleIcon != PropPanel::IconId::Invalid)
  {
    ImGui::SetCursorScreenPos(ImVec2(start_pos.x + framePadding.x, start_pos.y + framePadding.y));
    ImGui::Image(PropPanel::get_im_texture_id_from_icon_id(windowSubtitleIcon), fontSizedIconSize);
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, start_pos.y));
  }

  if (clippedTextWidth > 0.0f)
  {
    ImGui::SetNextItemWidth(clippedTextWidth);
    PropPanel::ImguiHelper::labelOnly(windowSubtitle.begin(), windowSubtitle.end());
    PropPanel::set_previous_imgui_control_tooltip(&windowSubtitle, windowSubtitle.begin(), windowSubtitle.end());
    ImGui::SameLine();
  }
}

void AssetSearchResultsWindow::customControlUpdate(int id)
{
  G_ASSERT(id == CustomControlHolderId);

  ImGui::PushID(this);

  if (!windowSubtitle.empty())
  {
    const ImVec2 startPos = ImGui::GetCursorScreenPos();
    const ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
    const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
    const ImVec2 imageButtonSize = PropPanel::ImguiHelper::getImageButtonSize(fontSizedIconSize);
    const float availableWidthBeforePinButton = contentRegionAvail.x - imageButtonSize.x - ImGui::GetStyle().ItemSpacing.x;

    if (availableWidthBeforePinButton > 0.0f)
      updateImguiSubtitle(startPos, availableWidthBeforePinButton);

    ImGui::SetCursorScreenPos(ImVec2(startPos.x + contentRegionAvail.x - imageButtonSize.x, startPos.y));
    if (ImGui::ImageButton("pin", PropPanel::get_im_texture_id_from_icon_id(pinned ? pinIcon : unpinIcon), fontSizedIconSize))
      pinned = !pinned;

    PropPanel::set_previous_imgui_control_tooltip(&pinned, "Toggle asset pinning.\n\n"
                                                           "When the selected asset is pinned then changing the asset in\n"
                                                           "the Assets Tree will not change the asset in this window.");

    ImGui::NewLine();
  }

  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

  bool inputFocused;
  ImGuiID inputId;
  const bool searchInputChanged = PropPanel::ImguiHelper::searchInput(&searchInputFocusId, "##searchInput", "Filter and search",
    textToSearch, searchIcon, closeIcon, &inputFocused, &inputId);

  PropPanel::set_previous_imgui_control_tooltip(inputId, AssetSelectorCommon::searchTooltip);

  if (searchInputChanged)
  {
    searchResultsList.setSearchText(textToSearch);
    searchResultsList.fill();
  }

  if (ImGui::Button("Save results"))
  {
    const String path = wingw::file_save_dlg(nullptr, "Save results...", "BLK (*.blk)|*.blk|All files(*.*)|*.*", "blk");
    if (!path.empty())
      saveResultsToFile(path);
  }

  ImGui::SameLine();

  if (ImGui::Button("Load results"))
  {
    const String path = wingw::file_open_dlg(nullptr, "Load results...", "BLK (*.blk)|*.blk|All files(*.*)|*.*", "blk");
    if (!path.empty())
      loadResultsFromFile(path);
  }

  if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_F))
    PropPanel::focus_helper.requestFocus(&searchInputFocusId);

  bool contextMenuRequested = false;
  searchResultsList.updateImgui(contextMenuRequested);

  if (contextMenuRequested)
  {
    contextMenu.reset(PropPanel::create_context_menu());
    if (const AssetSearchResultsListControl::SearchResult *searchResult = searchResultsList.getSelectedSearchResult())
      fillContextMenu(*contextMenu, *searchResult);
  }

  if (contextMenu && !PropPanel::render_context_menu(*contextMenu))
    contextMenu.reset();

  ImGui::PopID();
}
