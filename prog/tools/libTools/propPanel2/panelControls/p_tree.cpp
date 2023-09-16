// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_tree.h"
#include "../c_constants.h"

#include <debug/dag_debug.h>


CTree::CTree(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, int h,
  const char caption[]) :

  PropertyContainerControlBase(id, event_handler, parent, x, y, w, h),
  mCaption(this, parent->getWindow(), x, y, w, DEFAULT_CONTROL_HEIGHT),
  mTree(this, parent->getWindow(), x, y + mCaption.getHeight(), w, h - mCaption.getHeight()),
  manualChange(false)
{
  hasCaption = strlen(caption) > 0;

  if (hasCaption)
  {
    mCaption.setTextValue(caption);
    mH = DEFAULT_CONTROL_HEIGHT + mTree.getHeight();
  }
  else
  {
    mCaption.hide();
    mH = mTree.getHeight();
    this->moveTo(x, y);
  }

  imageList = new PictureList(16, 16);
  mTree.setPictureList(imageList);
}


CTree::~CTree()
{
  manualChange = true;
  mTree.clear();
  mTree.setPictureList(NULL);
  del_it(imageList);
}


PropertyContainerControlBase *CTree::createDefault(int id, PropertyContainerControlBase *parent, const char caption[], bool new_line)
{
  (void)new_line;
  return parent->createTree(id, caption, DEFAULT_LISTBOX_HEIGHT);
}


void CTree::onControlAdd(PropertyControlBase *control) { G_ASSERT(false && "TreeControl can contain only Tree leaves!"); }


TLeafHandle CTree::createTreeLeaf(TLeafHandle parent, const char caption[], const char image[])
{
  return mTree.addItem(caption, getImageIndex(image), parent);
}


void CTree::clear()
{
  manualChange = true;
  mTree.clear();
  manualChange = false;
}


bool CTree::removeLeaf(TLeafHandle id) { return mTree.remove(id); }


bool CTree::swapLeaf(TLeafHandle id, bool after) { return mTree.swapLeafs(id, !after); }

void CTree::setCaption(TLeafHandle id, const char value[]) { mTree.setItemName(id, const_cast<char *>(value)); }


String CTree::getCaption(TLeafHandle leaf) const { return mTree.getItemName(leaf); }


int CTree::getChildCount(TLeafHandle leaf) const { return mTree.getChildrenCount(leaf); }


const char *CTree::getImageName(TLeafHandle leaf) const
{
  TTreeNode *node = mTree.getItemNode(leaf);
  if (node)
    return imagesMap.getName(node->iconIndex);
  else
    return nullptr;
}


void CTree::setButtonPictures(TLeafHandle id, const char *fname) { mTree.changeItemImage(id, getImageIndex(fname)); }


void CTree::setColor(TLeafHandle id, E3DCOLOR value) { mTree.setItemColor(id, value); }


void CTree::setUserData(TLeafHandle id, const void *value) { mTree.setItemData(id, const_cast<void *>(value)); }


void *CTree::getUserData(TLeafHandle id) const { return mTree.getItemData(id); }


void CTree::setBool(TLeafHandle id, bool value)
{
  if (value)
    mTree.expand(id);
  else
    mTree.collapse(id);
}


bool CTree::getBool(TLeafHandle id) const { return mTree.isOpen(id); }


int CTree::getImageIndex(const char image[])
{
  int icon_index = imagesMap.getNameId(image);
  if (icon_index == -1)
  {
    String bmp(256, "%s%s%s", p2util::get_icon_path(), image, ".bmp");
    imageList->addImage(bmp);
    icon_index = imagesMap.addNameId(image);
  }
  return icon_index;
}


void CTree::setBoolValue(bool value) { mTree.setPictureList(value ? imageList : NULL); }


//-----------------------------------------------------------


void CTree::onWcChange(WindowBase *source)
{
  if (!manualChange)
    PropertyControlBase::onWcChange(source);
}


void CTree::onWcResize(WindowBase *source)
{
  if (!manualChange)
    PropertyControlBase::onWcResize(source);
}


TLeafHandle CTree::getSelLeaf() const { return mTree.getSelectedItem(); }


void CTree::setSelLeaf(TLeafHandle value)
{
  manualChange = true;
  mTree.setSelectedItem(value);
  manualChange = false;
}


void CTree::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CTree::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mTree.setEnabled(enabled);
}


void CTree::setWidth(unsigned w)
{
  PropertyControlBase::setWidth(w);
  mCaption.resizeWindow(w, mCaption.getHeight());
  mTree.resizeWindow(w, mTree.getHeight());
}


void CTree::setHeight(unsigned h)
{
  PropertyControlBase::setHeight(h);
  mTree.resizeWindow(mTree.getWidth(), h - mCaption.getHeight());
}


void CTree::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);
  mCaption.moveWindow(x, y);
  mTree.moveWindow(x, y + mCaption.getHeight());
  mTree.refresh(false);
}
