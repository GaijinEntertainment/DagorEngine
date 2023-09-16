// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_tree.h"

#include <windows.h>
#include <commctrl.h>

static const int STR_SIZE = 64;
static const DWORD UNFOCUSED_SELECTION_BG_COLOR_DEFAULT = RGB(240, 240, 240);
static const DWORD UNFOCUSED_SELECTION_BG_COLOR_EXPLORER = RGB(217, 217, 217);


inline static TLeafHandle insertTreeItemAfter(HWND hwnd, const char *name, int icon, TLeafHandle parent, TTreeNode *data,
  HTREEITEM insert_after)
{
  TVINSERTSTRUCT insert = {0};
  insert.hParent = (HTREEITEM)parent;
  insert.hInsertAfter = insert_after;
  insert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE;
  insert.item.pszText = (LPTSTR)name;
  insert.item.iImage = icon;
  insert.item.iSelectedImage = icon;
  insert.item.lParam = (LPARAM)data;

  return data->item = (TLeafHandle)TreeView_InsertItem(hwnd, &insert);
}


WTreeView::WTreeView(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :
  WindowControlBase(event_handler, parent, WC_TREEVIEW, 0,
    WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS, "", x, y, w, h),
  mPicList(NULL),
  mStatePicList(nullptr),
  mLeafParams(midmem),
  fakeChildren(midmem)
{
  setFont(WindowBase::getSmallPrettyFont());

  mShowPopupMenu = false;
  mMenu = IMenu::createPopupMenu(mHandle);

  mRootNode = new TTreeNode(0, -1, 0);
  mSelectedNode = NULL;
  scrollParentFirst = false;
}


WTreeView::~WTreeView()
{
  clear();
  del_it(mMenu);
}


void WTreeView::clear()
{
  TreeView_DeleteAllItems((HWND)mHandle);
  clear_all_ptr_items(mLeafParams);
  del_it(mRootNode);
  mRootNode = new TTreeNode(0, -1, 0);
  mSelectedNode = NULL;
}


TLeafHandle WTreeView::addItem(const char *caption, int icon_index, TLeafHandle parent, void *user_data)
{
  TLeafParam *param = new TLeafParam(user_data);
  mLeafParams.push_back(param);

  TTreeNode *node = new TTreeNode(caption, icon_index, param);

  TLeafHandle item = insertTreeItem(caption, icon_index, parent, node);
  if (!item)
  {
    del_it(node);
    return 0;
  }

  if (parent)
  {
    TTreeNode *pnode = getItemNode(parent);
    if (pnode)
      pnode->nodes.push_back(node);
  }
  else
    mRootNode->nodes.push_back(node);

  return item;
}


TLeafHandle WTreeView::addItemAsFirst(const char *caption, int icon_index, TLeafHandle parent, void *user_data)
{
  TLeafParam *param = new TLeafParam(user_data);
  mLeafParams.push_back(param);

  TTreeNode *node = new TTreeNode(caption, icon_index, param);

  TLeafHandle item = insertTreeItemAfter((HWND)getHandle(), caption, icon_index, parent, node, TVI_FIRST);
  if (!item)
  {
    del_it(node);
    return 0;
  }

  if (parent)
  {
    TTreeNode *pnode = getItemNode(parent);
    if (pnode)
      pnode->nodes.insert(pnode->nodes.begin(), node);
  }
  else
    mRootNode->nodes.insert(mRootNode->nodes.begin(), node);

  return item;
}


TLeafHandle WTreeView::insertTreeItem(const char *name, int icon, TLeafHandle parent, TTreeNode *data)
{
  return insertTreeItemAfter((HWND)getHandle(), name, icon, parent, data, TVI_LAST);
}


TTreeNode *WTreeView::getItemNode(TLeafHandle item) const
{
  if (!item)
    return NULL;
  TVITEM itemInfo = {0};
  itemInfo.mask = TVIF_HANDLE | TVIF_PARAM;
  itemInfo.hItem = (HTREEITEM)item;
  if (TreeView_GetItem((HWND)mHandle, &itemInfo))
    return (TTreeNode *)itemInfo.lParam;

  return NULL;
}


TLeafParam *WTreeView::getItemParam(TLeafHandle item) const
{
  TTreeNode *node = getItemNode(item);
  if (node)
    return (TLeafParam *)node->userData;

  return NULL;
}


const Tab<String> &WTreeView::getItemChildren(TLeafHandle item)
{
  TLeafParam *param = getItemParam(item);
  if (param)
    return param->mChildren;

  return fakeChildren;
}


void WTreeView::addItemChild(const char *name, TLeafHandle parent)
{
  TLeafParam *param = getItemParam(parent);
  if (param)
    param->mChildren.push_back() = name;
}


void WTreeView::setPictureList(PictureList *pictureList)
{
  mPicList = pictureList;
  SendMessage((HWND)mHandle, TVM_SETIMAGELIST, 0, (pictureList) ? (LPARAM)pictureList->getHandle() : 0);
}


void WTreeView::setStatePictureList(PictureList *pictureList)
{
  mStatePicList = pictureList;
  TreeView_SetImageList((HWND)mHandle, pictureList ? (HIMAGELIST)pictureList->getHandle() : 0, TVSIL_STATE);
}


void WTreeView::filter(void *param, TTreeNodeFilterFunc filter_func)
{
  TreeView_DeleteAllItems((HWND)mHandle);
  filterRoutine(mRootNode, 0, param, filter_func);
}


bool WTreeView::filterRoutine(TTreeNode *node, TLeafHandle parent, void *param, TTreeNodeFilterFunc filter_func)
{
  bool notEmpty = false;

  for (int i = 0; i < node->nodes.size(); ++i)
  {
    TTreeNode &nd = *node->nodes[i];

    G_ASSERT(filter_func);
    bool isVisible = filter_func(param, nd);

    nd.item = NULL;
    if (!isVisible && !nd.nodes.size())
      continue;

    TLeafHandle item = insertTreeItem(nd.name.str(), nd.iconIndex, parent, &nd);

    if (nd.nodes.size())
    {
      bool res = filterRoutine(&nd, item, param, filter_func);

      if (!res && !isVisible)
      {
        nd.item = NULL;
        TreeView_DeleteItem((HWND)getHandle(), (HTREEITEM)item);
        continue;
      }
    }

    if (&nd == mSelectedNode)
      TreeView_Select((HWND)getHandle(), (HTREEITEM)item, TVGN_CARET);

    if (nd.isExpand)
      TreeView_Expand((HWND)getHandle(), (HTREEITEM)item, TVE_EXPAND);

    notEmpty = true;
  }

  return notEmpty;
}


TLeafHandle WTreeView::search(const char *text, TLeafHandle first, bool forward)
{
  String searchText = String(text).toLower();
  TLeafHandle next = first;

  for (;;)
  {
    next = forward ? getNextNode(next) : getPrevNode(next);
    if (!next || (next == first))
      return 0;

    char buffer[STR_SIZE];
    TV_ITEM info = {0};
    info.hItem = (HTREEITEM)next;
    info.pszText = buffer;
    info.cchTextMax = STR_SIZE;
    info.mask = TVIF_TEXT;
    TreeView_GetItem((HWND)mHandle, &info);

    if (strstr(String(buffer).toLower().str(), searchText.str()))
      return next;
  }

  return 0;
}


TLeafHandle WTreeView::getSelectedItem() const { return (TLeafHandle)TreeView_GetSelection((HWND)mHandle); }


bool WTreeView::setSelectedItem(TLeafHandle item)
{
  bool is_ok = TreeView_Select((HWND)mHandle, item, TVGN_CARET);
  mSelectedNode = getItemNode(item);

  return is_ok;
}


intptr_t WTreeView::onControlNotify(void *info)
{
  LPNMHDR data = (LPNMHDR)info;
  switch (data->code)
  {
    case TVN_SELCHANGED:
    {
      NM_TREEVIEW *leaf_info = (NM_TREEVIEW *)data;
      TTreeNode *newSel = getItemNode((TLeafHandle)leaf_info->itemNew.hItem);
      if (newSel == mSelectedNode)
        break;
      mSelectedNode = newSel;
      mEventHandler->onWcChange(this);
    }
    break;

    case TVN_ITEMEXPANDED:
    {
      NM_TREEVIEW *leaf_info = (NM_TREEVIEW *)data;
      TTreeNode *node = getItemNode((TLeafHandle)leaf_info->itemNew.hItem);
      if (node)
        node->isExpand = leaf_info->itemNew.state & TVIS_EXPANDED;
    }
    break;

    case NM_CLICK:
    case NM_RCLICK:
    {
      POINT pt;
      GetCursorPos(&pt);
      ScreenToClient((HWND)mHandle, &pt);

      TVHITTESTINFO ht = {0};
      ht.pt = pt;
      HTREEITEM item = TreeView_HitTest((HWND)mHandle, &ht);

      if (NM_RCLICK == data->code || (NM_CLICK == data->code && !(ht.flags & TVHT_ONITEMBUTTON)))
        TreeView_SelectItem((HWND)mHandle, item);

      if ((ht.flags & TVHT_ONITEM) && NM_RCLICK == data->code)
      {
        mEventHandler->onWcClick(this);
        if (mShowPopupMenu)
          mMenu->showPopupMenu();
      }
    }
    break;

    case NM_DBLCLK: mEventHandler->onWcDoubleClick(this); break;

    case NM_CUSTOMDRAW:
    {
      NMTVCUSTOMDRAW *tvcd = (NMTVCUSTOMDRAW *)info;
      if (tvcd)
        switch (tvcd->nmcd.dwDrawStage)
        {
          case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW;

          case CDDS_ITEMPREPAINT:
          {
            TTreeNode *node = getItemNode((TLeafHandle)tvcd->nmcd.dwItemSpec);
            if (node && node->textColor != E3DCOLOR(0, 0, 0))
              tvcd->clrText = RGB(node->textColor.r, node->textColor.g, node->textColor.b);

            if (node == mSelectedNode && GetFocus() != (HWND)mHandle &&
                GetSysColor(COLOR_BTNFACE) == UNFOCUSED_SELECTION_BG_COLOR_DEFAULT)
              tvcd->clrTextBk = UNFOCUSED_SELECTION_BG_COLOR_EXPLORER;

            return CDRF_NEWFONT;
          }
        }
    }
    break;
  }

  return 0;
}


intptr_t WTreeView::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!notify_code && mMenu)
  {
    mMenu->click(elem_id);
  }

  return 1;
}


