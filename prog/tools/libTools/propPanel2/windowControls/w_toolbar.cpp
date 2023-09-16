// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_toolbar.h"
#include "../c_constants.h"

#include <propPanel2/c_panel_base.h>

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

// ---------------------------------------------------

const char image_ext_normal[] = ".bmp";
const char image_ext_hot[] = "_hot.bmp";
const char image_ext_disabled[] = "_dis.bmp";

// ---------------------------------------------------


WToolbar::WToolbar(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, const char *caption) :
  WindowControlBase(event_handler, parent, TOOLBARCLASSNAME,
    0,                      // TBSTYLE_EX_DRAWDDARROWS,
    WS_CHILD | WS_VISIBLE | // WS_BORDER |
      TBSTYLE_LIST | TBSTYLE_TOOLTIPS | TBSTYLE_AUTOSIZE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | CCS_ADJUSTABLE | CCS_NOPARENTALIGN |
      CCS_NORESIZE,
    caption, x, y, w, h)
{
  lastClickID = 0;

  SendMessage((HWND)mHandle, TB_SETEXTENDEDSTYLE, (WPARAM)NULL, (LPARAM)(TBSTYLE_EX_MIXEDBUTTONS | 0x00000080));

  SendMessage((HWND)mHandle, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

  for (int i = 0; i < IMAGE_LIST_COUNT; ++i)
  {
    mHImageList[i] = ImageList_Create(TOOLBAR_IMAGE_WH, TOOLBAR_IMAGE_WH, ILC_COLOR24 | ILC_MASK, 1, 10);
  }

  // settins for toolbar

  SendMessage((HWND)mHandle, TB_SETIMAGELIST, 0, (LPARAM)mHImageList[0]);
  SendMessage((HWND)mHandle, TB_SETHOTIMAGELIST, 0, (LPARAM)mHImageList[1]);
  SendMessage((HWND)mHandle, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)mHImageList[2]);

  long _sz = MAKELONG(TOOLBAR_IMAGE_WH, TOOLBAR_IMAGE_WH);
  SendMessage((HWND)mHandle, TB_SETBITMAPSIZE, 0, _sz);
  SendMessage((HWND)mHandle, TB_SETBUTTONSIZE, 0, _sz);
  SendMessage((HWND)mHandle, TB_AUTOSIZE, 0, 0);

  SendMessage((HWND)mHandle, TB_SETBUTTONWIDTH, 0, (LPARAM)MAKELONG(TOOLBAR_IMAGE_WH, DEFAULT_TOOLBAR_BUTTON_HEIGHT));

  ShowWindow((HWND)mHandle, 1);
}


WToolbar::~WToolbar()
{
  clear();

  for (int i = 0; i < IMAGE_LIST_COUNT; ++i)
  {
    ImageList_Destroy((HIMAGELIST)mHImageList[i]);
  }
}


intptr_t WToolbar::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!mEventHandler)
    return 0;

  {
    lastClickID = elem_id;
    mEventHandler->onWcClick(this);
    return 0;
  }

  return 0;
}


int WToolbar::prepareText(int id, const char *caption) { return SendMessage((HWND)mHandle, TB_ADDSTRING, 0, (LPARAM)(LPSTR)caption); }


int WToolbar::prepareImages(int id, const char *fname)
{
  int _image_index = 0;

  // filenames prepare

  char image_normal_fn[256];
  char image_hot_fn[256];
  char image_dis_fn[256];

  {
    char filename_base[256];
    sprintf(filename_base, "%s%s", p2util::get_icon_path(), fname);

    sprintf(image_hot_fn, "%s%s", filename_base, image_ext_hot);
    sprintf(image_dis_fn, "%s%s", filename_base, image_ext_disabled);
    sprintf(image_normal_fn, "%s%s", filename_base, image_ext_normal);
  }

  // images load

  HBITMAP a_bitmap = (HBITMAP)load_bmp_picture(image_normal_fn);
  HBITMAP p_bitmap = (p_bitmap = (HBITMAP)load_bmp_picture(image_hot_fn)) ? p_bitmap : (HBITMAP)load_bmp_picture(image_normal_fn);
  HBITMAP i_bitmap = (i_bitmap = (HBITMAP)load_bmp_picture(image_dis_fn)) ? i_bitmap : (HBITMAP)load_bmp_picture(image_normal_fn);

  if (a_bitmap)
  {
    // alpha colors select

    COLORREF a_t_color = get_alpha_color(a_bitmap);
    COLORREF p_t_color = (p_bitmap != a_bitmap) ? get_alpha_color(p_bitmap) : a_t_color;
    COLORREF i_t_color = (i_bitmap != a_bitmap) ? get_alpha_color(i_bitmap) : a_t_color;

    _image_index = ImageList_AddMasked((HIMAGELIST)mHImageList[0], a_bitmap, a_t_color);
    ImageList_AddMasked((HIMAGELIST)mHImageList[1], p_bitmap, p_t_color);
    ImageList_AddMasked((HIMAGELIST)mHImageList[2], i_bitmap, i_t_color);

    // delete pictures

    if (p_bitmap != a_bitmap)
      DeleteObject(p_bitmap);

    if (i_bitmap != a_bitmap)
      DeleteObject(i_bitmap);

    DeleteObject(a_bitmap);
  }
  else
  {
    _image_index = -1;
    debug("ERROR: toolbar button (id = %d) load bitmap \"%s\" ", id, image_normal_fn);
  }

  return _image_index;
}


