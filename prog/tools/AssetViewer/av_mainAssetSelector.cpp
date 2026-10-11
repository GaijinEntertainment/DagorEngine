// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_mainAssetSelector.h"
#include "av_tree.h"
#include "av_cm.h"
#include <assets/assetMgr.h>
#include <assetsGui/av_allAssetsTree.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <assetsGui/av_favoritesTab.h>
#include <assetsGui/av_recentlyUsedTab.h>
#include <assetsGui/av_ids.h>
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

MainAssetSelector::~MainAssetSelector() { PropPanel::remove_delayed_callback(*this); }

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

void MainAssetSelector::removeAssetFromFavorites(const DagorAsset &asset)
{
  if (!AssetSelectorGlobalState::removeFavorite(asset.getNameTypified()))
    AssetSelectorGlobalState::removeFavorite(String(asset.getName()));
  favoritesFilledGenerationId = -1;
}

void MainAssetSelector::goToAsset(const DagorAsset &asset)
{
  mainAllAssetsTab.selectAsset(asset);
  setActiveTab(ActiveTab::All);
}

void MainAssetSelector::setSelectedAssetInActiveTab(const DagorAsset &asset)
{
  lastSelectedAsset = &asset;

  // The tree selection fires its selection change event.
  G_ASSERT(notifyClientOnSelection);
  notifyClientOnSelection = false;

  if (activeTab == ActiveTab::All)
  {
    // Keep the user's filter. If it hides the asset, select nothing rather than the asset that was not opened.
    AllAssetsTree &tree = mainAllAssetsTab.getTree();
    if (!tree.selectAsset(&asset))
      tree.setSelectedItem(nullptr);
  }
  else if (activeTab == ActiveTab::Favorites)
    favoritesTab->setSelectedAsset(&asset);
  else if (activeTab == ActiveTab::RecentlyUsed)
    recentlyUsedTab->setSelectedAsset(&asset);

  notifyClientOnSelection = true;
}

void MainAssetSelector::addAssetToRecentlyUsed(const DagorAsset &asset)
{
  bool added = false;
  if (asset.isGloballyUnique())
    added = AssetSelectorGlobalState::addRecentlyUsed(asset.getName());
  else
    added = AssetSelectorGlobalState::addRecentlyUsed(asset.getNameTypified());

  if (added)
    recentlyUsedFilledGenerationId = -1;
}

void MainAssetSelector::selectNextItemInActiveTree(bool forward) { mainAllAssetsTab.getTree().selectNextItem(forward); }

void MainAssetSelector::onAssetRemoved()
{
  lastSelectedAsset = nullptr;
  pendingSelectedAsset = nullptr;
  PropPanel::remove_delayed_callback(*this);
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

DagorAssetFolder *MainAssetSelector::getSelectedAssetFolder() const
{
  if (activeTab == ActiveTab::All)
    return mainAllAssetsTab.getSelectedAssetFolder();
  else if (activeTab == ActiveTab::Favorites)
    return favoritesTab->getSelectedAssetFolder();
  else if (activeTab == ActiveTab::RecentlyUsed)
    return nullptr;

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

void MainAssetSelector::onAvSelectAsset(DagorAsset *asset, const char *)
{
  if (asset)
  {
    lastSelectedAsset = asset;

    if (notifyClientOnSelection)
    {
      // The client can show a save confirmation message box, and the ImGui-based one cannot open during an ImGui frame.
      if (!pendingSelectedAsset)
        PropPanel::request_delayed_callback(*this);

      pendingSelectedAsset = asset;
    }

    if (allowChangingRecentlyUsedList)
      addAssetToRecentlyUsed(*asset);
  }

  assetBrowserFill();
}

void MainAssetSelector::onImguiDelayedCallback(void *)
{
  DagorAsset *asset = pendingSelectedAsset;
  if (!asset)
    return;

  client.onAvSelectAsset(asset, asset->getNameTypified());
  pendingSelectedAsset = nullptr;
}

void MainAssetSelector::onAvSelectFolder(DagorAssetFolder *asset_folder, const char *asset_folder_name)
{
  client.onAvSelectFolder(asset_folder, asset_folder_name);
}

bool MainAssetSelector::onAssetSelectorContextMenu(PropPanel::IMenu &menu, DagorAsset *asset, DagorAssetFolder *asset_folder)
{
  using PropPanel::ROOT_MENU_ITEM;

  if (asset)
  {
    if (activeTab == ActiveTab::All)
    {
      menu.addItem(ROOT_MENU_ITEM, CM_ADD_ASSET_TO_FAVORITES, "Add to favorites");
    }
    else if (activeTab == ActiveTab::Favorites)
    {
      menu.addItem(ROOT_MENU_ITEM, CM_REMOVE_ASSET_FROM_FAVORITES, "Remove from favorites");
      menu.addItem(ROOT_MENU_ITEM, CM_GO_TO_ASSET, "Go to asset");
    }
    else if (activeTab == ActiveTab::RecentlyUsed)
    {
      menu.addItem(ROOT_MENU_ITEM, CM_ADD_ASSET_TO_FAVORITES, "Add to favorites");
      menu.addItem(ROOT_MENU_ITEM, CM_GO_TO_ASSET, "Go to asset");
    }

    menu.addSeparator(ROOT_MENU_ITEM);

    return menuEventHandler.onAssetSelectorContextMenu(menu, asset, nullptr);
  }

  if (activeTab == ActiveTab::All)
    return menuEventHandler.onAssetSelectorContextMenu(menu, nullptr, asset_folder);

  if (activeTab == ActiveTab::Favorites && asset_folder && favoritesTab)
  {
    favoritesTab->fillContextMenu(menu, nullptr, asset_folder);
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

void MainAssetSelector::refreshRecentlyUsedIfSafe()
{
  if (activeTab != ActiveTab::RecentlyUsed || !recentlyUsedTab)
    return;

  const int generation = AssetSelectorGlobalState::getRecentlyUsedGenerationId();
  if (recentlyUsedFilledGenerationId == generation)
    return;

  if (recentlyUsedTab->isContextMenuOpen())
    return;

  if (assetBrowserOpen && assetBrowser.isContextMenuOpen())
    return;

  // Updating on the RMB press or even just release updates the tab before context menu is open
  if (ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    return;

  G_ASSERT(allowChangingRecentlyUsedList);
  allowChangingRecentlyUsedList = false;

  recentlyUsedTab->fillTree(lastSelectedAsset);

  G_ASSERT(!allowChangingRecentlyUsedList); //-V547 Ignore expression is always true.
  allowChangingRecentlyUsedList = true;

  recentlyUsedFilledGenerationId = generation;
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

void MainAssetSelector::updateImgui(bool can_submit_contents)
{
  if (can_submit_contents && ImGui::BeginTabBar("TabBar"))
  {
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
      refreshRecentlyUsedIfSafe();

      recentlyUsedTab->updateImgui();
    }

    ImGui::EndTabBar();
  }

  // The Asset Browser is a separate window, there is no need to check can_submit_contents.
  if (assetBrowserOpen)
  {
    IDaEditor3Engine::ScopedImguiWindow window("Asset Browser", &assetBrowserOpen);
    assetBrowser.updateImgui(window.canSubmitContents());
  }
}
