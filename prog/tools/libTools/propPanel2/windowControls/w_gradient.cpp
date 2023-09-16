// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_gradient.h"
#include <propPanel2/comWnd/color_dialog.h>

#include <windows.h>
#include <stdlib.h>

// -----------------Track Gradient Button --------------------------

WTrackGradientButton::WTrackGradientButton(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, E3DCOLOR color,
  bool moveable, WGradient *owner, float _value) :
  WindowControlBase(event_handler, parent, "BUTTON", 0, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_NOTIFY, "",
    x - TRACK_GRADIENT_BUTTON_WIDTH / 2, y, TRACK_GRADIENT_BUTTON_WIDTH, TRACK_GRADIENT_BUTTON_HEIGHT),
  mTooltip(event_handler, this),
  value(0)
{
  mOwner = owner;
  value = _value > 1.0f ? 1.0f : _value;
  value = value < 0.0f ? 0.0f : value;
  mColor = color;
  canMove = moveable;
  useful = true;
  firstDragMsg = true;
  moving = false;

  updateTooltipText();
}

intptr_t WTrackGradientButton::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!moving)
  {
    switch (notify_code)
    {
      case BN_CLICKED:
      {
        E3DCOLOR _color = mColor;
        this->setEnabled(false);
        if (ColorDialog::select_color(this->getHandle(), "Select color", _color))
        {
          if (mEventHandler)
            mEventHandler->onWcChanging(this);

          mColor = _color;
          mOwner->updateCycled(this);

          if (mEventHandler)
            mEventHandler->onWcChange(this);
        }
        this->setEnabled(true);
      }
      break;

      case BN_DBLCLK:
        // hack: to make click from dbclick
        SendMessage((HWND)mHandle, WM_LBUTTONDOWN, 0, 0);
        break;

      default: break;
    }
  }

  moving = false;
  firstDragMsg = true;
  updateTooltipText();

  return 0;
}

intptr_t WTrackGradientButton::onDrag(int new_x, int new_y)
{
  if (firstDragMsg)
  {
    firstDragMsg = false;
    return 0;
  }

  int new_pos = new_x + mX - TRACK_GRADIENT_BUTTON_WIDTH / 2;

  int leftLimit, rightLimit;
  mOwner->getLimits(this, leftLimit, rightLimit);

  if (new_pos < leftLimit + TRACK_GRADIENT_BUTTON_WIDTH)
    new_pos = leftLimit + TRACK_GRADIENT_BUTTON_WIDTH;
  if (new_pos > rightLimit - TRACK_GRADIENT_BUTTON_WIDTH)
    new_pos = rightLimit - TRACK_GRADIENT_BUTTON_WIDTH;

  if (canMove)
  {
    new_x = new_pos - mX + TRACK_GRADIENT_BUTTON_WIDTH / 2;

    float valueNew = (float)(new_x + mX - mOwner->getX()) / (float)(mOwner->getWidth());
    valueNew = (valueNew > 1.0f) ? 1.0f : (valueNew < 0.0f) ? 0.0f : valueNew;

    if (valueNew != value)
    {
      if (!moving && mEventHandler)
        mEventHandler->onWcChanging(this);

      value = valueNew;
      moveWindow(new_pos, mY);

      updateTooltipText();

      if (mEventHandler)
        mEventHandler->onWcChange(this);
    }
  }

  moving = true;

  return 0;
}

intptr_t WTrackGradientButton::onRButtonUp(long, long)
{
  if (!canMove || !mOwner->canRemove())
    return 0;

  useful = false;
  mOwner->refresh();
  return 0;
}

