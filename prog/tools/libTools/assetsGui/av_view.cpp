// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assetsGui/av_view.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetFolder.h>
#include "av_ids.h"

#include <libTools/util/strUtil.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_direct.h>
#include <propPanel/control/listBoxInterface.h>
#include <propPanel/control/menu.h>
#include <sepGui/wndMenuInterface.h>

using PropPanel::TLeafHandle;

//==============================================================================
// AssetBaseView
//==============================================================================
AssetBaseView::AssetBaseView(IAssetBaseViewClient *client, IMenuEventHandler *menu_event_h, void *handle, int l, int t, int w, int h) :
  eh(client),
  menuEventHandler(menu_event_h),
  curMgr(NULL),
  curFilter(midmem),
  filter(midmem),
  mAssetList(midmem),
  canNotifySelChange(true),
  doShowEmptyGroups(true)
{
  mSelBuf[0] = 0;
  mTreeList = new PropPanel::TreeListWindow(this, handle, l, t, hdpi::_pxActual(w), hdpi::_pxActual(h), "");
}


AssetBaseView::~AssetBaseView() { del_it(mTreeList); }


//==============================================================================

void AssetBaseView::connectToAssetBase(DagorAssetMgr *mgr)
{
  curMgr = mgr;

  // asset types filter
  Tab<String> vals(midmem);
  if (mgr)
  {
    for (int i = 0; i < filter.size(); ++i)
      vals.push_back() = mgr->getAssetTypeName(filter[i]);
  }
  mTreeList->setFilterAssetNames(vals);
  curFilter = filter;

  if (mgr)
  {
    fill(filter);

    PropPanel::TLeafHandle _sel = mTreeList->getSelectedItem();

    if (_sel && eh)
      eh->onAvSelectFolder(mTreeList->getItemName(_sel).str());
  }
}


//==============================================================================
void AssetBaseView::reloadBase() { connectToAssetBase(curMgr); }


//==============================================================================
void AssetBaseView::fill(dag::ConstSpan<int> types)
{
  if (!curMgr)
    return;

  dag::ConstSpan<int> folderIdx = curMgr->getFilteredFolders(types);

  if (!curMgr->getRootFolder())
    return;

  mTreeList->clear();
  clear_and_shrink(mAssetList);
  TLeafHandle root = addAsset("\\", NULL, 0);
  fillGroups(root, 0, folderIdx);
  mTreeList->expand(root);
  mTreeList->setSelectedItem(root);

  fillObjects(root);
}

//==============================================================================

TLeafHandle AssetBaseView::addAsset(const char *name, TLeafHandle parent, int idx)
{
  TLeafHandle _leaf = mTreeList->addItem(name, 0, parent, (void *)idx);
  AssetTreeRec _a_rec = {_leaf, idx};
  mAssetList.push_back(_a_rec);

  return _leaf;
}


void AssetBaseView::fillGroups(TLeafHandle parent, int folder_idx, dag::ConstSpan<int> only_folders)
{
  Tab<short> f(curMgr->getFolder(folder_idx).subFolderIdx);
  if (!f.size())
    return;

  //== bubble sort for folder names
  for (int i = 0; i + 1 < f.size(); ++i)
    for (int j = i + 1; j < f.size(); ++j)
      if (stricmp(curMgr->getFolder(f[i]).folderName, curMgr->getFolder(f[j]).folderName) > 0)
      {
        short t = f[i];
        f[i] = f[j];
        f[j] = t;
      }

  for (int i = 0; i < f.size(); ++i)
  {
    int idx = f[i];
    bool found = false;
    for (int j = 0; j < only_folders.size(); j++)
      if (only_folders[j] == idx)
      {
        found = true;
        break;
      }
    if (!found)
      continue;

    TLeafHandle leaf = addAsset(curMgr->getFolder(idx).folderName, parent, idx);

    fillGroups(leaf, idx, only_folders);
    // mTreeList->expand(leaf);
    fillObjects(leaf);
  }
}

//==============================================================================

const char *AssetBaseView::getSelGroupName()
{
  TLeafHandle _sel = mTreeList->getSelectedItem();

  if (_sel)
    return mTreeList->getItemName(_sel).str();

  return "\\";
}


//==============================================================================

int AssetBaseView::getSelGroupId()
{
  TLeafHandle _sel = mTreeList->getSelectedItem();

  if (_sel)
    return (int)(intptr_t)mTreeList->getItemData(_sel);

  return -1;
}


//==============================================================================
const char *AssetBaseView::getSelObjName()
{
  mTreeList->getListSelText(mSelBuf, sizeof(mSelBuf));
  return mSelBuf;
}


//==============================================================================
void AssetBaseView::fillObjects(TLeafHandle parent)
{
  if (!curMgr)
    return;

  int folder_idx = (int)(intptr_t)mTreeList->getItemData(parent);

  dag::ConstSpan<int> a_idx = curMgr->getFilteredAssets(curFilter, folder_idx);

  for (int i = 0; i < a_idx.size(); ++i)
  {
    const DagorAsset &a = curMgr->getAsset(a_idx[i]);
    if (curMgr->isAssetNameUnique(a, curFilter))
      mTreeList->addChildName(a.getName(), parent);
    else
      mTreeList->addChildName(a.getNameTypified(), parent);
  }

  // libObjects->sort(&items_compare, true);
  // libObjects->select(-1);
}


