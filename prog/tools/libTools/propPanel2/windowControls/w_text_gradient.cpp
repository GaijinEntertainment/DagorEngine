// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_text_gradient.h"
#include <propPanel2/comWnd/dialog_window.h>

#include <windows.h>
#include <stdlib.h>


WTextGradient::WTextGradient(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w) :

  WindowControlBase(event_handler, parent, "BUTTON", 0, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_NOTIFY, "", x, y, w,
    _pxS(TEXT_GRADIENT_HEIGHT)),
  mKeys(midmem),
  mTooltip(event_handler, this),
  changeKeyIndex(-1),
  moveKeyIndex(-1),
  isKeyMove(false),
  minCount(0),
  maxCount(100),
  oldXPos(0),
  curValue(-1),
  mSelected(false)
{
  updateTooltipText(0);
}


void WTextGradient::setMinMax(int min, int max)
{
  minCount = min;
  maxCount = max;
}


void WTextGradient::setValues(const TextGradient &source)
{
  mKeys = source;
  refresh(true);
}


void WTextGradient::getValues(TextGradient &destGradient) const { destGradient = mKeys; }


void WTextGradient::setCurrentKeyText(const char *value)
{
  if (changeKeyIndex == -1)
  {
    newValue = value;
    return;
  }
  mKeys[changeKeyIndex].text = value;
}


void WTextGradient::setCurValue(float value)
{
  curValue = value;
  refresh();
}


void WTextGradient::setSelected(bool value)
{
  mSelected = value;
  refresh(true);
}


const char *WTextGradient::getCurrentKeyText() const
{
  if (changeKeyIndex == -1)
    return newValue.str();
  return mKeys[changeKeyIndex].text.str();
}


void WTextGradient::addKey(float _position, const char *_text)
{
  TextGradientKey key(_position, _text);
  for (int i = 0; i < mKeys.size(); ++i)
    if (mKeys[i].position > _position)
    {
      insert_item_at(mKeys, i, key);
      return;
    }
  mKeys.push_back(key);
}


void WTextGradient::moveWindow(int x, int y)
{
  WindowControlBase::moveWindow(x, y);
  refresh(true);
}


void WTextGradient::resizeWindow(int w, int h)
{
  WindowBase::resizeWindow(w, h);
  refresh(true);
}


void WTextGradient::reset()
{
  clear_and_shrink(mKeys);
  mKeys.push_back(TextGradientKey(0, "start"));
  mKeys.push_back(TextGradientKey(1, "end"));
}


intptr_t WTextGradient::onControlDrawItem(void *info)
{
  DRAWITEMSTRUCT *draw = (DRAWITEMSTRUCT *)info;
  HPEN dark_pen = CreatePen(PS_SOLID, 1, RGB(64, 64, 64));
  HPEN light_pen = CreatePen(PS_SOLID, 1, RGB(224, 224, 224));
  HPEN cur_pen = CreatePen(PS_DOT, 1, RGB(255, 255, 255));
  HPEN old_pen = (HPEN)SelectObject(draw->hDC, cur_pen);
  int rx = _pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2;
  int rw = mWidth - 1 - _pxS(TRACK_GRADIENT_BUTTON_WIDTH);

  clearForDraw(draw->hDC);
  drawCurrent(draw->hDC, rx + 1, 0, rw);
  SelectObject(draw->hDC, light_pen);
  drawRuller(draw->hDC, rx, _pxS(TRACK_GRADIENT_BUTTON_HEIGHT) * 1.5, rw);
  SelectObject(draw->hDC, dark_pen);
  drawRuller(draw->hDC, rx + 1, _pxS(TRACK_GRADIENT_BUTTON_HEIGHT) * 1.5 + 1, rw);

  SelectObject(draw->hDC, old_pen);
  DeleteObject(dark_pen);
  DeleteObject(light_pen);

  drawKeys(draw->hDC);

  if (mSelected)
  {
    HPEN rect_pen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    HPEN old_pen = (HPEN)SelectObject(draw->hDC, rect_pen);

    MoveToEx(draw->hDC, 0, 0, NULL);
    LineTo(draw->hDC, mWidth - 1, 0);
    LineTo(draw->hDC, mWidth - 1, mHeight - 1);
    LineTo(draw->hDC, 0, mHeight - 1);
    LineTo(draw->hDC, 0, 0);

    SelectObject(draw->hDC, old_pen);
    DeleteObject(rect_pen);
  }

  return 0;
}


