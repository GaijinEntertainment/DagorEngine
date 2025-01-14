// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_mainAssetSelector.h"
#include "av_tree.h"
#include <assets/assetMgr.h>
#include <assetsGui/av_allAssetsTree.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <assetsGui/av_favoritesTab.h>
#include <assetsGui/av_recentlyUsedTab.h>
#include <ioSys/dag_dataBlock.h>

MainAssetSelector::MainAllAssetsTab::MainAllAssetsTab(IAssetBaseViewClient &client,
  IAssetSelectorContextMenuHandler &asset_menu_handler) :
  allAssetsTab(&client, &asset_menu_handler)
{}

DagorAsset *MainAssetSelector::MainAllAssetsTab::getSelectedAsset() const { return getTree().getSelectedAsset(); }

DagorAssetFolder *MainAssetSelector::MainAllAssetsTab::getSelectedAssetFolder() const { return getTree().getSelectedAssetFolder(); }

bool MainAssetSelector::MainAllAssetsTab::isSelectedFolderExportable(bool *exported) const
{
  return AvTree::isFolderExportable(getTree(), getTree().getSelectedItem(), exported);
}

void MainAssetSelector::MainAllAssetsTab::fillTree(DagorAssetMgr &asset_mgr, const DataBlock *expansion_state)
{
  AvTree::loadIcons(asset_mgr);

  allAssetsTab.connectToAssetBase(&asset_mgr, AssetSelectorCommon::getAllAssetTypeIndexes(), expansion_state);
}

void MainAssetSelector::MainAllAssetsTab::refillTree()
{
  DataBlock expansionState;
  allAssetsTab.saveTreeData(expansionState);

  G_ASSERT(allAssetsTab.getCurAssetBase());
  getTree().fill(*allAssetsTab.getCurAssetBase(), &expansionState);
  getTree().refilter();
}

bool MainAssetSelector::MainAllAssetsTab::markExportedTree(int flags)
{
  AllAssetsTree &tree = getTree();
  return AvTree::markExportedTree(tree, tree.getRoot(), flags);
}

MainAssetSelector::MainAssetSelector(IAssetBaseViewClient &in_client, IAssetSelectorContextMenuHandler &asset_menu_handler) :
  client(in_client), mainAllAssetsTab(*this, asset_menu_handler)
{}

MainAssetSelector::~MainAssetSelector() {}

void MainAssetSelector::setAssetMgr(DagorAssetMgr &asset_mgr)
{
  assetMgr = &asset_mgr;

  AssetSelectorCommon::initialize(asset_mgr);
}

void MainAssetSelector::addAssetToFavorites(const DagorAsset &asset)
{
  AssetSelectorGlobalState::addFavorite(asset.getNameTypified());
  favoritesFilledGenerationId = -1;
}

void MainAssetSelector::goToAsset(const DagorAsset &asset)
{
  mainAllAssetsTab.selectAsset(asset);
  setActiveTab(ActiveTab::All);
}

void MainAssetSelector::addAssetToRecentlyUsed(const DagorAsset &asset)
{
  bool added = false;
  if (asset.isGloballyUnique())
    added = AssetSelectorGlobalState::addRecentlyUsed(asset.getName());
  else
    added = AssetSelectorGlobalState::addRecentlyUsed(asset.getNameTypified());

  if (!added)
    return;

  if (activeTab == ActiveTab::RecentlyUsed)
    recentlyUsedTab->fillTree();
  else
    recentlyUsedFilledGenerationId = -1;
}

void MainAssetSelector::selectNextItemInActiveTree(bool forward) { mainAllAssetsTab.getTree().selectNextItem(forward); }

void MainAssetSelector::onAssetRemoved() { mainAllAssetsTab.refillTree(); }

void MainAssetSelector::setActiveTab(ActiveTab tab)
{
  if (tab == activeTab)
    return;

  activeTab = tab;

  G_ASSERT(allowChangingRecentlyUsedList);
  allowChangingRecentlyUsedList = false;

  if (activeTab == ActiveTab::Favorites)
  {
    if (!favoritesTab)
    {
      favoritesTab.reset(new FavoritesTab(*this, *assetMgr));
      favoritesTab->onAllowedTypesChanged(AssetSelectorCommon::getAllAssetTypeIndexes());
      favoritesFilledGenerationId = AssetSelectorGlobalState::getFavoritesGenerationId();
    }
  }
  else if (activeTab == ActiveTab::RecentlyUsed)
  {
    if (!recentlyUsedTab)
    {
      recentlyUsedTab.reset(new RecentlyUsedTab(*this, *assetMgr));
      recentlyUsedTab->onAllowedTypesChanged(AssetSelectorCommon::getAllAssetTypeIndexes());
      recentlyUsedFilledGenerationId = AssetSelectorGlobalState::getRecentlyUsedGenerationId();
    }
  }

  onSelectionChanged(getSelectedAsset());

  G_ASSERT(!allowChangingRecentlyUsedList);
  allowChangingRecentlyUsedList = true;
}

