// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <propPanel2/comWnd/panel_window.h>
#include <propPanel2/comWnd/treeview_panel.h>
#include <propPanel2/comWnd/list_dialog.h>

#include <winGuiWrapper/wgw_input.h>

#include "../windowControls/w_tree.h"
#include "../windowControls/w_boxes.h"
#include "../windowControls/w_simple_controls.h"
#include "../windowControls/w_window.h"

#include <windows.h>


TreeBaseWindow::TreeBaseWindow(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, hdpi::Px w, hdpi::Px h,
  const char *caption, bool icons_show, bool state_icons_show) :
  mTreeWnd(new WWindow(this, phandle, x, y, _px(w), _px(h), caption, true)),
  mTree(new WTreeView(this, mTreeWnd, 0, 0, _px(w), _px(h))),
  mPicList(new PictureList(_pxS(16), _pxS(16))),
  mStatePicList(state_icons_show ? new PictureList(_pxS(16), _pxS(16)) : nullptr),
  mEventHandler(event_handler)
{
  mTree->setPictureList(icons_show ? mPicList : NULL);
  mTree->setStatePictureList(mStatePicList);
  mTree->setFont(WindowBase::getSmallPrettyFont());

  mTreeWnd->show();
}


TreeBaseWindow::~TreeBaseWindow()
{
  del_it(mPicList);
  del_it(mStatePicList);
  del_it(mTree);
  del_it(mTreeWnd);
}


void TreeBaseWindow::onWcChange(WindowBase *source)
{
  if (source == mTree && mEventHandler)
  {
    mEventHandler->onTvSelectionChange(*this, mTree->getSelectedItem());
  }
}


void TreeBaseWindow::onWcClick(WindowBase *source)
{
  if (source == mTree && mEventHandler)
  {
    IMenu *menu = mTree->getPopupMenu();
    G_ASSERT(menu && "TreeBaseWindow::onClick(): tree menu == NULL!");

    menu->clearMenu(ROOT_MENU_ITEM);

    bool showMenu = mEventHandler->onTvContextMenu(*this, mTree->getSelectedItem(), *menu);

    mTree->setPopupMenuShow(showMenu);
  }
}


void TreeBaseWindow::onWcDoubleClick(WindowBase *source)
{
  if (source == mTree && mEventHandler)
    mEventHandler->onTvDClick(*this, mTree->getSelectedItem());
}


int TreeBaseWindow::addImage(const char *filename) { return mPicList->addImage(filename); }


int TreeBaseWindow::addStateImage(const char *filename)
{
  G_ASSERT(mStatePicList);
  return mStatePicList ? mStatePicList->addImage(filename) : 0;
}


TLeafHandle TreeBaseWindow::addItem(const char *name, int icon_index, TLeafHandle parent, void *user_data)
{
  return mTree->addItem(name, icon_index, parent, user_data);
}


TLeafHandle TreeBaseWindow::addItemAsFirst(const char *name, int icon_index, TLeafHandle parent, void *user_data)
{
  return mTree->addItemAsFirst(name, icon_index, parent, user_data);
}


void TreeBaseWindow::removeItem(TLeafHandle item) { mTree->remove(item); }


void TreeBaseWindow::addChild(const char *name, TLeafHandle parent) { mTree->addItemChild(name, parent); }


void *TreeBaseWindow::getHandle() { return mTreeWnd->getHandle(); }


void *TreeBaseWindow::getParentWindowHandle() { return mTreeWnd->getParentHandle(); }


void TreeBaseWindow::changeItemImage(TLeafHandle item, int new_index) { mTree->changeItemImage(item, new_index); }


void TreeBaseWindow::changeItemStateImage(TLeafHandle item, int new_index) { mTree->changeItemStateImage(item, new_index); }


int TreeBaseWindow::getChildrenCount(TLeafHandle parent) { return mTree->getChildrenCount(parent); }


TLeafHandle TreeBaseWindow::getNextNode(TLeafHandle item, bool forward)
{
  return forward ? mTree->getNextNode(item) : mTree->getPrevNode(item);
}


TLeafHandle TreeBaseWindow::getChild(TLeafHandle parent, int i) { return mTree->getChild(parent, i); }


TTreeNode *TreeBaseWindow::getItemNode(TLeafHandle p) { return mTree->getItemNode(p); }

String TreeBaseWindow::getItemName(TLeafHandle item) { return mTree->getItemName(item); }


