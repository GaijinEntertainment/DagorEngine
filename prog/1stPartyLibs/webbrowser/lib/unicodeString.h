// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <string.h>
#include <wchar.h>


namespace webbrowser
{

// These functions allocate the resulting string
// with new[], use delete[] afterwards
wchar_t* utf8_to_wcs(const char *str);
char *wcs_to_utf8(const wchar_t *src);

class UnicodeString
{
public:
  UnicodeString(const char *fmt, ...);
  UnicodeString(const wchar_t *fmt, ...);
  UnicodeString() {}
  ~UnicodeString();

  operator bool() const { return this->shortStr && this->wideStr; }
  operator const wchar_t*() const { return this->wide(); }
  operator const char*() const { return this->utf8(); }

  const wchar_t* wide() const { return this->wideStr ? this->wideStr : L""; }
  const char* utf8() const { return this->shortStr ? this->shortStr : ""; }

  bool empty() const { return !shortStr || !*shortStr || !wideStr || !*wideStr; }

  void set(const char* str);
  void set(const wchar_t* str);

  void clear();

private:
  char *shortStr = nullptr;
  wchar_t *wideStr = nullptr;
}; // class UnicodeString

} // namespace webbrowser
