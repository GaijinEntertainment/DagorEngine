// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "av_assetSearchResultsListControl.h"

#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/customControl.h>
#include <propPanel/control/menu.h>
#include <propPanel/propPanel.h>

#include <EASTL/unique_ptr.h>

class DagorAsset;
class DagorAssetMgr;
class DataBlock;

class AssetSearchResultsWindow : public PropPanel::DialogWindow, public PropPanel::ICustomControl, public PropPanel::IMenuEventHandler
{
public:
  explicit AssetSearchResultsWindow(const char *caption);

  // Set optional subtitle displayed at the top of the window. If it is empty then nothing is displayed.
  void setWindowSubtitle(const char *window_subtitle) { windowSubtitle = window_subtitle; }

  void addColumnTitle(const char *title) { searchResultsList.addColumnTitle(title); }
  void clearColumnTitles() { searchResultsList.clearColumnTitles(); }

  void addResult(const DagorAsset &asset) { searchResultsList.addResult(asset); }
  void addResult(AssetSearchResultsListControl::SearchResult &&search_result)
  {
    searchResultsList.addResult(eastl::move(search_result));
  }
  void clearResults() { searchResultsList.clearResults(); }

  void fillResultsList() { searchResultsList.fill(); }

private:
  // PropPanel::ICustomControl
  void customControlUpdate(int id) override;

  // PropPanel::IMenuEventHandler
  int onMenuItemClick(unsigned id) override;

  const DagorAsset *getSelectedAsset() { return searchResultsList.getSelectedAsset(); }
  void fillContextMenu(PropPanel::IMenu &menu, const AssetSearchResultsListControl::SearchResult &search_result);
  void saveResults(const char *path) const;
  void loadResult(const DataBlock &result_blk, int column_name_id);
  void loadResults(const char *path);

  static constexpr int CustomControlHolderId = 1;

  AssetSearchResultsListControl searchResultsList;
  eastl::unique_ptr<PropPanel::IMenu> contextMenu;
  SimpleString windowSubtitle;
  String textToSearch;
  PropPanel::IconId closeIcon = PropPanel::IconId::Invalid;
  PropPanel::IconId searchIcon = PropPanel::IconId::Invalid;
  const bool searchInputFocusId = false; // Only the address of this member is used.
};

extern eastl::unique_ptr<AssetSearchResultsWindow> asset_search_results_window;