bool TreeBaseWindow::isOpen(TLeafHandle item) const { return mTree->isOpen(item); }


bool TreeBaseWindow::isSelected(TLeafHandle item) const { return mTree->isSelected(item); }


void *TreeBaseWindow::getItemData(TLeafHandle item) { return mTree->getItemData(item); }


bool TreeBaseWindowFilterStub(void *param, TTreeNode &node)
{
  TreeBaseWindow *tree = (TreeBaseWindow *)param;

  TTreeNodeInfo nodeInfo;
  nodeInfo.name = node.name.str();
  nodeInfo.iconIndex = node.iconIndex;
  nodeInfo.userData = 0;

  TLeafParam *leafParam = static_cast<TLeafParam *>(node.userData);
  for (int i = 0; i < leafParam->mChildren.size(); ++i)
    nodeInfo.mChildren.push_back(leafParam->mChildren[i]);

  if (node.userData)
    nodeInfo.userData = ((TLeafParam *)node.userData)->mUserData;

  return tree->handleNodeFilter(nodeInfo);
}


void TreeBaseWindow::startFilter() { mTree->filter(this, TreeBaseWindowFilterStub); }


void TreeBaseWindow::clear() { mTree->clear(); }


TLeafHandle TreeBaseWindow::search(const char *text, TLeafHandle first, bool forward) { return mTree->search(text, first, forward); }


void TreeBaseWindow::expand(TLeafHandle parent) { return mTree->expand(parent); }


void TreeBaseWindow::collapse(TLeafHandle item) { return mTree->collapse(item); }


void TreeBaseWindow::ensureVisible(TLeafHandle item) { return mTree->ensureVisible(item); }


TLeafHandle TreeBaseWindow::getRoot() { return mTree->getRoot(); }


TLeafHandle TreeBaseWindow::getSelectedItem() { return mTree->getSelectedItem(); }


bool TreeBaseWindow::setSelectedItem(TLeafHandle item) { return mTree->setSelectedItem(item); }


void TreeBaseWindow::show(bool is_visible) { is_visible ? mTree->show() : mTree->hide(); }


void TreeBaseWindow::resize(int width, int height) { onWmEmbeddedResize(width, height); }


void TreeBaseWindow::setFocus() { mTree->setFocus(); }


void TreeBaseWindow::resizeTree(int x, int y, int w, int h)
{
  mTree->moveWindow(x, y);
  mTree->resizeWindow(w, h);
}


void TreeBaseWindow::resizeBack(int x, int y, int w, int h)
{
  mTreeWnd->moveWindow(x, y);
  mTreeWnd->resizeWindow(w, h);
}


void TreeBaseWindow::onWmEmbeddedResize(int width, int height)
{
  resizeBack(mTreeWnd->getX(), mTreeWnd->getY(), width, height);
  resizeTree(mTree->getX(), mTree->getY(), width, height);
}


//=============================================================================


TreeViewWindow::TreeViewWindow(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, hdpi::Px w, hdpi::Px h, hdpi::Px ph,
  const char *caption, bool icons_show) :
  TreeBaseWindow(event_handler, phandle, x, y, w, h, caption, icons_show),
  mPanelFS(new CPanelWindow(this, getHandle(), x, _px(h - ph), w, ph, caption))
{
  resizeTree(0, 0, _px(w), _px(h - ph));
}


void TreeViewWindow::onWmEmbeddedResize(int width, int height)
{
  resizeBack(mTreeWnd->getX(), mTreeWnd->getY(), width, height);
  resizeTree(mTree->getX(), mTree->getY(), width, height - mPanelFS->getHeight());

  mPanelFS->moveTo(0, height - mPanelFS->getHeight());
  mPanelFS->setWidth(_pxActual(width));
}


//=============================================================================
const unsigned PANEL_HEIGHT = 130 + 40;
const unsigned PANEL_HEIGHT_WITHOUT_BUTTON = 97 + 40;

enum
{
  FILTER_CAPTION_MAX_TYPES_NUM = 5,

  FILTER_PANEL_FILTER_TYPES = 100,
  FILTER_PANEL_FILTER_EDIT,
  FILTER_PANEL_SEARCH_EDIT,
  FILTER_PANEL_EXPAND_ALL,
  FILTER_PANEL_COLLAPSE_ALL,
};


