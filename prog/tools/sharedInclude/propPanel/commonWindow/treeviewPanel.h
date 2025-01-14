//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/control/filteredTreeTypes.h>
#include <propPanel/c_control_event_handler.h>
#include <propPanel/c_window_event_handler.h>
#include <drv/3d/dag_resId.h>
#include <libTools/util/hdpiUtil.h>
#include <util/dag_simpleString.h>

#include <EASTL/unique_ptr.h>


namespace PropPanel
{

class FilteredTreeControlStandalone;
class IListBoxInterface;
class ITreeInterface;
class IMenu;
class ListBoxControlStandalone;
class ContainerPropertyControl;
class TreeBaseWindow;

class ITreeViewEventHandler
{
public:
  virtual void onTvSelectionChange(TreeBaseWindow &tree, TLeafHandle new_sel) = 0;
  virtual void onTvDClick(TreeBaseWindow &tree, TLeafHandle node) {}
  virtual void onTvListSelection(TreeBaseWindow &tree, int index) {}
  virtual void onTvListDClick(TreeBaseWindow &tree, int index) {}
  virtual void onTvAssetTypeChange(TreeBaseWindow &tree, const Tab<String> &vals) {}
  virtual bool onTvContextMenu(TreeBaseWindow &tree_base_window, ITreeInterface &tree) = 0;
  virtual bool onTvListContextMenu(TreeBaseWindow &tree, IListBoxInterface &list_box) { return false; }
};

class TreeBaseWindow : public WindowControlEventHandler
{
public:
  TreeBaseWindow(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, hdpi::Px w, hdpi::Px h, const char *caption,
    bool icons_show, bool state_icons_show = false);

  virtual ~TreeBaseWindow();

  TLeafHandle addItem(const char *name, TEXTUREID icon, TLeafHandle parent = NULL, void *user_data = nullptr);
  TLeafHandle addItem(const char *name, const char *icon_name, TLeafHandle parent = NULL, void *user_data = nullptr);
  TLeafHandle addItemAsFirst(const char *name, TEXTUREID icon, TLeafHandle parent = NULL, void *user_data = nullptr);
  TLeafHandle addItemAsFirst(const char *name, const char *icon_name, TLeafHandle parent = NULL, void *user_data = nullptr);
  void removeItem(TLeafHandle item);

  virtual void addChildName(const char *name, TLeafHandle parent);
  int getChildrenCount(TLeafHandle parent) const;
  TLeafHandle getChild(TLeafHandle parent, int i) const;

  TLeafHandle getNextNode(TLeafHandle item, bool forward) const;

  TEXTUREID addImage(const char *filename);

  void changeItemImage(TLeafHandle item, TEXTUREID new_id);
  void changeItemStateImage(TLeafHandle item, TEXTUREID new_id);

  String getItemName(TLeafHandle item) const;
  void *getItemData(TLeafHandle item) const;
  Tab<String> getChildNames(TLeafHandle item) const;
  TTreeNode *getItemNode(TLeafHandle item);
  const TTreeNode *getItemNode(TLeafHandle item) const;

  bool isOpen(TLeafHandle item) const;
  bool isSelected(TLeafHandle item) const;

  TLeafHandle getSelectedItem() const;
  void setSelectedItem(TLeafHandle item);
  TLeafHandle getRoot() const;

  void clear();
  TLeafHandle search(const char *text, TLeafHandle first, bool forward, bool use_wildcard_search = false);

  void collapse(TLeafHandle item);
  void expand(TLeafHandle item);
  void expandRecursive(TLeafHandle leaf, bool open = true);
  void ensureVisible(TLeafHandle item);

  void setFocus();
  void setMessage(const char *in_message);

  virtual bool handleNodeFilter(const TTreeNode &node) { return true; }

  // If control_height is 0 then it will use the entire available height.
  virtual void updateImgui(float control_height = 0.0f);

protected:
  typedef bool (*TTreeNodeFilterFunc)(void *param, TTreeNode &node);

  TLeafHandle addItemInternal(const char *name, TEXTUREID icon, TLeafHandle parent, void *user_data, bool as_first);
  void startFilter();

  // WindowControlEventHandler
  virtual void onWcChange(WindowBase *source) override;
  virtual void onWcRightClick(WindowBase *source) override;
  virtual void onWcDoubleClick(WindowBase *source) override;

  ITreeViewEventHandler *mEventHandler;
  WindowBase *treeWindowBase; // Just for identification.
  FilteredTreeControlStandalone *mTree;
};


class TreeViewWindow : public TreeBaseWindow, public ControlEventHandler
{
public:
  TreeViewWindow(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, hdpi::Px w, hdpi::Px h, hdpi::Px ph,
    const char *caption, bool icons_show = true);

  static constexpr const char *tooltipSearch = "- Up arrow: go to the previous match\n"
                                               "- Down arrow: go to the next match\n"
                                               "- Enter: go to the next match";
};


class TreeListWindow : public TreeViewWindow
{
public:
  TreeListWindow(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, hdpi::Px w, hdpi::Px h, const char *caption);

  ~TreeListWindow();

  int getListSelIndex() const;
  void setListSelIndex(int index);
  void getListSelText(char *buffer, int buflen) const;

  void setFilterAssetNames(const Tab<String> &vals);
  void setFilterStr(const char *str);
  SimpleString getFilterStr() const;

  // If control_height is 0 then it will use the entire available height.
  virtual void updateImgui(float control_height = 0.0f) override;

protected:
  // WindowControlEventHandler
  virtual void onWcChange(WindowBase *source) override;
  virtual void onWcDoubleClick(WindowBase *source) override;
  virtual void onWcRightClick(WindowBase *source) override;

  // ControlEventHandler
  virtual void onChange(int pid, ContainerPropertyControl *panel) override;
  virtual void onClick(int pid, ContainerPropertyControl *panel) override;
  virtual long onKeyDown(int pcb_id, ContainerPropertyControl *panel, unsigned v_key) override;

  virtual bool handleNodeFilter(const TTreeNode &node) override;

  void searchNext(const char *text, bool forward);
  void setCaptionFilterButton();
  void fillPanel(bool need_filter_button);

private:
  ListBoxControlStandalone *listBox;
  WindowBase *listBoxWindowBase; // Just for identification.
  eastl::unique_ptr<ContainerPropertyControl> mPanelFS;

  SimpleString mFilterString;
  void filterList(Tab<String> &list);

  Tab<String> mFilterAllStrings;
  Tab<String> mFilterSelectedStrings;

  int mPanelWidth, mPanelHeight;
  bool mNeedFilterButton;
  float panelLastHeight = 0.0f;
};

} // namespace PropPanel