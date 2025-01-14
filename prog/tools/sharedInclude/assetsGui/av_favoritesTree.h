// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assets/assetMgr.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <assetsGui/av_globalState.h>
#include <assetsGui/av_ids.h>
#include <assetsGui/av_textFilter.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/treeInterface.h>
#include <propPanel/propPanel.h>
#include <sepGui/wndMenuInterface.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class FavoritesTree : public IMenuEventHandler, public PropPanel::ITreeViewEventHandler
{
public:
  explicit FavoritesTree(IAssetSelectorFavoritesRecentlyUsedHost &tab_host, const DagorAssetMgr &asset_mgr) :
    tabHost(tab_host), assetMgr(asset_mgr), showHierarchy(AssetSelectorGlobalState::getShowHierarchyInFavorites())
  {
    tree = new PropPanel::TreeBaseWindow(this, nullptr, 0, 0, hdpi::_pxActual(0), hdpi::_pxActual(0), "",
      /*icons_show = */ true);

    folderIcon = PropPanel::load_icon("folder");

    shownTypes.resize(asset_mgr.getAssetTypesCount(), false);
  }

  void fillTree()
  {
    const dag::ConstSpan<int> allowedTypeIndexes = tabHost.getAllowedTypes();
    const DagorAsset *selectedAsset = getSelectedAsset();
    int assetsMatchingAllowedTypes = 0;

    closedFolders.clear();
    storeExpansionState(tree->getRoot());

    tree->clear();
    rootFavoriteTreeFolder.reset();
    folderIndexToFavoriteTreeFolderMap.clear();

    const dag::Vector<String> &favorites = AssetSelectorGlobalState::getFavorites();
    for (const String &assetName : favorites)
    {
      const DagorAsset *asset = AssetSelectorCommon::getAssetByName(assetMgr, assetName, allowedTypeIndexes);
      if (!asset)
        continue;

      ++assetsMatchingAllowedTypes;

      if (!shownTypes[asset->getType()])
        continue;

      if (!textFilter.matchesFilter(asset->getName()))
        continue;

      if (showHierarchy)
      {
        FavoriteTreeFolder &treeFolder = makeFavoriteTreeHierarchy(asset->getFolderIndex());
        G_ASSERT(assetMgr.getFolderPtr(asset->getFolderIndex()) == treeFolder.assetFolder);
        treeFolder.addAsset(*asset);
      }
      else
      {
        rootFavoriteTreeFolder.addAsset(*asset);
      }
    }

    sortFavoriteTreeFolder(rootFavoriteTreeFolder);
    fillTreeInternal(*tree, rootFavoriteTreeFolder, nullptr);

    if (assetsMatchingAllowedTypes == 0)
      tree->setMessage("No favorites");
    else if (rootFavoriteTreeFolder.isEmpty())
      tree->setMessage("No results found");
    else
      tree->setMessage("");

    if (selectedAsset)
    {
      PropPanel::TLeafHandle treeItem = getTreeItemByItemData(makeItemDataFromAsset(*selectedAsset));
      if (treeItem)
        tree->setSelectedItem(treeItem);
    }
  }

  DagorAsset *getSelectedAsset()
  {
    const PropPanel::TLeafHandle selectedNode = tree->getSelectedItem();
    return selectedNode ? getAssetFromItemData(selectedNode) : nullptr;
  }

  void expandAll(bool expand) { tree->expandRecursive(tree->getRoot(), expand); }

  bool getShowHierarchy() const { return showHierarchy; }

  void setShowHierarchy(bool show)
  {
    if (show == showHierarchy)
      return;

    showHierarchy = show;
    fillTree();
    AssetSelectorGlobalState::setShowHierarchyInFavorites(showHierarchy);
  }

  void setShownTypes(dag::ConstSpan<bool> new_shown_types)
  {
    G_ASSERT(new_shown_types.size() == shownTypes.size());

    for (int i = 0; i < new_shown_types.size(); ++i)
      shownTypes[i] = new_shown_types[i];
  }

  void setSearchText(const char *text) { textFilter.setSearchText(text); }

  void selectNextItem(bool forward = true)
  {
    PropPanel::TLeafHandle next = tree->getSelectedItem();
    if (!next)
    {
      next = forward ? tree->getRoot() : tree->getNextNode(tree->getRoot(), false);
      if (!next)
        return;

      if (getAssetFromItemData(next))
      {
        tree->setSelectedItem(next);
        return;
      }
    }

    PropPanel::TLeafHandle first = 0;
    for (;;)
    {
      next = tree->getNextNode(next, forward);
      if (!next || next == first)
        return;

      if (!first)
        first = next;

      if (getAssetFromItemData(next))
      {
        tree->setSelectedItem(next);
        return;
      }
    }
  }

  void updateImgui() { tree->updateImgui(); }

private:
  struct FavoriteTreeFolder
  {
    ~FavoriteTreeFolder() { clear_all_ptr_items(subFolders); }

    void reset()
    {
      assetFolder = nullptr;
      assets.clear();
      clear_all_ptr_items(subFolders);
    }

    void addAsset(const DagorAsset &asset)
    {
      G_ASSERT(eastl::find(assets.begin(), assets.end(), &asset) == assets.end());
      assets.push_back(&asset);
    }

    FavoriteTreeFolder *addFolder(const DagorAssetFolder &asset_folder)
    {
      G_ASSERT(eastl::find_if(subFolders.begin(), subFolders.end(), [&asset_folder](FavoriteTreeFolder *tree_folder) {
        return tree_folder->assetFolder == &asset_folder;
      }) == subFolders.end());

      FavoriteTreeFolder *childTreeFolder = new FavoriteTreeFolder();
      childTreeFolder->assetFolder = &asset_folder;
      subFolders.push_back(childTreeFolder);
      return childTreeFolder;
    }

    void sort()
    {
      eastl::sort(assets.begin(), assets.end(), assetCompareFunction);
      eastl::sort(subFolders.begin(), subFolders.end(), folderCompareFunction);
    }

    bool isEmpty() const { return subFolders.empty() && assets.empty(); }

    static bool assetCompareFunction(const DagorAsset *a, const DagorAsset *b)
    {
      const int result = stricmp(a->getName(), b->getName());
      if (result == 0)
        return a < b;
      return result < 0;
    }

    static bool folderCompareFunction(const FavoriteTreeFolder *a, const FavoriteTreeFolder *b)
    {
      const int result = stricmp(a->assetFolder->folderName, b->assetFolder->folderName);
      if (result == 0)
        return a < b;
      return result < 0;
    }

    const DagorAssetFolder *assetFolder = nullptr;
    dag::Vector<FavoriteTreeFolder *> subFolders;
    dag::Vector<const DagorAsset *> assets;
  };

  virtual int onMenuItemClick(unsigned id) override
  {
    switch (id)
    {
      case AssetsGuiIds::GoToAssetInSelectorMenuItem:
        if (DagorAsset *asset = getSelectedAsset())
          tabHost.goToAsset(*asset);
        return 1;

      case AssetsGuiIds::RemoveFromFavoritesMenuItem:
        if (DagorAsset *asset = getSelectedAsset())
        {
          if (!AssetSelectorGlobalState::removeFavorite(asset->getNameTypified()))
            AssetSelectorGlobalState::removeFavorite(String(asset->getName()));

          fillTree();
        }
        return 1;

      case AssetsGuiIds::CopyAssetFilePathMenuItem:
        if (const DagorAsset *asset = getSelectedAsset())
          AssetSelectorCommon::copyAssetFilePathToClipboard(*asset);
        return 1;

      case AssetsGuiIds::CopyAssetFolderPathMenuItem:
      {
        const PropPanel::TLeafHandle selectedItem = tree->getSelectedItem();
        if (const DagorAsset *asset = getAssetFromItemData(selectedItem))
          AssetSelectorCommon::copyAssetFolderPathToClipboard(*asset);
        else if (const DagorAssetFolder *folder = getAssetFolderFromItemData(selectedItem))
          AssetSelectorCommon::copyAssetFolderPathToClipboard(*folder);
        return 1;
      }

      case AssetsGuiIds::CopyAssetNameMenuItem:
      {
        const PropPanel::TLeafHandle selectedItem = tree->getSelectedItem();
        if (const DagorAsset *asset = getAssetFromItemData(selectedItem))
          AssetSelectorCommon::copyAssetNameToClipboard(*asset);
        else if (const DagorAssetFolder *folder = getAssetFolderFromItemData(selectedItem))
          AssetSelectorCommon::copyAssetFolderNameToClipboard(*folder);
        return 1;
      }

      case AssetsGuiIds::RevealInExplorerMenuItem:
      {
        const PropPanel::TLeafHandle selectedItem = tree->getSelectedItem();
        if (const DagorAsset *asset = getAssetFromItemData(selectedItem))
          AssetSelectorCommon::revealInExplorer(*asset);
        else if (const DagorAssetFolder *folder = getAssetFolderFromItemData(selectedItem))
          AssetSelectorCommon::revealInExplorer(*folder);
        return 1;
      }

      case AssetsGuiIds::ExpandChildrenMenuItem:
      case AssetsGuiIds::CollapseChildrenMenuItem:
        const PropPanel::TLeafHandle selectedItem = tree->getSelectedItem();
        if (getFolderFromItemData(selectedItem))
        {
          const bool expand = id == AssetsGuiIds::ExpandChildrenMenuItem;
          const int childCount = tree->getChildrenCount(selectedItem);
          for (int i = 0; i < childCount; ++i)
            tree->expandRecursive(tree->getChild(selectedItem, i), expand);
        }
        return 1;
    }

    return 0;
  }

  virtual void onTvSelectionChange(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::TLeafHandle new_sel) override
  {
    tabHost.onSelectionChanged(getSelectedAsset());
  }

  virtual void onTvDClick(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::TLeafHandle node) override
  {
    tabHost.onSelectionDoubleClicked(getSelectedAsset());
  }

  virtual bool onTvContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &in_tree) override
  {
    if (getSelectedAsset())
    {
      PropPanel::IMenu &menu = in_tree.createContextMenu();
      menu.setEventHandler(this);
      menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::GoToAssetInSelectorMenuItem, "Go to asset");
      AssetBaseView::addCommonMenuItems(menu);
      menu.addSeparator(ROOT_MENU_ITEM);
      menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::RemoveFromFavoritesMenuItem, "Remove from favorites");
      return true;
    }
    else if (getFolderFromItemData(tree->getSelectedItem()))
    {
      PropPanel::IMenu &menu = in_tree.createContextMenu();
      menu.setEventHandler(this);
      menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::ExpandChildrenMenuItem, "Expand children");
      menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::CollapseChildrenMenuItem, "Collapse children");
      menu.addSeparator(ROOT_MENU_ITEM);

      menu.addSubMenu(ROOT_MENU_ITEM, AssetsGuiIds::CopyMenuItem, "Copy");
      menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetFolderPathMenuItem, "Folder path");
      menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetNameMenuItem, "Name");

      menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::RevealInExplorerMenuItem, "Reveal in Explorer");
      return true;
    }

    return false;
  }

  FavoriteTreeFolder &makeFavoriteTreeHierarchy(int folder_index)
  {
    auto it = folderIndexToFavoriteTreeFolderMap.find(folder_index);
    if (it != folderIndexToFavoriteTreeFolderMap.end())
      return *it->second;

    const DagorAssetFolder *assetFolder = assetMgr.getFolderPtr(folder_index);
    if (!assetFolder)
      return rootFavoriteTreeFolder;

    FavoriteTreeFolder &parentTreeFolder = makeFavoriteTreeHierarchy(assetFolder->parentIdx);
    FavoriteTreeFolder *newFolder = parentTreeFolder.addFolder(*assetFolder);
    folderIndexToFavoriteTreeFolderMap.insert({folder_index, newFolder});
    return *newFolder;
  }

  void sortFavoriteTreeFolder(FavoriteTreeFolder &tree_folder)
  {
    for (FavoriteTreeFolder *subFolder : tree_folder.subFolders)
      sortFavoriteTreeFolder(*subFolder);

    tree_folder.sort();
  }

  void fillTreeInternal(PropPanel::TreeBaseWindow &favorites_tree, const FavoriteTreeFolder &tree_folder,
    PropPanel::TLeafHandle parent)
  {
    for (const FavoriteTreeFolder *subFolder : tree_folder.subFolders)
    {
      PropPanel::TLeafHandle subFolderTreeLeaf =
        favorites_tree.addItem(subFolder->assetFolder->folderName, folderIcon, parent, makeItemDataFromFolder(*subFolder));

      if (closedFolders.find(subFolder->assetFolder) == closedFolders.end())
        favorites_tree.expand(subFolderTreeLeaf);

      fillTreeInternal(favorites_tree, *subFolder, subFolderTreeLeaf);
    }

    String imageName;
    for (const DagorAsset *asset : tree_folder.assets)
    {
      const TEXTUREID icon = AssetSelectorCommon::getAssetTypeIcon(asset->getType());
      favorites_tree.addItem(asset->getName(), icon, parent, makeItemDataFromAsset(*asset));
    }
  }

  void storeExpansionState(PropPanel::TLeafHandle parent)
  {
    if (!parent)
      return;

    const int childCount = tree->getChildrenCount(parent);
    if (childCount == 0)
      return;

    if (!tree->isOpen(parent))
    {
      FavoriteTreeFolder *folder = getFolderFromItemData(parent);
      G_ASSERT(folder);
      closedFolders.insert(folder->assetFolder);
      return;
    }

    for (int i = 0; i < childCount; ++i)
      storeExpansionState(tree->getChild(parent, i));
  }

  PropPanel::TLeafHandle getTreeItemByItemData(void *item_data)
  {
    PropPanel::TLeafHandle rootItem = tree->getRoot();
    if (!rootItem)
      return nullptr;

    PropPanel::TLeafHandle item = rootItem;
    while (true)
    {
      if (tree->getItemData(item) == item_data)
        return item;

      item = tree->getNextNode(item, true);
      if (!item || item == rootItem)
        return nullptr;
    }

    return nullptr;
  }

  DagorAsset *getAssetFromItemData(PropPanel::TLeafHandle item)
  {
    void *itemData = tree->getItemData(item);
    if ((reinterpret_cast<uintptr_t>(itemData) & 1) == 0)
      return reinterpret_cast<DagorAsset *>(itemData);

    return nullptr;
  }

  const DagorAssetFolder *getAssetFolderFromItemData(PropPanel::TLeafHandle item)
  {
    FavoriteTreeFolder *folder = getFolderFromItemData(item);
    return folder ? folder->assetFolder : nullptr;
  }

  FavoriteTreeFolder *getFolderFromItemData(PropPanel::TLeafHandle item)
  {
    void *itemData = tree->getItemData(item);
    if ((reinterpret_cast<uintptr_t>(itemData) & 1) == 1)
      return reinterpret_cast<FavoriteTreeFolder *>(reinterpret_cast<uintptr_t>(itemData) & ~1);

    return nullptr;
  }

  static void *makeItemDataFromAsset(const DagorAsset &asset)
  {
    G_ASSERT((reinterpret_cast<uintptr_t>(&asset) & 1) == 0);
    return reinterpret_cast<void *>(&const_cast<DagorAsset &>(asset));
  }

  static void *makeItemDataFromFolder(const FavoriteTreeFolder &folder)
  {
    G_ASSERT((reinterpret_cast<uintptr_t>(&folder) & 1) == 0);
    return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(&folder) | 1);
  }

  IAssetSelectorFavoritesRecentlyUsedHost &tabHost;
  const DagorAssetMgr &assetMgr;
  PropPanel::TreeBaseWindow *tree;
  ska::flat_hash_map<int, FavoriteTreeFolder *> folderIndexToFavoriteTreeFolderMap;
  ska::flat_hash_set<const DagorAssetFolder *> closedFolders;
  FavoriteTreeFolder rootFavoriteTreeFolder;
  dag::Vector<bool> shownTypes;
  AssetsGuiTextFilter textFilter;
  TEXTUREID folderIcon;
  bool showHierarchy = true;
};