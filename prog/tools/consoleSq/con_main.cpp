#define __UNLIMITED_BASE_PATH 1
#define __DEBUG_FILEPATH      (::dgs_get_argv("debugfile") ? "debug" : NULL)
#include <startup/dag_mainCon.inc.cpp>
// stub functions to avoid linking workCycle, drv3d_vulkan and X11
void win32_set_window_title(const char *) {}
void win32_set_window_title_utf8(const char *) {}
void win32_set_window_title_tooltip_utf8(const char *, const char *) {}
