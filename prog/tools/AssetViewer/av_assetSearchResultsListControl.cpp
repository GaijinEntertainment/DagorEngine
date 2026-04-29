// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_assetSearchResultsListControl.h"

#include "av_appwnd.h"

#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <propPanel/imguiHelper.h>

#include <EASTL/sort.h>
#include <imgui/imgui_internal.h>

const char *AssetSearchResultsListControl::SearchResult::getColumnText(int index) const
{
  if (index == 0)
    return asset ? asset->getName() : assetText.c_str();
  if (index >= 1 && index <= additionalColumns.size())
    return additionalColumns[index - 1];
  return "";
}

AssetSearchResultsListControl::AssetSearchResultsListControl() { missingAssetIcon = PropPanel::load_icon("unknown"); }

void AssetSearchResultsListControl::addResult(const DagorAsset &asset)
{
  SearchResult &searchResult = searchResults.push_back();
  searchResult.asset = &asset;
}

void AssetSearchResultsListControl::addResult(SearchResult &&search_result)
{
  G_ASSERT(search_result.asset || !search_result.assetText.empty());

  searchResults.push_back(search_result);
}

void AssetSearchResultsListControl::clearResults()
{
  searchResults.clear();
  filteredSearchResults.clear();
  filteredSelectionIndex = -1;
}

const DagorAsset *AssetSearchResultsListControl::getSelectedAsset() const
{
  if (filteredSelectionIndex >= 0 && filteredSelectionIndex < filteredSearchResults.size())
    return filteredSearchResults[filteredSelectionIndex]->asset;
  else
    return nullptr;
}

const AssetSearchResultsListControl::SearchResult *AssetSearchResultsListControl::getSelectedSearchResult() const
{
  if (filteredSelectionIndex >= 0 && filteredSelectionIndex < filteredSearchResults.size())
    return filteredSearchResults[filteredSelectionIndex];
  else
    return nullptr;
}

void AssetSearchResultsListControl::fill()
{
  filteredSearchResults.clear();
  for (const SearchResult &searchResult : searchResults)
    if (matchesFilter(searchResult))
      filteredSearchResults.push_back(&searchResult);

  sortResults();
}

bool AssetSearchResultsListControl::matchesFilter(const SearchResult &search_result) const
{
  for (int i = 0; i < columnTitles.size(); ++i)
    if (textFilter.matchesFilter(search_result.getColumnText(i)))
      return true;

  return false;
}

bool AssetSearchResultsListControl::compareResults(const SearchResult &a, const SearchResult &b) const
{
  const char *textA = a.getColumnText(sortByColumn);
  const char *textB = b.getColumnText(sortByColumn);

  if (sortByColumn != 0)
  {
    float valueA;
    float valueB;
    if (sscanf(textA, "%g", &valueA) == 1 && sscanf(textB, "%g", &valueB) == 1)
      if (valueA != valueB)
        return valueA < valueB;
  }

  const int result = stricmp(textA, textB);
  if (result != 0)
    return result < 0;

  return &a < &b;
}

void AssetSearchResultsListControl::sortResults()
{
  if (sortByColumn >= columnTitles.size())
    sortByColumn = 0;

  eastl::sort(filteredSearchResults.begin(), filteredSearchResults.end(),
    [this](const SearchResult *a, const SearchResult *b) { return sortAscending ? compareResults(*a, *b) : compareResults(*b, *a); });
}

void AssetSearchResultsListControl::getAssetPath(const DagorAsset &asset, String &path)
{
  int folderIndex = asset.getFolderIndex();
  while (folderIndex >= 0)
  {
    const DagorAssetFolder &folder = asset.getMgr().getFolder(folderIndex);
    if (!path.empty())
      path.insert(0, "/");
    path.insert(0, folder.folderName);

    folderIndex = folder.parentIdx;
  }
}

