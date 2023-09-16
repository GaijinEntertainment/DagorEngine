#include "av_tree.h"
#include "av_appwnd.h"
#include "av_plugin.h"
#include "assetBuildCache.h"

#include <generic/dag_sort.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetFolder.h>
#include <debug/dag_debug.h>

#include <util/dag_string.h>

#include <propPanel2/comWnd/list_dialog.h>
#include <winGuiWrapper/wgw_busy.h>
#include <winGuiWrapper/wgw_input.h>

enum
{
  EDIT_BOX_FILTER_ID = 1001,
  EDIT_BOX_SEARCH_ID = 1002,

  TYPE_FILTER_BUTTON_ID,

  FS_HEIGHT = 135,

  FILTER_CAPTION_MAX_TYPES_NUM = 2,
  FILTER_START_DELAY_MS = 500,
};

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

static int folder_imidx = -1;
static int folder_packed_imidx = -1;
static int folder_changed_imidx = -1;
static Tab<int> asset_imidx(inimem);

AvTree::AvTree(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h, const char caption[]) :
  TreeViewWindow(event_handler, phandle, x, y, w, h, FS_HEIGHT, caption),
  mImages(midmem),
  mFirstSel(NULL),
  mSelectedTypesID(midmem),
  filterTimer(this, FILTER_START_DELAY_MS, false)
{
  mPanelFS->createButton(TYPE_FILTER_BUTTON_ID, "");
  mPanelFS->createEditBox(EDIT_BOX_FILTER_ID, "Filter:");
  mPanelFS->createEditBox(EDIT_BOX_SEARCH_ID, "Search:");
}


AvTree::~AvTree() {}


int AvTree::getImageIndex(const char *image_file_name)
{
  for (int i = 0; i < mImages.size(); ++i)
    if (mImages[i].mName == image_file_name)
      return mImages[i].mIndex;

  String bmp(256, "%s%s%s", p2util::get_icon_path(), image_file_name, ".bmp");

  int newIndex = addImage(bmp);
  mImages.push_back(ImageIndex(image_file_name, newIndex));
  return newIndex;
}


void AvTree::fill(DagorAssetMgr *mgr, const DataBlock &set_blk)
{
  folder_imidx = getImageIndex("folder");
  folder_packed_imidx = getImageIndex("folder_packed");
  folder_changed_imidx = getImageIndex("folder_changed");
  asset_imidx.resize(mgr->getAssetTypesCount() * 2);
  for (int i = 0; i < mgr->getAssetTypesCount(); i++)
    asset_imidx[i * 2 + 0] = getImageIndex(String(0, "asset_%s", mgr->getAssetTypeName(i))),
                        asset_imidx[i * 2 + 1] = getImageIndex(String(0, "asset_%s_changed", mgr->getAssetTypeName(i)));

  clear();
  mFirstSel = NULL;
  mDAMgr = mgr;
  if (mDAMgr->getRootFolder())
  {
    // rootIdx = 0;
    mFirstSel = addGroup(0, NULL, &set_blk);
  }

  setTypeFilterToAll();
  setCaptionFilterButton();
}


TLeafHandle AvTree::addGroup(int folder_idx, TLeafHandle parent, const DataBlock *blk)
{
  const DagorAssetFolder *folder = mDAMgr->getFolderPtr(folder_idx);
  if (!folder)
    return NULL;

  G_ASSERT((((uintptr_t)folder) & 3) == 0); // must be at least 4-byte aligned

  TLeafHandle ret =
    addItem(folder->folderName, getImageIndex("folder"), parent, (void *)(((uintptr_t)folder) | IS_DAGOR_ASSET_FOLDER));

  const DataBlock *subBlk = blk ? blk->getBlockByName(folder->folderName) : NULL;

  Tab<GrpRec> groups(tmpmem);
  int i;

  for (i = 0; i < folder->subFolderIdx.size(); ++i)
  {
    const DagorAssetFolder *g = mDAMgr->getFolderPtr(folder->subFolderIdx[i]);
    if (g)
    {
      GrpRec &rec = groups.push_back();
      rec.name = g->folderName;
      rec.folderIdx = folder->subFolderIdx[i];
    }
  }

  sort(groups, &grp_compare);

  for (i = 0; i < groups.size(); ++i)
    addGroup(groups[i].folderIdx, ret, subBlk);

  Tab<EntryRec> entries(tmpmem);

  int s_idx, e_idx;
  mDAMgr->getFolderAssetIdxRange(folder_idx, s_idx, e_idx);
  dag::ConstSpan<DagorAsset *> assets = mDAMgr->getAssets();

  for (i = s_idx; i < e_idx; ++i)
  {
    const DagorAsset &e = mDAMgr->getAsset(i);
    G_ASSERT(e.getFolderIndex() == folder_idx);

    EntryRec &rec = entries.push_back();
    rec.name = e.getName();
    rec.entry = &e;
  }

  sort(entries, &entry_compare);
  bool check_sel = subBlk && subBlk->paramCount();
  for (i = 0; i < entries.size(); ++i)
    addEntry(entries[i].entry, ret, check_sel ? subBlk->getBool(entries[i].entry->getNameTypified(), false) : false);

  if (!parent || subBlk)
    expand(ret);

  return ret;
}