TreeListWindow::TreeListWindow(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, hdpi::Px w, hdpi::Px h,
  const char *caption) :
  TreeViewWindow(event_handler, phandle, x, y, w, h, _pxScaled(PANEL_HEIGHT), caption, false),
  mFilterAllStrings(midmem),
  mFilterSelectedStrings(midmem)
{
  mContainer = new WContainer(this, mTreeWnd, 0, 0, 100, 100); // to process ListBox messages!!!
  mList = new WListBox(this, mContainer, 0, 0, 100, 100, mTree->getItemChildren(NULL), 0, /*allow_popup_menu = */ true);

  mList->setFont(WindowBase::getNormalFont());

  mPanelWidth = _px(w);
  mPanelHeight = _px(h);
  mNeedFilterButton = true;
}


void TreeListWindow::fillPanel(bool need_filter_button)
{
  mPanelFS->createButton(FILTER_PANEL_EXPAND_ALL, "Expand all");
  mPanelFS->createButton(FILTER_PANEL_COLLAPSE_ALL, "Collapse all", true, false);

  mNeedFilterButton = need_filter_button;
  if (mNeedFilterButton)
    mPanelFS->createButton(FILTER_PANEL_FILTER_TYPES, "");

  mPanelFS->createEditBox(FILTER_PANEL_FILTER_EDIT, "Filter:");
  mPanelFS->createEditBox(FILTER_PANEL_SEARCH_EDIT, "Search:");

  resize(mPanelWidth, mPanelHeight);
}


TreeListWindow::~TreeListWindow()
{
  del_it(mList);
  del_it(mContainer);
}


void TreeListWindow::onWmEmbeddedResize(int width, int height)
{
  int panelHeight = _pxS(mNeedFilterButton ? PANEL_HEIGHT : PANEL_HEIGHT_WITHOUT_BUTTON);
  mPanelFS->setHeight(_pxActual(panelHeight));

  resizeBack(mTreeWnd->getX(), mTreeWnd->getY(), width, height);
  resizeTree(mTree->getX(), mTree->getY(), width, (height - panelHeight) / 2);

  mContainer->moveWindow(0, (height - panelHeight) / 2 + 3);
  mContainer->resizeWindow(width, (height - panelHeight) / 2);
  mList->resizeWindow(width, (height - panelHeight) / 2);

  mPanelFS->moveTo(0, height - mPanelFS->getHeight());
  mPanelFS->setWidth(_pxActual(width));
}


void TreeListWindow::filterList(Tab<String> &list)
{
  for (int i = 0; i < list.size();)
  {
    if (strstr(String(list[i]).toLower().str(), mFilterString.str()))
      i++;
    else
      erase_items(list, i, 1);
  }
}


void TreeListWindow::onWcChange(WindowBase *source)
{
  if (source == mTree)
  {
    TLeafHandle sel_item = mTree->getSelectedItem();
    if (sel_item)
    {
      Tab<String> slist = mTree->getItemChildren(sel_item);
      filterList(slist);
      mList->setStrings(slist);
    }
  }

  if (source == mList)
  {
    mEventHandler->onTvListSelection(*this, getListSelIndex());
  }

  __super::onWcChange(source);
}


void TreeListWindow::onWcDoubleClick(WindowBase *source) { mEventHandler->onTvListDClick(*this, getListSelIndex()); }


void TreeListWindow::onWcRightClick(WindowBase *source)
{
  if (source == mList && mEventHandler)
  {
    IMenu *menu = mList->getPopupMenu();
    G_ASSERT(menu);

    menu->clearMenu(ROOT_MENU_ITEM);
    bool showMenu = mEventHandler->onTvListContextMenu(*this, mList->getSelectedIndex(), *menu);
    mList->setPopupMenuShow(showMenu);
  }
}


int TreeListWindow::getListSelIndex() { return mList->getSelectedIndex(); }


void TreeListWindow::getListSelText(char *buffer, int buflen) { mList->getSelectedText(buffer, buflen); }


void TreeListWindow::setFilterAssetNames(const Tab<String> &vals)
{
  mFilterAllStrings = vals;
  mFilterSelectedStrings = vals;

  bool needFilterButton = mFilterSelectedStrings.size() != 1;
  fillPanel(needFilterButton);

  if (needFilterButton)
    setCaptionFilterButton();
}


void TreeListWindow::setFilterStr(const char *str)
{
  mPanelFS->setText(FILTER_PANEL_FILTER_EDIT, str);
  onChange(FILTER_PANEL_FILTER_EDIT, mPanelFS.get());
}
const char *TreeListWindow::getFilterStr() const { return mPanelFS->getText(FILTER_PANEL_FILTER_EDIT); }