intptr_t WTrackGradientButton::onControlDrawItem(void *info)
{
  DRAWITEMSTRUCT *draw = (DRAWITEMSTRUCT *)info;
  HDC hDC = (HDC)draw->hDC;

  RECT curRect;
  curRect.left = 0;
  curRect.top = 0;
  curRect.right = mWidth;
  curRect.bottom = 1;

  HBRUSH hBrush;
  COLORREF inColor = RGB(0, 0, 0);
  hBrush = CreateSolidBrush(inColor);
  FillRect((HDC)hDC, &curRect, hBrush);
  DeleteObject(hBrush);

  int x = 0;
  int y = 1;

  COLORREF color = RGB(mColor.r, mColor.g, mColor.b);

  for (int i = 0; i <= mWidth / 2;)
  {
    for (int s = 0; s < 2; s++)
    {
      x = i;
      SetPixel(hDC, x, y, inColor);
      for (int j = i + 1; j < mWidth - i - 1; j++)
        SetPixel(hDC, j, y, color);
      SetPixel(hDC, mWidth - i - 1, y, inColor);
      y++;
      if (x == 0)
        break;
    }
    i++;
  }

  return 0;
}

int WTrackGradientButton::getPos() { return mX; }

float WTrackGradientButton::getValue() { return value; }

void WTrackGradientButton::setValue(float new_value)
{
  value = new_value;
  value = value > 1.0f ? 1.0f : value;
  value = value < 0.0f ? 0.0f : value;
  moveWindow(value * (float)(mOwner->getWidth()), mY);
}

void WTrackGradientButton::updateTooltipText()
{
  if (moving)
    mTooltip.setText("");
  else
  {
    String text(128, "Position: %1.3f, RGB: (%d, %d, %d)", getValue(), mColor.r, mColor.g, mColor.b);
    mTooltip.setText(text);
  }
}

// -------------- Gradient ---------------------------

WGradient::WGradient(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w) :

  WindowControlBase(event_handler, parent, "BUTTON", 0, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_NOTIFY, "", x,
    y + TRACK_GRADIENT_BUTTON_HEIGHT, w, GRADIENT_HEIGHT),
  mKeys(midmem),
  mCycled(false),
  mMinPtCount(2),
  mMaxPtCount(100),
  mSelected(false)
{
  reset();
}


WGradient::~WGradient() { clear_all_ptr_items(mKeys); }


intptr_t WGradient::onControlDrawItem(void *info)
{
  for (int i = 0; i < mKeys.size(); i++)
  {
    if (mKeys[i]->useless())
    {
      if (mEventHandler)
        mEventHandler->onWcChanging(this);

      delete mKeys[i];
      erase_items(mKeys, i, 1);

      if (mEventHandler)
        mEventHandler->onWcChange(this);
    }
  }

  DRAWITEMSTRUCT *draw = (DRAWITEMSTRUCT *)info;

  RECT curRect;
  curRect.left = 1;
  curRect.top = 1;
  curRect.right = 2;
  curRect.bottom = mHeight - 1;

  for (int i = 0; i < (int)mKeys.size() - 1; i++)
  {
    E3DCOLOR startColor = mKeys[i]->getColorValue();
    E3DCOLOR endColor = mKeys[i + 1]->getColorValue();
    E3DCOLOR color = startColor;

    float step[3];
    int w = mKeys[i + 1]->getPos() - mKeys[i]->getPos();

    step[0] = (float)(endColor.r - startColor.r) / w;
    step[1] = (float)(endColor.g - startColor.g) / w;
    step[2] = (float)(endColor.b - startColor.b) / w;

    int n = 0;
    while (curRect.left < (mKeys[i + 1]->getPos() - TRACK_GRADIENT_BUTTON_WIDTH / 2 - 2))
    {
      int r = startColor.r + step[0] * n;
      int g = startColor.g + step[1] * n;
      int b = startColor.b + step[2] * n;

      HBRUSH hBrush;
      hBrush = CreateSolidBrush(RGB(r, g, b));

      FillRect(draw->hDC, &curRect, hBrush);
      DeleteObject(hBrush);

      curRect.left += 1;
      curRect.right += 1;
      n++;
    }
  }

  // draw current X value
  if (mCurValue)
  {
    E3DCOLOR curColor = getColor(mCurValue);
    int mid_value = (curColor.r + curColor.g + curColor.b) / 3;
    HPEN cur_pen = (mid_value < 128) ? CreatePen(PS_DOT, 1, RGB(255, 255, 255)) : CreatePen(PS_DOT, 1, RGB(0, 0, 0));

    HPEN old_pen = (HPEN)SelectObject(draw->hDC, cur_pen);

    MoveToEx(draw->hDC, mWidth * mCurValue, 0, NULL);
    LineTo(draw->hDC, mWidth * mCurValue, mHeight - 1);

    SelectObject(draw->hDC, old_pen);
    DeleteObject(cur_pen);
  }

  // draw rect

  HPEN rect_pen = mSelected ? CreatePen(PS_SOLID, 1, RGB(255, 0, 0)) : CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
  HPEN old_pen = (HPEN)SelectObject(draw->hDC, rect_pen);

  MoveToEx(draw->hDC, 0, 0, NULL);
  LineTo(draw->hDC, mWidth - 1, 0);
  LineTo(draw->hDC, mWidth - 1, mHeight - 1);
  LineTo(draw->hDC, 0, mHeight - 1);
  LineTo(draw->hDC, 0, 0);

  SelectObject(draw->hDC, old_pen);
  DeleteObject(rect_pen);

  return 0;
}

