#if _TARGET_PC_WIN
#include <windows.h>
#endif

#include "drv_utils.h"

#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <math/integer/dag_IPoint2.h>
#include <osApiWrappers/dag_unicode.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <util/dag_parseResolution.h>
#include "gpuConfig.h"

#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include "drv_returnAddrStore.h"
#include <util/dag_string.h>

#include <ioSys/dag_memIo.h>
#include <memory/dag_framemem.h>
#include <EASTL/algorithm.h>


bool get_settings_resolution(int &width, int &height, bool &is_retina, int def_width, int def_height, bool &out_is_auto)
{
  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");
  const char *res_str = blk_video.getStr("testResolution", blk_video.getStr("resolution", NULL));
  const char *def_max_res = "8192 x 8192";
  const char *res_max_str = blk_video.getStr("max_resolution", def_max_res);
  int cur_w, cur_h, max_w, max_h;

  if (res_str && res_str[0] == 'x')
  {
    float scale = atof(res_str + 1);
    const int maxDimension = blk_video.getInt("maxLowerDimension", -1);
    const int smallerDimension = eastl::min(def_width, def_height);

    if (maxDimension > 0 && smallerDimension > maxDimension)
      scale = ((float)maxDimension * scale) / smallerDimension;
    width = (int)(scale * def_width);
    height = (int)(scale * def_height);
    width -= width % 16; // Grant a 4 pixel alignment for quarter-res targets.
    height -= height % 16;
    out_is_auto = false;
  }
  else if (res_str && get_resolution_from_str(res_str, cur_w, cur_h))
  {
    width = cur_w;
    height = cur_h;
    is_retina = strstr(res_str, ":R") != nullptr;
    out_is_auto = false;
  }
  else
  {
    width = def_width;
    height = def_height;
    out_is_auto = true;
  }

  if (res_max_str && get_resolution_from_str(res_max_str, max_w, max_h))
  {
    bool isOk = width > 0 && height > 0 && width <= max_w && height <= max_h;
    width = min(width, max_w);
    height = min(height, max_h);
    return isOk;
  }

  return width > 0 && height > 0;
}

#if _TARGET_APPLE
bool get_settings_use_retina()
{
  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");
  return blk_video.getBool("use_retina", true);
}
#endif

#if _TARGET_PC_WIN
bool get_default_display_settings(DEVMODE *devMode)
{
  DISPLAY_DEVICE displayDevice{};
  displayDevice.cb = sizeof(DISPLAY_DEVICE);
  for (DWORD deviceNum = 0; EnumDisplayDevices(nullptr, deviceNum, &displayDevice, 0); deviceNum++)
  {
    if (EnumDisplaySettings(displayDevice.DeviceName, ENUM_CURRENT_SETTINGS, devMode) && devMode->dmPosition.x == 0 &&
        devMode->dmPosition.y == 0 && // primary monitor
        displayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
      return true;
  }
  return false;
}

bool get_default_display_name(char (&displayName)[sizeof(DEVMODE::dmDeviceName)])
{
  DEVMODE devMode{};
  devMode.dmSize = sizeof(DEVMODE);
  if (get_default_display_settings(&devMode))
  {
    memcpy(displayName, devMode.dmDeviceName, sizeof(DEVMODE::dmDeviceName));
    return true;
  }
  return false;
}

bool get_display_settings(const char *displayName, DEVMODE *devMode)
{
  return displayName ? EnumDisplaySettings(displayName, ENUM_CURRENT_SETTINGS, devMode) > 0 : get_default_display_settings(devMode);
}

void get_current_display_screen_mode(int &out_def_left, int &out_def_top, int &out_def_width, int &out_def_height)
{
  const char *displayName = get_monitor_name_from_settings();

  RECT workAreaRect = {0};
  if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0))
    workAreaRect = {0}; // Yeah that works.

  int userDisplayWidth = 0;
  int userDisplayHeight = 0;
  int userDisplayLeft = 0;
  int userDisplayTop = 0;
  DEVMODE devMode = {0};
  devMode.dmSize = sizeof(DEVMODE);

  // Returns 0 x 0 on RDP connection.
  if (get_display_settings(displayName, &devMode) && devMode.dmPelsWidth > 0 && devMode.dmPelsHeight > 0)
  {
    userDisplayWidth = static_cast<int>(devMode.dmPelsWidth);
    userDisplayHeight = static_cast<int>(devMode.dmPelsHeight);
    userDisplayLeft = static_cast<int>(devMode.dmPosition.x);
    userDisplayTop = static_cast<int>(devMode.dmPosition.y);
  }

  debug("get_current_display_screen_mode - SPI_GETWORKAREA: %d,%d-%d,%d, EnumDisplaySettings: %dx%d", workAreaRect.left,
    workAreaRect.top, workAreaRect.right, workAreaRect.bottom, userDisplayWidth, userDisplayHeight);

  if (dgs_get_window_mode() == WindowMode::WINDOWED && workAreaRect.right - workAreaRect.left > 0 &&
      workAreaRect.bottom - workAreaRect.top > 0)
  {
    debug("get_current_display_screen_mode - use SPI_GETWORKAREA");
    out_def_left = workAreaRect.left;
    out_def_top = workAreaRect.top;
    out_def_width = workAreaRect.right - workAreaRect.left;
    out_def_height = workAreaRect.bottom - workAreaRect.top;
  }
  else if (userDisplayWidth > 0 && userDisplayHeight > 0)
  {
    debug("get_current_display_screen_mode - use EnumDisplaySettings");
    out_def_left = userDisplayLeft;
    out_def_top = userDisplayTop;
    out_def_width = userDisplayWidth;
    out_def_height = userDisplayHeight;
  }
  else
  {
    logwarn("get_current_display_screen_mode - fallback to default resolution");
    out_def_left = out_def_top = 0;
    out_def_width = FALLBACK_SCREEN_WIDTH;
    out_def_height = FALLBACK_SCREEN_HEIGHT;
  }
}

