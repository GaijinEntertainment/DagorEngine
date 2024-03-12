// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_toolbar.h"
#include "p_tool_button.h"
#include "../c_constants.h"

#include <debug/dag_debug.h>


CToolbar::CToolbar(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  const char *caption) :
  PropertyContainerControlBase(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_TOOLBAR_HEIGHT)),
  mContainer(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_TOOLBAR_HEIGHT)),
  mToolbar(this, &mContainer, 0, -_pxS(2) /* -2 для скрытия линии над тулбаром */, _px(w), _pxS(DEFAULT_TOOLBAR_BUTTON_HEIGHT),
    caption)
{}


CToolbar::~CToolbar() {}


PropertyContainerControlBase *CToolbar::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  (void)new_line;
  return parent->createToolbarPanel(id, caption);
}


void CToolbar::onWcClick(WindowBase *source)
{
  PropertyControlBase::onWcClick(source);
  if ((source) && (mEventHandler))
  {
    mEventHandler->onClick(mToolbar.getLastClickID(), getRootParent());
  }
}


void CToolbar::onWcResize(WindowBase *source)
{
  if (source == &mToolbar)
    this->setWidth(_pxActual(mToolbar.getWidth()));
  else
  {
    if (this->mParent)
      this->mParent->onChildResize(mId);
  }

  PropertyControlBase::onWcResize(source);
}


void CToolbar::setWidth(hdpi::Px w)
{
  mContainer.resizeWindow(_px(w), this->getHeight());

  PropertyControlBase::setWidth(w);
}


void CToolbar::setEnabled(bool enabled) { mContainer.setEnabled(enabled); }


void CToolbar::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mContainer.moveWindow(x, y);
}


// -------------- container interface -------------------


void CToolbar::addToolControl(int id)
{
  PropertyControlBase *pcontrol = new CToolButton(this->mEventHandler, this, id);

  PropertyContainerControlBase::addControl(pcontrol, false);
}


void CToolbar::addControl(PropertyControlBase *pcontrol, bool new_line) { G_ASSERT(false && "Toolbar can not contain this control!"); }


void CToolbar::clear()
{
  mToolbar.clear();
  PropertyContainerControlBase::clear();
}


// --------------- creators -------------


void CToolbar::createCheckBox(int id, const char caption[], bool value, bool enabled, bool new_line)
{
  mToolbar.addCheck(id, caption);
  mToolbar.setItemCheck(id, value);
  mToolbar.setItemEnabled(id, enabled);

  addToolControl(id);
}


void CToolbar::createButton(int id, const char caption[], bool enabled, bool new_line)
{
  mToolbar.addButton(id, caption);
  mToolbar.setItemEnabled(id, enabled);

  addToolControl(id);
}


void CToolbar::createSeparator(int id, bool new_line)
{
  mToolbar.addSep(id);

  addToolControl(id);
}


void CToolbar::createRadio(int id, const char caption[], bool enabled, bool new_line)
{
  mToolbar.addCheckGroup(id, caption);
  mToolbar.setItemEnabled(id, enabled);

  addToolControl(id);
}


// -------------- toolbar interface -------------------


bool CToolbar::setButtonEnabled(int id, bool enabled) { return mToolbar.setItemEnabled(id, enabled); }


bool CToolbar::setButtonValue(int id, bool value) { return mToolbar.setItemCheck(id, value); }


bool CToolbar::getButtonValue(int id) { return mToolbar.getItemCheck(id); }


void CToolbar::setButtonCaption(int id, const char *caption) { mToolbar.setItemText(id, caption); }


void CToolbar::setButtonPictures(int id, const char *fname) { mToolbar.setItemPictures(id, fname); }
