// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_color_button.h"
#include <propPanel2/comWnd/color_dialog.h>
#include <winGuiWrapper/wgw_input.h>

#include <windows.h>
#include <stdio.h>

#include <math/dag_math3d.h>
#include <math/dag_e3dColor.h>

// -------------- Color Button --------------

E3DCOLOR WColorButton::colorChangeBuffer = 0;

WColorButton::WColorButton(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :

  WindowControlBase(event_handler, parent, "BUTTON", 0, WS_CHILD | WS_VISIBLE, "", x, y, w, h),

  mStatic(event_handler, this, "STATIC", 0, WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, "...", 1, 1, w - 3, h - 3),

  mColor(0),
  mPushed(false)
{
  mHBrush = CreateSolidBrush(RGB(mColor.r, mColor.g, mColor.b));
  setTextValue("");
}


WColorButton::~WColorButton() { DeleteObject((HBRUSH)mHBrush); }


intptr_t WColorButton::onButtonDraw(void *hdc)
{
  mStatic.refresh();
  return (intptr_t)mHBrush;
}


intptr_t WColorButton::onButtonStaticDraw(void *hdc)
{
  RECT rcClient;
  GetClientRect((HWND)mStatic.getHandle(), &rcClient);
  FillRect((HDC)hdc, &rcClient, (HBRUSH)mHBrush);

  HGDIOBJ old_font = SelectObject((HDC)hdc, (HGDIOBJ)this->getNormalFont()); // store old font
  SetBkMode((HDC)hdc, TRANSPARENT);
  UINT uAlignPrev = SetTextAlign((HDC)hdc, TA_CENTER);
  SetTextColor((HDC)hdc, RGB(~mColor.r, ~mColor.g, ~mColor.b));
  TextOut((HDC)hdc, mStatic.getWidth() / 2, 0, mCaption, i_strlen(mCaption));
  SelectObject((HDC)hdc, old_font); // return old font back

  return (intptr_t)mHBrush;
}

intptr_t WColorButton::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!mEventHandler)
    return 0;

  switch (notify_code)
  {
    case BN_CLICKED: handleClick(); break;

    case BN_DBLCLK: SendMessage((HWND)mHandle, WM_LBUTTONDOWN, 0, 0); break;

    default: break;
  }

  return 0;
}


void WColorButton::handleClick()
{
  if (wingw::is_key_pressed(VK_CONTROL))
    colorChangeBuffer = mColor;
  else if (wingw::is_key_pressed(VK_SHIFT))
  {
    this->setColorValue(colorChangeBuffer);
    mEventHandler->onWcChange(this);
  }
  else
  {
    this->setEnabled(false);
    E3DCOLOR _color = mColor;
    if (ColorDialog::select_color(this->getHandle(), "Select color", _color))
    {
      this->setColorValue(_color);
      mEventHandler->onWcChange(this);
    }
    this->setEnabled(true);
  }
}


void WColorButton::setColorValue(E3DCOLOR color)
{
  mColor = color;

  HBRUSH oldBrush = (HBRUSH)mHBrush;
  mHBrush = CreateSolidBrush(RGB(mColor.r, mColor.g, mColor.b));
  DeleteObject(oldBrush);

  mStatic.refresh();
}


void WColorButton::setTextValue(const char value[]) { sprintf(mCaption, "%s", value); }


int WColorButton::getTextValue(char *buffer, int buflen) const
{
  SNPRINTF(buffer, buflen, "%s", mCaption);
  return i_strlen(buffer);
}


void WColorButton::resizeWindow(int w, int h)
{
  mStatic.resizeWindow(w - 3, h - 3);
  WindowBase::resizeWindow(w, h);
}


//-------- Two colors area for colorpicker -----------


WTwoColorArea::WTwoColorArea(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :

  WindowControlBase(event_handler, parent, "BUTTON", 0, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_NOTIFY, "", x, y, w, h),
  mHBrushOld(0),
  mHBrushNew(0)
{
  reset();
}

void WTwoColorArea::reset()
{
  setColorFValue(Point3(0, 0, 0));
  setColorValue(E3DCOLOR(0, 0, 0));
}


WTwoColorArea::~WTwoColorArea()
{
  if (mHBrushOld)
    DeleteObject((HBRUSH)mHBrushOld);

  if (mHBrushOld)
    DeleteObject((HBRUSH)mHBrushOld);
}


void WTwoColorArea::setColorFValue(Point3 value)
{
  if (mHBrushOld)
    DeleteObject((HBRUSH)mHBrushOld);

  mColorOld = E3DCOLOR(floor(value.x), floor(value.y), floor(value.z));
  mHBrushOld = CreateSolidBrush(RGB(mColorOld.r, mColorOld.g, mColorOld.b));

  refresh(false);
}


void WTwoColorArea::setColorValue(E3DCOLOR value)
{
  if (mHBrushNew)
    DeleteObject((HBRUSH)mHBrushNew);

  mColorNew = value;
  mHBrushNew = CreateSolidBrush(RGB(mColorNew.r, mColorNew.g, mColorNew.b));

  refresh(false);
}


intptr_t WTwoColorArea::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!mEventHandler)
    return 0;

  switch (notify_code)
  {
    case BN_CLICKED:
    {
      POINT pos;
      GetCursorPos(&pos);
      ScreenToClient((HWND)mHandle, &pos);
      if (pos.x < mWidth / 2)
        mEventHandler->onWcClick(this);
    }
      refresh(false);
      break;

    case BN_DBLCLK: SendMessage((HWND)mHandle, WM_LBUTTONDOWN, 0, 0); break;

    default: break;
  }

  return 0;
}


