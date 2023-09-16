//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <propPanel2/c_panel_base.h>
#include <sepGui/wndEmbeddedWindow.h>
#include <EASTL/unique_ptr.h>
#include <util/dag_simpleString.h>
#include <propPanel2/c_control_event_handler.h>


class WWindow;
class WTreeView;
class WListBox;
class WContainer;
class CPanelWindow;
class PictureList;
class TreeBaseWindow;
class IMenu;


class ITreeViewEventHandler
{
public:
  virtual void onTvSelectionChange(TreeBaseWindow &tree, TLeafHandle new_sel) = 0;
  virtual void onTvDClick(TreeBaseWindow &tree, TLeafHandle node) {}
  virtual void onTvListSelection(TreeBaseWindow &tree, int index) {}
  virtual void onTvListDClick(TreeBaseWindow &tree, int index) {}
  virtual void onTvAssetTypeChange(TreeBaseWindow &tree, const Tab<String> &vals) {}
  virtual bool onTvContextMenu(TreeBaseWindow &tree, TLeafHandle under_mouse, IMenu &menu) = 0;
  virtual bool onTvListContextMenu(TreeBaseWindow &tree, int index, IMenu &menu) { return false; }
};


struct TTreeNodeInfo
{
  const char *name;
  int iconIndex;
  Tab<String> mChildren;
  void *userData;

  TTreeNodeInfo() : mChildren(midmem) {}
};


struct TLeafParam
{
  void *mUserData;
  Tab<String> mChildren;

  TLeafParam(void *user_data) : mUserData(user_data), mChildren(midmem) {}
};


struct TTreeNode
{
  TTreeNode(const char *cap, int ico, TLeafParam *data) :
    nodes(midmem), name(cap), iconIndex(ico), isExpand(false), textColor(0, 0, 0), userData(data), item(NULL)
  {}

  ~TTreeNode() { clear_all_ptr_items(nodes); }

  SimpleString name;
  int iconIndex;
  bool isExpand;
  E3DCOLOR textColor;
  TLeafParam *userData;
  TLeafHandle item;

  Tab<TTreeNode *> nodes;
};


class TreeBaseWindow : public WindowControlEventHandler, public IWndEmbeddedWindow
{
public:
  TreeBaseWindow(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h, const char *caption,
    bool icons_show, bool state_icons_show = false);

  virtual ~TreeBaseWindow();

  TLeafHandle addItem(const char *name, int icon_index, TLeafHandle parent = NULL, void *user_data = NULL);
  TLeafHandle addItemAsFirst(const char *name, int icon_index, TLeafHandle parent = NULL, void *user_data = NULL);
  void removeItem(TLeafHandle item);

  virtual void addChild(const char *name, TLeafHandle parent);
  int getChildrenCount(TLeafHandle parent);
  TLeafHandle getChild(TLeafHandle parent, int i);

  TLeafHandle getNextNode(TLeafHandle item, bool forward);

  int addImage(const char *filename);
  void changeItemImage(TLeafHandle item, int new_index);

  // Image zero in the state image list cannot be used as a state image.
  int addStateImage(const char *filename);

  // To indicate that the item has no state image, set the index to zero.
  void changeItemStateImage(TLeafHandle item, int new_index);

  String getItemName(TLeafHandle item);
  void *getItemData(TLeafHandle item);
  TTreeNode *getItemNode(TLeafHandle item);

  bool isOpen(TLeafHandle item) const;
  bool isSelected(TLeafHandle item) const;

  TLeafHandle getSelectedItem();
  bool setSelectedItem(TLeafHandle item);
  TLeafHandle getRoot();

  void clear();
  TLeafHandle search(const char *text, TLeafHandle first, bool forward);

  void collapse(TLeafHandle item);
  void expand(TLeafHandle item);
  void ensureVisible(TLeafHandle item);

  void *getHandle();
  void *getParentWindowHandle();

  void show(bool is_visible);
  void resize(int width, int height);
  void setFocus();

  virtual bool handleNodeFilter(TTreeNodeInfo &node) { return true; }

protected:
  void startFilter();
  void resizeTree(int x, int y, int w, int h);
  void resizeBack(int x, int y, int w, int h);

  // IWndEmbeddedWindow
  virtual void onWmEmbeddedResize(int width, int height);
  virtual bool onWmEmbeddedMakingMovable(int &w, int &h) { return true; }

  // WindowControlEventHandler
  virtual void onWcChange(WindowBase *source);
  virtual void onWcClick(WindowBase *source);
  virtual void onWcDoubleClick(WindowBase *source) override;

  ITreeViewEventHandler *mEventHandler;

  WWindow *mTreeWnd;
  WTreeView *mTree;
  PictureList *mPicList;
  PictureList *mStatePicList;
};


class TreeViewWindow : public TreeBaseWindow, public ControlEventHandler
{
public:
  TreeViewWindow(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h, unsigned ph,
    const char *caption, bool icons_show = true);

protected:
  // IWndEmbeddedWindow
  virtual void onWmEmbeddedResize(int width, int height);

  eastl::unique_ptr<CPanelWindow> mPanelFS;
};


class TreeListWindow : public TreeViewWindow
{
public:
  TreeListWindow(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h, const char *caption);

  ~TreeListWindow();

  int getListSelIndex();
  void setListSelIndex(int index);
  void getListSelText(char *buffer, int buflen);

  void setFilterAssetNames(const Tab<String> &vals);
  void setFilterStr(const char *str);
  const char *getFilterStr() const;

protected:
  // WindowControlEventHandler
  virtual void onWcChange(WindowBase *source);
  virtual void onWcDoubleClick(WindowBase *source);
  virtual void onWcRightClick(WindowBase *source) override;

  virtual bool handleNodeFilter(TTreeNodeInfo &node);

  // IWndEmbeddedWindow
  virtual void onWmEmbeddedResize(int width, int height);

  // ControlEventHandler
  virtual void onChange(int pid, PropertyContainerControlBase *panel);
  virtual void onClick(int pid, PropertyContainerControlBase *panel);
  virtual long onKeyDown(int pcb_id, PropertyContainerControlBase *panel, unsigned v_key);

  void searchNext(const char *text, bool forward);
  void setCaptionFilterButton();
  void fillPanel(bool need_filter_button);

private:
  WContainer *mContainer;
  WListBox *mList;

  SimpleString mFilterString;
  void filterList(Tab<String> &list);

  Tab<String> mFilterAllStrings;
  Tab<String> mFilterSelectedStrings;

  int mPanelWidth, mPanelHeight;
  bool mNeedFilterButton;
};