intptr_t WGradient::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!mEventHandler)
    return 0;

  switch (notify_code)
  {
    case BN_DBLCLK:
      POINT mouse_pos;
      GetCursorPos(&mouse_pos);
      ScreenToClient((HWND)this->getHandle(), &mouse_pos);

      mEventHandler->onWcChanging(this);

      addKey((float)mouse_pos.x / (float)mWidth);

      mEventHandler->onWcChange(this);

      // SetFocus((HWND) mParent->getHandle());
      break;

    default: break;
  }
  return 0;
}


bool WGradient::canRemove() { return (mKeys.size() > mMinPtCount); }

void WGradient::addKey(float position, E3DCOLOR *color)
{
  if ((position < 0) || (position > 1))
    return;

  if (mKeys.size() >= mMaxPtCount)
    return;

  E3DCOLOR curColor;
  if (color == NULL)
    curColor = getColor(position);
  else
    curColor = *color;

  int x = position * mWidth;

  int posTest = x + mX - TRACK_GRADIENT_BUTTON_WIDTH / 2;
  for (int i = 0; i < mKeys.size(); i++)
  {
    if (abs(posTest - mKeys[i]->getPos()) < TRACK_GRADIENT_BUTTON_WIDTH)
      return;
  }

  WTrackGradientButton *key =
    new WTrackGradientButton(mEventHandler, mParent, mX + x, mY - TRACK_GRADIENT_BUTTON_HEIGHT, curColor, true, this, position);

  for (int i = 0; i < (int)mKeys.size() - 1; i++)
  {
    if ((x > mKeys[i]->getPos()) && (x < mKeys[i + 1]->getPos()))
    {
      insert_item_at(mKeys, i + 1, key);
      return;
    }
  }

  //??????????
  delete key;
}

E3DCOLOR WGradient::getColor(float position)
{
  int x = position * mWidth;
  int curX = 0;

  for (int i = 0; i < (int)mKeys.size() - 1; i++)
  {
    E3DCOLOR startColor = mKeys[i]->getColorValue();
    E3DCOLOR endColor = mKeys[i + 1]->getColorValue();
    E3DCOLOR color = startColor;

    float step[3];
    int w = mKeys[i + 1]->getPos() - mKeys[i]->getPos();

    step[0] = (float)(endColor.r - startColor.r) / w;
    step[1] = (float)(endColor.g - startColor.g) / w;
    step[2] = (float)(endColor.b - startColor.b) / w;

    int n = 0;
    while (curX < mKeys[i + 1]->getPos())
    {
      if (curX == x)
      {
        int r = startColor.r + step[0] * n;
        int g = startColor.g + step[1] * n;
        int b = startColor.b + step[2] * n;
        return E3DCOLOR(r, g, b);
      }
      curX++;
      n++;
    }
  }
  return E3DCOLOR(0, 0, 0);
}


void WGradient::moveWindow(int x, int y)
{
  for (int i = 0; i < mKeys.size(); i++)
  {
    mKeys[i]->moveWindow(x + mKeys[i]->getValue() * mWidth - TRACK_GRADIENT_BUTTON_WIDTH / 2, y);
  }
  WindowControlBase::moveWindow(x, y + TRACK_GRADIENT_BUTTON_HEIGHT);
  refresh(true);
}

