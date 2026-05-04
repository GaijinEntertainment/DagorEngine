// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqmodules/sqmodules.h>
#include <quirrel/bindQuirrelEx/bindQuirrelEx.h>
#include <startup/dag_globalSettings.h>
#include <workCycle/dag_gameSettings.h>
#include <debug/dag_logSys.h>
#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <util/dag_compilerDefs.h>

#if _TARGET_PC_WIN
#include <windows.h>
#endif

#if _TARGET_PC_LINUX
extern SQInteger linux_get_primary_screen_info(HSQUIRRELVM vm);
#endif

#if _TARGET_PC_MACOSX
extern SQInteger macosx_get_primary_screen_info(HSQUIRRELVM vm);
#endif

namespace bindquirrel
{

static SqExitFunctionPtr sq_exit_function_ptr = nullptr;

void set_sq_exit_function_ptr(SqExitFunctionPtr func) { sq_exit_function_ptr = func; }

static SQInteger get_all_arg_values_by_name(HSQUIRRELVM vm)
{
  const char *name = nullptr;
  sq_getstring(vm, 2, &name);

  sq_newarray(vm, 0);
  int it = 1;
  for (const char *ap; (ap = ::dgs_get_argv(name, it)) != NULL;)
  {
    sq_pushstring(vm, ap, -1);
    sq_arrayappend(vm, -2);
  }

  return 1;
}

static void exit_func(int exit_code)
{
  if (sq_exit_function_ptr)
    sq_exit_function_ptr(exit_code);
  else
    logerr("dagor.system.exit(%d) called but sq_exit_function_ptr not set", exit_code);
}


static SQInteger get_primary_screen_info(HSQUIRRELVM vm)
{
#if _TARGET_PC_WIN
  HDC desktopDc = GetDC(nullptr);
  Sqrat::Table res(vm);
  res.SetValue("horizDpi", GetDeviceCaps(desktopDc, LOGPIXELSX));
  res.SetValue("vertDpi", GetDeviceCaps(desktopDc, LOGPIXELSY));
  res.SetValue("physWidth", GetDeviceCaps(desktopDc, HORZSIZE));
  res.SetValue("physHeight", GetDeviceCaps(desktopDc, VERTSIZE));
  res.SetValue("pixelsWidth", GetDeviceCaps(desktopDc, HORZRES));
  res.SetValue("pixelsHeight", GetDeviceCaps(desktopDc, VERTRES));
  ReleaseDC(nullptr, desktopDc);
  sq_pushobject(vm, res);
  return 1;
#elif _TARGET_PC_MACOSX
  return macosx_get_primary_screen_info(vm);
#elif _TARGET_PC_LINUX
  return linux_get_primary_screen_info(vm);
#else
  return sq_throwerror(vm, "get_primary_screen_info not implemented on this platform");
#endif
}


static SQInteger message_box(HSQUIRRELVM vm)
{
  const char *text = nullptr, *caption = nullptr;
  SQInteger flags = 0;
  sq_getstring(vm, 2, &text);
  sq_getstring(vm, 3, &caption);
  if (sq_gettop(vm) > 3)
    sq_getinteger(vm, 4, &flags);
  int res = os_message_box(text, caption, flags);
  sq_pushinteger(vm, res);
  return 1;
}


static void set_app_window_title(const char *title)
{
  G_UNUSED(title);
#if _TARGET_PC
  ::win32_set_window_title(title);
#endif
}

static void set_run_in_background(bool value) { ::dgs_dont_use_cpu_in_background = !value; }

static bool get_run_in_background() { return !::dgs_dont_use_cpu_in_background; }


void register_dagor_system(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table exports(vm);
  Sqrat::Array argv(vm);
  for (int i = 0; i < ::dgs_argc; i++)
    argv.Append(::dgs_argv[i]);
  argv.FreezeSelf();

#if defined(_TARGET_64BIT)
  const int targetArch = 64;
#else
  const int targetArch = 32;
#endif

#define GUI_CONST(x) .SetValue(#x, GUI_##x)

  ///@module dagor.system
  exports //
    .Func("dgs_get_settings", dgs_get_settings)
    ///@return DataBlock : datablock with game settings
    .Func("exit", exit_func)
    .Func("get_arg_value_by_name", [](const char *name) { return dgs_get_argv(name); })
    ///@return s|null : null if no named argument provided
    ///@param name s : return commandline named argument value if provied (-name:value)

    .Func("get_log_directory", get_log_directory)
    .SquirrelFuncDeclString(get_all_arg_values_by_name, "get_all_arg_values_by_name(name: string): array|null",
      "returns null or list [value1, value2,...] of all command line arguments provided in -name:value1 -name2:value2")
    ///@param name s
    ///@return a|null : returns null or list [value1, value2,...] of all command line arguments provided in -name:value1 -name2:value2
    /// format
    .SquirrelFuncDeclString(get_primary_screen_info, "get_primary_screen_info(): table")
    .SquirrelFuncDeclString(message_box, "message_box(text: string, caption: string, [flags: int]): int")
    ///@param mbx_title s
    ///@param mbx_text s
    ///@param mbx_level i : can be error, warning, info level
    .Func("set_app_window_title", set_app_window_title)
    ///@param s : title
    .Func("set_run_in_background", set_run_in_background)
    ///@param value b : if true, the app will use CPU when in background
    .Func("get_run_in_background", get_run_in_background)
    ///@return b : true if the app is set to use CPU when in background

    /// @skipline
    .SetValue("argv", argv)
    /// @const argv
    ///@ftype a

    /// @skipline
    .SetValue("DBGLEVEL", DAGOR_DBGLEVEL)
  /// @const DBGLEVEL

  /// @skipline
#if DAGOR_ADDRESS_SANITIZER
    .SetValue("SANITIZER", "address")
#elif DAGOR_THREAD_SANITIZER
    .SetValue("SANITIZER", "thread")
#else
    .SetValue("SANITIZER", /*null*/ Sqrat::Object{})
#endif
    /// @const SANITIZER

    /// @skipline
    .SetValue("ARCH_BITS", targetArch)
    /// @const ARCH_BITS

    // Flags for message_box.
    /// @const MB_OK
    GUI_CONST(MB_OK)
    /// @const MB_OK_CANCEL
    GUI_CONST(MB_OK_CANCEL)
    /// @const MB_YES_NO
    GUI_CONST(MB_YES_NO)
    /// @const MB_RETRY_CANCEL
    GUI_CONST(MB_RETRY_CANCEL)
    /// @const MB_ABORT_RETRY_IGNORE
    GUI_CONST(MB_ABORT_RETRY_IGNORE)
    /// @const MB_YES_NO_CANCEL
    GUI_CONST(MB_YES_NO_CANCEL)
    /// @const MB_CANCEL_TRY_CONTINUE
    GUI_CONST(MB_CANCEL_TRY_CONTINUE)

    /// @const MB_DEF_BUTTON_1
    GUI_CONST(MB_DEF_BUTTON_1)
    /// @const MB_DEF_BUTTON_2
    GUI_CONST(MB_DEF_BUTTON_2)
    /// @const MB_DEF_BUTTON_3
    GUI_CONST(MB_DEF_BUTTON_3)

    /// @const MB_ICON_ERROR
    GUI_CONST(MB_ICON_ERROR)
    /// @const MB_ICON_WARNING
    GUI_CONST(MB_ICON_WARNING)
    /// @const MB_ICON_QUESTION
    GUI_CONST(MB_ICON_QUESTION)
    /// @const MB_ICON_INFORMATION
    GUI_CONST(MB_ICON_INFORMATION)

    /// @const MB_TOPMOST
    GUI_CONST(MB_TOPMOST)
    /// @const MB_FOREGROUND
    GUI_CONST(MB_FOREGROUND)
    /// @const MB_NATIVE_DLG
    GUI_CONST(MB_NATIVE_DLG)

    // Return values for message_box.
    /// @const MB_FAIL
    GUI_CONST(MB_FAIL)
    /// @const MB_CLOSE
    GUI_CONST(MB_CLOSE)
    /// @const MB_BUTTON_1
    GUI_CONST(MB_BUTTON_1)
    /// @const MB_BUTTON_2
    GUI_CONST(MB_BUTTON_2)
    /// @const MB_BUTTON_3
    GUI_CONST(MB_BUTTON_3)
    /**/;

#undef GUI_CONST

  module_mgr->addNativeModule("dagor.system", exports);
}

} // namespace bindquirrel
