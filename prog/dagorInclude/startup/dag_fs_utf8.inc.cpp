// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#pragma once

#if _TARGET_PC_WIN && defined(_MSC_VER)

#include <windows.h>
#include <stdio.h>
#include <direct.h>
#include <sys/stat.h>
#include <string.h>
#include <malloc.h>
#include <wchar.h>
#include <fcntl.h>

#if 0
#include <osApiWrappers/dag_dbgStr.h>
#define LOG_CALL out_debug_str_fmt
#else
#define LOG_CALL skip_log
static inline void skip_log(...) {}
#endif

#define CVT_TO_U16(NM)                                                                                     \
  int NM##_slen = (int)strlen(NM);                                                                         \
  wchar_t *NM##_u16 = (wchar_t *)alloca((NM##_slen + 1) * sizeof(wchar_t));                                \
  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, NM, NM##_slen + 1, NM##_u16, NM##_slen + 1) == 0) \
    MultiByteToWideChar(CP_ACP, 0, NM, NM##_slen + 1, NM##_u16, NM##_slen + 1);

#ifdef __cplusplus
extern "C"
{
#endif
#if _MSC_VER < 1900 // pre-VC2015
  int __cdecl access(const char *fn, int mode)
  {
    CVT_TO_U16(fn);
    LOG_CALL("access(%s)", fn);
    return ::_waccess(fn_u16, mode);
  }

  FILE *__cdecl fopen(const char *fn, const char *mode)
  {
    CVT_TO_U16(fn);
    CVT_TO_U16(mode);
    LOG_CALL("fopen(%s, %s)", fn, mode);
    return ::_wfopen(fn_u16, mode_u16);
  }
  FILE *__cdecl _fsopen(const char *fn, const char *mode, int _ShFlag)
  {
    CVT_TO_U16(fn);
    CVT_TO_U16(mode);
    LOG_CALL("_fsopen(%s, %s)", fn, mode);
    return ::_wfsopen(fn_u16, mode_u16, _ShFlag);
  }
  errno_t __cdecl fopen_s(FILE **fp, const char *fn, const char *mode)
  {
    CVT_TO_U16(fn);
    CVT_TO_U16(mode);
    LOG_CALL("fopen_s(%s, %s)", fn, mode);
    return ::_wfopen_s(fp, fn_u16, mode_u16);
  }
  int __cdecl _stat32(const char *fn, struct _stat32 *_Stat)
  {
    CVT_TO_U16(fn);
    LOG_CALL("_stat32(%s, %p)", fn, _Stat);
    return ::_wstat32(fn_u16, _Stat);
  }
  int __cdecl _stat32i64(const char *fn, struct _stat32i64 *_Stat)
  {
    CVT_TO_U16(fn);
    LOG_CALL("_stat32i64(%s, %p)", fn, _Stat);
    return ::_wstat32i64(fn_u16, _Stat);
  }
  int __cdecl _stat64i32(const char *fn, struct _stat64i32 *_Stat)
  {
    CVT_TO_U16(fn);
    LOG_CALL("_stat64i32(%s, %p)", fn, _Stat);
    return ::_wstat64i32(fn_u16, _Stat);
  }
  int __cdecl _stat64(const char *fn, struct _stat64 *_Stat)
  {
    CVT_TO_U16(fn);
    LOG_CALL("_stat64(%s, %p)", fn, _Stat);
    return ::_wstat64(fn_u16, _Stat);
  }

  int __cdecl mkdir(const char *fn)
  {
    CVT_TO_U16(fn);
    LOG_CALL("mkdir(%s)", fn);
    return ::_wmkdir(fn_u16);
  }
  int __cdecl rmdir(const char *fn)
  {
    CVT_TO_U16(fn);
    LOG_CALL("rmdir(%s)", fn);
    return ::_wrmdir(fn_u16);
  }

  int __cdecl remove(const char *fn)
  {
    CVT_TO_U16(fn);
    LOG_CALL("remove(%s)", fn);
    return ::_wremove(fn_u16);
  }
  int __cdecl _unlink(const char *fn)
  {
    CVT_TO_U16(fn);
    LOG_CALL("_unlink(%s)", fn);
    return ::_wremove(fn_u16);
  }
#if !__STDC__
  int __cdecl unlink(const char *fn)
  {
    CVT_TO_U16(fn);
    LOG_CALL("unlink(%s)", fn);
    return ::_wremove(fn_u16);
  }
  int __cdecl open(const char *fn, int mode, ...)
  {
    CVT_TO_U16(fn);
    va_list ap;
    va_start(ap, mode);
    int pmode = (mode & _O_CREAT) ? va_arg(ap, int) : 0;
    va_end(ap);
    LOG_CALL("open(%s,%d,%d)", fn, mode, pmode);
    return _wopen(fn_u16, mode, pmode);
  }
#endif

#elif UCRT_10_0_10586_0_OR_LESS // VC2015
#include <crtdbg.h>
BOOL __cdecl __acrt_copy_path_to_wide_string(char const *const fn, wchar_t **const result)
{
  CVT_TO_U16(fn);
  *result = _wcsdup_dbg(fn_u16, _CRT_BLOCK, __FILE__, __LINE__);
  return TRUE;
}
BOOL __cdecl __acrt_copy_to_char(wchar_t const *const string, char **const result)
{
  static constexpr int MAX_BCNT = 3; // 3 bytes for utf8 to encode unicode char upto 0x26218 (enough to 16-bit wchar_t)
  int slen = (int)wcslen(string);
  *result = (char *)_malloc_dbg(slen * MAX_BCNT + 1, _CRT_BLOCK, __FILE__, __LINE__);
  int ret = WideCharToMultiByte(CP_UTF8, 0, string, slen + 1, *result, slen * MAX_BCNT + 1, NULL, NULL);
  return ret && ret <= slen * MAX_BCNT;
}
#else
#include <locale.h>
static const char *applied_locale = setlocale(LC_CTYPE, "en-US.utf8");
#endif


// replacements that fit both pre-VC2015 and VC2015 CRT
#if _MSC_VER < 1900 || UCRT_10_0_10586_0_OR_LESS // pre-VC2015 or VC2015 UCRT-10.0.10586.0
  int __cdecl chdir(const char *fn)
  {
    CVT_TO_U16(fn);
    LOG_CALL("chdir(%s)", fn);
    return ::_wchdir(fn_u16);
  }
  char *__cdecl getcwd(char *dest, int sz)
  {
    wchar_t *dest_u16 = (wchar_t *)alloca(sz * sizeof(wchar_t));
    if (!_wgetcwd(dest_u16, sz))
      return NULL;
    if (WideCharToMultiByte(CP_UTF8, 0, dest_u16, -1, dest, sz, NULL, NULL) < sz)
    {
      LOG_CALL("getcwd(%p,%d) -> <%s>", dest, sz, dest);
      return dest;
    }
    return NULL;
  }
  int __cdecl rename(const char *old_fn, const char *new_fn)
  {
    CVT_TO_U16(old_fn);
    CVT_TO_U16(new_fn);
    LOG_CALL("rename(%s, %s)", old_fn, new_fn);
    return ::_wrename(old_fn_u16, new_fn_u16);
  }
#endif

#ifdef __cplusplus
}
#endif

#undef CVT_TO_U16

#endif
