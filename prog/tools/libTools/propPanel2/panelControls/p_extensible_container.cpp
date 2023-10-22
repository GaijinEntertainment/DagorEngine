// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_extensible_container.h"

CExtensible::CExtensible(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  hdpi::Px h) :

  PropertyContainerVert(id, event_handler, parent, x, y, w, h),

  mRect(this, parent->getWindow(), x, y, _px(w) - _pxS(DEFAULT_CONTROL_HEIGHT), _px(h)),

  mPlusButton(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_CONTROL_HEIGHT)),
  mQuartButtons(this, parent->getWindow(), x + _px(w) - _pxS(DEFAULT_CONTROL_HEIGHT), y, _pxS(DEFAULT_CONTROL_HEIGHT),
    _pxS(DEFAULT_CONTROL_HEIGHT)),

  mButtonStatus(EXT_BUTTON_NONE),
  mWResize(false)
{
  __super::setVerticalInterval(hdpi::Px::ZERO);

  mPlusButton.setTextValue("+");
  mRect.hide();
  mQuartButtons.hide();
}


PropertyContainerControlBase *CExtensible::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  return parent->createExtensible(id, new_line);
}


int CExtensible::getIntValue() const
{
  int result = mButtonStatus;
  mButtonStatus = EXT_BUTTON_NONE;
  return result;
}


void CExtensible::setIntValue(int value) { mQuartButtons.setItemFlags(value); }


void CExtensible::setTextValue(const char value[]) { mPlusButton.setTextValue(value); }

int CExtensible::getCaptionValue(char *buffer, int buflen) const { return mPlusButton.getTextValue(buffer, buflen); }


void CExtensible::onWcClick(WindowBase *source)
{
  if (source == &mPlusButton)
    mButtonStatus = EXT_BUTTON_APPEND;
  if (source == &mQuartButtons)
    mButtonStatus = mQuartButtons.getValue();

  PropertyControlBase::onWcClick(source);
}


WindowBase *CExtensible::getWindow() { return &mRect; }


void CExtensible::setEnabled(bool enabled)
{
  mRect.setEnabled(enabled);
  mPlusButton.setEnabled(enabled);
  mQuartButtons.setEnabled(enabled);

  for (int i = 0; i < mControlArray.size(); i++)
    mControlArray[i]->setEnabled(enabled);
}


void CExtensible::setWidth(hdpi::Px w)
{
  mWResize = true;
  this->resizeControl(_px(w), mControlArray.size() ? this->getNextControlY() : _pxS(DEFAULT_CONTROL_HEIGHT));

  PropertyContainerVert::setWidth(w);
  this->moveTo(this->getX(), this->getY());
  mWResize = false;
}


void CExtensible::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);
  mRect.moveWindow(x, y);
  mPlusButton.moveWindow(x, y);

  mQuartButtons.moveWindow(x + mW - _pxS(DEFAULT_CONTROL_HEIGHT), y);
}

// container

void CExtensible::onControlAdd(PropertyControlBase *control)
{
  if (control)
  {
    mH = this->getNextControlY();
    mRect.resizeWindow(mW - _pxS(DEFAULT_CONTROL_HEIGHT), mH);

    mPlusButton.hide();
    mQuartButtons.show();
    mRect.show();
  }
}


void CExtensible::resizeControl(unsigned w, unsigned h)
{
  mH = h;
  mW = w;

  mRect.resizeWindow(mW - _pxS(DEFAULT_CONTROL_HEIGHT), mH);
  mPlusButton.resizeWindow(mW, mH);
  this->moveTo(this->getX(), this->getY());
}


void CExtensible::onChildResize(int id)
{
  if (!mWResize)
    __super::onChildResize(id);
}


int CExtensible::getNextControlX(bool new_line)
{
  int c = mControlArray.size();
  if ((c) && (!new_line))
  {
    return mControlArray[c - 1]->getX() + mControlArray[c - 1]->getWidth() + _pxS(DEFAULT_CONTROLS_INTERVAL);
  }
  return 0;
}


hdpi::Px CExtensible::getClientWidth()
{
  WindowBase *win = this->getWindow();
  if (win)
  {
    int result = win->getWidth();
    return _pxActual((result > 0) ? result : 0);
  }

  debug("CExtensible::getClientWidth(): getWindow() == NULL!");
  return hdpi::Px::ZERO;
}