void WTextGradient::drawCurrent(void *hdc, int x, int y, int w)
{
  if (curValue == -1)
    return;

  int x_val = x + w * curValue;
  MoveToEx((HDC)hdc, x_val, y, NULL);
  LineTo((HDC)hdc, x_val, _pxS(TEXT_GRADIENT_HEIGHT));
}

void WTextGradient::drawRuller(void *hdc, int x, int y, int w)
{
  const int STROKE_SIZE = 8;
  MoveToEx((HDC)hdc, x, y, NULL);
  LineTo((HDC)hdc, x + w, y);
  LineTo((HDC)hdc, x + w, y + STROKE_SIZE);
  MoveToEx((HDC)hdc, x, y, NULL);
  LineTo((HDC)hdc, x, y + STROKE_SIZE);
  MoveToEx((HDC)hdc, x + w / 2, y, NULL);
  LineTo((HDC)hdc, x + w / 2, y + STROKE_SIZE);
  MoveToEx((HDC)hdc, x + w / 4, y, NULL);
  LineTo((HDC)hdc, x + w / 4, y + STROKE_SIZE / 1.5);
  MoveToEx((HDC)hdc, x + w * 3 / 4, y, NULL);
  LineTo((HDC)hdc, x + w * 3 / 4, y + STROKE_SIZE / 1.5);
}


void WTextGradient::clearForDraw(void *hdc)
{
  RECT curRect;
  int y_top = getStartYPos();
  curRect.top = 0;
  curRect.bottom = mHeight;
  curRect.left = 0;
  curRect.right = mWidth;
  HBRUSH hBrush = CreateSolidBrush(USER_GUI_COLOR);
  FillRect((HDC)hdc, &curRect, hBrush);
  DeleteObject(hBrush);
}


void WTextGradient::drawKeys(void *hdc)
{
  int xstart = getStartXPos();
  int xend = getEndXPos();
  int y_top = getStartYPos();

  HPEN dark_pen = CreatePen(PS_SOLID, 1, RGB(64, 64, 64));
  HPEN light_pen = CreatePen(PS_SOLID, 1, RGB(224, 224, 224));
  HPEN old_pen = (HPEN)SelectObject((HDC)hdc, light_pen);

  for (int i = 0; i < mKeys.size(); ++i)
  {
    int x_pos = xstart + mKeys[i].position * (xend - xstart);
    SelectObject((HDC)hdc, light_pen);

    for (int j = 1; j < _pxS(TRACK_GRADIENT_BUTTON_HEIGHT) - 1; ++j)
    {
      int _x = _pxS(TRACK_GRADIENT_BUTTON_WIDTH) * 0.5 * (_pxS(TRACK_GRADIENT_BUTTON_HEIGHT) - j - 1) /
               (_pxS(TRACK_GRADIENT_BUTTON_HEIGHT) - 2);

      MoveToEx((HDC)hdc, x_pos - _x, y_top + j, NULL);
      LineTo((HDC)hdc, x_pos + _x, y_top + j);
    }

    SelectObject((HDC)hdc, dark_pen);
    MoveToEx((HDC)hdc, x_pos, y_top + _pxS(TRACK_GRADIENT_BUTTON_HEIGHT), NULL);
    LineTo((HDC)hdc, x_pos - _pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2, y_top);
    LineTo((HDC)hdc, x_pos + _pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2, y_top);
    LineTo((HDC)hdc, x_pos, y_top + _pxS(TRACK_GRADIENT_BUTTON_HEIGHT));
  }

  SelectObject((HDC)hdc, old_pen);
  DeleteObject(dark_pen);
  DeleteObject(light_pen);
}


int WTextGradient::getStartXPos() { return _pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2 + 1; }


int WTextGradient::getStartYPos() { return _pxS(TRACK_GRADIENT_BUTTON_HEIGHT) / 2 - 1; }


int WTextGradient::getEndXPos() { return mWidth - _pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2 - 1; }


int WTextGradient::posToX(float _position) { return getStartXPos() + _position * (getEndXPos() - getStartXPos()); }