int WToolbar::addTB(int id, int string_id, int bitmap_id, unsigned char style)
{
  TBBUTTON tbb;

  tbb.dwData = 0;
  tbb.iBitmap = bitmap_id;
  tbb.fsState = TBSTATE_ENABLED;
  tbb.fsStyle = style;
  tbb.iString = string_id;

  tbb.idCommand = id;

  int result = SendMessage((HWND)mHandle, TB_ADDBUTTONS, (WPARAM)1, (LPARAM)(LPTBBUTTON)&tbb);

  SendMessage((HWND)mHandle, TB_AUTOSIZE, 0, 0);

  SIZE sz;
  SendMessage((HWND)mHandle, TB_GETMAXSIZE, 0, (LPARAM)&sz);
  resizeWindow(sz.cx, this->getHeight());

  return result;
}


void WToolbar::addButton(int id, const char *caption)
{
  this->addTB(id, this->prepareText(id, caption), I_IMAGENONE, BTNS_BUTTON | BTNS_AUTOSIZE);
}


void WToolbar::addCheck(int id, const char *caption)
{
  this->addTB(id, this->prepareText(id, caption), I_IMAGENONE, BTNS_CHECK | BTNS_AUTOSIZE);
}


void WToolbar::addCheckGroup(int id, const char *caption)
{
  this->addTB(id, this->prepareText(id, caption), I_IMAGENONE, BTNS_CHECKGROUP | BTNS_AUTOSIZE);
}


void WToolbar::addSep(int id) { this->addTB(id, 0, 0, BTNS_SEP); }


unsigned WToolbar::getLastClickID() { return lastClickID; }


bool WToolbar::setItemCheck(int id, bool value) { return SendMessage((HWND)mHandle, TB_CHECKBUTTON, id, (LPARAM)MAKELONG(value, 0)); }


bool WToolbar::getItemCheck(int id) { return (SendMessage((HWND)mHandle, TB_ISBUTTONCHECKED, (WPARAM)id, 0) > 0); }


void WToolbar::setItemText(int id, const char *caption)
{
  TBBUTTONINFO tbi;
  tbi.cbSize = sizeof(tbi);
  tbi.pszText = (LPTSTR)caption;
  tbi.dwMask = TBIF_TEXT;
  tbi.cchText = i_strlen(caption);

  SendMessage((HWND)mHandle, TB_SETBUTTONINFO, (WPARAM)id, (LPARAM)&tbi);
}


void WToolbar::setItemPictures(int id, const char *fname)
{
  int image_index = prepareImages(id, fname);

  if (image_index > -1)
  {
    TBBUTTONINFO tbi = {0};
    tbi.cbSize = sizeof(tbi);
    tbi.dwMask = TBIF_IMAGE;
    tbi.iImage = image_index;

    SendMessage((HWND)mHandle, TB_SETBUTTONINFO, (WPARAM)id, (LPARAM)&tbi);
  }
}


bool WToolbar::setItemEnabled(int id, bool enabled)
{
  return SendMessage((HWND)mHandle, TB_ENABLEBUTTON, id, (LPARAM)MAKELONG(enabled, 0));
}


void WToolbar::clear()
{
  // remove buttons
  int c = SendMessage((HWND)mHandle, TB_BUTTONCOUNT, 0, 0);
  for (int i = 0; i < c; ++i)
  {
    SendMessage((HWND)mHandle, TB_DELETEBUTTON, 0, 0);
  }

  // remove lists

  for (int i = 0; i < IMAGE_LIST_COUNT; ++i)
  {
    ImageList_RemoveAll((HIMAGELIST)mHImageList[i]);
  }
}
