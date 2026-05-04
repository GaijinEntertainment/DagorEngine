// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/container.h>
#include "treeStandalone.h"
#include <osApiWrappers/dag_localConv.h>

namespace PropPanel
{

class TreePropertyControl : public ContainerPropertyControl
{
public:
  TreePropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    hdpi::Px h, const char caption[], bool has_checkboxes = false, bool multi_select = false) :
    ContainerPropertyControl(id, event_handler, parent, x, y, w, h),
    controlCaption(caption),
    tree(/*selection_change_events_by_code = */ false, has_checkboxes, multi_select)
  {
    tree.setEventHandler(this);
  }

  unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_INT | CONTROL_CAPTION; }
  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_INT | CONTROL_CAPTION; }

  void setEnabled(bool enabled) override { tree.setEnabled(enabled); }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  TLeafHandle createTreeLeaf(TLeafHandle parent, const char caption[], const char image[]) override
  {
    return tree.createTreeLeaf(parent, caption, image);
  }

  void clear() override { tree.clear(); }

  bool removeLeaf(TLeafHandle id) override { return tree.removeLeaf(id); }

  bool swapLeaf(TLeafHandle id, bool after) override { return tree.swapLeaf(id, after); }

  void setCaption(TLeafHandle leaf, const char value[]) override { tree.setTitle(leaf, value); }

  void setButtonPictures(TLeafHandle leaf, const char *fname = NULL) override { tree.setButtonPictures(leaf, fname); }

  void copyButtonPictures(TLeafHandle from, TLeafHandle to) override { tree.copyButtonPictures(from, to); }

  void setColor(TLeafHandle leaf, E3DCOLOR value) override { tree.setColor(leaf, value); }

  void setFocus() override { tree.setFocus(); }

  void setUserData(TLeafHandle leaf, const void *value) override { tree.setUserData(leaf, const_cast<void *>(value)); }

  void setTreeEventHandler(ITreeControlEventHandler *event_handler) override { treeEventHandler = event_handler; }

  void setCheckboxEnable(TLeafHandle leaf, bool is_enable) override { tree.setCheckboxEnabled(leaf, is_enable); }
  void setCheckboxValue(TLeafHandle leaf, bool is_checked) override { tree.setCheckboxValue(leaf, is_checked); }

  void setImageState(TLeafHandle leaf, const char *fname) override
  {
    IconId icon = tree.getTexture(fname);
    tree.setStateIcon(leaf, icon);
  }

  void setNewParent(TLeafHandle leaf, TLeafHandle new_parent_leaf) override { tree.setNewParent(leaf, new_parent_leaf); }

  String getCaption(TLeafHandle leaf) const override
  {
    const String *title = tree.getItemTitle(leaf);
    return title ? *title : String();
  }

  int getChildCount(TLeafHandle leaf) const override { return tree.getChildCount(leaf); }
  int getChildCountFiltered(TLeafHandle leaf) const override { return tree.getChildCountFiltered(leaf); }
  int getChildIndex(TLeafHandle leaf) const override { return tree.getChildIndex(leaf); }
  void setChildIndex(TLeafHandle leaf, int idx) override { tree.setChildIndex(leaf, idx); }

  const char *getImageName(TLeafHandle leaf) const override
  {
    // NOTE: ImGui porting: unused.
    G_ASSERT_LOG(false, "TreePropertyControl::getImageName is not supported!");
    return nullptr;
  }

  TLeafHandle getChildLeaf(TLeafHandle leaf, unsigned idx) override { return tree.getChildLeaf(leaf, idx); }

  TLeafHandle getParentLeaf(TLeafHandle leaf) override { return tree.getParentLeaf(leaf); }

  TLeafHandle getNextLeaf(TLeafHandle leaf) override { return tree.getNextLeaf(leaf); }

  TLeafHandle getPrevLeaf(TLeafHandle leaf) override { return tree.getPrevLeaf(leaf); }

  TLeafHandle getRootLeaf() override { return tree.getRootLeaf(); }

  void *getUserData(TLeafHandle leaf) const override { return tree.getUserData(leaf); }

  bool getCheckboxValue(TLeafHandle leaf) const override { return tree.getCheckboxValue(leaf); }
  bool isCheckboxEnable(TLeafHandle leaf) const override { return tree.getCheckboxEnabled(leaf); }

  // show pictures
  void setBoolValue(bool value) override
  {
    // NOTE: ImGui porting: unused.
    G_ASSERT_LOG(false, "TreePropertyControl::setBoolValue is not supported!");
  }

  // work with active leaf handle
  void setSelLeaf(TLeafHandle leaf, bool keep_selected = false) override { tree.setSelectedLeaf(leaf, keep_selected); }

  TLeafHandle getSelLeaf() const override { return tree.getSelectedLeaf(); }

  void getSelectedLeafs(dag::Vector<TLeafHandle> &leafs, bool search_in_collapsed, bool include_filtered_out) const override
  {
    tree.getSelectedLeafs(leafs, search_in_collapsed, include_filtered_out);
  }

  void setExpanded(TLeafHandle leaf, bool val) override { tree.setExpanded(leaf, val); }

  void setExpandedRecursively(TLeafHandle leaf, bool val) override { tree.setExpandedRecursive(leaf, val); }

  bool isExpanded(TLeafHandle leaf) const override { return tree.isExpanded(leaf); }
  bool isFilteredIn(TLeafHandle leaf) const override { return tree.isFilteredIn(leaf); }

  void setTreeRenderEx(ITreeRenderEx *interface) override { tree.setTreeRenderEx(interface); }

  bool isRealContainer() override { return false; }

  void setTreeCheckboxIcons(const char *checked, const char *unchecked) override
  {
    tree.setCheckboxIcons(tree.getTexture(checked), tree.getTexture(unchecked));
  }

  void setTreeDragHandler(ITreeDragHandler *handler) override { tree.setDragHandler(handler); }
  void setTreeDropHandler(ITreeDropHandler *handler) override { tree.setDropHandler(handler); }
  void setTreeFilter(ITreeFilter *filter) override { tree.setTreeFilter(filter); }
  void filterTree() override { tree.filter(); }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!tree.isEnabled());

    separateLineLabelWithTooltip(controlCaption.begin(), controlCaption.end());

    tree.updateImgui(mH);
  }

private:
  void onControlAdd(PropertyControlBase *control) override { G_ASSERT_LOG(false, "The Tree control can contain only tree leaves!"); }

  void onWcRightClick(WindowBase *source) override
  {
    if (treeEventHandler)
      treeEventHandler->onTreeContextMenu(*this, getID(), tree);
  }

  String controlCaption;
  TreeControlStandalone tree;
  ITreeControlEventHandler *treeEventHandler = nullptr;
};

} // namespace PropPanel