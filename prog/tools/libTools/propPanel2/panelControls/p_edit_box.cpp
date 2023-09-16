// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_edit_box.h"
#include "../c_constants.h"

CEditBox::CEditBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w,
  const char caption[], bool multiline) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, DEFAULT_CONTROL_HEIGHT * 2),
  mCaption(this, parent->getWindow(), x, y, w, DEFAULT_CONTROL_HEIGHT),
  mEdit(NULL),
  manualChange(false)
{
  mCaption.setTextValue(caption);
  hasCaption = strlen(caption) > 0;
  mEdit = new WEdit(this, parent->getWindow(), x, y + DEFAULT_CONTROL_HEIGHT, w, DEFAULT_CONTROL_HEIGHT, multiline);

  if (!hasCaption)
  {
    mCaption.hide();
    mH = mEdit->getHeight();
    this->moveTo(x, y);
  }
  initTooltip(mEdit);
}

CEditBox::~CEditBox() { del_it(mEdit); }

PropertyContainerControlBase *CEditBox::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createEditBox(id, caption, "", true, new_line);
  return NULL;
}


void CEditBox::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mEdit->setEnabled(enabled);
}


void CEditBox::reset()
{
  this->setTextValue("");
  PropertyControlBase::reset();
}


void CEditBox::setWidth(unsigned w)
{
  int minw = DEFAULT_CONTROL_WIDTH;
  w = (w < minw) ? minw : w;

  PropertyControlBase::setWidth(w);

  mCaption.resizeWindow(w, mCaption.getHeight());
  mEdit->resizeWindow(w, mEdit->getHeight());

  this->moveTo(this->getX(), this->getY());
}


void CEditBox::setHeight(unsigned h)
{
  PropertyControlBase::setHeight(h);
  if (h > DEFAULT_CONTROL_HEIGHT * 2)
  {
    int w = mEdit->getWidth();
    int x = mEdit->getX();
    int y = mEdit->getY();
    char buf[512];
    getTextValue(buf, 512);

    del_it(mEdit);
    mEdit = new WEdit(this, mParent->getWindow(), x, y, w, DEFAULT_CONTROL_HEIGHT, true);
    setTextValue(buf);
  }

  mEdit->resizeWindow(mEdit->getWidth(), h - mCaption.getHeight());
}


void CEditBox::setTextValue(const char value[])
{
  manualChange = true;
  mEdit->setTextValue(value);
  manualChange = false;
}


void CEditBox::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CEditBox::setBoolValue(bool value) { mEdit->setColorIndication(value); }


void CEditBox::setFocus() { mEdit->setFocus(); }


int CEditBox::getTextValue(char *buffer, int buflen) const { return mEdit->getTextValue(buffer, buflen); }


void CEditBox::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  mEdit->moveWindow(x, (hasCaption) ? y + mCaption.getHeight() : y);
}


void CEditBox::onWcChange(WindowBase *source)
{
  if (!manualChange)
    PropertyControlBase::onWcChange(source);
}
