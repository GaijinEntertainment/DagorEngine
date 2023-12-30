#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/setProgGlobals.h>
#if _TARGET_PC_WIN
#include <windows.h>
#include <debug/dag_debug.h>
#endif

static void *prog_hinstance = NULL;
static void *prog_hwnd = NULL;

void *win32_get_instance() { return prog_hinstance; }
void *win32_get_main_wnd() { return prog_hwnd; }

void win32_set_instance(void *hinst) { prog_hinstance = hinst; }
void win32_set_main_wnd(void *hwnd) { prog_hwnd = hwnd; }

bool win32_rdp_compatible_mode = false;
void *win32_empty_mouse_cursor = nullptr;
int win32_system_dpi = 96;

void *win32_init_empty_mouse_cursor()
{
#if _TARGET_PC_WIN
  if (!win32_empty_mouse_cursor && win32_rdp_compatible_mode)
  {
    static constexpr int MAX_MASK_DATA_SZ = 128 / 8 * 128;
    static char andMaskCursor[MAX_MASK_DATA_SZ], xorMaskCursor[MAX_MASK_DATA_SZ];
    int cursorX = 32, cursorY = 32;
    int sys_cursorX = GetSystemMetrics(SM_CXCURSOR);
    int sys_cursorY = GetSystemMetrics(SM_CYCURSOR);
    memset(andMaskCursor, 0xFF, sizeof(andMaskCursor));
    memset(xorMaskCursor, 0, sizeof(xorMaskCursor));
    win32_empty_mouse_cursor = (void *)::CreateCursor((HINSTANCE)prog_hinstance, 0, 0, cursorX, cursorY, andMaskCursor, xorMaskCursor);
    if (!win32_empty_mouse_cursor && sys_cursorX <= 128 && sys_cursorY <= 128)
      win32_empty_mouse_cursor = (void *)::CreateCursor((HINSTANCE)prog_hinstance, 0, 0, cursorX = sys_cursorX, cursorY = sys_cursorY,
        andMaskCursor, xorMaskCursor);
    logmessage(win32_empty_mouse_cursor ? LOGLEVEL_DEBUG : LOGLEVEL_ERR,
      "initing win32_empty_mouse_cursor=%p (%dx%d): supported size=%dx%d, bitmap size=%d (max %d)", win32_empty_mouse_cursor, cursorX,
      cursorY, sys_cursorX, sys_cursorY, (cursorX + 15) / 16 * 2 * cursorY, MAX_MASK_DATA_SZ);
  }
#endif
  return win32_empty_mouse_cursor;
}

#define EXPORT_PULL dll_pull_osapiwrappers_progGlobals
#include <supp/exportPull.h>
