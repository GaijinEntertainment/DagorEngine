// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_track.h"

#include <windows.h>

#include <debug/dag_debug.h>

// -------------- TrackBar --------------


WTrackBar::WTrackBar(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, float min, float max,
  float step, float power) :

  WindowControlBase(event_handler, parent, "BUTTON", 0, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, "", // | BS_FLAT
    x, y, w, h),

  mLeader(event_handler, this, "STATIC", 0, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_ETCHEDHORZ, "", 0, (h - TRACK_LEADER_HEIGHT) / 2, w,
    TRACK_LEADER_HEIGHT),

  mRunner(event_handler, this, "STATIC", 0, WS_CHILD | WS_VISIBLE | WS_DLGFRAME, "", 0, 0, h / 2, h),

  mVal(min),
  mMin(min),
  mMax(max),
  mStep(step),
  mPower(power)
{
  mRunner.setZAfter(mLeader.getHandle());
}


void WTrackBar::setValue(float value)
{
  mVal = (value < mMin) ? mMin : value;
  mVal = (value > mMax) ? mMax : mVal;
  mVal = (mStep) ? mMin + floorf(0.5f + (mVal - mMin) / mStep) * mStep : mVal;

  int x, lastx;
  lastx = this->getWidth() - mRunner.getWidth();
  x = (mMax != mMin) ? lastx * calculateXScale((mVal - mMin) / (mMax - mMin)) : 0;

  if (value > mMax)
  {
    debug("Max = %f, value = %f, mVal = %f, lastx = %d, x = %d", mMax, value, mVal, lastx, x);
  }

  mRunner.moveWindow(x, 0);
  mLeader.updateWindow();
}


void WTrackBar::setMinMaxStepValue(float min, float max, float step)
{
  mMin = min;
  mMax = max;
  mStep = step;

  this->setValue(this->getValue());
}


void WTrackBar::setPower(float power)
{
  mPower = power;

  this->setValue(this->getValue());
}


void WTrackBar::setRunnerPos(int new_x)
{
  int x, lastx;
  float value;

  lastx = this->getWidth() - mRunner.getWidth();
  x = new_x - mRunner.getWidth() / 2;
  x = (x < 0) ? 0 : x;
  x = (x > lastx) ? lastx : x;
  value = (lastx > 0) ? (mMin + calculateValueScale((float)x / lastx) * (mMax - mMin)) : 0;

  this->setValue(value);
}


intptr_t WTrackBar::onDrag(int new_x, int new_y)
{
  this->setRunnerPos(new_x);
  mEventHandler->onWcChange(this);
  return 0;
}


intptr_t WTrackBar::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!mEventHandler)
    return 0;

  switch (notify_code)
  {
    case BN_CLICKED:

      POINT mouse_pos;
      GetCursorPos(&mouse_pos);
      ScreenToClient((HWND)this->getHandle(), &mouse_pos);
      this->setRunnerPos(mouse_pos.x);

      mEventHandler->onWcChange(this);
      break;

    default: break;
  }

  return 0;
}


void WTrackBar::setEnabled(bool enabled)
{
  mRunner.setEnabled(enabled);
  mLeader.setEnabled(enabled);

  WindowBase::setEnabled(enabled);
}


void WTrackBar::resizeWindow(int w, int h)
{
  mLeader.resizeWindow(w, TRACK_LEADER_HEIGHT);
  mLeader.moveWindow(0, (h - TRACK_LEADER_HEIGHT) / 2);

  WindowBase::resizeWindow(w, h);
  this->setValue(this->getValue());
}


float WTrackBar::calculateXScale(float value_scale) const { return pow(value_scale, 1.0f / mPower); }


float WTrackBar::calculateValueScale(float x_scale) const { return pow(x_scale, mPower); }
