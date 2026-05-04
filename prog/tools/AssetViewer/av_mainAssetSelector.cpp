// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_mainAssetSelector.h"
#include "av_tree.h"
#include <assets/assetMgr.h>
#include <assetsGui/av_allAssetsTree.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <assetsGui/av_favoritesTab.h>
#include <assetsGui/av_recentlyUsedTab.h>
#include <ioSys/dag_dataBlock.h>
#include <propPanel/colors.h>
#include <de3_interface.h>

MainAssetSelector::MainAllAssetsTab::MainAllAssetsTab(MainAssetSelector &client) : allAssetsTab(&client, &client, client)
{
  allAssetsTab.setAssetTagsView(&client);
}

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
  client(in_client), menuEventHandler(asset_menu_handler), mainAllAssetsTab(*this), assetBrowser(*this)
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
    recentlyUsedTab->fillTree(recentlyUsedTab->getSelectedAsset());
  else
    recentlyUsedFilledGenerationId = -1;
}

void MainAssetSelector::selectNextItemInActiveTree(bool forward) { mainAllAssetsTab.getTree().selectNextItem(forward); }

void MainAssetSelector::onAssetRemoved()
{
  lastSelectedAsset = nullptr;
  mainAllAssetsTab.refillTree();
}

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

    favoritesTab->setSelectedAsset(lastSelectedAsset);
  }
  else if (activeTab == ActiveTab::RecentlyUsed)
  {
    if (!recentlyUsedTab)
    {
      recentlyUsedTab.reset(new RecentlyUsedTab(*this, *assetMgr));
      recentlyUsedTab->onAllowedTypesChanged(AssetSelectorCommon::getAllAssetTypeIndexes());
      recentlyUsedFilledGenerationId = AssetSelectorGlobalState::getRecentlyUsedGenerationId();
    }

    recentlyUsedTab->setSelectedAsset(lastSelectedAsset);
  }
  else if (activeTab == ActiveTab::All)
  {
    if (lastSelectedAsset)
      mainAllAssetsTab.selectAsset(*lastSelectedAsset);
  }

  assetBrowserFill();

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

void MainAssetSelector::onAvAssetDblClick(DagorAsset *asset, const char *asset_name)
{
  if (!asset)
    return;

  lastSelectedAsset = asset;

  client.onAvAssetDblClick(asset, asset_name);
}

void MainAssetSelector::onAvSelectAsset(DagorAsset *asset, const char *asset_name)
{
  if (asset)
  {
    lastSelectedAsset = asset;

    client.onAvSelectAsset(asset, asset_name);

    if (allowChangingRecentlyUsedList)
      addAssetToRecentlyUsed(*asset);
  }

  assetBrowserFill();
}

void MainAssetSelector::onAvSelectFolder(DagorAssetFolder *asset_folder, const char *asset_folder_name)
{
  client.onAvSelectFolder(asset_folder, asset_folder_name);
}

bool MainAssetSelector::onAssetSelectorContextMenu(PropPanel::IMenu &menu, DagorAsset *asset, DagorAssetFolder *asset_folder)
{
  if (activeTab == ActiveTab::All)
  {
    menuEventHandler.onAssetSelectorContextMenu(menu, asset, asset_folder);
    return true;
  }
  else if (activeTab == ActiveTab::Favorites)
  {
    favoritesTab->fillContextMenu(menu, asset, asset_folder);
    return true;
  }
  else if (activeTab == ActiveTab::RecentlyUsed)
  {
    recentlyUsedTab->fillContextMenu(menu, asset);
    return true;
  }

  return false;
}

dag::ConstSpan<int> MainAssetSelector::getAllowedTypes() const { return AssetSelectorCommon::getAllAssetTypeIndexes(); }

void MainAssetSelector::onSelectionChanged(const DagorAsset *asset)
{
  String name;
  if (asset)
    name = asset->getNameTypified();

  onAvSelectAsset(const_cast<DagorAsset *>(asset), name);
}