float WTextGradient::xToPos(int _x) { return 1.0 * (_x - getStartXPos()) / (getEndXPos() - getStartXPos()); }

int WTextGradient::getKeyIndexByX(int _x)
{
  for (int i = 0; i < mKeys.size(); ++i)
  {
    int key_x = posToX(mKeys[i].position);
    if (_x > key_x - _pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2 && _x < key_x + _pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2)
      return i;
  }
  return -1;
}


bool WTextGradient::showKeyDialog(SimpleString &_text, float _position)
{
  bool result = false;
  CDialogWindow *dialog = new CDialogWindow(NULL, _pxScaled(250), _pxScaled(150), "Key edit");
  dialog->getPanel()->createEditBox(1, "Text", _text.str());
  String sval(32, "Position: %.2f", _position);
  dialog->getPanel()->createStatic(2, sval.str());
  if (dialog->showDialog() == DIALOG_ID_OK)
  {
    _text = dialog->getPanel()->getText(1);
    result = true;
  }
  delete dialog;
  return result;
}


void WTextGradient::updateTooltipText(int x)
{
  int index = getKeyIndexByX(x);
  if (index != -1 && moveKeyIndex == -1 && changeKeyIndex == -1)
  {
    String text(128, "Position: %1.3f, Text: \"%s\"", mKeys[index].position, mKeys[index].text.str());
    mTooltip.setText(text);
  }
  else
    mTooltip.setText("");
}


intptr_t WTextGradient::onLButtonDClick(long x, long y)
{
  if (!mEventHandler || mKeys.size() >= maxCount)
    return 0;

  float position = xToPos(x);
  if (position < 0 || position > 1)
    return 0;

  if (getKeyIndexByX(x) != -1)
    return 0;

  SimpleString value("new key");
  changeKeyIndex = -1;
  if (mEventHandler->onWcChanging(this))
  {
    // if (!newValue.isEmpty())
    addKey(position, newValue.str());
  }
  else if (showKeyDialog(value, position))
    addKey(position, value.str());

  mEventHandler->onWcChange(this);
  return 0;
}


intptr_t WTextGradient::onLButtonDown(long x, long y)
{
  moveKeyIndex = getKeyIndexByX(x);
  oldXPos = x;
  return 0;
}


intptr_t WTextGradient::onLButtonUp(long x, long y)
{
  if (!mEventHandler)
    return 0;

  if (isKeyMove)
  {
    isKeyMove = false;
    moveKeyIndex = -1;
    return 0;
  }

  changeKeyIndex = getKeyIndexByX(x);
  if (changeKeyIndex == -1)
    return 0;

  if (!mEventHandler->onWcChanging(this))
    showKeyDialog(mKeys[changeKeyIndex].text, mKeys[changeKeyIndex].position);
  changeKeyIndex = -1;
  mEventHandler->onWcChange(this);
  return 0;
}


intptr_t WTextGradient::onRButtonUp(long x, long y)
{
  if (!mEventHandler || mKeys.size() <= minCount)
    return 0;

  int index = getKeyIndexByX(x);
  if (index != -1)
  {
    erase_items(mKeys, index, 1);
    mEventHandler->onWcChange(this);
    refresh(true);
  }
  return 0;
}


intptr_t WTextGradient::onDrag(int x, int y)
{
  if (moveKeyIndex != -1 && abs(oldXPos - x) > 3)
  {
    isKeyMove = true;
    int min_x = (moveKeyIndex == 0) ? getStartXPos() : posToX(mKeys[moveKeyIndex - 1].position) + _pxS(TRACK_GRADIENT_BUTTON_WIDTH);
    int max_x =
      (moveKeyIndex == mKeys.size() - 1) ? getEndXPos() : posToX(mKeys[moveKeyIndex + 1].position) - _pxS(TRACK_GRADIENT_BUTTON_WIDTH);
    int new_pos = x;
    new_pos = (new_pos < min_x) ? min_x : ((new_pos > max_x) ? max_x : new_pos);
    mKeys[moveKeyIndex].position = xToPos(new_pos);
    if (mEventHandler)
      mEventHandler->onWcChange(this);
    refresh(false);
    oldXPos = -1000;
  }
  return 0;
}


intptr_t WTextGradient::onMouseMove(int x, int y)
{
  updateTooltipText(x);
  return 0;
}
