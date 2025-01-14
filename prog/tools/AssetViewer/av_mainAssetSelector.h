// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assetsGui/av_view.h>
#include <EASTL/unique_ptr.h>

class AssetBaseView;
class DagorAsset;
class DagorAssetFolder;
class DagorAssetMgr;
class IAssetBaseViewClient;
class FavoritesTab;
class RecentlyUsedTab;

class MainAssetSelector : public IAssetBaseViewClient, public IAssetSelectorFavoritesRecentlyUsedHost
{
public:
  class MainAllAssetsTab
  {
  public:
    MainAllAssetsTab(IAssetBaseViewClient &client, IAssetSelectorContextMenuHandler &asset_menu_handler);

    void fillTree(DagorAssetMgr &asset_mgr, const DataBlock *expansion_state);
    void refillTree();
    void saveTreeData(DataBlock &blk) { allAssetsTab.saveTreeData(blk); }

    bool markExportedTree(int flags);
    bool isSelectedFolderExportable(bool *exported = nullptr) const;

    void setFilterStr(const char *str) { allAssetsTab.setFilterStr(str); }
    const String &getFilterStr() const { return allAssetsTab.getFilterStr(); }

    void setShownAssetTypeIndexes(dag::ConstSpan<int> shown_type_indexes)
    {
      allAssetsTab.setShownAssetTypeIndexes(shown_type_indexes);
    }

    dag::ConstSpan<int> getShownAssetTypeIndexes() const { return allAssetsTab.getShownAssetTypeIndexes(); }

    void selectAsset(const DagorAsset &asset) { allAssetsTab.selectAsset(asset, true); }
    DagorAsset *getSelectedAsset() const;
    DagorAssetFolder *getSelectedAssetFolder() const;

    AllAssetsTree &getTree() { return allAssetsTab.getTree(); }
    const AllAssetsTree &getTree() const { return allAssetsTab.getTree(); }

    void updateImgui() { allAssetsTab.updateImgui(); }

  private:
    AssetBaseView allAssetsTab;
  };

  MainAssetSelector(IAssetBaseViewClient &in_client, IAssetSelectorContextMenuHandler &asset_menu_handler);
  ~MainAssetSelector();

  // IAssetSelectorFavoritesRecentlyUsedHost
  virtual void addAssetToFavorites(const DagorAsset &asset) override;
  virtual void goToAsset(const DagorAsset &asset) override;

  void setAssetMgr(DagorAssetMgr &asset_mgr);

  void addAssetToRecentlyUsed(const DagorAsset &asset);
  void selectNextItemInActiveTree(bool forward = true);
  void onAssetRemoved();

  MainAllAssetsTab &getAllAssetsTab() { return mainAllAssetsTab; }
  const MainAllAssetsTab &getAllAssetsTab() const { return mainAllAssetsTab; }

  void updateImgui();

private:
  enum class ActiveTab
  {
    All,
    Favorites,
    RecentlyUsed,
  };

  // IAssetBaseViewClient
  virtual void onAvClose() override;
  virtual void onAvAssetDblClick(DagorAsset *asset, const char *asset_name) override;
  virtual void onAvSelectAsset(DagorAsset *asset, const char *asset_name) override;
  virtual void onAvSelectFolder(DagorAssetFolder *asset_folder, const char *asset_folder_name) override;

  // IAssetSelectorFavoritesRecentlyUsedHost
  virtual dag::ConstSpan<int> getAllowedTypes() const override;
  virtual void onSelectionChanged(const DagorAsset *asset) override;
  virtual void onSelectionDoubleClicked(const DagorAsset *asset) override;

  void setActiveTab(ActiveTab tab);

  DagorAsset *getSelectedAsset() const;

  static bool tabPage(const char *title, bool selected);

  IAssetBaseViewClient &client;
  MainAllAssetsTab mainAllAssetsTab;
  eastl::unique_ptr<FavoritesTab> favoritesTab;
  eastl::unique_ptr<RecentlyUsedTab> recentlyUsedTab;
  DagorAssetMgr *assetMgr = nullptr;
  ActiveTab activeTab = ActiveTab::All;
  int favoritesFilledGenerationId = -1;
  int recentlyUsedFilledGenerationId = -1;
  bool allowChangingRecentlyUsedList = true;
};
