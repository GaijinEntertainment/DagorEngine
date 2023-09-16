#pragma once

#include <propPanel2/comWnd/treeview_panel.h>
#include "../c_window_controls.h"
#include <sepGui/wndMenuInterface.h>
#include <util/dag_simpleString.h>


class PictureList;


typedef bool (*TTreeNodeFilterFunc)(void *param, TTreeNode &node);


class WTreeView : public WindowControlBase
{
public:
  WTreeView(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);
  ~WTreeView();

  // add standart tree item
  TLeafHandle addItem(const char *name, int icon_index, TLeafHandle parent = NULL, void *user_data = NULL);
  TLeafHandle addItemAsFirst(const char *name, int icon_index, TLeafHandle parent = NULL, void *user_data = NULL);

  // add childrens of tree item (for TreeList control)
  void addItemChild(const char *name, TLeafHandle parent);
  const Tab<String> &getItemChildren(TLeafHandle item);
  int getChildrenCount(TLeafHandle item) const;
  TLeafHandle getChild(TLeafHandle item, int child_index);

  TLeafHandle getParentNode(TLeafHandle item);
  TLeafHandle getNextNode(TLeafHandle item);
  TLeafHandle getPrevNode(TLeafHandle item);

  TLeafHandle getRoot();
  TLeafHandle getLastSubNode(TLeafHandle item);

  void setPictureList(PictureList *pictureList);
  void setStatePictureList(PictureList *pictureList);

  void clear();
  bool remove(TLeafHandle item);
  bool swapLeafs(TLeafHandle item, bool with_prior = true);

  void filter(void *param, TTreeNodeFilterFunc filter_func);
  TLeafHandle search(const char *text, TLeafHandle first, bool forward);

  TLeafHandle getSelectedItem() const;
  bool setSelectedItem(TLeafHandle item);

  void changeItemImage(TLeafHandle item, int new_item_index);
  void changeItemStateImage(TLeafHandle item, int new_index);

  TTreeNode *getItemNode(TLeafHandle item) const;
  String getItemName(TLeafHandle item) const;
  void *getItemData(TLeafHandle item) const;

  void setItemName(TLeafHandle item, char name[]);
  void setItemData(TLeafHandle item, void *data);
  void setItemColor(TLeafHandle item, E3DCOLOR value);

  bool isOpen(TLeafHandle item) const;
  bool isSelected(TLeafHandle item) const;

  void collapse(TLeafHandle item);
  void expand(TLeafHandle item);
  void ensureVisible(TLeafHandle item);

  void setPopupMenuShow(bool is_show) { mShowPopupMenu = is_show; }
  IMenu *getPopupMenu() const { return mMenu; }

protected:
  virtual intptr_t onControlNotify(void *info);
  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);

  bool filterRoutine(TTreeNode *node, TLeafHandle parent, void *param, TTreeNodeFilterFunc filter_func);

  TLeafParam *getItemParam(TLeafHandle item) const;

  TLeafHandle insertTreeItem(const char *name, int icon, TLeafHandle parent, TTreeNode *node);

private:
  Tab<String> fakeChildren;

  PictureList *mPicList;
  PictureList *mStatePicList;

  bool mShowPopupMenu;
  IMenu *mMenu;

  Tab<TLeafParam *> mLeafParams; // for simplify deletion in clear()
  TTreeNode *mRootNode;
  TTreeNode *mSelectedNode;
};


class PictureList
{
public:
  PictureList(int width, int height, int size = 32);
  ~PictureList();

  int addImage(const char *filename);
  void *getHandle() { return mImageList; }

private:
  void *mImageList;
};
