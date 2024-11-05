//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <generic/dag_tabFwd.h>

#include <supp/dag_define_KRNLIMP.h>

KRNLIMP wchar_t *utf8_to_wcs(const char *utf8_str, wchar_t *wcs_buf, int wcs_buf_len);
KRNLIMP int utf8_to_wcs_ex(const char *utf8_str, int utf8_len, wchar_t *wcs_buf, int wcs_buf_len);

KRNLIMP char *wcs_to_utf8(const wchar_t *wcs_str, char *utf8_buf, int utf8_buf_len);
KRNLIMP char *wchar_to_utf8(wchar_t wchar, char *utf8_buf, int utf8_buf_len);
KRNLIMP char *wchar_to_acp(wchar_t wchar, char *acp_buf, int acp_buf_len);

// convert UTF8 to UTF16 using highly temporary buffer
KRNLIMP wchar_t *convert_utf8_to_u16(const char *s, int len);

// convert UTF8 to UTF16 using explicit buffer
KRNLIMP wchar_t *convert_utf8_to_u16_buf(Tab<wchar_t> &dest_u16, const char *s, int len);

// converts char* to wchar_t* (tries UTF8 first, and then ACP, if UTF8 fails); returns dest_u16.data()
KRNLIMP wchar_t *convert_path_to_u16(Tab<wchar_t> &dest_u16, const char *s, int len = -1);
KRNLIMP wchar_t *convert_path_to_u16_c(wchar_t *dest_u16, int dest_sz, const char *s, int len = -1);

// converts wchar_t* to UTF-8 char*; returns dest_utf8.data()
KRNLIMP char *convert_to_utf8(Tab<char> &dest_utf8, const wchar_t *s, int len = -1);

// counts number of SYMBOLS in utf8 NULL-terminated string
KRNLIMP int utf8_strlen(const char *utf8_str);

// dir = 1: converts acp to utf8; dir = -1: converts utf8 to acp
// returns out_str.data()
// on non-win32, returns copy of in_str
KRNLIMP char *acp_to_utf8(char *in_str, Tab<char> &out_str, int dir = 1);

KRNLIMP int utf8_to_utf16(const char *src, int src_len, uint16_t *dst, int dst_len);

#include <supp/dag_undef_KRNLIMP.h>
