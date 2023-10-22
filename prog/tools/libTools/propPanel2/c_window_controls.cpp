// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "c_window_controls.h"

#include <windows.h>
#include <commctrl.h>
#include <olectl.h>


// -----------------------------------------------


WindowControlBase::WindowControlBase(WindowControlEventHandler *event_handler, WindowBase *parent, const char class_name[],
  unsigned long ex_style, unsigned long style, const char caption[], int x, int y, int w, int h) :

  WindowBase(parent, class_name, ex_style, style, caption, x, y, w, h), mEventHandler(event_handler)
{
  G_ASSERT(event_handler && "WindowControlBase constructor error: WindowEventHandler is NULL");
}


intptr_t WindowControlBase::onResize(unsigned new_width, unsigned new_height)
{
  WindowBase::onResize(new_width, new_height);
  mEventHandler->onWcResize(this);
  return 0;
}


intptr_t WindowControlBase::onKeyDown(unsigned v_key, int flags) { return mEventHandler->onWcKeyDown(this, v_key); }


intptr_t WindowControlBase::onClipboardCopy(DataBlock &blk) { return mEventHandler->onWcClipboardCopy(this, blk); }


intptr_t WindowControlBase::onClipboardPaste(const DataBlock &blk) { return mEventHandler->onWcClipboardPaste(this, blk); }


intptr_t WindowControlBase::onDropFiles(const dag::Vector<String> &files) { return mEventHandler->onDropFiles(this, files); }

// ------------------- Routines ---------------------------


void *load_bmp_picture(const char *lpwszFileName, unsigned final_w, unsigned final_h)
{
  return LoadImage(0, lpwszFileName, IMAGE_BITMAP, final_w, final_h, LR_LOADFROMFILE);
}
void *clone_bmp_picture(void *image) { return (HBITMAP)CopyImage((HBITMAP)image, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG); }

unsigned get_alpha_color(void *hbitmap)
{
  HDC hdc = CreateCompatibleDC(0);
  HBITMAP oldbmp = (HBITMAP)SelectObject(hdc, (HGDIOBJ)hbitmap);
  COLORREF t_color = GetPixel(hdc, 0, 0);
  SelectObject(hdc, (HGDIOBJ)oldbmp);
  DeleteDC(hdc);

  return t_color;
}
