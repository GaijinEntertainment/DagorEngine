#pragma once

#include "../windowControls/w_tree.h"
#include "../windowControls/w_simple_controls.h"
#include <propPanel2/c_panel_base.h>
#include <util/dag_oaHashNameMap.h>

class CTree : public PropertyContainerControlBase
{
public:
  CTree(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w, hdpi::Px h,
    const char caption[]);

  ~CTree();

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  // Work with leafs
  virtual TLeafHandle createTreeLeaf(TLeafHandle parent, const char caption[], const char image[]);
  virtual void clear();

  virtual bool removeLeaf(TLeafHandle id);
  virtual bool swapLeaf(TLeafHandle id, bool after);

  virtual void setCaption(TLeafHandle leaf, const char value[]);
  virtual void setButtonPictures(TLeafHandle leaf, const char *fname = NULL);
  virtual void setColor(TLeafHandle leaf, E3DCOLOR value);
  virtual void setBool(TLeafHandle leaf, bool open);
  virtual void setUserData(TLeafHandle leaf, const void *value);

  virtual String getCaption(TLeafHandle leaf) const;
  virtual int getChildCount(TLeafHandle leaf) const;
  virtual const char *getImageName(TLeafHandle leaf) const;
  virtual TLeafHandle getChildLeaf(TLeafHandle leaf, unsigned idx) { return mTree.getChild(leaf, idx); }
  virtual TLeafHandle getParentLeaf(TLeafHandle leaf) { return mTree.getParentNode(leaf); }
  virtual TLeafHandle getNextLeaf(TLeafHandle leaf) { return mTree.getNextNode(leaf); }
  virtual TLeafHandle getPrevLeaf(TLeafHandle leaf) { return mTree.getPrevNode(leaf); }
  virtual TLeafHandle getRootLeaf() { return mTree.getRoot(); }
  virtual bool getBool(TLeafHandle leaf) const;
  virtual void *getUserData(TLeafHandle leaf) const;

  void setBoolValue(bool value); // show pictures

  // control routines

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_INT | CONTROL_CAPTION; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_INT | CONTROL_CAPTION; }

  // work with active leaf handle
  virtual void setSelLeaf(TLeafHandle leaf);
  virtual TLeafHandle getSelLeaf() const;

  void setCaptionValue(const char value[]);

  void setEnabled(bool enabled);
  void setWidth(hdpi::Px w);
  void setHeight(hdpi::Px h);
  void moveTo(int x, int y);

  // virtual int saveState(DataBlock &datablk) { return 0; } TODO
  // virtual int loadState(DataBlock &datablk) { return 0; } TODO

  virtual bool isRealContainer() { return false; }

protected:
  virtual void onControlAdd(PropertyControlBase *control);
  int getImageIndex(const char image[]);

  virtual void onWcChange(WindowBase *source);
  virtual void onWcResize(WindowBase *source);

private:
  WStaticText mCaption;
  WTreeView mTree;
  bool manualChange;

  FastNameMapEx imagesMap;
  PictureList *imageList;
};