void AssetBaseView::onTvSelectionChange(PropPanel::TreeBaseWindow &tree, TLeafHandle new_sel)
{
  mTreeList->setListSelIndex(0);
  onTvListSelection(tree, 0);
}

void AssetBaseView::onTvListSelection(PropPanel::TreeBaseWindow &tree, int index) { eh->onAvSelectAsset(getSelObjName()); }

void AssetBaseView::onTvListDClick(PropPanel::TreeBaseWindow &tree, int index) { eh->onAvAssetDblClick(getSelObjName()); }

void AssetBaseView::onTvAssetTypeChange(PropPanel::TreeBaseWindow &tree, const Tab<String> &vals)
{
  String objName(getSelObjName());

  if (vals.size() == curMgr->getAssetTypesCount())
  {
    curFilter = filter;
    fill(filter);
  }
  else
  {
    Tab<int> simpleFilter(tmpmem);
    for (int i = 0; i < vals.size(); ++i)
      simpleFilter.push_back(curMgr->getAssetTypeId(vals[i]));

    curFilter = simpleFilter;
    fill(simpleFilter);
  }

  selectObjInBase(objName.str());
}

//==============================================================================

bool AssetBaseView::selectObjInBase(const char *_name)
{
  const DagorAsset *asset = NULL;

  String name(dd_get_fname(_name));
  remove_trailing_string(name, ".res.blk");
  name = DagorAsset::fpath2asset(name);

  const char *type = strchr(name, ':');
  if (!type)
    asset = curMgr->findAsset(_name, curFilter);
  else
  {
    int asset_type = curMgr->getAssetTypeId(type + 1);
    if (asset_type != -1)
      for (int i = 0; i < curFilter.size(); i++)
        if (curFilter[i] == asset_type)
        {
          asset = curMgr->findAsset(String::mk_sub_str(name, type), asset_type);
          break;
        }
  }


  if (asset)
  {
    int idx = asset->getFolderIndex();
    for (int i = 0; i < mAssetList.size(); ++i)
      if (mAssetList[i].index == idx)
      {
        mTreeList->setSelectedItem(mAssetList[i].handle);
        break;
      }

    dag::ConstSpan<int> a_idx = curMgr->getFilteredAssets(curFilter, idx);
    int _sel = -1;

    for (int i = 0; i < a_idx.size(); ++i)
    {
      const DagorAsset &a = curMgr->getAsset(a_idx[i]);
      if (asset->getNameId() == a.getNameId())
      {
        _sel = i;
        break;
      }
    }

    mTreeList->setListSelIndex(_sel);
    eh->onAvSelectAsset(_name);
    return true;
  }

  return false;
}


void AssetBaseView::getTreeNodesExpand(Tab<bool> &exps)
{
  clear_and_shrink(exps);
  TLeafHandle leaf, root;
  leaf = root = mTreeList->getRoot();
  exps.push_back(mTreeList->isOpen(leaf));
  while ((leaf = mTreeList->getNextNode(leaf, true)) && (leaf != root))
    exps.push_back(mTreeList->isOpen(leaf));
}


void AssetBaseView::setTreeNodesExpand(dag::ConstSpan<bool> exps)
{
  if (!exps.size())
    return;

  TLeafHandle leaf, root;
  leaf = root = mTreeList->getRoot();
  if (exps[0] && leaf)
    mTreeList->expand(leaf);
  else
    mTreeList->collapse(leaf);

  int i = 0;
  while ((leaf = mTreeList->getNextNode(leaf, true)) && (leaf != root) && (++i < exps.size()))
    if (exps[i])
      mTreeList->expand(leaf);
    else
      mTreeList->collapse(leaf);
}


void AssetBaseView::addCommonMenuItems(PropPanel::IMenu &menu)
{
  menu.addSeparator(ROOT_MENU_ITEM);
  menu.addSubMenu(ROOT_MENU_ITEM, AssetsGuiIds::CopyMenuItem, "Copy");
  menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetFilePathMenuItem, "File path");
  menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetFolderPathMenuItem, "Folder path");
  menu.addItem(AssetsGuiIds::CopyMenuItem, AssetsGuiIds::CopyAssetNameMenuItem, "Name");
  menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::RevealInExplorerMenuItem, "Reveal in Explorer");
}


bool AssetBaseView::onTvListContextMenu(PropPanel::TreeBaseWindow &tree, PropPanel::IListBoxInterface &list_box)
{
  PropPanel::IMenu &menu = list_box.createContextMenu();
  menu.setEventHandler(menuEventHandler);
  menu.addItem(ROOT_MENU_ITEM, AssetsGuiIds::AddToFavoritesMenuItem, "Add to favorites");
  addCommonMenuItems(menu);
  return true;
}

void AssetBaseView::updateImgui(float control_height) { mTreeList->updateImgui(control_height); }
