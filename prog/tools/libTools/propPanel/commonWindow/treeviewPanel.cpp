// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/commonWindow/treeviewPanel.h>
#include <propPanel/control/container.h>
#include "../control/listBoxStandalone.h"
#include "../control/filteredTreeStandalone.h"
#include <propPanel/commonWindow/multiListDialog.h>
#include <libTools/util/strUtil.h>
#include <winGuiWrapper/wgw_input.h>

namespace PropPanel
{

static bool TreeBaseWindowFilterStub(void *param, TTreeNode &node)
{
  TreeBaseWindow *tree = (TreeBaseWindow *)param;
  return tree->handleNodeFilter(node);
}

TreeBaseWindow::TreeBaseWindow(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, hdpi::Px w, hdpi::Px h,
  const char *caption, bool icons_show, bool state_icons_show) :
  mTree(new FilteredTreeControlStandalone(true)), mEventHandler(event_handler)
{
  // The window base pointer is only used for comparison. It is intentionally a bad pointer.
  treeWindowBase = reinterpret_cast<WindowBase *>(~reinterpret_cast<uintptr_t>(mTree));

  mTree->setEventHandler(this);
  mTree->setWindowBaseForEventHandler(treeWindowBase);
}

TreeBaseWindow::~TreeBaseWindow() { del_it(mTree); }

void TreeBaseWindow::onWcChange(WindowBase *source)
{
  if (source == treeWindowBase && mEventHandler)
  {
    mEventHandler->onTvSelectionChange(*this, mTree->getSelectedLeaf());
  }
}

void TreeBaseWindow::onWcRightClick(WindowBase *source)
{
  if (source == treeWindowBase && mEventHandler)
    mEventHandler->onTvContextMenu(*this, *mTree);
}

void TreeBaseWindow::onWcDoubleClick(WindowBase *source)
{
  if (source == treeWindowBase && mEventHandler)
    mEventHandler->onTvDClick(*this, mTree->getSelectedLeaf());
}

TLeafHandle TreeBaseWindow::addItem(const char *name, TEXTUREID icon, TLeafHandle parent, void *user_data)
{
  return mTree->addItem(name, icon, parent, user_data);
}

TLeafHandle TreeBaseWindow::addItem(const char *name, const char *icon_name, TLeafHandle parent, void *user_data)
{
  return mTree->addItem(name, icon_name, parent, user_data);
}

TLeafHandle TreeBaseWindow::addItemAsFirst(const char *name, TEXTUREID icon, TLeafHandle parent, void *user_data)
{
  return mTree->addItemAsFirst(name, icon, parent, user_data);
}

TLeafHandle TreeBaseWindow::addItemAsFirst(const char *name, const char *icon_name, TLeafHandle parent, void *user_data)
{
  return mTree->addItemAsFirst(name, icon_name, parent, user_data);
}

void TreeBaseWindow::removeItem(TLeafHandle item) { mTree->removeItem(item); }

void TreeBaseWindow::addChildName(const char *name, TLeafHandle parent) { mTree->addChildName(name, parent); }

TEXTUREID TreeBaseWindow::addImage(const char *filename) { return mTree->getTexture(filename); }

void TreeBaseWindow::changeItemImage(TLeafHandle item, TEXTUREID new_id) { mTree->setIcon(item, new_id); }

void TreeBaseWindow::changeItemStateImage(TLeafHandle item, TEXTUREID new_id) { mTree->setStateIcon(item, new_id); }

int TreeBaseWindow::getChildrenCount(TLeafHandle parent) const { return mTree->getChildCount(parent); }

TLeafHandle TreeBaseWindow::getNextNode(TLeafHandle item, bool forward) const
{
  return forward ? mTree->getNextLeaf(item) : mTree->getPrevLeaf(item);
}

TLeafHandle TreeBaseWindow::getChild(TLeafHandle parent, int i) const { return mTree->getChildLeaf(parent, i); }

TTreeNode *TreeBaseWindow::getItemNode(TLeafHandle p) { return mTree->getItemNode(p); }

const TTreeNode *TreeBaseWindow::getItemNode(TLeafHandle p) const { return mTree->getItemNode(p); }

String TreeBaseWindow::getItemName(TLeafHandle item) const
{
  const String *title = mTree->getItemTitle(item);
  return title ? *title : String();
}

bool TreeBaseWindow::isOpen(TLeafHandle item) const { return mTree->isExpanded(item); }

bool TreeBaseWindow::isSelected(TLeafHandle item) const { return mTree->isLeafSelected(item); }

void *TreeBaseWindow::getItemData(TLeafHandle item) const { return mTree->getItemData(item); }

Tab<String> TreeBaseWindow::getChildNames(TLeafHandle item) const { return mTree->getChildNames(item); }

void TreeBaseWindow::startFilter() { mTree->filter(this, TreeBaseWindowFilterStub); }

void TreeBaseWindow::clear() { mTree->clear(); }

TLeafHandle TreeBaseWindow::search(const char *text, TLeafHandle first, bool forward, bool use_wildcard_search)
{
  return mTree->search(text, first, forward, use_wildcard_search);
}

void TreeBaseWindow::collapse(TLeafHandle item) { mTree->setExpanded(item, false); }

void TreeBaseWindow::expand(TLeafHandle parent) { mTree->setExpanded(parent, true); }

void TreeBaseWindow::expandRecursive(TLeafHandle leaf, bool open) { mTree->setExpandedRecursive(leaf, open); }

void TreeBaseWindow::ensureVisible(TLeafHandle item) { mTree->ensureVisible(item); }

TLeafHandle TreeBaseWindow::getRoot() const { return mTree->getRootLeaf(); }

TLeafHandle TreeBaseWindow::getSelectedItem() const { return mTree->getSelectedLeaf(); }

void TreeBaseWindow::setSelectedItem(TLeafHandle item) { mTree->setSelectedLeaf(item); }

void TreeBaseWindow::setFocus() { mTree->setFocus(); }

void TreeBaseWindow::setMessage(const char *in_message) { mTree->setMessage(in_message); }

void TreeBaseWindow::updateImgui(float control_height) { mTree->updateImgui(control_height); }

//=============================================================================

TreeViewWindow::TreeViewWindow(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, hdpi::Px w, hdpi::Px h, hdpi::Px ph,
  const char *caption, bool icons_show) :
  TreeBaseWindow(event_handler, phandle, x, y, w, h, caption, icons_show)
{}

//=============================================================================
const unsigned PANEL_HEIGHT = 130 + 40;
const unsigned PANEL_HEIGHT_WITHOUT_BUTTON = 97 + 40;

enum
{
  TREELISTVIEW_LISTBOX = 10,