void WTreeView::changeItemImage(TLeafHandle item, int new_item_index)
{
  TVITEM item_info;
  item_info.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
  item_info.iImage = item_info.iSelectedImage = new_item_index;
  item_info.hItem = (HTREEITEM)item;
  TreeView_SetItem((HWND)mHandle, &item_info);

  TTreeNode *node = getItemNode(item);
  if (node)
    node->iconIndex = new_item_index;
}


void WTreeView::changeItemStateImage(TLeafHandle item, int new_index)
{
  TVITEM item_info;
  item_info.mask = TVIF_HANDLE | TVIF_STATE;
  item_info.hItem = (HTREEITEM)item;
  item_info.stateMask = TVIS_STATEIMAGEMASK;
  item_info.state = INDEXTOSTATEIMAGEMASK(new_index);
  TreeView_SetItem((HWND)mHandle, &item_info);
}


int WTreeView::getChildrenCount(TLeafHandle item) const
{
  HTREEITEM first_child = TreeView_GetChild((HWND)mHandle, item);
  if (!first_child)
    return 0;

  int count = 1;
  HTREEITEM next_child = first_child;
  while (next_child)
  {
    count++;
    next_child = TreeView_GetNextSibling((HWND)mHandle, next_child);
  }
  return count - 1;
}


