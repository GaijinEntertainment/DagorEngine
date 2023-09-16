#include <osApiWrappers/dag_unicode.h>
#include <supp/_platform.h>
#include <util/dag_globDef.h>

#if _TARGET_PC_WIN | _TARGET_XBOX
wchar_t *utf8_to_wcs(const char *utf8_str, wchar_t *wcs_buf, int wcs_buf_len)
{
  int cnt = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wcs_buf, wcs_buf_len);
  if (!cnt)
    return NULL;
  wcs_buf[cnt < wcs_buf_len ? cnt : wcs_buf_len - 1] = L'\0';
  return wcs_buf;
}
int utf8_to_wcs_ex(const char *utf8_str, int utf8_len, wchar_t *wcs_buf, int wcs_buf_len)
{
  if (utf8_len == 0)
  {
    if (wcs_buf_len > 0)
      wcs_buf[0] = L'\0';
    return 0;
  }

  int cnt = MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, wcs_buf, wcs_buf_len);
  if (!cnt)
    return 0;
  wcs_buf[cnt < wcs_buf_len ? cnt : wcs_buf_len - 1] = L'\0';
  return cnt;
}
char *wcs_to_utf8(const wchar_t *wcs_str, char *utf8_buf, int utf8_buf_len)
{
  int cnt = WideCharToMultiByte(CP_UTF8, 0, wcs_str, -1, utf8_buf, utf8_buf_len, NULL, NULL);
  if (!cnt)
    return NULL;
  utf8_buf[cnt < utf8_buf_len ? cnt : utf8_buf_len - 1] = L'\0';
  return utf8_buf;
}
char *wchar_to_utf8(wchar_t wchar, char *utf8_buf, int utf8_buf_len)
{
  int cnt = WideCharToMultiByte(CP_UTF8, 0, &wchar, 1, utf8_buf, utf8_buf_len, NULL, NULL);
  if (!cnt)
    return NULL;
  utf8_buf[cnt < utf8_buf_len ? cnt : utf8_buf_len - 1] = L'\0';
  return utf8_buf;
}
char *wchar_to_acp(wchar_t wchar, char *acp_buf, int acp_buf_len)
{
  int cnt = WideCharToMultiByte(CP_ACP, 0, &wchar, 1, acp_buf, acp_buf_len, NULL, NULL);
  if (!cnt)
    return NULL;
  acp_buf[cnt < acp_buf_len ? cnt : acp_buf_len - 1] = L'\0';
  return acp_buf;
}

int utf8_to_utf16(const char *src, int src_len, uint16_t *dst, int dst_len)
{
  G_STATIC_ASSERT(sizeof(wchar_t) == sizeof(uint16_t));
  return utf8_to_wcs_ex(src, src_len, (wchar_t *)dst, dst_len);
}

#elif _TARGET_C1 | _TARGET_C2



































































#elif _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID | _TARGET_C3
#include "convertUTF.h"
#include <string.h>
#include <wchar.h>

wchar_t *utf8_to_wcs(const char *utf8_str, wchar_t *wcs_buf, int wcs_buf_len)
{
  G_STATIC_ASSERT(sizeof(wchar_t) == 4);
  G_STATIC_ASSERT(sizeof(UTF32) == 4);
  UTF32 *pwcs = (UTF32 *)wcs_buf;
  const UTF8 *pstr = (const UTF8 *)utf8_str;
  ConversionResult ret = cvt_UTF8toUTF32(&pstr, pstr + strlen(utf8_str), &pwcs, pwcs + wcs_buf_len - 1, strictConversion);
  if (ret == conversionOK || ret == targetExhausted)
  {
    *pwcs = L'\0';
    return wcs_buf;
  }
  return NULL;
}
int utf8_to_wcs_ex(const char *utf8_str, int utf8_len, wchar_t *wcs_buf, int wcs_buf_len)
{
  UTF32 *pwcs = (UTF32 *)wcs_buf;
  const UTF8 *pstr = (const UTF8 *)utf8_str;
  ConversionResult ret = cvt_UTF8toUTF32(&pstr, pstr + utf8_len, &pwcs, pwcs + wcs_buf_len - 1, strictConversion);
  if (ret == conversionOK || ret == targetExhausted)
  {
    *pwcs = L'\0';
    return (wchar_t *)pwcs - wcs_buf;
  }
  return 0;
}
char *wcs_to_utf8(const wchar_t *wcs_str, char *utf8_buf, int utf8_buf_len)
{
  const UTF32 *pwcs = (const UTF32 *)wcs_str;
  UTF8 *pstr = (UTF8 *)utf8_buf;
  ConversionResult ret = cvt_UTF32toUTF8(&pwcs, pwcs + wcslen(wcs_str), &pstr, pstr + utf8_buf_len - 1, strictConversion);
  if (ret == conversionOK || ret == targetExhausted)
  {
    *pstr = '\0';
    return utf8_buf;
  }
  return NULL;
}
char *wchar_to_utf8(wchar_t wchar, char *utf8_buf, int utf8_buf_len)
{
  const UTF32 *pwcs = (const UTF32 *)&wchar;
  UTF8 *pstr = (UTF8 *)utf8_buf;
  ConversionResult ret = cvt_UTF32toUTF8(&pwcs, pwcs + 1, &pstr, pstr + utf8_buf_len - 1, strictConversion);
  if (ret == conversionOK || ret == targetExhausted)
  {
    *pstr = '\0';
    return utf8_buf;
  }
  return NULL;
}
char *wchar_to_acp(wchar_t wchar, char *acp_buf, int acp_buf_len)
{
  G_UNREFERENCED(acp_buf_len);
  acp_buf[0] = (wchar < 128) ? (char)wchar : '?';
  acp_buf[1] = '\0';
  return acp_buf;
}

int utf8_to_utf16(const char *src, int src_len, uint16_t *dst, int dst_len)
{
  UTF16 *pd = (UTF16 *)dst;
  const UTF8 *ps = (const UTF8 *)src;
  ConversionResult r = cvt_UTF8toUTF16(&ps, ps + src_len, &pd, pd + dst_len, strictConversion);
  if (r == conversionOK || r == targetExhausted)
  {
    *pd = L'\0';
    return (pd - dst);
  }

  return 0;
}


#else
#error "Not supported on this platform"
#endif

int utf8_strlen(const char *utf8_str)
{
  int len = 0;
  for (unsigned char *ptr = (unsigned char *)utf8_str; ptr && *ptr; ptr++)
    if (((*ptr) & 0xC0) != 0x80)
      len++;
  return len;
}

#define EXPORT_PULL dll_pull_osapiwrappers_unicode
#include <supp/exportPull.h>
