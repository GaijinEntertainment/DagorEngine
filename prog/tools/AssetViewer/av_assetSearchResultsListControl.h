// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assetsGui/av_textFilter.h>
#include <propPanel/propPanel.h>

class DagorAsset;

class AssetSearchResultsListControl
{
public:
  struct SearchResult
  {
    const char *getColumnText(int index) const;

    const DagorAsset *asset = nullptr; // Can be null to display broken asset links. In this case assetText must be set.
    SimpleString assetText;
    dag::Vector<SimpleString> additionalColumns;
  };

  AssetSearchResultsListControl();

  void addColumnTitle(const char *title) { columnTitles.emplace_back(title); }
  void clearColumnTitles() { columnTitles.clear(); }
  dag::ConstSpan<SimpleString> getColumnTitles() const { return columnTitles; }

  void addResult(const DagorAsset &asset);
  void addResult(SearchResult &&search_result);
  void clearResults();
  dag::ConstSpan<SearchResult> getSearchResults() const { return searchResults; }

  const DagorAsset *getSelectedAsset() const;
  const SearchResult *getSelectedSearchResult() const;

  void setSearchText(const char *text) { textFilter.setSearchText(text); }

  void fill();

  void updateImgui(bool &context_menu_requested);

private:
  bool matchesFilter(const SearchResult &search_result) const;
  bool compareResults(const SearchResult &a, const SearchResult &b) const;
  void sortResults();
  void renderSearchResults(bool &context_menu_requested);

  // Similar to DagorAsset::getFolderPath() but it does not include the full, absolute filesystem path for the root folder.
  static void getAssetPath(const DagorAsset &asset, String &path);

  dag::Vector<SearchResult> searchResults;
  dag::Vector<const SearchResult *> filteredSearchResults;
  dag::Vector<SimpleString> columnTitles;
  AssetsGuiTextFilter textFilter;
  PropPanel::IconId missingAssetIcon = PropPanel::IconId::Invalid;
  int filteredSelectionIndex = -1;
  int sortByColumn = 0;
  bool sortAscending = true;
};