TLeafHandle AvTree::addEntry(const DagorAsset *asset, TLeafHandle parent, bool selected)
{
  String bmp(128, "asset_%s", asset->getTypeStr());

  G_ASSERT((((uintptr_t)asset) & 3) == 0); // must be at least 4-byte aligned

  TLeafHandle ret = addItem(asset->getName(), getImageIndex(bmp), parent, (void *)asset);
  if ((selected) && (!mFirstSel))
  {
    mFirstSel = ret;
    // selectItemMultiple(ret);
    // enshureVisibleItem(ret);
  }
  return ret;
}


bool AvTree::isFolderExportable(TLeafHandle parent, bool *exported)
{
  for (int i = 0; i < getChildrenCount(parent); ++i)
  {
    TLeafHandle child = getChild(parent, i);
    void *data = getItemData(child);
    if (get_dagor_asset_folder(data)) // folder
    {
      if (isFolderExportable(child))
        return true;
    }
    else // asset
    {
      DagorAsset *a = (DagorAsset *)data;
      if (a && ::is_asset_exportable(a))
      {
        if (exported)
          *exported = true;
        return true;
      }
    }
  }

  if (exported)
    *exported = false;

  return false;
}


bool AvTree::markExportedTree(TTreeNode *n, int flags)
{
  bool changed = false;
  for (int i = 0; i < n->nodes.size(); ++i)
    if (const DagorAssetFolder *g = get_dagor_asset_folder(n->nodes[i]->userData->mUserData))
    {
      int imidx = (g->flags & g->FLG_EXPORT_ASSETS) ? folder_packed_imidx : folder_imidx;

      if (markExportedTree(n->nodes[i], flags))
        imidx = folder_changed_imidx;

      if (imidx != n->nodes[i]->iconIndex && n->nodes[i]->item)
        changeItemImage(n->nodes[i]->item, imidx);
    }
    else if (DagorAsset *a = (DagorAsset *)n->nodes[i]->userData->mUserData)
    {
      int imidx = asset_imidx[a->getType() * 2];
      String bmp(128, "asset_%s", a->getTypeStr());

      if (::is_asset_exportable(a) && (a->testUserFlags(flags) != flags))
      {
        changed = true;
        imidx = asset_imidx[a->getType() * 2 + 1];
      }

      if (imidx != n->nodes[i]->iconIndex && n->nodes[i]->item)
        changeItemImage(n->nodes[i]->item, imidx);
    }
  return changed;
}


void AvTree::saveTreeData(DataBlock &blk) { scanOpenTree(getRoot(), &blk); }


void AvTree::loadSelectedItem() { setSelectedItem(mFirstSel); }


void AvTree::scanOpenTree(TLeafHandle parent, DataBlock *blk)
{
  int childCount = getChildrenCount(parent);

  if (childCount == 0 || !isOpen(parent))
    return;

  int curChildIndex = 0;

  DataBlock *subBlk = blk->addBlock(getItemName(parent));

  while (curChildIndex < childCount)
  {
    TLeafHandle child = getChild(parent, curChildIndex);
    scanOpenTree(child, subBlk);

    if (isSelected(child) && !get_dagor_asset_folder(getItemData(child)))
    {
      DagorAsset *a = (DagorAsset *)getItemData(child);
      subBlk->addBool(a->getNameTypified(), true);
    }

    ++curChildIndex;
  }
}


void AvTree::setTypeFilterToAll()
{
  clear_and_shrink(mSelectedTypesID);
  for (int i = 0; i < mDAMgr->getAssetTypesCount(); ++i)
    mSelectedTypesID.push_back(i);
}


void AvTree::resetFilter()
{
  if (mSelectedTypesID.size() == mDAMgr->getAssetTypesCount() && mFilterString.empty())
    return;

  setTypeFilterToAll();
  setCaptionFilterButton();
  setFilterString("");
}


