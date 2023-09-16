// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_point3.h"
#include "../c_constants.h"

CPoint3::CPoint3(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w,
  const char caption[], int prec) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, DEFAULT_CONTROL_HEIGHT * 2),
  mCaption(this, parent->getWindow(), x, y, w, DEFAULT_CONTROL_HEIGHT),
  mSpinWidth(w / 3),
  mSpin1(this, parent->getWindow(), x, y + DEFAULT_CONTROL_HEIGHT, mSpinWidth, DEFAULT_CONTROL_HEIGHT, 0, 0, 0, prec),
  mSpin2(this, parent->getWindow(), x + mSpinWidth, y + DEFAULT_CONTROL_HEIGHT, mSpinWidth, DEFAULT_CONTROL_HEIGHT, 0, 0, 0, prec),
  mSpin3(this, parent->getWindow(), x + 2 * mSpinWidth, y + DEFAULT_CONTROL_HEIGHT, mSpinWidth, DEFAULT_CONTROL_HEIGHT, 0, 0, 0, prec)
{
  hasCaption = strlen(caption) > 0;

  if (hasCaption)
  {
    mCaption.setTextValue(caption);
    mH = DEFAULT_CONTROL_HEIGHT + mSpin1.getHeight();
  }
  else
  {
    mCaption.hide();
    mH = mSpin1.getHeight();
    this->moveTo(x, y);
  }
  initTooltip(&mSpin1);
  initTooltip(&mSpin2);
  initTooltip(&mSpin3);

  this->setPoint3Value(Point3(0, 0, 0));
}


PropertyContainerControlBase *CPoint3::createDefault(int id, PropertyContainerControlBase *parent, const char caption[], bool new_line)
{
  parent->createPoint3(id, caption, Point3(0, 0, 0), 2, true, new_line);
  return NULL;
}


Point3 CPoint3::getPoint3Value() const { return mValue; }


void CPoint3::setPoint3Value(Point3 value)
{
  mValue = value;
  mSpin1.setValue(mValue.x);
  mSpin2.setValue(mValue.y);
  mSpin3.setValue(mValue.z);
}

void CPoint3::setPrecValue(int prec)
{
  mSpin1.setPrecValue(prec);
  mSpin2.setPrecValue(prec);
  mSpin3.setPrecValue(prec);
}


void CPoint3::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CPoint3::reset()
{
  this->setPoint3Value(Point3(0, 0, 0));
  PropertyControlBase::reset();
}


void CPoint3::onWcChange(WindowBase *source)
{
  if (source == &mSpin1)
    mValue.x = mSpin1.getValue();
  else if (source == &mSpin2)
    mValue.y = mSpin2.getValue();
  else if (source == &mSpin3)
    mValue.z = mSpin3.getValue();

  PropertyControlBase::onWcChange(source);
}


void CPoint3::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  int spin_y = (hasCaption) ? y + mCaption.getHeight() : y;
  mSpin1.moveWindow(x, spin_y);
  mSpin2.moveWindow(x + mSpinWidth, spin_y);
  mSpin3.moveWindow(x + 2 * mSpinWidth, spin_y);
}


void CPoint3::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mSpin1.setEnabled(enabled);
  mSpin2.setEnabled(enabled);
  mSpin3.setEnabled(enabled);
}


void CPoint3::setWidth(unsigned w)
{
  PropertyControlBase::setWidth(w);

  mSpinWidth = w / 3;

  mCaption.resizeWindow(w, mCaption.getHeight());
  mSpin1.resizeWindow(mSpinWidth, mSpin1.getHeight());
  mSpin2.resizeWindow(mSpinWidth, mSpin2.getHeight());
  mSpin3.resizeWindow(mSpinWidth, mSpin3.getHeight());

  this->moveTo(this->getX(), this->getY());
}
