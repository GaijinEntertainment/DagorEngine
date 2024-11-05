// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bindQuirrelEx/bindQuirrelEx.h>
#include <osApiWrappers/dag_miscApi.h>
#include <sqModules/sqModules.h>
#include <osApiWrappers/dag_unicode.h>
#if _TARGET_PC_WIN | _TARGET_XBOX
#include <windows.h>
#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_C3

#endif

// From https://docs.microsoft.com/en-us/windows/win32/intl/locale-name-constants
// "The maximum number of characters allowed for this string is nine, including a terminating null character."
#define MAX_LOCAL_STR_LEN 9

#if _TARGET_C1 | _TARGET_C2










#elif _TARGET_C3












#else

static const wchar_t *get_locale_lang(wchar_t *out_buf, size_t n)
{
#if _TARGET_PC_WIN
  return GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, out_buf, n) ? out_buf : nullptr;
#elif _TARGET_XBOX
  return GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SISO639LANGNAME, out_buf, n) ? out_buf : nullptr;
#else
  G_UNUSED(out_buf);
  G_UNUSED(n);
  return L"en";
#endif
}

static const wchar_t *get_locale_country(wchar_t *out_buf, size_t n)
{
#if _TARGET_PC_WIN
  return GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, out_buf, n) ? out_buf : nullptr;
#elif _TARGET_XBOX
  return GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, out_buf, n) ? out_buf : nullptr;
#else
  G_UNUSED(out_buf);
  G_UNUSED(n);
  return L"__";
#endif
}

template <const wchar_t *func(wchar_t *, size_t)>
static SQInteger sq_marshal_locale_attr(HSQUIRRELVM vm)
{
  wchar_t tmpBuf[MAX_LOCAL_STR_LEN];
  if (const wchar_t *lattr = func(tmpBuf, MAX_LOCAL_STR_LEN))
  {
    char tmpu8[MAX_LOCAL_STR_LEN * 2];
    Sqrat::PushVar(vm, wcs_to_utf8(lattr, tmpu8, sizeof(tmpu8)));
  }
  else
    sq_pushnull(vm);
  return 1;
}

#endif

const char *get_default_lang();

static int sq_get_console_model() { return (int)get_console_model(); }
static const char *sq_get_console_model_revision(int consoleModelType)
{
  return get_console_model_revision((ConsoleModel)consoleModelType);
}

static bool is_gdk_used()
{
#if _TARGET_GDK
  return true;
#else
  return false;
#endif
}

static Sqrat::Table make_platform_exports(HSQUIRRELVM vm)
{
  Sqrat::Table tbl(vm);
  /// @module platform
  tbl //
    .SquirrelFunc("get_locale_lang", &sq_marshal_locale_attr<get_locale_lang>, 1, ".")
    .SquirrelFunc("get_locale_country", &sq_marshal_locale_attr<get_locale_country>, 1, ".")
    .Func("get_platform_string_id", get_platform_string_id)
    .Func("get_console_model", sq_get_console_model)
    .Func("get_console_model_revision", sq_get_console_model_revision)
    .Func("get_default_lang", get_default_lang)
    .Func("is_gdk_used", is_gdk_used)
    /**/;
#define SET_CONST(n) tbl.SetValue(#n, SQInteger((int)ConsoleModel::n));
  /// @const UNKNOWN
  SET_CONST(UNKNOWN)
  /// @const PS4
  SET_CONST(PS4)
  /// @const PS4_PRO
  SET_CONST(PS4_PRO)
  /// @const XBOXONE
  SET_CONST(XBOXONE)
  /// @const XBOXONE_S
  SET_CONST(XBOXONE_S)
  /// @const XBOXONE_X
  SET_CONST(XBOXONE_X)
  /// @const XBOX_LOCKHART
  SET_CONST(XBOX_LOCKHART)
  /// @const XBOX_ANACONDA
  SET_CONST(XBOX_ANACONDA)
  /// @const PS5
  SET_CONST(PS5)
  /// @const PS5_PRO
  SET_CONST(PS5_PRO)
  /// @const NINTENDO_SWITCH
  SET_CONST(NINTENDO_SWITCH)
#undef SET_CONST
  return tbl;
}


void bindquirrel::register_platform_module(SqModules *module_mgr)
{
  module_mgr->addNativeModule("platform", make_platform_exports(module_mgr->getVM()));
}
