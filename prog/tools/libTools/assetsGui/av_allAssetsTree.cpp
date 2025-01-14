// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assetsGui/av_allAssetsTree.h>

#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetFolder.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <generic/dag_sort.h>
#include <propPanel/propPanel.h>
#include <util/dag_string.h>

struct GrpRec
{
  const char *name;
  int folderIdx;
};

struct EntryRec
{
  const char *name;
  const DagorAsset *entry;
};

static int grp_compare(const GrpRec *a, const GrpRec *b) { return stricmp(a->name, b->name); }

static int entry_compare(const EntryRec *a, const EntryRec *b) { return stricmp(a->name, b->name); }

static TEXTUREID folder_textureId = BAD_TEXTUREID;

AllAssetsTree::AllAssetsTree(PropPanel::ITreeViewEventHandler *event_handler) :
  TreeBaseWindow(event_handler, nullptr, 0, 0, hdpi::_pxActual(0), hdpi::_pxActual(0), "", /*icons_show = */ true)
{}

void AllAssetsTree::fill(DagorAssetMgr &asset_mgr, const DataBlock *expansion_state)
{
  folder_textureId = PropPanel::load_icon("folder");
  shownTypes.resize(asset_mgr.getAssetTypesCount(), false);

  clear();
  firstSel = nullptr;
  assetMgr = &asset_mgr;
  if (asset_mgr.getRootFolder())
    firstSel = addGroup(0, nullptr, expansion_state);
}

void AllAssetsTree::refilter()
{
  startFilter();
  setMessage(getRoot() ? "" : "No results found");
}

PropPanel::TLeafHandle AllAssetsTree::addGroup(int folder_idx, PropPanel::TLeafHandle parent, const DataBlock *blk)
{
  const DagorAssetFolder *folder = assetMgr->getFolderPtr(folder_idx);
  if (!folder)
    return nullptr;

  G_ASSERT((((uintptr_t)folder) & 3) == 0); // must be at least 4-byte aligned

  PropPanel::TLeafHandle ret =
    addItem(folder->folderName, folder_textureId, parent, (void *)(((uintptr_t)folder) | IS_DAGOR_ASSET_FOLDER));

  const DataBlock *subBlk = blk ? blk->getBlockByName(folder->folderName) : NULL;

  Tab<GrpRec> groups(tmpmem);

  for (int i = 0; i < folder->subFolderIdx.size(); ++i)
  {
    const DagorAssetFolder *subFolder = assetMgr->getFolderPtr(folder->subFolderIdx[i]);
    if (subFolder)
    {
      GrpRec &rec = groups.push_back();
      rec.name = subFolder->folderName;
      rec.folderIdx = folder->subFolderIdx[i];
    }
  }

  sort(groups, &grp_compare);

  for (const GrpRec &group : groups)
    addGroup(group.folderIdx, ret, subBlk);

  Tab<EntryRec> entries(tmpmem);

  int startIdx, endIdx;
  assetMgr->getFolderAssetIdxRange(folder_idx, startIdx, endIdx);

  for (int i = startIdx; i < endIdx; ++i)
  {
    const DagorAsset &asset = assetMgr->getAsset(i);
    G_ASSERT(asset.getFolderIndex() == folder_idx);

    EntryRec &rec = entries.push_back();
    rec.name = asset.getName();
    rec.entry = &asset;
  }

  sort(entries, &entry_compare);
  bool checkSel = subBlk && subBlk->paramCount();
  for (const EntryRec &entry : entries)
    addEntry(entry.entry, ret, checkSel && subBlk->getBool(entry.entry->getNameTypified(), false));

  if (!parent || subBlk)
    expand(ret);

  return ret;
}

PropPanel::TLeafHandle AllAssetsTree::addEntry(const DagorAsset *asset, PropPanel::TLeafHandle parent, bool selected)
{
  G_ASSERT((((uintptr_t)asset) & 3) == 0); // must be at least 4-byte aligned

  const TEXTUREID icon = AssetSelectorCommon::getAssetTypeIcon(asset->getType());
  PropPanel::TLeafHandle ret = addItem(asset->getName(), icon, parent, (void *)asset);
  if (selected && !firstSel)
    firstSel = ret;

  return ret;
}