void get_render_window_settings(RenderWindowSettings &p, Driver3dInitCallback *cb)
{
  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");

  int scr_wdt, scr_hgt;
  int base_scr_wdt, base_scr_hgt;
  int base_scr_left, base_scr_top;
  get_current_display_screen_mode(base_scr_left, base_scr_top, base_scr_wdt, base_scr_hgt);
  scr_wdt = base_scr_wdt;
  scr_hgt = base_scr_hgt;

  debug("base_scr = %dx%d", base_scr_wdt, base_scr_hgt);

  bool isAutoResolution = false, isRetina = false;
  G_VERIFY(get_settings_resolution(scr_wdt, scr_hgt, isRetina, base_scr_wdt, base_scr_hgt, isAutoResolution));

  auto windowMode = dgs_get_window_mode();
  bool verifyResolution = blk_video.getBool("verifyResolution", true);
  if (verifyResolution && cb)
    cb->verifyResolutionSettings(scr_wdt, scr_hgt, base_scr_wdt, base_scr_hgt, windowMode == WindowMode::WINDOWED);

  bool force_window_caption_visible = blk_video.getBool("force_window_caption_visible", false);

  RECT wr = {0}, cr = {0};
  p.winStyle = WS_POPUP;
  if (windowMode == WindowMode::WINDOWED)
    p.winStyle |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER;

  if (windowMode == WindowMode::WINDOWED_FULLSCREEN)
  {
    const char *displayName = get_monitor_name_from_settings();

    RECT monitorRect;
    if (get_monitor_rect(displayName, monitorRect))
    {
      base_scr_wdt = monitorRect.right - monitorRect.left;
      base_scr_hgt = monitorRect.bottom - monitorRect.top;
      base_scr_left = monitorRect.left;
      base_scr_top = monitorRect.top;
    }

    scr_wdt = min(scr_hgt * base_scr_wdt / base_scr_hgt, base_scr_wdt);
    wr.left = base_scr_left;
    wr.top = base_scr_top;
    wr.right = base_scr_left + base_scr_wdt;
    wr.bottom = base_scr_top + base_scr_hgt;
    cr = wr;
  }
  else if (windowMode != WindowMode::FULLSCREEN_EXCLUSIVE)
  {
    if (int idx = blk_video.findParam("position"); idx != -1)
    {
      const IPoint2 &position = blk_video.getIPoint2(idx);
      wr.left = position.x;
      wr.top = position.y;
    }
    else
    {
      wr.left = (base_scr_wdt - scr_wdt) / 2 + base_scr_left;
      wr.top = (base_scr_hgt - scr_hgt) / 2 + base_scr_top;
    }

    wr.right = wr.left + scr_wdt;
    wr.bottom = wr.top + scr_hgt;

    RECT br = {0}, newWr = wr;
    AdjustWindowRectEx(&newWr, p.winStyle, FALSE, WS_EX_APPWINDOW);
    br.left = newWr.left - wr.left;
    br.top = newWr.top - wr.top;
    br.right = newWr.right - wr.right;
    br.bottom = newWr.bottom - wr.bottom;
    wr = newWr;

    if (isAutoResolution)
    {
      wr.right = min(wr.right - wr.left, (LONG)base_scr_wdt);
      wr.bottom = min(wr.bottom - wr.top, (LONG)base_scr_hgt);
      wr.left = max(wr.left, (LONG)base_scr_left);
      wr.top = max(wr.top, (LONG)base_scr_top);
      wr.right += wr.left;
      wr.bottom += wr.top;

      scr_wdt = wr.right - wr.left + br.left - br.right;
      scr_hgt = wr.bottom - wr.top + br.top - br.bottom;
    }
    else
    {
      wr.right += max(-wr.left, 0L);
      wr.bottom += max(-wr.top, 0L);
      wr.left = max(wr.left, 0L);
      wr.top = max(wr.top, 0L);
    }

    if (force_window_caption_visible && wr.top < -3)
    {
      int dy = wr.top + 3;
      int dsz = scr_hgt - ((scr_hgt + dy) & ~0xF);
      scr_hgt -= dsz;
      wr.top -= dy;
      wr.bottom -= dy + dsz;
    }

    cr.left = cr.top = 0; // -V1048
    cr.right = scr_wdt;
    cr.bottom = scr_hgt;
  }
  else
  {
    wr.left = base_scr_left;
    wr.top = base_scr_top;
    wr.right = wr.left + scr_wdt;
    wr.bottom = wr.top + scr_hgt;
    cr = wr;
  }

  p.resolutionX = scr_wdt;
  p.resolutionY = scr_hgt;
  p.clientWidth = cr.right - cr.left;
  p.clientHeight = cr.bottom - cr.top;
  p.aspect = scr_hgt ? float(scr_wdt) / float(scr_hgt) : 1.0f;

  p.winRectLeft = wr.left;
  p.winRectRight = wr.right;
  p.winRectTop = wr.top;
  p.winRectBottom = wr.bottom;
}