TLeafHandle WTreeView::getChild(TLeafHandle item, int child_index)
{
  HTREEITEM first_child = TreeView_GetChild((HWND)mHandle, item);
  if (!first_child)
    return 0;

  int index = 0;
  HTREEITEM next_child = first_child;
  while (next_child)
  {
    if (index == child_index)
      return (TLeafHandle)next_child;

    next_child = TreeView_GetNextSibling((HWND)mHandle, next_child);
    index++;
  }
  return 0;
}


TLeafHandle WTreeView::getParentNode(TLeafHandle item)
{
  HTREEITEM parent = TreeView_GetParent((HWND)mHandle, item);
  return (TLeafHandle)parent;
}


TLeafHandle WTreeView::getNextNode(TLeafHandle item)
{
  if (!item)
    return 0;

  HTREEITEM child = TreeView_GetChild((HWND)mHandle, item);
  if (child)
    return (TLeafHandle)child;

  HTREEITEM sibling = TreeView_GetNextSibling((HWND)mHandle, item);
  if (sibling)
    return (TLeafHandle)sibling;

  for (;;)
  {
    HTREEITEM parent = TreeView_GetParent((HWND)mHandle, item);
    if (!parent)
      return (TLeafHandle)TreeView_GetRoot((HWND)mHandle);

    HTREEITEM siblingParent = TreeView_GetNextSibling((HWND)mHandle, parent);
    if (siblingParent)
      return (TLeafHandle)siblingParent;

    item = (TLeafHandle)parent;
  }
}


