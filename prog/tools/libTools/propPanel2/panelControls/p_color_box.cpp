// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_color_box.h"
#include "../c_constants.h"

CColorBox::CColorBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  const char caption[]) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_CONTROL_HEIGHT) * 2),
  mCaption(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_CONTROL_HEIGHT)),
  mSpinWidth((_px(w) - _pxS(DEFAULT_CONTROL_HEIGHT)) / 4),
  mRSpin(this, parent->getWindow(), x, y + _pxS(DEFAULT_CONTROL_HEIGHT), mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 255, 1, 1),
  mGSpin(this, parent->getWindow(), x + mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT), mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 255,
    1, 1),
  mBSpin(this, parent->getWindow(), x + 2 * mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT), mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0,
    255, 1, 1),
  mASpin(this, parent->getWindow(), x + 3 * mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT), mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0,
    255, 1, 1),
  mButton(this, parent->getWindow(), x + 4 * mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT), _pxS(DEFAULT_CONTROL_HEIGHT),
    _pxS(DEFAULT_CONTROL_HEIGHT))
{
  mCaption.setTextValue(caption);
  this->setColorValue(E3DCOLOR(0, 0, 0, 255));
}


PropertyContainerControlBase *CColorBox::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createColorBox(id, caption, E3DCOLOR(0, 0, 0, 255), true, new_line);
  return NULL;
}


enum
{
  COLOR_R_CHANGE = 1 << 0,
  COLOR_G_CHANGE = 1 << 1,
  COLOR_B_CHANGE = 1 << 2,
  COLOR_A_CHANGE = 1 << 3,
  COLOR_BUT_CHANGE = 1 << 4,
  COLOR_ALL = 0xFF,
};


void CColorBox::updateInterface(char changes)
{
  if (changes & COLOR_R_CHANGE)
    mRSpin.setValue(mColor.r);

  if (changes & COLOR_G_CHANGE)
    mGSpin.setValue(mColor.g);

  if (changes & COLOR_B_CHANGE)
    mBSpin.setValue(mColor.b);

  if (changes & COLOR_A_CHANGE)
    mASpin.setValue(mColor.a);

  if (changes & COLOR_BUT_CHANGE)
    mButton.setColorValue(mColor);
}


void CColorBox::setColorValue(E3DCOLOR value)
{
  mColor = value;
  updateInterface(COLOR_ALL);
}


void CColorBox::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CColorBox::reset()
{
  this->setColorValue(E3DCOLOR(0));
  PropertyControlBase::reset();
}


void CColorBox::onWcChange(WindowBase *source)
{
  if (source == &mButton)
  {
    mColor = mButton.getColorValue();
    // updateInterface(~(COLOR_BUT_CHANGE | COLOR_A_CHANGE));
    updateInterface(~(COLOR_BUT_CHANGE));
  }
  else if (source == &mRSpin)
  {
    mColor.r = mRSpin.getValue();
    updateInterface(COLOR_BUT_CHANGE);
  }
  else if (source == &mGSpin)
  {
    mColor.g = mGSpin.getValue();
    updateInterface(COLOR_BUT_CHANGE);
  }
  else if (source == &mBSpin)
  {
    mColor.b = mBSpin.getValue();
    updateInterface(COLOR_BUT_CHANGE);
  }
  else if (source == &mASpin)
  {
    mColor.a = mASpin.getValue();
    updateInterface(COLOR_BUT_CHANGE);
  }

  PropertyControlBase::onWcChange(source);
}


void CColorBox::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  mRSpin.moveWindow(x, y + _pxS(DEFAULT_CONTROL_HEIGHT));
  mGSpin.moveWindow(x + mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT));
  mBSpin.moveWindow(x + 2 * mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT));
  mASpin.moveWindow(x + 3 * mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT));
  mButton.moveWindow(x + 4 * mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT));
}


void CColorBox::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mRSpin.setEnabled(enabled);
  mGSpin.setEnabled(enabled);
  mBSpin.setEnabled(enabled);
  mASpin.setEnabled(enabled);
  mButton.setEnabled(enabled);
}


void CColorBox::setWidth(hdpi::Px w)
{
  int minw = _pxS(DEFAULT_CONTROL_HEIGHT);
  w = (_px(w) < minw) ? _pxActual(minw) : w;

  PropertyControlBase::setWidth(w);

  mSpinWidth = (_px(w) - _pxS(DEFAULT_CONTROL_HEIGHT)) / 4;

  mCaption.resizeWindow(_px(w), mCaption.getHeight());
  mRSpin.resizeWindow(mSpinWidth, mRSpin.getHeight());
  mGSpin.resizeWindow(mSpinWidth, mRSpin.getHeight());
  mBSpin.resizeWindow(mSpinWidth, mRSpin.getHeight());
  mASpin.resizeWindow(mSpinWidth, mRSpin.getHeight());

  this->moveTo(this->getX(), this->getY());
}