  FILTER_CAPTION_MAX_TYPES_NUM = 5,

  FILTER_PANEL_FILTER_TYPES = 100,
  FILTER_PANEL_FILTER_EDIT,
  FILTER_PANEL_SEARCH_EDIT,
  FILTER_PANEL_EXPAND_ALL,
  FILTER_PANEL_COLLAPSE_ALL,
};

TreeListWindow::TreeListWindow(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, hdpi::Px w, hdpi::Px h,
  const char *caption) :
  TreeViewWindow(event_handler, phandle, x, y, w, h, hdpi::_pxScaled(PANEL_HEIGHT), caption, false),
  mFilterAllStrings(midmem),
  mFilterSelectedStrings(midmem)
{
  mPanelFS.reset(new ContainerPropertyControl(0, this, nullptr, 0, 0, hdpi::Px(0), hdpi::Px(0)));

  Tab<String> list;
  listBox = new ListBoxControlStandalone(list, -1);

  // The window base pointer is only used for comparison. It is intentionally a bad pointer.
  listBoxWindowBase = reinterpret_cast<WindowBase *>(~reinterpret_cast<uintptr_t>(listBox));

  listBox->setEventHandler(this);
  listBox->setWindowBaseForEventHandler(listBoxWindowBase);

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

  mPanelFS->setTooltipId(FILTER_PANEL_SEARCH_EDIT, TreeViewWindow::tooltipSearch);
}

TreeListWindow::~TreeListWindow() { del_it(listBox); }

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
  if (source == treeWindowBase)
  {
    TLeafHandle sel_item = mTree->getSelectedLeaf();
    if (sel_item)
    {
      Tab<String> names = getChildNames(sel_item);
      filterList(names);
      listBox->setValues(names);
    }
  }
  else if (source == listBoxWindowBase)
  {
    mEventHandler->onTvListSelection(*this, getListSelIndex());
  }