void AllAssetsTree::saveTreeData(DataBlock &blk) { scanOpenTree(getRoot(), &blk); }

void AllAssetsTree::loadSelectedItem() { setSelectedItem(firstSel); }

void AllAssetsTree::scanOpenTree(PropPanel::TLeafHandle parent, DataBlock *blk)
{
  const int childCount = getChildrenCount(parent);
  if (childCount == 0 || !isOpen(parent))
    return;

  DataBlock *subBlk = blk->addBlock(getItemName(parent));

  for (int curChildIndex = 0; curChildIndex < childCount; ++curChildIndex)
  {
    PropPanel::TLeafHandle child = getChild(parent, curChildIndex);
    scanOpenTree(child, subBlk);

    if (isSelected(child) && !get_dagor_asset_folder(getItemData(child)))
    {
      DagorAsset *asset = (DagorAsset *)getItemData(child);
      subBlk->addBool(asset->getNameTypified(), true);
    }
  }
}

bool AllAssetsTree::handleNodeFilter(const PropPanel::TTreeNode &node)
{
  if (get_dagor_asset_folder(node.userData))
    return false;

  const DagorAsset &asset = *reinterpret_cast<DagorAsset *>(node.userData);

  if (!shownTypes[asset.getType()])
    return false;

  return textFilter.matchesFilter(node.name);
}

void AllAssetsTree::selectNextItem(bool forward)
{
  PropPanel::TLeafHandle next = getSelectedItem() ? getSelectedItem() : getRoot();
  if (!next)
    return;

  PropPanel::TLeafHandle first = 0;
  for (;;)
  {
    next = getNextNode(next, forward);
    if (!next || next == first)
      return;

    if (!first)
      first = next;

    if (!get_dagor_asset_folder(getItemData(next)))
    {
      setSelectedItem(next);
      return;
    }
  }
}

bool AllAssetsTree::selectAsset(const DagorAsset *asset)
{
  PropPanel::TLeafHandle sel = getRoot();
  if (!sel || !asset)
    return false;

  PropPanel::TLeafHandle next = sel;
  PropPanel::TLeafHandle first = 0;
  for (;;)
  {
    next = getNextNode(next, true);
    if (!next || (next == first))
      return false;
    if (!first)
      first = next;

    void *itemData = getItemData(next);
    if (asset == itemData && !get_dagor_asset_folder(itemData))
    {
      setSelectedItem(next);
      return true;
    }
  }
}

DagorAsset *AllAssetsTree::getSelectedAsset() const
{
  PropPanel::TLeafHandle selectedItem = getSelectedItem();
  void *itemData = getItemData(selectedItem);
  return get_dagor_asset_folder(itemData) ? nullptr : reinterpret_cast<DagorAsset *>(itemData);
}

DagorAssetFolder *AllAssetsTree::getSelectedAssetFolder() const
{
  PropPanel::TLeafHandle selectedItem = getSelectedItem();
  void *itemData = getItemData(selectedItem);
  return get_dagor_asset_folder(itemData);
}

void AllAssetsTree::setShownTypes(dag::ConstSpan<bool> new_shown_types)
{
  G_ASSERT(new_shown_types.size() == shownTypes.size());

  for (int i = 0; i < new_shown_types.size(); ++i)
    shownTypes[i] = new_shown_types[i];
}

void AllAssetsTree::setSearchText(const char *text) { textFilter.setSearchText(text); }

bool AllAssetsTree::isFolder(PropPanel::TLeafHandle node) const
{
  void *itemData = getItemData(node);
  return get_dagor_asset_folder(itemData) != nullptr;
}

void AllAssetsTree::updateImgui(float control_height) { TreeBaseWindow::updateImgui(control_height); }