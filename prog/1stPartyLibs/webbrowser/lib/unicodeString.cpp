#include "unicodeString.h"

#include <Windows.h>
#include <stdio.h>


namespace webbrowser
{

char *wcs_to_utf8(const wchar_t *src)
{
  int needed = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
  char *dst = new char[needed];
  int converted = WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, needed, NULL, NULL);
  if (converted)
    return dst;

  delete [] dst;
  return NULL;
}


wchar_t* utf8_to_wcs(const char* str)
{
  int required = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
  wchar_t *dst = new wchar_t[required]; // NULL-terminator included

  int converted = MultiByteToWideChar(CP_UTF8, 0, str, -1, dst, required);
  if (converted)
    return dst;

  delete [] dst;
  return nullptr;
}


UnicodeString::UnicodeString(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  va_list cp;
  va_copy(cp, ap);
  int needed = vsnprintf(NULL, 0, fmt, cp);
  va_end(cp);

  if (++needed > 0)
  {
    this->shortStr = new char[needed];
    vsnprintf(this->shortStr, needed, fmt, ap);
    this->wideStr = utf8_to_wcs(this->shortStr);
  }

  va_end(ap);
}


UnicodeString::UnicodeString(const wchar_t *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  va_list cp;
  va_copy(cp, ap);
  int needed = _vsnwprintf(NULL, 0, fmt, cp);
  va_end(cp);

  if (++needed > 0)
  {
    this->wideStr = new wchar_t[needed];
    _vsnwprintf(this->wideStr, needed, fmt, ap);
    this->shortStr = wcs_to_utf8(this->wideStr);
  }

  va_end(ap);
}


UnicodeString::~UnicodeString()
{
  clear();
}


void UnicodeString::clear()
{
  delete [] this->wideStr;
  delete [] this->shortStr;

  this->wideStr = nullptr;
  this->shortStr = nullptr;
}


void UnicodeString::set(const char* str)
{
  clear();

  if (str)
  {
    size_t len = strlen(str);

    this->shortStr = new char[len + 1];
    ::memcpy(this->shortStr, str, len);
    this->shortStr[len] = 0;

    this->wideStr = utf8_to_wcs(this->shortStr);
  }
}


void UnicodeString::set(const wchar_t* str)
{
  clear();

  if (str)
  {
    size_t len = wcslen(str);

    this->wideStr = new wchar_t[len + 1];
    ::memcpy(this->wideStr, str, sizeof(wchar_t) * len);
    this->wideStr[len] = 0;

    this->shortStr = wcs_to_utf8(this->wideStr);
  }
}

} // namespace webbrowser
