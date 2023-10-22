// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_track_bar_float.h"
#include "../c_constants.h"

CTrackBarFloat::CTrackBarFloat(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y,
  hdpi::Px w, const char caption[], float min, float max, float step, float power) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_CONTROL_HEIGHT) * 2),
  mCaption(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_CONTROL_HEIGHT)),
  mTrackBar(this, parent->getWindow(), x, y + _pxS(DEFAULT_CONTROL_HEIGHT),
    _px(w) - _pxS(DEFAULT_CONTROL_WIDTH) - _pxS(UP_DOWN_BUTTON_WIDTH) - _pxS(DEFAULT_CONTROLS_INTERVAL), _pxS(DEFAULT_CONTROL_HEIGHT),
    min, max, step, power),
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


PropertyContainerControlBase *CTrackBarFloat::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createTrackFloat(id, caption, 0, 0, 1, 0.1f, true, new_line);
  return NULL;
}


float CTrackBarFloat::getFloatValue() const { return mValue; }


void CTrackBarFloat::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mEditor.setEnabled(enabled);
  mTrackBar.setEnabled(enabled);
}


void CTrackBarFloat::setWidth(hdpi::Px w)
{
  int minw = _pxS(DEFAULT_CONTROL_WIDTH) + _pxS(DEFAULT_CONTROLS_INTERVAL) + _pxS(UP_DOWN_BUTTON_WIDTH);
  w = (_px(w) < minw) ? _pxActual(minw) : w;

  PropertyControlBase::setWidth(w);

  mCaption.resizeWindow(_px(w), mCaption.getHeight());
  mTrackBar.resizeWindow(_px(w) - minw, mTrackBar.getHeight());
  mEditor.resizeWindow(_pxS(DEFAULT_CONTROL_WIDTH) + _pxS(UP_DOWN_BUTTON_WIDTH), mEditor.getHeight());

  this->moveTo(this->getX(), this->getY());
}


void CTrackBarFloat::setFloatValue(float value)
{
  mTrackBar.setValue(value);
  mValue = mTrackBar.getValue();
  mEditor.setValue(mValue);
}


void CTrackBarFloat::setMinMaxStepValue(float min, float max, float step)
{
  mEditor.setMinMaxStepValue(min, max, step);
  mTrackBar.setMinMaxStepValue(min, max, step);
}


void CTrackBarFloat::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CTrackBarFloat::reset()
{
  this->setFloatValue(mEditor.getMinValue());
  PropertyControlBase::reset();
}


void CTrackBarFloat::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  mTrackBar.moveWindow(x, (hasCaption) ? y + mCaption.getHeight() : y);
  mEditor.moveWindow(x + mTrackBar.getWidth() + _pxS(DEFAULT_CONTROLS_INTERVAL), (hasCaption) ? y + mCaption.getHeight() : y);
}


void CTrackBarFloat::onWcChange(WindowBase *source)
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