void MainAssetSelector::onSelectionDoubleClicked(const DagorAsset *) {}

AssetTagManager *MainAssetSelector::getVisibleTagManager() { return DAEDITOR3.getVisibleTagManagerWindow(); }

void MainAssetSelector::showTagManager(bool show) { DAEDITOR3.showTagManagerWindow(show); }

bool MainAssetSelector::tabPage(const char *title, bool selected)
{
  // Use the same style as in TabPanelPropertyControl.
  if (selected)
    ImGui::PushStyleColor(ImGuiCol_Tab, ImGui::GetStyleColorVec4(ImGuiCol_TabSelected));

  ImGui::PushStyleColor(ImGuiCol_Text, PropPanel::getOverriddenColor(PropPanel::ColorOverride::TAB_BAR_TITLE));
  const bool pressed = ImGui::TabItemButton(title);
  ImGui::PopStyleColor();

  if (selected)
    ImGui::PopStyleColor();

  return pressed;
}

void MainAssetSelector::assetBrowserFill()
{
  if (!assetBrowserIsOpen())
    return;

  dag::Vector<DagorAsset *> assets;
  if (activeTab == ActiveTab::All)
    mainAllAssetsTab.getFilteredAssetsFromTheCurrentFolder(assets);
  else if (activeTab == ActiveTab::Favorites)
    favoritesTab->getFilteredAssetsFromTheCurrentFolder(assets);
  else if (activeTab == ActiveTab::RecentlyUsed)
    recentlyUsedTab->getFilteredAssetsFromTheCurrentFolder(assets);

  assetBrowser.setAssets(make_span(assets), getSelectedAsset());
}

bool MainAssetSelector::assetBrowserIsOpen() const { return assetBrowserOpen; }

void MainAssetSelector::assetBrowserSetOpen(bool open)
{
  if (open == assetBrowserOpen)
    return;

  assetBrowserOpen = open;

  if (assetBrowserOpen)
    assetBrowserFill();
}

void MainAssetSelector::assetBrowserOnAssetSelected(DagorAsset *asset, bool)
{
  if (!asset)
    return;

  if (activeTab == ActiveTab::All)
    mainAllAssetsTab.selectAsset(*asset);
  else if (activeTab == ActiveTab::Favorites)
    favoritesTab->setSelectedAsset(asset);
  else if (activeTab == ActiveTab::RecentlyUsed)
    recentlyUsedTab->setSelectedAsset(asset);
}

void MainAssetSelector::assetBrowserOnContextMenu(DagorAsset *asset)
{
  // Because the context menu click handlers work with the selected tree item we must ensure that the asset matches the selection.
  if (!asset || asset != getSelectedAsset())
    return;

  PropPanel::IMenu &menu = assetBrowser.createContextMenu();
  onAssetSelectorContextMenu(menu, asset, nullptr);
}

void MainAssetSelector::saveAssetBrowserSettings(DataBlock &blk) const
{
  blk.setInt("thumbnailImageSize", assetBrowser.getThumbnailImageSize());
  blk.setBool("open", assetBrowserOpen);
}

void MainAssetSelector::loadAssetBrowserSettings(DataBlock &blk)
{
  assetBrowser.setThumbnailImageSize(blk.getInt("thumbnailImageSize", assetBrowser.getThumbnailImageSize()));
  assetBrowserOpen = blk.getBool("open", assetBrowserOpen);
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

      favoritesTab->fillTree(lastSelectedAsset);

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

      recentlyUsedTab->fillTree(lastSelectedAsset);

      G_ASSERT(!allowChangingRecentlyUsedList); //-V547 Ignore expression is always true.
      allowChangingRecentlyUsedList = true;
    }

    recentlyUsedTab->updateImgui();
  }

  ImGui::EndTabBar();

  if (assetBrowserOpen)
  {
    DAEDITOR3.imguiBegin("Asset Browser", &assetBrowserOpen);
    assetBrowser.updateImgui();
    DAEDITOR3.imguiEnd();
  }
}
