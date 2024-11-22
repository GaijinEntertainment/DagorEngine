// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/container.h>
#include "filteredTreeStandalone.h"
#include <Shlwapi.h> // StrStrIA

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

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_INT | CONTROL_CAPTION; }
  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_INT | CONTROL_CAPTION; }

  virtual void setEnabled(bool enabled) override { tree.setEnabled(enabled); }

  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }

  virtual TLeafHandle createTreeLeaf(TLeafHandle parent, const char caption[], const char image[]) override
  {
    return tree.addItem(caption, image, parent);
  }

  virtual void clear() override { tree.clear(); }

  virtual bool removeLeaf(TLeafHandle id) override { return tree.removeItem(id); }

  virtual bool swapLeaf(TLeafHandle id, bool after) override { return tree.swapLeaf(id, after); }

  virtual void setCaption(TLeafHandle leaf, const char value[]) override { tree.setTitle(leaf, value); }

  virtual void setButtonPictures(TLeafHandle leaf, const char *fname = NULL) override { tree.setButtonPictures(leaf, fname); }

  virtual void setColor(TLeafHandle leaf, E3DCOLOR value) override { tree.setColor(leaf, value); }

  virtual void setBool(TLeafHandle leaf, bool open) override { tree.setExpanded(leaf, open); }

  virtual void setFocus() override { tree.setFocus(); }

  virtual void setUserData(TLeafHandle leaf, const void *value) override { tree.setUserData(leaf, const_cast<void *>(value)); }

  virtual void setTreeEventHandler(ITreeControlEventHandler *event_handler) override { treeEventHandler = event_handler; }

  virtual void setCheckboxEnable(TLeafHandle leaf, bool is_enable) override
  {
    tree.setCheckboxState(leaf,
      is_enable ? TreeControlStandalone::CheckboxState::Unchecked : TreeControlStandalone::CheckboxState::NoCheckbox);
  }

  virtual void setCheckboxValue(TLeafHandle leaf, bool is_checked) override
  {
    tree.setCheckboxState(leaf,
      is_checked ? TreeControlStandalone::CheckboxState::Checked : TreeControlStandalone::CheckboxState::Unchecked);
  }

  virtual void setImageState(TLeafHandle leaf, const char *fname) override
  {
    TEXTUREID icon = tree.getTexture(fname);
    tree.setStateIcon(leaf, icon);
  }

  virtual String getCaption(TLeafHandle leaf) const override
  {
    const String *title = tree.getItemTitle(leaf);
    return title ? *title : String();
  }

  virtual int getChildCount(TLeafHandle leaf) const override { return tree.getChildCount(leaf); }

  virtual const char *getImageName(TLeafHandle leaf) const override
  {
    // NOTE: ImGui porting: unused.
    G_ASSERT_LOG(false, "TreePropertyControl::getImageName is not supported!");
    return nullptr;
  }

  virtual TLeafHandle getChildLeaf(TLeafHandle leaf, unsigned idx) override { return tree.getChildLeaf(leaf, idx); }

  virtual TLeafHandle getParentLeaf(TLeafHandle leaf) override { return tree.getParentLeaf(leaf); }

  virtual TLeafHandle getNextLeaf(TLeafHandle leaf) override { return tree.getNextLeaf(leaf); }

  virtual TLeafHandle getPrevLeaf(TLeafHandle leaf) override { return tree.getPrevLeaf(leaf); }

  virtual TLeafHandle getRootLeaf() override { return tree.getRootLeaf(); }

  virtual bool getBool(TLeafHandle leaf) const override { return tree.isExpanded(leaf); }

  virtual void *getUserData(TLeafHandle leaf) const override { return tree.getItemData(leaf); }

  virtual bool getCheckboxValue(TLeafHandle leaf) const override
  {
    return tree.getCheckboxState(leaf) == TreeControlStandalone::CheckboxState::Checked;
  }

  virtual bool isCheckboxEnable(TLeafHandle leaf) const override
  {
    return tree.getCheckboxState(leaf) != TreeControlStandalone::CheckboxState::NoCheckbox;
  }

  // show pictures
  virtual void setBoolValue(bool value) override
  {
    // NOTE: ImGui porting: unused.
    G_ASSERT_LOG(false, "TreePropertyControl::setBoolValue is not supported!");
  }

  // work with active leaf handle
  virtual void setSelLeaf(TLeafHandle leaf, bool keep_selected = false) override { tree.setSelectedLeaf(leaf, keep_selected); }

  virtual TLeafHandle getSelLeaf() const override { return tree.getSelectedLeaf(); }

  virtual void getSelectedLeafs(dag::Vector<TLeafHandle> &leafs) const override { tree.getSelectedLeafs(leafs); }

  virtual void setTextValue(const char value[]) override { tree.filter((void *)value, nodeFilter); }

  virtual bool isRealContainer() override { return false; }

  virtual void setTreeCheckboxIcons(const char *checked, const char *unchecked) override
  {
    tree.setCheckboxIcons(tree.getTexture(checked), tree.getTexture(unchecked));
  }

  virtual void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!tree.isEnabled());

    ImguiHelper::separateLineLabel(controlCaption);

    tree.updateImgui(mH);
  }

private:
  virtual void onControlAdd(PropertyControlBase *control) override
  {
    G_ASSERT_LOG(false, "The Tree control can contain only tree leaves!");
  }

  virtual void onWcRightClick(WindowBase *source) override
  {
    if (treeEventHandler)
      treeEventHandler->onTreeContextMenu(*this, getID(), tree);
  }

  static bool nodeFilter(void *param, TTreeNode &node)
  {
    const char *filterText = (const char *)param;
    return filterText[0] == '\0' || StrStrIA(node.name.str(), filterText);
  }

  String controlCaption;
  FilteredTreeControlStandalone tree;
  ITreeControlEventHandler *treeEventHandler = nullptr;
};

} // namespace PropPanel