TLeafHandle WTreeView::getLastSubNode(TLeafHandle item)
{
  if (!item)
    return 0;

  HTREEITEM child = TreeView_GetChild((HWND)mHandle, item);
  if (!child)
    return item;

  while (HTREEITEM siblingChild = TreeView_GetNextSibling((HWND)mHandle, child))
    child = siblingChild;

  return getLastSubNode((TLeafHandle)child);
}


TLeafHandle WTreeView::getPrevNode(TLeafHandle item)
{
  if (!item)
    return 0;

  HTREEITEM sibling = TreeView_GetPrevSibling((HWND)mHandle, item);
  if (sibling)
    return getLastSubNode((TLeafHandle)sibling);

  HTREEITEM parent = TreeView_GetParent((HWND)mHandle, item);
  if (parent)
    return (TLeafHandle)parent;

  return getLastSubNode(getRoot());
}


String WTreeView::getItemName(TLeafHandle item) const
{
  char buffer[STR_SIZE];
  TVITEM item_info;
  item_info.mask = TVIF_HANDLE | TVIF_TEXT;
  item_info.hItem = (HTREEITEM)item;
  item_info.pszText = buffer;
  item_info.cchTextMax = STR_SIZE;
  TreeView_GetItem((HWND)mHandle, &item_info);
  return String(buffer);
}


void WTreeView::setItemName(TLeafHandle item, char name[])
{
  TVITEM item_info;
  item_info.mask = TVIF_HANDLE | TVIF_TEXT;
  item_info.hItem = (HTREEITEM)item;
  item_info.pszText = name;
  item_info.cchTextMax = i_strlen(name);
  TreeView_SetItem((HWND)mHandle, &item_info);
}


void *WTreeView::getItemData(TLeafHandle item) const
{
  TLeafParam *param = getItemParam(item);
  if (param)
    return param->mUserData;

  return NULL;
}


void WTreeView::setItemData(TLeafHandle item, void *data)
{
  TLeafParam *param = getItemParam(item);
  if (param)
    param->mUserData = data;
}


void WTreeView::setItemColor(TLeafHandle item, E3DCOLOR value)
{
  TTreeNode *node = getItemNode(item);
  if (node)
    node->textColor = value;
}


TLeafHandle WTreeView::getRoot() { return (TLeafHandle)TreeView_GetRoot((HWND)mHandle); }


bool WTreeView::remove(TLeafHandle item)
{
  TTreeNode *node = getItemNode(item);
  TLeafParam *param = getItemParam(item);
  HTREEITEM parent_item = TreeView_GetParent((HWND)mHandle, item);
  TTreeNode *parent_node = (parent_item) ? getItemNode((TLeafHandle)parent_item) : mRootNode;

  node->item = NULL;
  if (TreeView_DeleteItem((HWND)mHandle, item))
  {
    // remove node from parent list
    if (parent_node)
      for (int i = 0; i < parent_node->nodes.size(); ++i)
        if (parent_node->nodes[i] == node)
        {
          erase_items(parent_node->nodes, i, 1);
          break;
        }
    del_it(node);

    // remove param from params list
    for (int i = 0; i < mLeafParams.size(); ++i)
      if (mLeafParams[i] == param)
      {
        erase_items(mLeafParams, i, 1);
        break;
      }
    del_it(param);
    return true;
  }
  return false;
}


