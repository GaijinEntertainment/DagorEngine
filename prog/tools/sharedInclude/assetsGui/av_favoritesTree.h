// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assets/assetMgr.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <assetsGui/av_assetTreeDragHandler.h>
#include <assetsGui/av_globalState.h>
#include <assetsGui/av_ids.h>
#include <assetsGui/av_textFilter.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/treeInterface.h>
#include <propPanel/control/treeNode.h>
#include <propPanel/propPanel.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class FavoritesTree : public PropPanel::ITreeViewEventHandler
{
public:
  explicit FavoritesTree(IAssetSelectorFavoritesRecentlyUsedHost &tab_host, const DagorAssetMgr &asset_mgr) :
    tabHost(tab_host), assetMgr(asset_mgr), showHierarchy(AssetSelectorGlobalState::getShowHierarchyInFavorites())
  {
    tree = new PropPanel::TreeBaseWindow(this, nullptr, 0, 0, hdpi::_pxActual(0), hdpi::_pxActual(0), "",
      /*icons_show = */ true);
    tree->setDragHandler(&dragHandler);
    dragHandler.tree = tree;

    folderIcon = PropPanel::load_icon("folder");

    shownTypes.resize(asset_mgr.getAssetTypesCount(), false);
  }

  void fillTree(const DagorAsset *asset_to_select)
  {
    const dag::ConstSpan<int> allowedTypeIndexes = tabHost.getAllowedTypes();
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

    if (asset_to_select)
    {
      PropPanel::TLeafHandle treeItem = getTreeItemByItemData(makeItemDataFromAsset(*asset_to_select));
      if (treeItem)
        tree->setSelectedItem(treeItem);
    }

    tabHost.getAssetBrowserHost().assetBrowserFill();
  }

  DagorAsset *getSelectedAsset()
  {
    const PropPanel::TLeafHandle selectedNode = tree->getSelectedItem();
    return selectedNode ? getAssetFromItemData(selectedNode) : nullptr;
  }

  DagorAssetFolder *getSelectedAssetFolder()
  {
    const PropPanel::TLeafHandle selectedNode = tree->getSelectedItem();
    return selectedNode ? getAssetFolderFromItemData(selectedNode) : nullptr;
  }

  void setSelectedAsset(const DagorAsset *asset)
  {
    PropPanel::TLeafHandle treeItem = getTreeItemByItemData(makeItemDataFromAsset(*asset));
    tree->setSelectedItem(treeItem);
  }

  void getFilteredAssetsFromTheCurrentFolder(dag::Vector<DagorAsset *> &assets)
  {
    PropPanel::TLeafHandle selectedFolder = tree->getSelectedItem();
    if (!getFolderFromItemData(selectedFolder))
      selectedFolder = tree->getParent(selectedFolder);

    const int childrenCount = tree->getChildrenCount(selectedFolder);
    for (int i = 0; i < childrenCount; ++i)
    {
      PropPanel::TLeafHandle child = tree->getChild(selectedFolder, i);
      if (tree->getItemNode(child)->getFlagValue(PropPanel::TreeNode::FILTERED_IN))
      {
        if (DagorAsset *asset = getAssetFromItemData(child))
          assets.push_back(asset);
      }
    }
  }

  void expandAll(bool expand) { tree->expandRecursive(tree->getRoot(), expand); }

  void expandSelected(bool expand)
  {
    const PropPanel::TLeafHandle selectedItem = tree->getSelectedItem();
    if (getFolderFromItemData(selectedItem))
    {
      const int childCount = tree->getChildrenCount(selectedItem);
      for (int i = 0; i < childCount; ++i)
        tree->expandRecursive(tree->getChild(selectedItem, i), expand);
    }
  }

  bool getShowHierarchy() const { return showHierarchy; }

  void setShowHierarchy(bool show)
  {
    if (show == showHierarchy)
      return;

    showHierarchy = show;
    fillTree(getSelectedAsset());
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

  void onTvSelectionChange(PropPanel::TreeBaseWindow &, PropPanel::TLeafHandle) override
  {
    tabHost.onSelectionChanged(getSelectedAsset());
  }

  void onTvDClick(PropPanel::TreeBaseWindow &, PropPanel::TLeafHandle) override
  {
    tabHost.onSelectionDoubleClicked(getSelectedAsset());
  }

  bool onTvContextMenu(PropPanel::TreeBaseWindow &, PropPanel::ITreeInterface &in_tree) override
  {
    PropPanel::IMenu &menu = in_tree.createContextMenu();
    DagorAsset *asset = getSelectedAsset();
    DagorAssetFolder *assetFolder = getSelectedAssetFolder();
    return tabHost.getAssetSelectorContextMenuHandler().onAssetSelectorContextMenu(menu, asset, assetFolder);
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
      const PropPanel::IconId icon = AssetSelectorCommon::getAssetTypeIcon(asset->getType());
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

  DagorAssetFolder *getAssetFolderFromItemData(PropPanel::TLeafHandle item)
  {
    FavoriteTreeFolder *folder = getFolderFromItemData(item);
    return folder ? const_cast<DagorAssetFolder *>(folder->assetFolder) : nullptr;
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
  AssetTreeDragHandler dragHandler;
  PropPanel::IconId folderIcon;
  bool showHierarchy = true;
};