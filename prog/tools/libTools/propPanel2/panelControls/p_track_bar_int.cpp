// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_track_bar_int.h"
#include "../c_constants.h"

CTrackBarInt::CTrackBarInt(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  const char caption[], int min, int max, int step) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_CONTROL_HEIGHT) * 2),
  mCaption(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_CONTROL_HEIGHT)),
  mTrackBar(this, parent->getWindow(), x, y + _pxS(DEFAULT_CONTROL_HEIGHT),
    _px(w) - _pxS(DEFAULT_CONTROL_WIDTH) - _pxS(UP_DOWN_BUTTON_WIDTH) - _pxS(DEFAULT_CONTROLS_INTERVAL), _pxS(DEFAULT_CONTROL_HEIGHT),
    min, max, step),
  mEditor(this, parent->getWindow(), x + mTrackBar.getWidth() + _pxS(DEFAULT_CONTROLS_INTERVAL), y + _pxS(DEFAULT_CONTROL_HEIGHT),
    _pxS(DEFAULT_CONTROL_WIDTH) + _pxS(UP_DOWN_BUTTON_WIDTH), _pxS(DEFAULT_CONTROL_HEIGHT), min, max, step, 0)
{
  hasCaption = strlen(caption) > 0;
  mCaption.setTextValue(caption);
  mEditor.setValue(min);
  mTrackBar.setValue(min);

  if (!hasCaption)
  {
    mCaption.hide();
    mH = mTrackBar.getHeight();
    this->moveTo(x, y);
  }
  initTooltip(&mEditor);
}


PropertyContainerControlBase *CTrackBarInt::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createTrackInt(id, caption, 0, 0, 10, 1, true, new_line);
  return NULL;
}


void CTrackBarInt::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mEditor.setEnabled(enabled);
  mTrackBar.setEnabled(enabled);
}


void CTrackBarInt::setWidth(hdpi::Px w)
{
  int minw = _pxS(DEFAULT_CONTROL_WIDTH) + _pxS(DEFAULT_CONTROLS_INTERVAL) + _pxS(UP_DOWN_BUTTON_WIDTH);
  w = (_px(w) < minw) ? _pxActual(minw) : w;

  PropertyControlBase::setWidth(w);

  mCaption.resizeWindow(_px(w), mCaption.getHeight());
  mTrackBar.resizeWindow(_px(w) - minw, mTrackBar.getHeight());
  mEditor.resizeWindow(_pxS(DEFAULT_CONTROL_WIDTH) + _pxS(UP_DOWN_BUTTON_WIDTH), mEditor.getHeight());

  this->moveTo(this->getX(), this->getY());
}


int CTrackBarInt::getIntValue() const { return mValue; }


void CTrackBarInt::setIntValue(int value)
{
  mTrackBar.setValue(value);
  mValue = mTrackBar.getValue();
  mEditor.setValue(mValue);
}


void CTrackBarInt::setMinMaxStepValue(float min, float max, float step)
{
  mEditor.setMinMaxStepValue(floor(min), ceil(max), ceil(step));
  mTrackBar.setMinMaxStepValue(floor(min), ceil(max), ceil(step));
}


void CTrackBarInt::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CTrackBarInt::reset()
{
  this->setIntValue(floor(mEditor.getMinValue()));
  PropertyControlBase::reset();
}


void CTrackBarInt::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  mTrackBar.moveWindow(x, (hasCaption) ? y + mCaption.getHeight() : y);
  mEditor.moveWindow(x + mTrackBar.getWidth() + _pxS(DEFAULT_CONTROLS_INTERVAL), (hasCaption) ? y + mCaption.getHeight() : y);
}


void CTrackBarInt::onWcChange(WindowBase *source)
{
  if (source == &mTrackBar)
  {
    mValue = mTrackBar.getValue();
    mEditor.setValue(mValue);
  }
  else if (source == &mEditor)
  {
    mValue = mEditor.getValue();
    mTrackBar.setValue(mValue);
  }

  PropertyControlBase::onWcChange(source);
}