  TreeViewWindow::onWcChange(source);
}

void TreeListWindow::onWcDoubleClick(WindowBase *source)
{
  if (source == listBoxWindowBase && mEventHandler)
    mEventHandler->onTvListDClick(*this, getListSelIndex());
  else
    TreeViewWindow::onWcDoubleClick(source);
}

void TreeListWindow::onWcRightClick(WindowBase *source)
{
  if (source == listBoxWindowBase && mEventHandler)
    mEventHandler->onTvListContextMenu(*this, *listBox);
  else
    TreeViewWindow::onWcRightClick(source);
}

int TreeListWindow::getListSelIndex() const { return listBox->getSelectedIndex(); }

void TreeListWindow::getListSelText(char *buffer, int buflen) const
{
  const String *text = listBox->getSelectedText();
  if (text)
    ImguiHelper::getTextValueForString(*text, buffer, buflen);
  else if (buffer && buflen > 0)
    buffer[0] = 0;
}

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

SimpleString TreeListWindow::getFilterStr() const { return mPanelFS->getText(FILTER_PANEL_FILTER_EDIT); }

void TreeListWindow::setListSelIndex(int index) { listBox->setSelectedIndex(index); }

void TreeListWindow::onChange(int pid, ContainerPropertyControl *panel)
{
  switch (pid)
  {
    case FILTER_PANEL_FILTER_EDIT:
    {
      String buf(panel->getText(FILTER_PANEL_FILTER_EDIT));
      mFilterString = buf.toLower().str();
      startFilter();
      onWcChange(treeWindowBase);
    }
    break;
  }
}

void TreeListWindow::onClick(int pid, ContainerPropertyControl *panel)
{
  if (pid == FILTER_PANEL_FILTER_TYPES)
  {
    MultiListDialog selectAssets("Asset types filter:", hdpi::_pxScaled(250), hdpi::_pxScaled(450), mFilterAllStrings,
      mFilterSelectedStrings);

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

long TreeListWindow::onKeyDown(int pcb_id, ContainerPropertyControl *panel, unsigned v_key)
{
  if (pcb_id == FILTER_PANEL_SEARCH_EDIT)
  {
    if (v_key == wingw::V_ENTER || v_key == wingw::V_DOWN || v_key == wingw::V_UP)
    {
      bool forward = v_key != wingw::V_UP;
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

bool TreeListWindow::handleNodeFilter(const TTreeNode &node)
{
  for (int i = 0; i < node.childNames.size(); ++i)
    if (strstr(String(node.childNames[i]).toLower().str(), mFilterString.str()))
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
    nodeStrings = getChildNames(next);
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
        onWcChange(listBoxWindowBase);
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

void TreeListWindow::updateImgui(float control_height)
{
  if (control_height <= 0.0f)
    control_height = ImGui::GetContentRegionAvail().y;

  // TODO: ImGui porting: implement a generic sizing logic. With this the size will not be perfect in the first frame.
  const float availableHeight = max(100.0f, control_height - panelLastHeight);
  const float treeHeight = floorf(availableHeight * 0.4f);
  const float listBoxHeight = availableHeight - treeHeight;
  const float cursorStarPosY = ImGui::GetCursorPosY();

  TreeBaseWindow::updateImgui(treeHeight);
  listBox->updateImgui(0.0f, listBoxHeight);
  mPanelFS->updateImgui();

  panelLastHeight = ImguiHelper::getCursorPosYWithoutLastItemSpacing() - cursorStarPosY - treeHeight - listBoxHeight;
}

} // namespace PropPanel