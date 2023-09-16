#include <supp/_platform.h>
#include <util/dag_globDef.h>
#include <util/limBufWriter.h>
#include <util/dag_cropMultiLineStr.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <startup/dag_restart.h>
#include <workCycle/dag_gameSettings.h>
#include <osApiWrappers/dag_unicode.h>
#include <osApiWrappers/dag_wndProcComponent.h>

void messagebox_win_report_fatal_error(const char *title, const char *msg, const char *call_stack)
{
  if (!::dgs_execute_quiet)
  {
    static char text[8192];

    LimitedBufferWriter lbw(text, sizeof(text));
    if (const char *stk_end = get_multi_line_str_end(call_stack, 10 * 2 + 1, 4096))
      lbw.aprintf("%s\n%.*s%s", msg, int(stk_end - call_stack), call_stack, *stk_end ? "\n..." : "");
    else
      lbw.aprintf("%s", msg);
    lbw.done();

    HWND hwnd = (HWND)win32_get_main_wnd();
    if (hwnd && (GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE))
      ShowWindow(hwnd,
        dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE ? SW_NORMAL : (is_main_thread() ? SW_MINIMIZE : SW_FORCEMINIMIZE));

    wchar_t titlewcs[8192];
    wchar_t textwcs[8192];
    utf8_to_wcs(title, titlewcs, countof(titlewcs));
    utf8_to_wcs(text, textwcs, countof(textwcs));

    ScopeDetachAllWndComponents wndCompsGuard; // stop handling windows input events during fatal message box
    MessageBoxW(NULL, textwcs, titlewcs, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
  }
}
