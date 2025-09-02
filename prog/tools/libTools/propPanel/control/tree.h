// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/container.h>
#include "filteredTreeStandalone.h"
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
    return tree.addItem(caption, image, parent);
  }

  void clear() override { tree.clear(); }

  bool removeLeaf(TLeafHandle id) override { return tree.removeItem(id); }

  bool swapLeaf(TLeafHandle id, bool after) override { return tree.swapLeaf(id, after); }

  void setCaption(TLeafHandle leaf, const char value[]) override { tree.setTitle(leaf, value); }

  void setButtonPictures(TLeafHandle leaf, const char *fname = NULL) override { tree.setButtonPictures(leaf, fname); }

  void copyButtonPictures(TLeafHandle from, TLeafHandle to) override { tree.copyButtonPictures(from, to); }

  void setColor(TLeafHandle leaf, E3DCOLOR value) override { tree.setColor(leaf, value); }

  void setBool(TLeafHandle leaf, bool open) override { tree.setExpanded(leaf, open); }

  void setFocus() override { tree.setFocus(); }

  void setUserData(TLeafHandle leaf, const void *value) override { tree.setUserData(leaf, const_cast<void *>(value)); }

  void setTreeEventHandler(ITreeControlEventHandler *event_handler) override { treeEventHandler = event_handler; }

  void setCheckboxEnable(TLeafHandle leaf, bool is_enable) override
  {
    tree.setCheckboxState(leaf,
      is_enable ? TreeControlStandalone::CheckboxState::Unchecked : TreeControlStandalone::CheckboxState::NoCheckbox);
  }

  void setCheckboxValue(TLeafHandle leaf, bool is_checked) override
  {
    tree.setCheckboxState(leaf,
      is_checked ? TreeControlStandalone::CheckboxState::Checked : TreeControlStandalone::CheckboxState::Unchecked);
  }

  void setImageState(TLeafHandle leaf, const char *fname) override
  {
    IconId icon = tree.getTexture(fname);
    tree.setStateIcon(leaf, icon);
  }

  String getCaption(TLeafHandle leaf) const override
  {
    const String *title = tree.getItemTitle(leaf);
    return title ? *title : String();
  }

  int getChildCount(TLeafHandle leaf) const override { return tree.getChildCount(leaf); }

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

  bool getBool(TLeafHandle leaf) const override { return tree.isExpanded(leaf); }

  void *getUserData(TLeafHandle leaf) const override { return tree.getItemData(leaf); }

  bool getCheckboxValue(TLeafHandle leaf) const override
  {
    return tree.getCheckboxState(leaf) == TreeControlStandalone::CheckboxState::Checked;
  }

  bool isCheckboxEnable(TLeafHandle leaf) const override
  {
    return tree.getCheckboxState(leaf) != TreeControlStandalone::CheckboxState::NoCheckbox;
  }

  // show pictures
  void setBoolValue(bool value) override
  {
    // NOTE: ImGui porting: unused.
    G_ASSERT_LOG(false, "TreePropertyControl::setBoolValue is not supported!");
  }

  // work with active leaf handle
  void setSelLeaf(TLeafHandle leaf, bool keep_selected = false) override { tree.setSelectedLeaf(leaf, keep_selected); }

  TLeafHandle getSelLeaf() const override { return tree.getSelectedLeaf(); }

  void getSelectedLeafs(dag::Vector<TLeafHandle> &leafs) const override { tree.getSelectedLeafs(leafs); }

  void setTextValue(const char value[]) override { tree.filter((void *)value, nodeFilter); }

  bool isRealContainer() override { return false; }

  void setTreeCheckboxIcons(const char *checked, const char *unchecked) override
  {
    tree.setCheckboxIcons(tree.getTexture(checked), tree.getTexture(unchecked));
  }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!tree.isEnabled());

    ImguiHelper::separateLineLabel(controlCaption);

    tree.updateImgui(mH);
  }

private:
  void onControlAdd(PropertyControlBase *control) override { G_ASSERT_LOG(false, "The Tree control can contain only tree leaves!"); }

  void onWcRightClick(WindowBase *source) override
  {
    if (treeEventHandler)
      treeEventHandler->onTreeContextMenu(*this, getID(), tree);
  }

  static bool nodeFilter(void *param, TTreeNode &node)
  {
    const char *filterText = (const char *)param;
    return filterText[0] == '\0' || dd_stristr(node.name.str(), filterText);
  }

  String controlCaption;
  FilteredTreeControlStandalone tree;
  ITreeControlEventHandler *treeEventHandler = nullptr;
};

} // namespace PropPanel