bool WTreeView::swapLeafs(TLeafHandle item, bool with_prior)
{
  HTREEITEM prior_item = with_prior ? TreeView_GetPrevSibling((HWND)mHandle, item) : TreeView_GetNextSibling((HWND)mHandle, item);
  if (!prior_item)
    return false;

  char buffer[STR_SIZE], p_buffer[STR_SIZE];
  TVITEM item_info, p_item_info;

  item_info.mask = p_item_info.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
  item_info.cchTextMax = p_item_info.cchTextMax = STR_SIZE;

  item_info.hItem = (HTREEITEM)item;
  p_item_info.hItem = prior_item;

  item_info.pszText = buffer;
  p_item_info.pszText = p_buffer;

  TreeView_GetItem((HWND)mHandle, &item_info);
  TreeView_GetItem((HWND)mHandle, &p_item_info);

  p_item_info.hItem = (HTREEITEM)item;
  item_info.hItem = prior_item;

  TreeView_SetItem((HWND)mHandle, &item_info);
  TreeView_SetItem((HWND)mHandle, &p_item_info);

  if (getSelectedItem() == item)
    setSelectedItem((TLeafHandle)prior_item);

  refresh(false);
  return true;
}


void WTreeView::collapse(TLeafHandle item)
{
  TreeView_Expand((HWND)mHandle, item, TVE_COLLAPSE);

  TTreeNode *node = getItemNode(item);
  if (node)
    node->isExpand = false;
}


void WTreeView::expand(TLeafHandle item)
{
  TreeView_Expand((HWND)mHandle, item, TVE_EXPAND);

  TTreeNode *node = getItemNode(item);
  if (node)
    node->isExpand = true;
}

void WTreeView::ensureVisible(TLeafHandle item) { TreeView_EnsureVisible((HWND)mHandle, item); }

bool WTreeView::isOpen(TLeafHandle item) const
{
  TVITEM tvi;
  ::memset(&tvi, 0, sizeof(tvi));

  tvi.mask = TVIF_STATE;
  tvi.hItem = (HTREEITEM)item;
  tvi.stateMask = 0xFF;

  if (TreeView_GetItem((HWND)mHandle, &tvi))
    return tvi.state & TVIS_EXPANDED;

  return false;
}


bool WTreeView::isSelected(TLeafHandle item) const
{
  TVITEM tvi;
  ::memset(&tvi, 0, sizeof(tvi));

  tvi.mask = TVIF_STATE;
  tvi.hItem = (HTREEITEM)item;
  tvi.stateMask = 0xFF;

  if (TreeView_GetItem((HWND)mHandle, &tvi))
    return tvi.state & TVIS_SELECTED;

  return false;
}


// ---------------------------------------------------


PictureList::PictureList(int width, int height, int size)
{
  mImageList = ImageList_Create(width, height, ILC_COLOR32 | ILC_MASK, size, 16);
  if (!mImageList)
    debug("ERROR: ImageList cannot be created");
}


PictureList::~PictureList() { ImageList_Destroy((HIMAGELIST)mImageList); }


int PictureList::addImage(const char *file_name)
{
  HBITMAP bitmap = (HBITMAP)load_bmp_picture(file_name);
  if (!bitmap)
  {
    debug("ERROR: cannot load bitmap \"%s\" ", file_name);
    return -1;
  }

  // select first pixel for transparent

  COLORREF t_color = get_alpha_color(bitmap);

  int ind = ImageList_AddMasked((HIMAGELIST)mImageList, bitmap, t_color);
  if (ind == -1)
  {
    debug("ERROR: cannot add bitmap to image list \"%s\" ", file_name);
  }

  DeleteObject(bitmap);
  return ind;
}
