#include <osApiWrappers/dag_clipboard.h>

#if _TARGET_PC_WIN
#include <windows.h> // FIXME
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
  int len = i_strlen(buf);
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
  int len = i_strlen(buf);
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
} // namespace clipboard
#endif