bool set_render_window_params(RenderWindowParams &p, const RenderWindowSettings &s)
{
  if (!p.rwnd)
  {
    static wchar_t wcsbuf[2][256];

    if (p.wcname && p.mainProc)
    {
      WNDCLASSW wc{};
      wc.style = CS_HREDRAW | CS_VREDRAW;
      wc.lpfnWndProc = (WNDPROC)p.mainProc;
      wc.hInstance = (HINSTANCE)p.hinst;
      wc.hIcon = (HICON)p.icon;
      wc.hCursor = (HCURSOR)win32_init_empty_mouse_cursor();
      wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
      wc.lpszClassName = utf8_to_wcs(p.wcname, wcsbuf[0], 256);
      if (!RegisterClassW(&wc))
      {
        int err = GetLastError();
        if (err && err != ERROR_ALREADY_EXISTS && err != ERROR_CLASS_ALREADY_EXISTS)
        {
          logerr("Can't register window class (%08X)", err);
          return false;
        }
      }
    }

    debug("window = %dx%d .. %dx%d", s.winRectLeft, s.winRectTop, s.winRectRight, s.winRectBottom);

    p.hwnd = p.rwnd = CreateWindowExW(WS_EX_APPWINDOW, utf8_to_wcs(p.wcname, wcsbuf[0], 256), utf8_to_wcs(p.title, wcsbuf[1], 256),
      s.winStyle, s.winRectLeft, s.winRectTop, s.winRectRight - s.winRectLeft, s.winRectBottom - s.winRectTop, NULL, NULL,
      (HINSTANCE)p.hinst, NULL);

    if (!p.hwnd)
    {
      logerr("Can't create window (%08X)", GetLastError());
      return false;
    }

    ShowWindow((HWND)p.hwnd, p.ncmdshow);
    UpdateWindow((HWND)p.hwnd);

    // Adjust window size if the client area is incorrect.
    if (dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE)
    {
      RECT clientRect = {0};
      if (GetClientRect((HWND)p.hwnd, &clientRect))
      {
        if (clientRect.right - clientRect.left != s.clientWidth || clientRect.bottom - clientRect.top != s.clientHeight)
        {
          debug("Incorrect window client area size: rect=%d,%d-%d,%d, required=%dx%d", clientRect.left, clientRect.top,
            clientRect.right, clientRect.bottom, s.clientWidth, s.clientHeight);

          int addW = s.clientWidth - (clientRect.right - clientRect.left);
          int addH = s.clientHeight - (clientRect.bottom - clientRect.top);

          SetWindowPos((HWND)p.hwnd, HWND_TOP, s.winRectLeft, s.winRectTop, s.winRectRight - s.winRectLeft + addW,
            s.winRectBottom - s.winRectTop + addH, SWP_SHOWWINDOW);
        }
      }
    }
  }
  else
  {
    bool isVisible = IsWindowVisible((HWND)p.rwnd);
    SetWindowLong((HWND)p.rwnd, GWL_STYLE, s.winStyle);
    SetWindowPos((HWND)p.rwnd, HWND_TOP, s.winRectLeft, s.winRectTop, s.winRectRight - s.winRectLeft, s.winRectBottom - s.winRectTop,
      isVisible ? SWP_SHOWWINDOW : 0); // don't change visibility for hidden windows
  }

  return true;
}

