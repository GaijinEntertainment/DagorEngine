// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_point2.h"
#include "../c_constants.h"


CPoint2::CPoint2(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  const char caption[], int prec) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_CONTROL_HEIGHT) * 2),
  mCaption(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_CONTROL_HEIGHT)),
  mSpinWidth(_px(w) / 2),
  mSpin1(this, parent->getWindow(), x, y + _pxS(DEFAULT_CONTROL_HEIGHT), mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 0, prec),
  mSpin2(this, parent->getWindow(), x + mSpinWidth, y + _pxS(DEFAULT_CONTROL_HEIGHT), mSpinWidth, _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0,
    0, prec)
{
  hasCaption = strlen(caption) > 0;
  mCaption.setTextValue(caption);
  this->setPoint2Value(Point2(0, 0));

  if (!hasCaption)
  {
    mCaption.hide();
    mH = mSpin1.getHeight();
    this->moveTo(x, y);
  }
  initTooltip(&mSpin1);
  initTooltip(&mSpin2);
}


PropertyContainerControlBase *CPoint2::createDefault(int id, PropertyContainerControlBase *parent, const char caption[], bool new_line)
{
  parent->createPoint2(id, caption, Point2(0, 0), 2, true, new_line);
  return NULL;
}


Point2 CPoint2::getPoint2Value() const { return mValue; }


void CPoint2::setPoint2Value(Point2 value)
{
  mValue = value;
  mSpin1.setValue(mValue.x);
  mSpin2.setValue(mValue.y);
}


void CPoint2::setPrecValue(int prec)
{
  mSpin1.setPrecValue(prec);
  mSpin2.setPrecValue(prec);
}


void CPoint2::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CPoint2::reset()
{
  this->setPoint2Value(Point2(0, 0));
  PropertyControlBase::reset();
}


void CPoint2::onWcChange(WindowBase *source)
{
  if (source == &mSpin1)
    mValue.x = mSpin1.getValue();
  else if (source == &mSpin2)
    mValue.y = mSpin2.getValue();

  PropertyControlBase::onWcChange(source);
}


void CPoint2::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  mSpin1.moveWindow(x, (hasCaption) ? y + mCaption.getHeight() : y);
  mSpin2.moveWindow(x + mSpinWidth, (hasCaption) ? y + mCaption.getHeight() : y);
}


void CPoint2::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mSpin1.setEnabled(enabled);
  mSpin2.setEnabled(enabled);
}


void CPoint2::setWidth(hdpi::Px w)
{
  PropertyControlBase::setWidth(w);

  mSpinWidth = _px(w) / 2;

  mCaption.resizeWindow(_px(w), mCaption.getHeight());
  mSpin1.resizeWindow(mSpinWidth, mSpin1.getHeight());
  mSpin2.resizeWindow(mSpinWidth, mSpin2.getHeight());

  this->moveTo(this->getX(), this->getY());
}
