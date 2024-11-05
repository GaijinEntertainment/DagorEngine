// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_tree.h"
#include "av_appwnd.h"
#include "av_plugin.h"
#include "assetBuildCache.h"

#include <generic/dag_sort.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetFolder.h>
#include <debug/dag_debug.h>

#include <libTools/util/strUtil.h>
#include <util/dag_string.h>

#include <gui/dag_imguiUtil.h>
#include <propPanel/commonWindow/multiListDialog.h>
#include <propPanel/control/container.h>
#include <propPanel/imguiHelper.h>
#include <winGuiWrapper/wgw_busy.h>
#include <winGuiWrapper/wgw_input.h>

using hdpi::_pxActual;
using hdpi::_pxScaled;

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

static TEXTUREID folder_textureId = BAD_TEXTUREID;
static TEXTUREID folder_packed_textureId = BAD_TEXTUREID;
static TEXTUREID folder_changed_textureId = BAD_TEXTUREID;
static Tab<TEXTUREID> asset_textureId(inimem);

AvTree::AvTree(PropPanel::ITreeViewEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h,
  const char caption[]) :
  TreeViewWindow(event_handler, phandle, x, y, _pxActual(w), _pxActual(h), _pxScaled(FS_HEIGHT), caption),
  mImages(midmem),
  mFirstSel(NULL),
  mSelectedTypesID(midmem),
  filterTimer(this, FILTER_START_DELAY_MS, false)
{
  mPanelFS.reset(new PropPanel::ContainerPropertyControl(0, this, nullptr, 0, 0, hdpi::Px(0), hdpi::Px(0)));
  mPanelFS->createButton(TYPE_FILTER_BUTTON_ID, "");
  mPanelFS->createEditBox(EDIT_BOX_FILTER_ID, "Filter:");
  mPanelFS->createEditBox(EDIT_BOX_SEARCH_ID, "Search:");

  mPanelFS->setTooltipId(EDIT_BOX_FILTER_ID, TreeViewWindow::tooltipWildcard);
  static const String tooltipWildcardAndSearch(0, "%s\n%s", TreeViewWindow::tooltipWildcard, TreeViewWindow::tooltipSearch);
  mPanelFS->setTooltipId(EDIT_BOX_SEARCH_ID, tooltipWildcardAndSearch);
}


AvTree::~AvTree() {}


TEXTUREID AvTree::getIconTextureId(const char *image_file_name)
{
  for (int i = 0; i < mImages.size(); ++i)
    if (mImages[i].mName == image_file_name)
      return mImages[i].mId;

  TEXTUREID newId = addImage(image_file_name);
  mImages.push_back(ImageIndex(image_file_name, newId));
  return newId;
}