int set_display_device_mode(bool inwin, int res_x, int res_y, int scr_bpp, DEVMODE &out_devm)
{
  const char *displayName = get_monitor_name_from_settings();
  char defaultDisplayName[sizeof(DEVMODE::dmDeviceName)];

  if (!displayName && get_default_display_name(defaultDisplayName))
    displayName = defaultDisplayName;

  DEVMODE dm;
  int mode_w = 0;
  int mode_h = 0;
  int mode_b = 0;
  int mode_rd = 0x7FFFFFFF;
  int mode_ok = 0;
  dm.dmSize = sizeof(dm);
  for (int mi = 0;; ++mi)
  {
    if (!EnumDisplaySettings(displayName, mi, &dm))
      break;

    int b = dm.dmBitsPerPel;
    if (b < mode_b || b > scr_bpp)
      continue;

    int w = dm.dmPelsWidth, h = dm.dmPelsHeight;
    int d = (w - res_x) * (w - res_x) + (h - res_y) * (h - res_y);
    if (b != mode_b || d < mode_rd)
    {
      mode_w = w;
      mode_h = h;
      mode_b = b;
      mode_rd = d;
      out_devm = dm;
      mode_ok = 1;
    }
  }

  if (!mode_ok)
  {
    logerr("No suitable mode found (%dx%dx%d requested)", res_x, res_y, scr_bpp);
    return 0;
  }

  out_devm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

  LONG res = ChangeDisplaySettings(&out_devm, inwin ? 0 : CDS_FULLSCREEN);
  if (res != DISP_CHANGE_SUCCESSFUL)
  {
    if (res == DISP_CHANGE_RESTART)
      logerr("You should set display mode %dx%dx%d manually", mode_w, mode_h, mode_b);
    else
      logerr("Error setting display mode %dx%dx%d", mode_w, mode_h, mode_b);
    return 0;
  }

  return 1;
}

bool get_monitor_info(const char *monitorName, String *friendlyMonitorName, int *uniqueIndex)
{
  int index = 0;
  DISPLAY_DEVICE device;
  device.cb = sizeof(device);
  for (int deviceIndex = 0; EnumDisplayDevices(nullptr, deviceIndex, &device, 0); ++deviceIndex)
  {
    DISPLAY_DEVICE deviceMonitor;
    deviceMonitor.cb = sizeof(deviceMonitor);
    if (EnumDisplayDevices(device.DeviceName, 0, &deviceMonitor, 0))
    {
      if (strcmp(device.DeviceName, monitorName) == 0)
      {
        if (friendlyMonitorName)
          friendlyMonitorName->setStr(deviceMonitor.DeviceString);
        if (uniqueIndex)
          *uniqueIndex = index;
        return true;
      }
      ++index;
    }
  }
  return false;
}

bool get_monitor_rect(const char *monitorName, RECT &monitorRect)
{
  DEVMODE devmode{};
  devmode.dmSize = sizeof(DEVMODE);
  if (get_display_settings(monitorName, &devmode))
  {
    monitorRect.left = devmode.dmPosition.x;
    monitorRect.top = devmode.dmPosition.y;
    monitorRect.right = devmode.dmPosition.x + devmode.dmPelsWidth;
    monitorRect.bottom = devmode.dmPosition.y + devmode.dmPelsHeight;
    return true;
  }
  return false;
}