intptr_t WTwoColorArea::onControlDrawItem(void *info)
{
  DRAWITEMSTRUCT *draw = (DRAWITEMSTRUCT *)info;
  HDC hDC = (HDC)draw->hDC;
  RECT curRect;

  curRect.left = 1;
  curRect.top = 1;
  curRect.right = mWidth / 2;
  curRect.bottom = mHeight - 1;
  FillRect((HDC)hDC, &curRect, (HBRUSH)mHBrushOld);

  curRect.left = mWidth / 2;
  curRect.top = 1;
  curRect.right = mWidth - 1;
  curRect.bottom = mHeight - 1;
  FillRect((HDC)hDC, &curRect, (HBRUSH)mHBrushNew);

  // border

  HPEN black_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
  HPEN old_pen = (HPEN)SelectObject(hDC, black_pen);
  MoveToEx(hDC, 0, 0, NULL);

  LineTo(hDC, mWidth - 1, 0);
  LineTo(hDC, mWidth - 1, mHeight - 1);
  LineTo(hDC, 0, mHeight - 1);
  LineTo(hDC, 0, 0);

  SelectObject(hDC, old_pen);
  DeleteObject(black_pen);

  return 0;
}


// gui routines

void WTwoColorArea::moveWindow(int x, int y)
{
  WindowControlBase::moveWindow(x, y);
  refresh(true);
}


void WTwoColorArea::resizeWindow(int w, int h)
{
  WindowBase::resizeWindow(w, h);
  refresh(true);
}


//-------------- Palette cell ---------------


WPaletteCell::WPaletteCell(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :

  WindowControlBase(event_handler, parent, "BUTTON", 0, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_NOTIFY, "", x, y, w, h),
  mTooltip(event_handler, this),
  mHBrush(0),
  mSelected(false)
{
  reset();
}

void WPaletteCell::reset()
{
  setColorValue(E3DCOLOR(255, 255, 255));
  setTextValue("");
  setSelection(false);
}


WPaletteCell::~WPaletteCell()
{
  if (mHBrush)
    DeleteObject((HBRUSH)mHBrush);
}


void WPaletteCell::setColorValue(E3DCOLOR value)
{
  if (mHBrush)
    DeleteObject((HBRUSH)mHBrush);

  mColor = value;
  mHBrush = CreateSolidBrush(RGB(mColor.r, mColor.g, mColor.b));

  refresh(false);
}


void WPaletteCell::setTextValue(const char value[])
{
  String text(128, "%s (%d, %d, %d, %d)", value, mColor.r, mColor.g, mColor.b, mColor.a);
  mTooltip.setText(text);
}


intptr_t WPaletteCell::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!mEventHandler)
    return 0;

  switch (notify_code)
  {
    case BN_CLICKED:
      mSelected = true;
      mEventHandler->onWcChange(this);
      refresh(false);
      break;

    case BN_DBLCLK: SendMessage((HWND)mHandle, WM_LBUTTONDOWN, 0, 0); break;

    default: break;
  }

  return 0;
}


intptr_t WPaletteCell::onControlDrawItem(void *info)
{
  DRAWITEMSTRUCT *draw = (DRAWITEMSTRUCT *)info;
  HDC hDC = (HDC)draw->hDC;
  RECT cr;

  cr.left = 4;
  cr.top = 4;
  cr.right = mWidth - 4;
  cr.bottom = mHeight - 4;
  FillRect((HDC)hDC, &cr, (HBRUSH)mHBrush);

  // border

  HPEN old_pen = (HPEN)SelectObject(hDC, GetStockObject(DC_PEN));

  SetDCPenColor(hDC, RGB(128, 128, 128));
  MoveToEx(hDC, cr.left - 2, cr.bottom + 1, NULL);
  LineTo(hDC, cr.left - 2, cr.top - 2);
  LineTo(hDC, cr.right + 1, cr.top - 2);

  SetDCPenColor(hDC, RGB(255, 255, 255));
  LineTo(hDC, cr.right + 1, cr.bottom + 1);
  LineTo(hDC, cr.left - 3, cr.bottom + 1);

  SetDCPenColor(hDC, RGB(64, 64, 64));
  MoveToEx(hDC, cr.left - 1, cr.bottom, NULL);
  LineTo(hDC, cr.left - 1, cr.top - 1);
  LineTo(hDC, cr.right, cr.top - 1);

  SetDCPenColor(hDC, RGB(212, 208, 200));
  LineTo(hDC, cr.right, cr.bottom);
  LineTo(hDC, cr.left - 2, cr.bottom);

  // selection

  HPEN sel_pen = (mSelected) ? CreatePen(PS_DOT, 1, RGB(0, 0, 0)) : CreatePen(PS_SOLID, 1, USER_GUI_COLOR);

  SelectObject(hDC, sel_pen);
  MoveToEx(hDC, 0, 0, NULL);

  LineTo(hDC, mWidth - 1, 0);
  LineTo(hDC, mWidth - 1, mHeight - 1);
  LineTo(hDC, 0, mHeight - 1);
  LineTo(hDC, 0, 0);

  SelectObject(hDC, old_pen);
  DeleteObject(sel_pen);

  return 0;
}


// gui routines

void WPaletteCell::moveWindow(int x, int y)
{
  WindowControlBase::moveWindow(x, y);
  refresh(true);
}


void WPaletteCell::resizeWindow(int w, int h)
{
  WindowBase::resizeWindow(w, h);
  refresh(true);
}

//-----------------------------------------------------------