void AssetSearchResultsListControl::renderSearchResults(bool &context_menu_requested)
{
  if (columnTitles.empty())
    return;

  // Use the same background color as ImGui::BeginChild() with ImGuiChildFlags_FrameStyle does.
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetColorU32(ImGuiCol_FrameBg));

  const bool tableBegun = ImGui::BeginTable("resultsTable", columnTitles.size(),
    ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersInner | ImGuiTableFlags_PadOuterX |
      ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY);

  ImGui::PopStyleColor(1);

  if (!tableBegun)
    return;

  ImGui::TableSetupColumn("Asset", ImGuiTableColumnFlags_DefaultSort, 0.0f, 0);
  for (int columnIndex = 1; columnIndex < columnTitles.size(); ++columnIndex)
    ImGui::TableSetupColumn(columnTitles[columnIndex], ImGuiTableColumnFlags_None, 0.0f, columnIndex);
  ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible.
  ImGui::TableHeadersRow();

  ImGuiTableSortSpecs *sortSpecs = ImGui::TableGetSortSpecs();
  if (sortSpecs && sortSpecs->SpecsDirty)
  {
    sortSpecs->SpecsDirty = false;

    sortByColumn = 0;
    sortAscending = true;
    if (sortSpecs->SpecsCount == 1)
    {
      sortByColumn = sortSpecs->Specs[0].ColumnIndex;
      sortAscending = sortSpecs->Specs[0].SortDirection != ImGuiSortDirection_Descending;
    }

    sortResults();
  }

  String pathBuffer, tooltipBuffer;
  for (int filteredIndex = 0; filteredIndex < filteredSearchResults.size(); ++filteredIndex)
  {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    const SearchResult *searchResult = filteredSearchResults[filteredIndex];
    const PropPanel::IconId icon =
      searchResult->asset ? AssetSelectorCommon::getAssetTypeIcon(searchResult->asset->getType()) : missingAssetIcon;
    ImGui::Image(PropPanel::get_im_texture_id_from_icon_id(icon), PropPanel::ImguiHelper::getFontSizedIconSize());

    ImGui::SameLine();

    const bool selected = filteredIndex == filteredSelectionIndex;
    const bool itemClicked = ImGui::Selectable(searchResult->getColumnText(0), selected, ImGuiSelectableFlags_SpanAllColumns);
    const bool hovered = ImGui::IsItemHovered();
    const bool itemDoubleClicked = hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
    const bool itemRightClicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right);

    if (itemClicked || itemDoubleClicked || itemRightClicked || ImGui::IsItemFocused())
      filteredSelectionIndex = filteredIndex;

    if (searchResult->asset)
    {
      pathBuffer.clear();
      getAssetPath(*searchResult->asset, pathBuffer);
      tooltipBuffer.printf(0, "%s\n\nPath: %s\nType: %s", searchResult->asset->getName(), pathBuffer.c_str(),
        searchResult->asset->getTypeStr());
    }
    else
    {
      tooltipBuffer.printf(0, "%s\n\nMissing asset.", searchResult->assetText.c_str());
    }
    PropPanel::set_previous_imgui_control_tooltip(ImGui::GetItemID(), tooltipBuffer.begin(), tooltipBuffer.end());

    if (itemDoubleClicked)
    {
      if (const DagorAsset *asset = getSelectedAsset())
        get_app().selectAsset(*asset);
    }
    else if (itemRightClicked)
    {
      context_menu_requested = true;
    }

    for (int columnIndex = 1; columnIndex < columnTitles.size(); ++columnIndex)
    {
      ImGui::TableNextColumn();
      if (columnIndex <= searchResult->additionalColumns.size())
        ImGui::TextUnformatted(searchResult->getColumnText(columnIndex));
    }
  }

  ImGui::EndTable();
}

void AssetSearchResultsListControl::updateImgui(bool &context_menu_requested)
{
  const ImGuiID childWindowId = ImGui::GetCurrentWindow()->GetID("c"); // "c" stands for child. It could be anything.
  const ImVec2 childSize = ImGui::GetContentRegionAvail();
  if (ImGui::BeginChild(childWindowId, childSize, ImGuiChildFlags_FrameStyle, ImGuiWindowFlags_NoSavedSettings))
    renderSearchResults(context_menu_requested);
  ImGui::EndChild();
}
