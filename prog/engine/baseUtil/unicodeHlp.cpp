// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_unicode.h>
#include <generic/dag_tab.h>
#include <util/dag_globDef.h>
#include <wchar.h>
#include <osApiWrappers/dag_miscApi.h>
#if _TARGET_PC_WIN && defined(_MSC_VER)
#include <windows.h>
#endif
#include <supp/dag_alloca.h>


wchar_t *convert_utf8_to_u16(const char *s, int len)
{
  G_ASSERT(is_main_thread());
  if (!is_main_thread())
  {
    static wchar_t bad_str[] = L"???";
    return bad_str;
  }

  static Tab<wchar_t> tmpU16(strmem);
  return convert_utf8_to_u16_buf(tmpU16, s, len);
}

wchar_t *convert_utf8_to_u16_buf(Tab<wchar_t> &tmpU16, const char *s, int len)
{
  if (len < 0)
    len = i_strlen(s);
  else
  {
    const char *e = (const char *)memchr(s, 0, len);
    if (e)
      len = e - s;
  }
  if (!len)
    return NULL;

  tmpU16.resize(len + 1);
  int used = utf8_to_wcs_ex(s, len, tmpU16.data(), tmpU16.size());
  G_ASSERT(used <= len);
  tmpU16[used] = 0;
  tmpU16.resize(used + 1);
  return tmpU16.data();
}

wchar_t *convert_path_to_u16(Tab<wchar_t> &dest_u16, const char *s, int len)
{
  if (len < 0)
    len = i_strlen(s);
  dest_u16.resize(len + 1);
#if _TARGET_PC_WIN && defined(_MSC_VER)
  int new_sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, len + 1, dest_u16.data(), len + 1);
  if (!new_sz)
    new_sz = MultiByteToWideChar(CP_ACP, 0, s, len + 1, dest_u16.data(), len + 1);
#else
  int new_sz = utf8_to_wcs_ex(s, len, dest_u16.data(), dest_u16.size());
#endif
  dest_u16.resize(new_sz);
  return dest_u16.data();
}
wchar_t *convert_path_to_u16_c(wchar_t *dest_u16, int dest_sz, const char *s, int len)
{
  if (len < 0)
    len = i_strlen(s);

  if (len == 0 && dest_sz >= 1)
  {
    dest_u16[0] = 0;
    return dest_u16;
  }

#if _TARGET_PC_WIN && defined(_MSC_VER)
  int new_sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, len + 1, dest_u16, dest_sz);
  if (!new_sz)
    new_sz = MultiByteToWideChar(CP_ACP, 0, s, len + 1, dest_u16, dest_sz);
#else
  int new_sz = utf8_to_wcs_ex(s, len, dest_u16, dest_sz);
#endif
  return new_sz ? dest_u16 : NULL;
}
char *convert_to_utf8(Tab<char> &dest_utf8, const wchar_t *s, int len)
{
  if (len < 0)
    len = (int)wcslen(s);

  dest_utf8.resize(len * 6 + 1);
  if (!wcs_to_utf8(s, dest_utf8.data(), dest_utf8.size()))
    return NULL;
  dest_utf8.resize(strlen(dest_utf8.data()) + 1);
  return dest_utf8.data();
}

#if _TARGET_PC_WIN
char *acp_to_utf8(char *in_str, Tab<char> &out_str, int dir)
{
  out_str.resize(1);
  out_str[0] = 0;
  if (!in_str)
    return out_str.data();
  int sz = i_strlen(in_str);
  int buflen = sz + 1;
  wchar_t *tmp = (wchar_t *)alloca(buflen * sizeof(wchar_t));
  int cnt = MultiByteToWideChar(dir > 0 ? CP_ACP : CP_UTF8, 0, in_str, -1, tmp, buflen);
  if (cnt)
  {
    int outcnt = cnt * 3; // up to 3 bytes utf8
    out_str.resize(outcnt);
    WideCharToMultiByte(dir > 0 ? CP_UTF8 : CP_ACP, 0, tmp, -1, out_str.data(), outcnt, NULL, NULL);
  }
  return out_str.data();
}
#else
char *acp_to_utf8(char *in_str, Tab<char> &out_str, int /*dir*/)
{
  if (!in_str)
  {
    out_str.resize(1);
    out_str[0] = 0;
    return out_str.data();
  }
  size_t sz = strlen(in_str);
  out_str.resize(sz + 1);
  memcpy(out_str.data(), in_str, sz + 1);
  return out_str.data();
}
#endif

#define EXPORT_PULL dll_pull_baseutil_unicodeHlp
#include <supp/exportPull.h>