void AvTree::setCaptionFilterButton()
{
  String newButtonName;

  if (mSelectedTypesID.size() == 0)
    newButtonName.append("None");
  else if (mSelectedTypesID.size() == mDAMgr->getAssetTypesCount())
    newButtonName.append("All");
  else
  {
    for (int i = 0; i < mSelectedTypesID.size(); ++i)
    {
      if (i < FILTER_CAPTION_MAX_TYPES_NUM)
      {
        if (i > 0)
          newButtonName.append(", ");
        newButtonName.append(mDAMgr->getAssetTypeName(mSelectedTypesID[i]));
      }
      else
      {
        newButtonName.append(", [...]");
        break;
      }
    }
  }

  mPanelFS->setText(TYPE_FILTER_BUTTON_ID, newButtonName.str());
}


void AvTree::getFilterAssetTypes(Tab<int> &types) { types = mSelectedTypesID; }


void AvTree::setFilterAssetTypes(const Tab<int> &types)
{
  if (types == mSelectedTypesID)
    return;

  mSelectedTypesID = types;

  setCaptionFilterButton();
  startFilter();
}


SimpleString AvTree::getFilterString() { return mFilterString; }


void AvTree::setFilterString(const char *filter)
{
  mPanelFS->setText(EDIT_BOX_FILTER_ID, filter);
  onFilterEditBoxChanged();
}

void AvTree::onFilterEditBoxChanged()
{
  filterTimer.stop();

  String buf(mPanelFS->getText(EDIT_BOX_FILTER_ID));
  mFilterString = buf.toLower().str();

  const bool wasBusy = wingw::set_busy(true);
  startFilter();
  if (!wasBusy)
    wingw::set_busy(false);
}

void AvTree::onChange(int pcb_id, PropertyContainerControlBase *panel)
{
  switch (pcb_id)
  {
    case EDIT_BOX_FILTER_ID:
    {
      filterTimer.stop();
      filterTimer.start();
    }
    break;
  }
}


void AvTree::onClick(int pcb_id, PropertyContainerControlBase *panel)
{
  if (pcb_id == TYPE_FILTER_BUTTON_ID)
  {
    Tab<String> allTypes(midmem);
    for (int i = 0; i < mDAMgr->getAssetTypesCount(); ++i)
      allTypes.push_back() = mDAMgr->getAssetTypeName(i);

    Tab<String> selectTypes(midmem);
    for (int i = 0; i < mSelectedTypesID.size(); ++i)
      selectTypes.push_back() = mDAMgr->getAssetTypeName(mSelectedTypesID[i]);

    MultiListDialog assetTypesDialog("Asset types filter", 250, 450, allTypes, selectTypes);
    if (assetTypesDialog.showDialog() == DIALOG_ID_OK)
    {
      Tab<int> newTypesID;
      for (int i = 0; i < selectTypes.size(); ++i)
        newTypesID.push_back() = mDAMgr->getAssetTypeId(selectTypes[i]);

      setFilterAssetTypes(newTypesID);
    }
  }
}


long AvTree::onKeyDown(int pcb_id, PropertyContainerControlBase *panel, unsigned v_key)
{
  if (pcb_id == EDIT_BOX_SEARCH_ID)
  {
    if ((v_key == wingw::V_ENTER || v_key == wingw::V_DOWN || v_key == wingw::V_UP) && !wingw::is_special_pressed())
    {
      bool forward = (v_key == wingw::V_UP) ? false : true;

      String buf(panel->getText(EDIT_BOX_SEARCH_ID));

      searchNext(buf.str(), forward);
      return 1;
    }
  }

  return 0;
}


bool AvTree::handleNodeFilter(TTreeNodeInfo &node)
{
  bool assetRes = false;
  if (!get_dagor_asset_folder(node.userData))
  {
    int typeID = ((DagorAsset *)node.userData)->getType();
    for (int i = 0; i < mSelectedTypesID.size(); ++i)
      if (mSelectedTypesID[i] == typeID)
        assetRes = true;
  }

  bool strRes = (mFilterString == "");
  if (!strRes)
    strRes = strstr(String(node.name).toLower().str(), mFilterString.str());

  return assetRes && strRes;
}

void AvTree::update() { onFilterEditBoxChanged(); }

void AvTree::searchNext(const char *text, bool forward)
{
  TLeafHandle sel = getSelectedItem() ? getSelectedItem() : getRoot();
  if (!sel)
    return;

  TLeafHandle next = sel;
  TLeafHandle first = 0;
  for (;;)
  {
    next = search(text, next, forward);
    if (!next || (next == first))
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
bool AvTree::selectAsset(const DagorAsset *a)
{
  TLeafHandle sel = getRoot();
  if (!sel || !a)
    return false;

  TLeafHandle next = sel;
  TLeafHandle first = 0;
  for (;;)
  {
    next = getNextNode(next, true);
    if (!next || (next == first))
      return false;
    if (!first)
      first = next;

    void *d = getItemData(next);
    if (a == d && !get_dagor_asset_folder(d))
    {
      setSelectedItem(next);
      return true;
    }
  }
}
