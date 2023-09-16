// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_spin_edit.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// -------------- SpinEdit --------------


WSpinEdit::WSpinEdit(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, float min, float max,
  float step, int prec) :

  mExternalHandler(event_handler),
  WEdit(this, parent, x, y, w - h, h),
  mUpDown(this, parent, x + w - h, y, h, h),

  mMin(min),
  mMax(max),
  mStep(step),
  mManualSet(false)
{
  mPrec = (prec > 0) ? prec - 1 : 0;
  this->setValueInternal(0);
  this->setMinMaxStepValue(min, max, step);
  mExternalVal = this->getValue();
}


void WSpinEdit::setEnabled(bool enabled)
{
  WEdit::setEnabled(enabled);
  mUpDown.setEnabled(enabled);

  WindowBase::setEnabled(enabled);
}


void WSpinEdit::resizeWindow(int w, int h)
{
  WEdit::resizeWindow(w - h, h);
  mUpDown.resizeWindow(h, h);
  mUpDown.moveWindow(getX() + w - h, getY());
}


void WSpinEdit::moveWindow(int x, int y)
{
  WEdit::moveWindow(x, y);
  mUpDown.moveWindow(getX() + getWidth(), getY());
}


float WSpinEdit::correctBounds(float value)
{
  if (mMin != mMax)
  {
    value = (value < mMin) ? mMin : value;
    value = (value > mMax) ? mMax : value;
  }

  return value;
}

void WSpinEdit::sendWcChangeIfVarChanged()
{
  if (mExternalVal == this->getValue())
    return;

  mExternalVal = this->getValue();
  mExternalHandler->onWcChange(this);
}

void WSpinEdit::setValue(float value)
{
  if (isActive()) // dont allow to overwrite our value from external source while it's being edited
    return;

  this->setValueInternal(value);
  mExternalVal = value;
}

void WSpinEdit::setValueInternal(float value)
{
  value = this->correctBounds(value);
  mManualSet = true;
  char buf[64];
  float minval = mPrec > 0 ? powf(10.0f, -mPrec) : 1.0f;
  int prec = mPrec;
  if (fabs(value) > 1e-16)
    while (prec < 10 && fabs(value) < minval * 100.0f)
    {
      prec++;
      minval *= 0.1f;
    }
  sprintf(buf, "%.*f", prec, value);
  char *p = buf + strlen(buf) - 1;
  if (strchr(buf, '.'))
  {
    while (p > buf && *p == '0')
    {
      *p = '\0';
      p--;
    }
    if (p > buf && *p == '.')
      *p = '\0';
  }

  this->setTextValue(buf);
  mInternalVal = value;
}


void WSpinEdit::setMinMaxStepValue(float min, float max, float step)
{
  int p = 0;

  mMin = min;
  mMax = max;
  mStep = step;

  if (mMin == mMax)
    mStep = pow(0.1, mPrec);
  else
  {
    p = ceil(log10(1 / mStep));
    mPrec = (p > 0) ? p : 0;
  }

  this->setValueInternal(this->getValue());
}


void WSpinEdit::setPrecValue(int prec)
{
  mPrec = prec - 1;
  mStep = pow(0.1, mPrec);
  this->setValueInternal(this->getValue());
}


void WSpinEdit::onWcClick(WindowBase *source)
{
  float val = this->getValue();

  if (source == &mUpDown)
  {
    if (mUpDown.lastUpClicked())
      val += mStep;
    else
      val -= mStep;
  }

  this->setValueInternal(val);
  sendWcChangeIfVarChanged();
}


void WSpinEdit::onWcChange(WindowBase *source)
{
  if ((!mManualSet) && (source == this))
  {
    char buf[20];
    if (this->getTextValue((char *)&buf, 20))
    {
      float val = atof(buf);
      val = this->correctBounds(val);
      mInternalVal = val;
    }
  }
  else
  {
    mManualSet = false;
  }
}


intptr_t WSpinEdit::onKeyDown(unsigned v_key, int flags)
{
  float _val = this->getValue();
  float _step = -mStep;

  switch (v_key)
  {
    case VK_UP:
      _step = mStep;
      // fallthrough!

    case VK_DOWN:
      this->setValueInternal(_val + _step);
      // fallthrough!

    case VK_RETURN: sendWcChangeIfVarChanged(); return 1;
  }

  return 0;
}

void WSpinEdit::onKillFocus() { sendWcChangeIfVarChanged(); }

static const char *VALID_CHARACTERS_FOR_INT = "0123456789-";

intptr_t WSpinEdit::onChar(unsigned code, int flags)
{
  WORD vkCode = LOWORD(code);
  if (vkCode == 0x3 || vkCode == 0x16 || vkCode == 0x18 || vkCode == '\n' || vkCode == VK_ESCAPE || vkCode == '\b')
    return 0;
  return strchr(VALID_CHARACTERS_FOR_INT, code) == nullptr;
}

// -------------- FloatSpinEdit --------------

static const char *VALID_CHARACTERS_FOR_FLOAT = ".eE";

WFloatSpinEdit::WFloatSpinEdit(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, float min,
  float max, float step, int prec) :
  WSpinEdit(event_handler, parent, x, y, w, h, min, max, step, prec)
{}

intptr_t WFloatSpinEdit::onChar(unsigned code, int flags)
{
  if (!WSpinEdit::onChar(code, flags))
    return 0;
  return strchr(VALID_CHARACTERS_FOR_FLOAT, code) == nullptr;
}
