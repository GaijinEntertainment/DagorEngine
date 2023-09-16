// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_track_bar_int.h"
#include "../c_constants.h"

CTrackBarInt::CTrackBarInt(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w,
  const char caption[], int min, int max, int step) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, DEFAULT_CONTROL_HEIGHT * 2),
  mCaption(this, parent->getWindow(), x, y, w, DEFAULT_CONTROL_HEIGHT),
  mTrackBar(this, parent->getWindow(), x, y + DEFAULT_CONTROL_HEIGHT,
    w - DEFAULT_CONTROL_WIDTH - UP_DOWN_BUTTON_WIDTH - DEFAULT_CONTROLS_INTERVAL, DEFAULT_CONTROL_HEIGHT, min, max, step),
  mEditor(this, parent->getWindow(), x + mTrackBar.getWidth() + DEFAULT_CONTROLS_INTERVAL, y + DEFAULT_CONTROL_HEIGHT,
    DEFAULT_CONTROL_WIDTH + UP_DOWN_BUTTON_WIDTH, DEFAULT_CONTROL_HEIGHT, min, max, step, 0)
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


void CTrackBarInt::setWidth(unsigned w)
{
  int minw = DEFAULT_CONTROL_WIDTH + DEFAULT_CONTROLS_INTERVAL + UP_DOWN_BUTTON_WIDTH;
  w = (w < minw) ? minw : w;

  PropertyControlBase::setWidth(w);

  mCaption.resizeWindow(w, mCaption.getHeight());
  mTrackBar.resizeWindow(w - minw, mTrackBar.getHeight());
  mEditor.resizeWindow(DEFAULT_CONTROL_WIDTH + UP_DOWN_BUTTON_WIDTH, mEditor.getHeight());

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
  mEditor.moveWindow(x + mTrackBar.getWidth() + DEFAULT_CONTROLS_INTERVAL, (hasCaption) ? y + mCaption.getHeight() : y);
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
