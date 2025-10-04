// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_clipboard.h>

#if _TARGET_PC_WIN
#include <windows.h> // FIXME
#include <Shlobj.h>
#include <direct.h>
#include <util/dag_string.h>
#include <osApiWrappers/basePath.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <image/dag_texPixel.h>

namespace clipboard
{
bool get_clipboard_ansi_text(char *dest, int bufSize)
{
  if (dest && bufSize > 0 && OpenClipboard(NULL))
  {
    *dest = 0;
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData)
    {
      char *buffer = (char *)GlobalLock(hData);
      if (buffer)
      {
        strncpy(dest, buffer, bufSize);
        dest[bufSize - 1] = '\0';
      }
      GlobalUnlock(hData);
    }
    CloseClipboard();
    return true;
  }
  return false;
}

bool set_clipboard_ansi_text(const char *buf)
{
  int len = (int)strlen(buf);
  if (len && OpenClipboard(NULL))
  {
    HGLOBAL clipbuffer;
    char *buffer;
    EmptyClipboard();
    clipbuffer = GlobalAlloc(GMEM_DDESHARE, len + 1);
    buffer = (char *)GlobalLock(clipbuffer);
    strcpy(buffer, LPCSTR(buf));
    GlobalUnlock(clipbuffer);
    SetClipboardData(CF_TEXT, clipbuffer);
    CloseClipboard();
    return true;
  }
  return false;
}

bool get_clipboard_utf8_text(char *dest, int bufSize)
{
  if (dest && bufSize > 0 && OpenClipboard(NULL))
  {
    *dest = 0;
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData)
    {
      wchar_t *buffer = (wchar_t *)GlobalLock(hData);
      if (buffer)
      {
        // strncpy(dest, buffer, bufSize);
        WideCharToMultiByte(CP_UTF8, 0, buffer, -1, dest, bufSize, NULL, NULL);
        dest[bufSize - 1] = '\0';
      }
      GlobalUnlock(hData);
    }
    CloseClipboard();
    return true;
  }
  return false;
}

bool set_clipboard_utf8_text(const char *buf)
{
  int len = (int)strlen(buf);
  if (len && OpenClipboard(NULL))
  {
    HGLOBAL clipbuffer;
    wchar_t *buffer;
    int wlen;
    EmptyClipboard();
    wlen = MultiByteToWideChar(CP_UTF8, 0, buf, -1, NULL, 0);
    clipbuffer = GlobalAlloc(GMEM_DDESHARE, wlen * 2); // this is max buffer
    buffer = (wchar_t *)GlobalLock(clipbuffer);
    MultiByteToWideChar(CP_UTF8, 0, buf, -1, buffer, wlen);
    GlobalUnlock(clipbuffer);
    SetClipboardData(CF_UNICODETEXT, clipbuffer);
    CloseClipboard();
    return true;
  }
  return false;
}

bool set_clipboard_bmp_image(TexPixel32 *im, int wd, int ht, int stride)
{
  if (wd && ht && im && OpenClipboard(NULL))
  {
    EmptyClipboard();
    int lineWd = wd * 4;
    int sizeToAlloc = sizeof(BITMAPINFOHEADER) + lineWd * ht;
    HGLOBAL clipbuffer = GlobalAlloc(GMEM_MOVEABLE, sizeToAlloc); // this is max buffer
    char *buffer = (char *)GlobalLock(clipbuffer);

    BITMAPINFOHEADER *info = (BITMAPINFOHEADER *)buffer;
    memset(info, 0, sizeToAlloc);
    info->biSize = sizeof(BITMAPINFOHEADER);
    info->biWidth = wd;
    info->biHeight = ht;
    info->biPlanes = 1;
    info->biBitCount = 32;
    info->biCompression = BI_RGB;

    for (int i = 0; i < ht; ++i)
      memcpy(buffer + sizeof(BITMAPINFO) + i * lineWd, ((char *)im) + (ht - i - 1) * stride, wd * 4);

    GlobalUnlock(clipbuffer);
    bool res = SetClipboardData(CF_DIB, clipbuffer) == NULL;
    if (!res)
      GlobalFree(clipbuffer);
    CloseClipboard();
    return res;
  }
  return false;
}

bool set_clipboard_file(const char *filename)
{
  if (!filename || !*filename)
    return false;

  String path;
  if (is_path_abs(filename))
    path = filename;
  else
  {
    char buf[DAGOR_MAX_PATH * 3]; // x3 to compensate to UTF-8 large 3-byte code points in asian characters
    if (getcwd(buf, sizeof(buf)))
      path = buf;
    if (!path.empty())
      path += PATH_DELIM;
    path += filename;
  }

  bool lockSetDone = false;
  bool copyClipboardDone = false;
  // DROPFILES has two '\0' at the end: one from last path null-terminated string, another one to signal the end of the path list
  // [struct DROPFILES][path_0][\n][path_i][\n][\n]
  int fileDescSize = sizeof(DROPFILES) + path.size() + 1;
  HGLOBAL fileDescMemGlobal = GlobalAlloc(GHND, fileDescSize);
  if (fileDescMemGlobal)
  {
    DROPFILES *df = (DROPFILES *)GlobalLock(fileDescMemGlobal);
    if (df)
    {
      df->pFiles = sizeof(DROPFILES);
      df->fWide = FALSE;
      char *dfPointer = (char *)df + df->pFiles;
      memcpy(dfPointer, path.c_str(), path.size());
      dfPointer += path.size();
      memset(dfPointer, '\0', 1);
      GlobalUnlock(df);
      lockSetDone = true;
    }
    HWND thisHWND = (HWND)::win32_get_main_wnd();
    if (lockSetDone && thisHWND && OpenClipboard(thisHWND))
    {
      // We do not use delayed rendering, therefore SetClipboardData should return NULL only on failure
      if (EmptyClipboard())
        copyClipboardDone = SetClipboardData(CF_HDROP, fileDescMemGlobal) != NULL;
      CloseClipboard();
    }
  }
  // If SetClipboardData succeeded, we must not call GlobalFree, because the ownership is transferred to the OS from us.
  if (fileDescMemGlobal && (!lockSetDone || !copyClipboardDone))
  {
    GlobalFree(fileDescMemGlobal);
    return false;
  }
  return copyClipboardDone;
}
} // namespace clipboard

#define EXPORT_PULL dll_pull_osapiwrappers_clipboard
#include <supp/exportPull.h>

#else

namespace clipboard
{
bool get_clipboard_ansi_text(char *, int) { return false; }
bool set_clipboard_ansi_text(const char *) { return false; }
bool get_clipboard_utf8_text(char *, int) { return false; }
bool set_clipboard_utf8_text(const char *) { return false; }
bool set_clipboard_bmp_image(TexPixel32 * /*im*/, int /*wd*/, int /*ht*/, int /*stride*/) { return false; }
bool set_clipboard_file(const char * /*filename*/) { return false; }
} // namespace clipboard
#endif