/*
 *  Returns if the window is completely occluded by the current foreground window.
 *  Since it only checks occlusion with one window for performance reasons, we need to ignore the
 *  system tray area to handle the windowed fullscreen case properly.
 *  This function is needed because there is no concept of occluded windows with DWM enabled,
 *  all windows are offscreens which are never occluded and can be presented as live preview pictures
 *  so Win32 and DXGI swapchain Present do not return true occlusion info in general.
 */
bool is_window_occluded(HWND window)
{
  HWND foregroundWindow = ::GetForegroundWindow();
  if (foregroundWindow == window)
    return false;

  // Layered windows can be transparent so let's ignore them
  if (foregroundWindow == NULL || (GetWindowLong(foregroundWindow, GWL_EXSTYLE) & WS_EX_LAYERED) != 0)
    return false;

  RECT foregroundRect;
  GetWindowRect(foregroundWindow, &foregroundRect);
  RECT windowRect;
  GetWindowRect(window, &windowRect);
  HMONITOR hMonitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mInfo = {sizeof(MONITORINFO)};
  if (GetMonitorInfo(hMonitor, &mInfo))
  {
    // Subtract the system tray area from the window rect to ignore it
    RECT nonWorkRect;
    SubtractRect(&nonWorkRect, &mInfo.rcMonitor, &mInfo.rcWork);
    SubtractRect(&windowRect, &windowRect, &nonWorkRect);
  }
  bool occluded = false;
  if (!SubtractRect(&windowRect, &windowRect, &foregroundRect))
  {
    // Let's check out if the window is not forced into the topmost z-band while not having the focus
    POINT pt = {(foregroundRect.left + foregroundRect.right) / 2, (foregroundRect.top + foregroundRect.bottom) / 2};
    occluded = (WindowFromPoint(pt) != window);
  }
  return occluded;
}

bool has_focus(HWND hWnd, UINT message, WPARAM wParam)
{
  return ((message == WM_ACTIVATEAPP && wParam) ||
           (message == WM_ACTIVATE && (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE))) &&
         !IsIconic(hWnd);
}

DAGOR_NOINLINE void paint_window(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, void *proc)
{
  PAINTSTRUCT ps;
  BeginPaint(hWnd, &ps);
  EndPaint(hWnd, &ps);
  if (proc)
    reinterpret_cast<main_wnd_f *>(proc)(hWnd, message, wParam, lParam);
}
#endif

static bool isHDRBlackListed(const DataBlock &blk, const char *name)
{
  if (!name || name[0] == 0)
    return false;
  bool result = false;
  dblk::iterate_params_by_name(blk, "hdrBlackList",
    [&](int param_idx, auto, auto) { result |= strcmp(blk.getStr(param_idx), name) == 0; });
  return result;
}

bool get_enable_hdr_from_settings(const char *name)
{
  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");
  // directx/enableHdr should become deprecated in time
#if _TARGET_IOS | _TARGET_TVOS
  const char *drvName = "metal_ios";
#elif _TARGET_PC_MACOSX
  const char *drvName = "metal";
#else
  const char *drvName = "directx";
#endif
  const DataBlock &blkDrv = *dgs_get_settings()->getBlockByNameEx(drvName);
  return blk_video.getBool("enableHdr", blkDrv.getBool("enableHdr", false)) && !isHDRBlackListed(blkDrv, name);
}

const char *get_monitor_name_from_settings()
{
  const char *displayName = ::dgs_get_settings()->getBlockByNameEx("video")->getStr("monitor", nullptr);
  // displayName should become deprecated in time
  if (!displayName)
    displayName = ::dgs_get_settings()->getBlockByNameEx("directx")->getStr("displayName", nullptr);
  return resolve_monitor_name(displayName);
}

const char *resolve_monitor_name(const char *displayName)
{
  if (displayName && *displayName != '\0' && strcmp(displayName, "auto") != 0)
    return displayName;
  return nullptr;
}

namespace d3d
{
GpuAutoLock::GpuAutoLock()
{
  BEFORE_LOCK();
  driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);
  AFTER_SUCCESSFUL_LOCK();
}
GpuAutoLock ::~GpuAutoLock() { driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL); }
} // namespace d3d
