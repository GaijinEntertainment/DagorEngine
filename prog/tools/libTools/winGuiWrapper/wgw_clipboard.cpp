// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <windows.h>
#include <winGuiWrapper/wgw_clipboard.h>


namespace wingw
{
HGLOBAL gc_buf_handle = NULL;

long copyStringToClipboard(void *handle, const SimpleString &str_buffer)
{
  if (!OpenClipboard((HWND)handle))
    return 0;
  EmptyClipboard();

  if (str_buffer.empty())
    return 0;

  int len = str_buffer.length();
  if (gc_buf_handle)
    GlobalFree(gc_buf_handle);
  gc_buf_handle = GlobalAlloc(GMEM_MOVEABLE, len + 1);
  if (gc_buf_handle == NULL)
  {
    CloseClipboard();
    return 0;
  }

  LPTSTR lptstrCopy = (LPTSTR)GlobalLock(gc_buf_handle);
  memcpy(lptstrCopy, str_buffer.str(), len);
  lptstrCopy[len] = (TCHAR)0;
  GlobalUnlock(gc_buf_handle);
  SetClipboardData(CF_TEXT, gc_buf_handle);
  CloseClipboard();
  return 1;
}

long pasteStringFromClipboard(void *handle, SimpleString &str_buffer)
{
  if (!IsClipboardFormatAvailable(CF_TEXT))
    return 0;
  if (!OpenClipboard((HWND)handle))
    return 0;

  HGLOBAL hglb = GetClipboardData(CF_TEXT);

  if (hglb != NULL)
  {
    LPSTR lptstr = (LPSTR)GlobalLock(hglb);
    if (lptstr != NULL)
    {
      str_buffer = lptstr;
      GlobalUnlock(hglb);
    }
  }
  CloseClipboard();
  return 1;
}
} // namespace wingw