void TreeListWindow::setListSelIndex(int index) { mList->setSelectedIndex(index); }

void TreeListWindow::onChange(int pid, PropertyContainerControlBase *panel)
{
  switch (pid)
  {
    case FILTER_PANEL_FILTER_EDIT:
    {
      String buf(panel->getText(FILTER_PANEL_FILTER_EDIT));
      mFilterString = buf.toLower().str();
      startFilter();
      onWcChange(mTree);
    }
    break;
  }
}


void TreeListWindow::onClick(int pid, PropertyContainerControlBase *panel)
{
  if (pid == FILTER_PANEL_FILTER_TYPES)
  {
    MultiListDialog selectAssets("Asset types filter:", _pxScaled(250), _pxScaled(450), mFilterAllStrings, mFilterSelectedStrings);

    if (selectAssets.showDialog() == DIALOG_ID_OK)
    {
      setCaptionFilterButton();
      mEventHandler->onTvAssetTypeChange(*this, mFilterSelectedStrings);
    }
  }
  else if (pid == FILTER_PANEL_EXPAND_ALL)
  {
    TLeafHandle leaf, root;
    leaf = root = getRoot();
    expand(leaf);

    while ((leaf = getNextNode(leaf, true)) && leaf != root)
      expand(leaf);
  }
  else if (pid == FILTER_PANEL_COLLAPSE_ALL)
  {
    TLeafHandle leaf, root;
    leaf = root = getRoot();
    expand(leaf);

    while ((leaf = getNextNode(leaf, true)) && leaf != root)
      collapse(leaf);
  }
}


long TreeListWindow::onKeyDown(int pcb_id, PropertyContainerControlBase *panel, unsigned v_key)
{
  if (pcb_id == FILTER_PANEL_SEARCH_EDIT)
  {
    if (v_key == wingw::V_ENTER || v_key == wingw::V_DOWN || v_key == wingw::V_UP)
    {
      bool forward = (v_key == wingw::V_UP) ? false : true;
      searchNext(panel->getText(FILTER_PANEL_SEARCH_EDIT).str(), forward);
      return 1;
    }
  }

  return 0;
}


void TreeListWindow::setCaptionFilterButton()
{
  String newButtonName;

  if (mFilterSelectedStrings.size() == 0)
    newButtonName.append("None");
  else
  {
    for (int i = 0; i < mFilterSelectedStrings.size(); ++i)
    {
      if (i < FILTER_CAPTION_MAX_TYPES_NUM)
      {
        if (i > 0)
          newButtonName.append(", ");
        newButtonName.append(mFilterSelectedStrings[i]);
      }
      else
      {
        newButtonName.append(", [...]");
        break;
      }
    }
  }

  mPanelFS->setText(FILTER_PANEL_FILTER_TYPES, newButtonName.str());
}


bool TreeListWindow::handleNodeFilter(TTreeNodeInfo &node)
{
  for (int i = 0; i < node.mChildren.size(); ++i)
    if (strstr(String(node.mChildren[i]).toLower().str(), mFilterString.str()))
      return true;

  return false;
}


void TreeListWindow::searchNext(const char *text, bool forward)
{
  String searchText = String(text).toLower();

  TLeafHandle sel = getSelectedItem() ? getSelectedItem() : getRoot();
  if (!sel)
    return;

  int selIndex = getListSelIndex();

  TLeafHandle next = sel;
  TLeafHandle first = 0;
  for (;;)
  {
    Tab<String> nodeStrings(midmem);
    nodeStrings = mTree->getItemChildren(next);
    filterList(nodeStrings);

    if (selIndex < 0 || selIndex >= nodeStrings.size())
      selIndex = forward ? -1 : nodeStrings.size();

    int ind = forward ? (selIndex + 1) : (selIndex - 1);
    while (ind >= 0 && ind < nodeStrings.size())
    {
      if (strstr(String(nodeStrings[ind]).toLower().str(), searchText.str()))
      {
        setSelectedItem(next);
        setListSelIndex(ind);
        onWcChange(mList);
        return;
      }

      ind += forward ? 1 : -1;
    }

    if (next == first)
      return;
    if (!first)
      first = next;

    next = search("", next, forward);
    if (!next)
      return;

    selIndex = -1;
  }
}