void AvTree::fill(DagorAssetMgr *mgr, const DataBlock &set_blk)
{
  folder_textureId = getIconTextureId("folder");
  folder_packed_textureId = getIconTextureId("folder_packed");
  folder_changed_textureId = getIconTextureId("folder_changed");
  asset_textureId.resize(mgr->getAssetTypesCount() * 2);
  for (int i = 0; i < mgr->getAssetTypesCount(); i++)
  {
    asset_textureId[i * 2 + 0] = getIconTextureId(String(0, "asset_%s", mgr->getAssetTypeName(i)));
    asset_textureId[i * 2 + 1] = getIconTextureId(String(0, "asset_%s_changed", mgr->getAssetTypeName(i)));
  }

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


PropPanel::TLeafHandle AvTree::addGroup(int folder_idx, PropPanel::TLeafHandle parent, const DataBlock *blk)
{
  const DagorAssetFolder *folder = mDAMgr->getFolderPtr(folder_idx);
  if (!folder)
    return NULL;

  G_ASSERT((((uintptr_t)folder) & 3) == 0); // must be at least 4-byte aligned

  PropPanel::TLeafHandle ret =
    addItem(folder->folderName, folder_textureId, parent, (void *)(((uintptr_t)folder) | IS_DAGOR_ASSET_FOLDER));

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


PropPanel::TLeafHandle AvTree::addEntry(const DagorAsset *asset, PropPanel::TLeafHandle parent, bool selected)
{
  G_ASSERT((((uintptr_t)asset) & 3) == 0); // must be at least 4-byte aligned

  TEXTUREID icon = asset_textureId[asset->getType() * 2];
  PropPanel::TLeafHandle ret = addItem(asset->getName(), icon, parent, (void *)asset);
  if ((selected) && (!mFirstSel))
  {
    mFirstSel = ret;
    // selectItemMultiple(ret);
    // enshureVisibleItem(ret);
  }
  return ret;
}


bool AvTree::isFolderExportable(PropPanel::TLeafHandle parent, bool *exported)
{
  for (int i = 0; i < getChildrenCount(parent); ++i)
  {
    PropPanel::TLeafHandle child = getChild(parent, i);
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


bool AvTree::markExportedTree(PropPanel::TTreeNode *n, int flags)
{
  bool changed = false;
  for (int i = 0; i < n->nodes.size(); ++i)
    if (const DagorAssetFolder *g = get_dagor_asset_folder(n->nodes[i]->userData))
    {
      TEXTUREID imidx = (g->flags & g->FLG_EXPORT_ASSETS) ? folder_packed_textureId : folder_textureId;

      if (markExportedTree(n->nodes[i], flags))
        imidx = folder_changed_textureId;

      if (imidx != n->nodes[i]->icon && n->nodes[i]->item)
        changeItemImage(n->nodes[i]->item, imidx);
    }
    else if (DagorAsset *a = (DagorAsset *)n->nodes[i]->userData)
    {
      TEXTUREID imidx = asset_textureId[a->getType() * 2];

      if (::is_asset_exportable(a) && (a->testUserFlags(flags) != flags))
      {
        changed = true;
        imidx = asset_textureId[a->getType() * 2 + 1];
      }

      if (imidx != n->nodes[i]->icon && n->nodes[i]->item)
        changeItemImage(n->nodes[i]->item, imidx);
    }
  return changed;
}


void AvTree::saveTreeData(DataBlock &blk) { scanOpenTree(getRoot(), &blk); }


void AvTree::loadSelectedItem() { setSelectedItem(mFirstSel); }


void AvTree::scanOpenTree(PropPanel::TLeafHandle parent, DataBlock *blk)
{
  int childCount = getChildrenCount(parent);

  if (childCount == 0 || !isOpen(parent))
    return;

  int curChildIndex = 0;

  DataBlock *subBlk = blk->addBlock(getItemName(parent));

  while (curChildIndex < childCount)
  {
    PropPanel::TLeafHandle child = getChild(parent, curChildIndex);
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
  filterHasWildcardCharacter = str_has_wildcard_character(mFilterString);

  const bool wasBusy = wingw::set_busy(true);
  startFilter();
  if (!wasBusy)
    wingw::set_busy(false);
}

void AvTree::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
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


void AvTree::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == TYPE_FILTER_BUTTON_ID)
  {
    Tab<String> allTypes(midmem);
    for (int i = 0; i < mDAMgr->getAssetTypesCount(); ++i)
      allTypes.push_back() = mDAMgr->getAssetTypeName(i);

    Tab<String> selectTypes(midmem);
    for (int i = 0; i < mSelectedTypesID.size(); ++i)
      selectTypes.push_back() = mDAMgr->getAssetTypeName(mSelectedTypesID[i]);

    PropPanel::MultiListDialog assetTypesDialog("Asset types filter", _pxScaled(250), _pxScaled(450), allTypes, selectTypes);
    if (assetTypesDialog.showDialog() == PropPanel::DIALOG_ID_OK)
    {
      Tab<int> newTypesID;
      for (int i = 0; i < selectTypes.size(); ++i)
        newTypesID.push_back() = mDAMgr->getAssetTypeId(selectTypes[i]);

      setFilterAssetTypes(newTypesID);
    }
  }
}


long AvTree::onKeyDown(int pcb_id, PropPanel::ContainerPropertyControl *panel, unsigned v_key)
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


bool AvTree::handleNodeFilter(const PropPanel::TTreeNode &node)
{
  bool assetRes = false;
  if (!get_dagor_asset_folder(node.userData))
  {
    int typeID = ((DagorAsset *)node.userData)->getType();
    for (int i = 0; i < mSelectedTypesID.size(); ++i)
      if (mSelectedTypesID[i] == typeID)
        assetRes = true;
  }

  if (!assetRes || mFilterString.empty())
    return assetRes;

  if (filterHasWildcardCharacter)
    return str_matches_wildcard_pattern(String(node.name).toLower().str(), mFilterString.str());
  else
    return strstr(String(node.name).toLower().str(), mFilterString.str());
}

void AvTree::update() { onFilterEditBoxChanged(); }

void AvTree::searchNext(const char *text, bool forward)
{
  PropPanel::TLeafHandle sel = getSelectedItem() ? getSelectedItem() : getRoot();
  if (!sel)
    return;

  const bool useWildcardSearch = str_has_wildcard_character(text);

  PropPanel::TLeafHandle next = sel;
  PropPanel::TLeafHandle first = 0;
  for (;;)
  {
    next = search(text, next, forward, useWildcardSearch);
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
  PropPanel::TLeafHandle sel = getRoot();
  if (!sel || !a)
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

    void *d = getItemData(next);
    if (a == d && !get_dagor_asset_folder(d))
    {
      setSelectedItem(next);
      return true;
    }
  }
}

void AvTree::updateImgui(float control_height)
{
  if (control_height <= 0.0f)
    control_height = ImGui::GetContentRegionAvail().y;

  // TODO: ImGui porting: AV: implement a generic sizing logic. With this the size will not be perfect in the first frame.
  const float panelStart = ImGui::GetCursorPosY();
  const float treeHeight = max(100.0f, control_height - panelLastHeight);

  TreeBaseWindow::updateImgui(treeHeight);

  mPanelFS->updateImgui();
  panelLastHeight = PropPanel::ImguiHelper::getCursorPosYWithoutLastItemSpacing() - panelStart - treeHeight;
}