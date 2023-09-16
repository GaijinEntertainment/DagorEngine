// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_tab.h"

#include <windows.h>
#include <commctrl.h>


// -------------- TabPanel ---------------------------

WTabPanel::WTabPanel(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h)

  :
  WindowControlBase(event_handler, parent, WC_TABCONTROL, WS_EX_CONTROLPARENT,
    WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | TCS_TABS | TCS_SINGLELINE | TCS_OWNERDRAWFIXED, "", x, y, w, h)
{
  this->setFont(WindowBase::getSmallPrettyFont());
}

void WTabPanel::addPage(int id, const char caption[])
{
  int index = TabCtrl_GetItemCount((HWND)getHandle());
  insertPageByIndex(id, index, caption);
}

void WTabPanel::insertPage(int id, int id_after, const char caption[])
{
  int index = getIndexById(id_after);
  insertPageByIndex(id, index, caption);
}

void WTabPanel::insertPageByIndex(int id, int index, const char caption[])
{
  TCITEM item = {};

  item.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
  item.iImage = -1;
  item.pszText = (LPTSTR)caption;
  item.lParam = (LPARAM)id;

  TabCtrl_InsertItem((HWND)getHandle(), index, &item);
}

int WTabPanel::getIndexById(int id) const
{
  int index = 0;
  int itemCount = TabCtrl_GetItemCount((HWND)getHandle());
  while (index < itemCount)
  {
    if (getIdByIndex(index) == id)
      break;

    index++;
  }

  return index;
}

int WTabPanel::getIdByIndex(int index) const
{
  TCITEM item = {};
  item.mask = TCIF_PARAM;

  if (!TabCtrl_GetItem((HWND)getHandle(), index, &item))
    item.lParam = -1;

  return (int)item.lParam;
}

void WTabPanel::deletePage(int id)
{
  int index = getIndexById(id);
  TabCtrl_DeleteItem((HWND)getHandle(), index);
}

void WTabPanel::deleteAllPages() { TabCtrl_DeleteAllItems((HWND)getHandle()); }

int WTabPanel::getCurrentPageId() const
{
  int selected = TabCtrl_GetCurSel((HWND)getHandle());
  if (selected != -1)
    selected = getIdByIndex(selected);

  return selected;
}

void WTabPanel::setCurrentPage(int id)
{
  if (id == -1)
  {
    TabCtrl_SetCurSel((HWND)getHandle(), -1);
    return;
  }

  int index = getIndexById(id);
  TabCtrl_SetCurSel((HWND)getHandle(), index);
}

void WTabPanel::setPageCaption(int id, const char caption[])
{
  TCITEM item = {};

  item.mask = TCIF_TEXT;
  item.pszText = (LPTSTR)caption;

  TabCtrl_SetItem((HWND)getHandle(), getIndexById(id), &item);
}

int WTabPanel::getPageCount() const { return TabCtrl_GetItemCount((HWND)getHandle()); }

intptr_t WTabPanel::onControlNotify(void *info)
{
  if (!mEventHandler)
    return 0;

  NMHDR *param = (NMHDR *)info;

  switch (param->code)
  {
    case TCN_SELCHANGE:
      mEventHandler->onWcChange(this);
      refresh(false);
      break;

    case TCN_SELCHANGING: return mEventHandler->onWcChanging(this);

    default: break;
  }

  return 0;
}


intptr_t WTabPanel::onControlDrawItem(void *info)
{
  DRAWITEMSTRUCT *draw = (DRAWITEMSTRUCT *)info;

  RECT rcClient, rcTab;
  GetClientRect((HWND)getHandle(), &rcClient);
  HBRUSH gui_brush = getUserColorsFlag() ? CreateSolidBrush(USER_GUI_COLOR) : (HBRUSH)COLOR_BTNSHADOW;
  HPEN gray_pen = CreatePen(PS_SOLID, 1, RGB(64, 64, 64));
  HPEN light_gray_pen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
  HPEN white_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));

  char caption[64];
  TCITEM item = {};
  item.mask = TCIF_TEXT;
  item.pszText = caption;
  item.cchTextMax = sizeof(caption);

  // area

  FillRect(draw->hDC, &rcClient, gui_brush);

  // frame

  TabCtrl_AdjustRect((HWND)getHandle(), FALSE, &rcClient);

  rcClient.left -= 4;
  rcClient.top -= 2;
  rcClient.right += 3;
  rcClient.bottom += 2;

  HPEN old_pen = (HPEN)SelectObject(draw->hDC, white_pen);

  MoveToEx(draw->hDC, rcClient.left, rcClient.bottom, NULL);
  LineTo(draw->hDC, rcClient.left, rcClient.top);
  LineTo(draw->hDC, rcClient.right, rcClient.top);

  SelectObject(draw->hDC, gray_pen);
  LineTo(draw->hDC, rcClient.right, rcClient.bottom);
  LineTo(draw->hDC, rcClient.left, rcClient.bottom);

  SelectObject(draw->hDC, light_gray_pen);
  MoveToEx(draw->hDC, rcClient.right - 1, rcClient.top + 1, NULL);
  LineTo(draw->hDC, rcClient.right - 1, rcClient.bottom - 1);
  LineTo(draw->hDC, rcClient.left + 1, rcClient.bottom - 1);
  SelectObject(draw->hDC, old_pen);

  // tabs

  for (int i = 0; i < TabCtrl_GetItemCount((HWND)getHandle()); ++i)
  {
    TabCtrl_GetItemRect((HWND)getHandle(), i, &rcTab);

    rcTab.right -= 1;
    if (TabCtrl_GetCurSel((HWND)getHandle()) == i)
    {
      rcTab.top -= 2;
      rcTab.bottom += 3;
      rcTab.right += 1;
      FillRect(draw->hDC, &rcTab, gui_brush);
    }

    if (TabCtrl_GetItem((HWND)getHandle(), i, &item))
    {
      HGDIOBJ old_font = SelectObject(draw->hDC, (HGDIOBJ)this->getSmallPrettyFont());
      SetBkMode(draw->hDC, TRANSPARENT);
      UINT uAlignPrev = SetTextAlign(draw->hDC, TA_CENTER);

      TextOut(draw->hDC, rcTab.left + (rcTab.right - rcTab.left) / 2, rcTab.top + 2, caption, i_strlen(caption));

      SelectObject(draw->hDC, old_font);
    }

    if (TabCtrl_GetCurSel((HWND)getHandle()) != i)
    {
      HPEN old_pen = (HPEN)SelectObject(draw->hDC, white_pen);
      MoveToEx(draw->hDC, rcTab.left, rcTab.bottom, NULL);
      LineTo(draw->hDC, rcTab.left, rcTab.top + 3);
      LineTo(draw->hDC, rcTab.left + 2, rcTab.top);
      LineTo(draw->hDC, rcTab.right, rcTab.top);

      SelectObject(draw->hDC, gray_pen);
      MoveToEx(draw->hDC, rcTab.right, rcTab.top + 1, NULL);
      LineTo(draw->hDC, rcTab.right, rcTab.bottom);

      SelectObject(draw->hDC, light_gray_pen);
      MoveToEx(draw->hDC, rcTab.right - 1, rcTab.top + 2, NULL);
      LineTo(draw->hDC, rcTab.right - 1, rcTab.bottom);
      SelectObject(draw->hDC, old_pen);
    }
  }

  if (getUserColorsFlag())
    DeleteObject(gui_brush);
  DeleteObject(light_gray_pen);
  DeleteObject(gray_pen);
  DeleteObject(white_pen);

  return 0;
}
