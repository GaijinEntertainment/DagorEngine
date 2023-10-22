// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_point4.h"
#include "../c_constants.h"

CPoint4::CPoint4(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  const char caption[], int prec) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_CONTROL_HEIGHT) * 2),
  mCaption(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_CONTROL_HEIGHT)),
  mSpinWidth(_px(w) / 4),
  mSpin1(this, parent->getWindow(), x, y + _pxS(DEFAULT_CONTROL_HEIGHT), mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 0, prec),
  mSpin2(this, parent->getWindow(), x + mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT), mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0,
    0, prec),
  mSpin3(this, parent->getWindow(), x + 2 * mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT), mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0,
    0, 0, prec),
  mSpin4(this, parent->getWindow(), x + 3 * mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT), mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0,
    0, 0, prec)

{
  mCaption.setTextValue(caption);
  this->setPoint4Value(Point4(0, 0, 0, 0));
  initTooltip(&mSpin1);
  initTooltip(&mSpin2);
  initTooltip(&mSpin3);
  initTooltip(&mSpin4);
}


PropertyContainerControlBase *CPoint4::createDefault(int id, PropertyContainerControlBase *parent, const char caption[], bool new_line)
{
  parent->createPoint4(id, caption, Point4(0, 0, 0, 0), 2, true, new_line);
  return NULL;
}


Point4 CPoint4::getPoint4Value() const { return mValue; }


void CPoint4::setPoint4Value(Point4 value)
{
  mValue = value;
  mSpin1.setValue(mValue.x);
  mSpin2.setValue(mValue.y);
  mSpin3.setValue(mValue.z);
  mSpin4.setValue(mValue.w);
}


void CPoint4::setPrecValue(int prec)
{
  mSpin1.setPrecValue(prec);
  mSpin2.setPrecValue(prec);
  mSpin3.setPrecValue(prec);
  mSpin4.setPrecValue(prec);
}


void CPoint4::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CPoint4::reset()
{
  this->setPoint4Value(Point4(0, 0, 0, 0));
  PropertyControlBase::reset();
}


void CPoint4::onWcChange(WindowBase *source)
{
  if (source == &mSpin1)
    mValue.x = mSpin1.getValue();
  else if (source == &mSpin2)
    mValue.y = mSpin2.getValue();
  else if (source == &mSpin3)
    mValue.z = mSpin3.getValue();
  else if (source == &mSpin4)
    mValue.w = mSpin4.getValue();

  PropertyControlBase::onWcChange(source);
}


void CPoint4::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  mSpin1.moveWindow(x, y + _pxS(DEFAULT_CONTROL_HEIGHT));
  mSpin2.moveWindow(x + mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT));
  mSpin3.moveWindow(x + 2 * mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT));
  mSpin4.moveWindow(x + 3 * mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT));
}


void CPoint4::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mSpin1.setEnabled(enabled);
  mSpin2.setEnabled(enabled);
  mSpin3.setEnabled(enabled);
  mSpin4.setEnabled(enabled);
}


void CPoint4::setWidth(hdpi::Px w)
{
  PropertyControlBase::setWidth(w);

  mSpinWidth = _px(w) / 4;

  mCaption.resizeWindow(_px(w), mCaption.getHeight());
  mSpin1.resizeWindow(mSpinWidth, mSpin1.getHeight());
  mSpin2.resizeWindow(mSpinWidth, mSpin2.getHeight());
  mSpin3.resizeWindow(mSpinWidth, mSpin3.getHeight());
  mSpin4.resizeWindow(mSpinWidth, mSpin3.getHeight());

  this->moveTo(this->getX(), this->getY());
}