void WGradient::getLimits(WTrackGradientButton *key, int &left_limit, int &right_limit)
{
  left_limit = right_limit = key->getPos();

  for (int i = 0; i < mKeys.size(); i++)
  {
    if (key == mKeys[i])
    {
      if (i > 0)
        left_limit = mKeys[i - 1]->getPos();

      if (i + 1 < mKeys.size())
        right_limit = mKeys[i + 1]->getPos();

      return;
    }
  }
}

void WGradient::setValue(PGradient source)
{
  if (!source)
    return;

  Gradient &value = *source;

  if (value.size() < 2)
    return;

  for (int i = 0; i < value.size(); i++)
  {
    if ((value[i].position < 0) || (value[i].position > 1.0f))
      return;
  }

  for (int i = 0; i < mKeys.size(); i++)
  {
    if (mKeys[i]->isActive())
      setFocus();
    delete mKeys[i];
  }
  clear_and_shrink(mKeys);

  WTrackGradientButton *left =
    new WTrackGradientButton(mEventHandler, mParent, mX, mY - TRACK_GRADIENT_BUTTON_HEIGHT, value[0].color, false, this, 0);

  WTrackGradientButton *right = new WTrackGradientButton(mEventHandler, mParent, mX + mWidth - 1, mY - TRACK_GRADIENT_BUTTON_HEIGHT,
    (mCycled) ? value[0].color : value.back().color, false, this, 1);

  mKeys.push_back(left);
  mKeys.push_back(right);

  for (int i = 1; i < (int)value.size() - 1; i++)
    addKey(value[i].position, &value[i].color);
}

void WGradient::setCurValue(float value)
{
  mCurValue = value;
  refresh();
}


void WGradient::setMinMax(int min, int max)
{
  mMinPtCount = min;
  mMaxPtCount = max;

  if (mMinPtCount < 2)
    mMinPtCount = 2;

  if (mMaxPtCount < mMinPtCount)
    mMaxPtCount = mMinPtCount;
}


void WGradient::setSelected(bool value)
{
  mSelected = value;
  refresh(true);
}


void WGradient::getValue(PGradient destGradient) const
{
  if (!destGradient)
    return;

  clear_and_shrink(*destGradient);

  GradientKey current;

  for (int i = 0; i < mKeys.size(); i++)
  {
    current.position = mKeys[i]->getValue();
    current.color = mKeys[i]->getColorValue();
    destGradient->push_back(current);
  }
}

void WGradient::resizeWindow(int w, int h)
{
  WindowBase::resizeWindow(w, h);
  for (int i = 0; i < mKeys.size(); i++)
    mKeys[i]->setValue(mKeys[i]->getValue());
}

void WGradient::reset()
{
  for (int i = 0; i < mKeys.size(); i++)
    delete mKeys[i];
  clear_and_shrink(mKeys);

  WTrackGradientButton *left =
    new WTrackGradientButton(mEventHandler, mParent, mX, mY - TRACK_GRADIENT_BUTTON_HEIGHT, E3DCOLOR(255, 255, 255), false, this, 0);

  WTrackGradientButton *right = new WTrackGradientButton(mEventHandler, mParent, mX + mWidth - 1, mY - TRACK_GRADIENT_BUTTON_HEIGHT,
    E3DCOLOR(0, 0, 0), false, this, 1);

  mKeys.push_back(left);
  mKeys.push_back(right);
}

void WGradient::redrawKeys()
{
  for (int i = 0; i < mKeys.size(); i++)
  {
    mKeys[i]->refresh(true);
  }
}


void WGradient::updateCycled(WTrackGradientButton *button)
{
  if (!mCycled || !(button == mKeys[0] || button == mKeys.back()))
    return;

  int i = (button == mKeys[0]) ? 0 : mKeys.size() - 1;
  int j = (i == 0) ? mKeys.size() - 1 : 0;

  mKeys[j]->setColorValue(mKeys[i]->getColorValue());
}