DagorAsset *MainAssetSelector::getSelectedAsset() const
{
  if (activeTab == ActiveTab::All)
    return mainAllAssetsTab.getSelectedAsset();
  else if (activeTab == ActiveTab::Favorites)
    return favoritesTab->getSelectedAsset();
  else if (activeTab == ActiveTab::RecentlyUsed)
    return recentlyUsedTab->getSelectedAsset();

  G_ASSERT(false);
  return nullptr;
}

void MainAssetSelector::onAvClose() { client.onAvClose(); }

void MainAssetSelector::onAvAssetDblClick(DagorAsset *asset, const char *asset_name) { client.onAvAssetDblClick(asset, asset_name); }

void MainAssetSelector::onAvSelectAsset(DagorAsset *asset, const char *asset_name)
{
  client.onAvSelectAsset(asset, asset_name);

  if (allowChangingRecentlyUsedList && asset)
    addAssetToRecentlyUsed(*asset);
}

void MainAssetSelector::onAvSelectFolder(DagorAssetFolder *asset_folder, const char *asset_folder_name)
{
  client.onAvSelectFolder(asset_folder, asset_folder_name);
}

dag::ConstSpan<int> MainAssetSelector::getAllowedTypes() const { return AssetSelectorCommon::getAllAssetTypeIndexes(); }

void MainAssetSelector::onSelectionChanged(const DagorAsset *asset)
{
  String name;
  if (asset)
    name = asset->getNameTypified();

  client.onAvSelectAsset(const_cast<DagorAsset *>(asset), name);

  if (allowChangingRecentlyUsedList && asset)
    addAssetToRecentlyUsed(*asset);
}

void MainAssetSelector::onSelectionDoubleClicked(const DagorAsset *asset) {}

bool MainAssetSelector::tabPage(const char *title, bool selected)
{
  // Use the same style as in TabPanelPropertyControl.
  if (selected)
    ImGui::PushStyleColor(ImGuiCol_Tab, ImGui::GetStyleColorVec4(ImGuiCol_TabSelected));

  const bool pressed = ImGui::TabItemButton(title);

  if (selected)
    ImGui::PopStyleColor();

  return pressed;
}

void MainAssetSelector::updateImgui()
{
  if (!ImGui::BeginTabBar("TabBar"))
    return;

  if (tabPage("All", activeTab == ActiveTab::All))
    setActiveTab(ActiveTab::All);
  else if (tabPage("Favorites", activeTab == ActiveTab::Favorites))
    setActiveTab(ActiveTab::Favorites);
  else if (tabPage("Recently used", activeTab == ActiveTab::RecentlyUsed))
    setActiveTab(ActiveTab::RecentlyUsed);

  if (activeTab == ActiveTab::All)
  {
    mainAllAssetsTab.updateImgui();
  }
  else if (activeTab == ActiveTab::Favorites)
  {
    if (favoritesFilledGenerationId != AssetSelectorGlobalState::getFavoritesGenerationId())
    {
      favoritesFilledGenerationId = AssetSelectorGlobalState::getFavoritesGenerationId();

      G_ASSERT(allowChangingRecentlyUsedList);
      allowChangingRecentlyUsedList = false;

      favoritesTab->fillTree();

      G_ASSERT(!allowChangingRecentlyUsedList); //-V547 Ignore expression is always true.
      allowChangingRecentlyUsedList = true;
    }

    favoritesTab->updateImgui();
  }
  else if (activeTab == ActiveTab::RecentlyUsed)
  {
    if (recentlyUsedFilledGenerationId != AssetSelectorGlobalState::getRecentlyUsedGenerationId())
    {
      recentlyUsedFilledGenerationId = AssetSelectorGlobalState::getRecentlyUsedGenerationId();

      G_ASSERT(allowChangingRecentlyUsedList);
      allowChangingRecentlyUsedList = false;

      recentlyUsedTab->fillTree();

      G_ASSERT(!allowChangingRecentlyUsedList); //-V547 Ignore expression is always true.
      allowChangingRecentlyUsedList = true;
    }

    recentlyUsedTab->updateImgui();
  }

  ImGui::EndTabBar();
}
