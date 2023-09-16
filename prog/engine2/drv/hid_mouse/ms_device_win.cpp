#include <humanInput/dag_hiGlobals.h>
#include "ms_device_win.h"
#include <generic/dag_tab.h>
#include <debug/dag_fatal.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <startup/dag_globalSettings.h>
#include "api_wrappers.h"
#include <perfMon/dag_cpuFreq.h>
#include <3d/dag_drv3d.h>
#include <startup/dag_inpDevClsDrv.h>
#include <debug/dag_debug.h>
#include <debug/dag_traceInpDev.h>
#include <math/dag_mathBase.h>
#include <EASTL/algorithm.h>
#include <stdlib.h>
#include "../hid_commonCode/touch_api_impl.inc.cpp"

#if _TARGET_PC_WIN
#include <workCycle/threadedWindow.h>
#include <tpcshrd.h>
#endif

#if _TARGET_PC_WIN && WINVER < 0x0601
#define SM_DIGITIZER      94
#define SM_MAXIMUMTOUCHES 95

/*
 * GetSystemMetrics(SM_DIGITIZER) flag values
 */
#define NID_INTEGRATED_TOUCH 0x00000001
#define NID_EXTERNAL_TOUCH   0x00000002
#define NID_INTEGRATED_PEN   0x00000004
#define NID_EXTERNAL_PEN     0x00000008
#define NID_MULTI_INPUT      0x00000040
#define NID_READY            0x00000080

#define WM_TOUCH 0x0240

/*
 * Touch Input defines and functions
 */

/*
 * Touch input handle
 */
DECLARE_HANDLE(HTOUCHINPUT);

typedef struct tagTOUCHINPUT
{
  LONG x;
  LONG y;
  HANDLE hSource;
  DWORD dwID;
  DWORD dwFlags;
  DWORD dwMask;
  DWORD dwTime;
  ULONG_PTR dwExtraInfo;
  DWORD cxContact;
  DWORD cyContact;
} TOUCHINPUT, *PTOUCHINPUT;
typedef TOUCHINPUT const *PCTOUCHINPUT;


/*
 * Conversion of touch input coordinates to pixels
 */
#define TOUCH_COORD_TO_PIXEL(l) ((l) / 100)

/*
 * Touch input flag values (TOUCHINPUT.dwFlags)
 */
#define TOUCHEVENTF_MOVE       0x0001
#define TOUCHEVENTF_DOWN       0x0002
#define TOUCHEVENTF_UP         0x0004
#define TOUCHEVENTF_INRANGE    0x0008
#define TOUCHEVENTF_PRIMARY    0x0010
#define TOUCHEVENTF_NOCOALESCE 0x0020
#define TOUCHEVENTF_PEN        0x0040
#define TOUCHEVENTF_PALM       0x0080

/*
 * Touch input mask values (TOUCHINPUT.dwMask)
 */
#define TOUCHINPUTMASKF_TIMEFROMSYSTEM 0x0001 // the dwTime field contains a system generated value
#define TOUCHINPUTMASKF_EXTRAINFO      0x0002 // the dwExtraInfo field is valid
#define TOUCHINPUTMASKF_CONTACTAREA    0x0004 // the cxContact and cyContact fields are valid

BOOL(WINAPI *GetTouchInputInfo)
(__in HTOUCHINPUT hTouchInput,               // input event handle; from touch message lParam
  __in UINT cInputs,                         // number of elements in the array
  __out_ecount(cInputs) PTOUCHINPUT pInputs, // array of touch inputs
  __in int cbSize) = NULL;                   // sizeof(TOUCHINPUT)

BOOL(WINAPI *CloseTouchInputHandle)(__in HTOUCHINPUT hTouchInput) = NULL; // input event handle; from touch message lParam

/*
 * RegisterTouchWindow flag values
 */
#define TWF_FINETOUCH (0x00000001)
#define TWF_WANTPALM  (0x00000002)

BOOL(WINAPI *RegisterTouchWindow)(__in HWND hwnd, __in ULONG ulFlags) = NULL;
#endif
#define MOUSEEVENTF_FROMTOUCH 0xFF515700

static const int linux_wrap_edge_distance = 5;
static bool touchscreen_present = false;

using namespace HumanInput;

static RECT rectFromClip(const GenericMouseDevice::ClipRect &cp)
{
  RECT rc;
  rc.left = cp.l;
  rc.top = cp.t;
  rc.right = cp.r;
  rc.bottom = cp.b;
  return rc;
}

bool WinMouseDevice::getCursorPos(POINT &pt, RECT *wr)
{
  if (!::mouse_api_GetCursorPosRel(&pt, win32_get_main_wnd()))
  {
    pt.x = state.mouse.x + state.mouse.deltaX;
    pt.y = state.mouse.y + state.mouse.deltaY;
  }

  RECT r;
  ::mouse_api_GetClientRect(win32_get_main_wnd(), &r);
  if (wr)
    *wr = r;

  bool isIn = (pt.x >= 0 && pt.y >= 0 && pt.x <= r.right && pt.y <= r.bottom);

#if _TARGET_PC_WIN || _TARGET_PC_LINUX
  int w = 1, h = 1;
  d3d::get_screen_size(w, h);
  if (w && h)
  {
    pt.x = pt.x * w / max<int>(r.right - r.left, 1);
    pt.y = pt.y * h / max<int>(r.bottom - r.top, 1);
  }
#endif
  return isIn;
}


static bool wrapCursorPos(POINT &pt, RECT r, int test_margin = 0, int wrap_margin = 0)
{
  if (stg_pnt.touchScreen || stg_pnt.emuTouchScreenWithMouse)
    return false;
  if (pt.x < r.left + test_margin || pt.y < r.top + test_margin || pt.x >= r.right - test_margin || pt.y >= r.bottom - test_margin)
  {
    if (wrap_margin)
    {
      if (pt.x < r.left + test_margin)
        pt.x -= wrap_margin;
      else if (pt.x >= r.right - test_margin)
        pt.x += wrap_margin;
      if (pt.y < r.top + test_margin)
        pt.y -= wrap_margin;
      else if (pt.y >= r.bottom - test_margin)
        pt.y += wrap_margin;
    }
    r.right--;
    r.bottom--;
    pt.x += (pt.x < r.left && r.right > r.left) ? ((r.left - pt.x) / (r.right - r.left) + 1) * (r.right - r.left) : 0;
    pt.y += (pt.y < r.top && r.bottom > r.top) ? ((r.top - pt.y) / (r.bottom - r.top) + 1) * (r.bottom - r.top) : 0;
    pt.x = r.left + (pt.x - r.left) % max<int>(r.right - r.left, 1);
    pt.y = r.top + (pt.y - r.top) % max<int>(r.bottom - r.top, 1);
    return true;
  }
  return false;
}


inline float get_dpi_scale_factor_at(float /*x*/, float /*y*/)
{
#if _TARGET_PC_WIN
  // To conisder: use DPI per monitor instead (MonitorFromPoint/GetDpiForMonitor)
  HDC ctx = GetDC(NULL);
  int dpi = GetDeviceCaps(ctx, LOGPIXELSX);
  float scale = dpi / 96.f;
  ReleaseDC(NULL, ctx);
  return scale;
#else
  return 1.f;
#endif
}

WinMouseDevice::WinMouseDevice()
{
  add_wnd_proc_component(this);
  lbmPressedT0 = lbmReleasedT0 = lbmMotion = lbmMotionDx = lbmMotionDy = 0;

  hidden = true;

  debug_ctx("init start");
#if _TARGET_PC_WIN
  areButtonsSwapped = GetSystemMetrics(SM_SWAPBUTTON);
  dgs_inpdev_rawinput_mouse_inited = false;
  if (dgs_inpdev_allow_rawinput && !win32_rdp_compatible_mode)
  {
    unsigned int ridNum = 0;
    GetRegisteredRawInputDevices(NULL, &ridNum, sizeof(RAWINPUTDEVICE));

    RAWINPUTDEVICE *rid = new RAWINPUTDEVICE[ridNum + 1];
    GetRegisteredRawInputDevices(rid, &ridNum, sizeof(RAWINPUTDEVICE));

    for (int i = 0; i < ridNum; i++)
      if (rid[i].usUsagePage == 0x01 && rid[i].usUsage == 0x02)
        return; // mouse device already registered

    // add HID mouse
    rid[ridNum].usUsagePage = 0x01;
    rid[ridNum].usUsage = 0x02;
    rid[ridNum].dwFlags = 0; // RIDEV_NOLEGACY;
    rid[ridNum].hwndTarget = (HWND)win32_get_main_wnd();

    if (RegisterRawInputDevices(&rid[ridNum], 1, sizeof(RAWINPUTDEVICE)))
      dgs_inpdev_rawinput_mouse_inited = true;
    else
      logerr("Mouse: RegisterRawInputDevices() failed hr=%08X hwnd=%p ridNum=%d", GetLastError(), win32_get_main_wnd(), ridNum);

    delete[] rid;
  }
  else
    debug("Mouse: rawinput disabled");

  int value = GetSystemMetrics(SM_DIGITIZER);
  if (value & NID_READY)
  {
#if WINVER < 0x0601
    HMODULE libHandle = LoadLibrary("user32.dll");
    DEBUG_DUMP_VAR(libHandle);
    G_ASSERT(libHandle);
    GetTouchInputInfo = (BOOL(WINAPI *)(__in HTOUCHINPUT, __in UINT, __out_ecount(cInputs) PTOUCHINPUT, __in int))GetProcAddress(
      libHandle, "GetTouchInputInfo");
    CloseTouchInputHandle = (BOOL(WINAPI *)(__in HTOUCHINPUT))GetProcAddress(libHandle, "CloseTouchInputHandle");
    RegisterTouchWindow = (BOOL(WINAPI *)(__in HWND, __in ULONG))GetProcAddress(libHandle, "RegisterTouchWindow");
    FreeLibrary(libHandle);
    DEBUG_DUMP_VAR(GetTouchInputInfo);
    DEBUG_DUMP_VAR(CloseTouchInputHandle);
    DEBUG_DUMP_VAR(RegisterTouchWindow);
#endif
    if (value & NID_MULTI_INPUT)
      debug("INP: Multitouch found");
    if (value & NID_INTEGRATED_TOUCH)
      debug("INP: Integrated touch device");
    RegisterTouchWindow((HWND)win32_get_main_wnd(), 0);
    touchscreen_present = true;

    const DWORD_PTR dwHwndTabletProperty = TABLET_DISABLE_PRESSANDHOLD |      // disables press and hold (right-click) gesture
                                           TABLET_DISABLE_PENTAPFEEDBACK |    // disables UI feedback on pen up (waves)
                                           TABLET_DISABLE_PENBARRELFEEDBACK | // disables UI feedback on pen button down (circle)
                                           TABLET_DISABLE_FLICKS; // disables pen flicks (back, forward, drag down, drag up)

#define _T(X) "" X
    SetProp((HWND)win32_get_main_wnd(), MICROSOFT_TABLETPENSERVICE_PROPERTY, reinterpret_cast<HANDLE>(dwHwndTabletProperty));
#undef _T
  }

#else
  dgs_inpdev_rawinput_mouse_inited = true;
#endif
  debug_ctx("init done");

  POINT pt;
  overWnd = getCursorPos(pt);
  raw_state_pnt.mouse.x = state.mouse.x = pt.x;
  raw_state_pnt.mouse.y = state.mouse.y = pt.y;
  ::mouse_api_hide_cursor(true);


#if _TARGET_PC_WIN
  HWND hwnd = (HWND)win32_get_main_wnd();
  if (hwnd == GetForegroundWindow())
    setPosition(pt.x, pt.y);
  stg_pnt.tapStillCircleRad *= eastl::max(get_dpi_scale_factor_at(pt.x, pt.y), 1.f);
#endif

  setRelativeMovementMode(relMode);
}


WinMouseDevice::~WinMouseDevice()
{
  del_wnd_proc_component(this);
#if _TARGET_PC_WIN
  releaseCursorClipping();
#endif
}


#if _TARGET_PC_WIN
bool WinMouseDevice::canClipCursor()
{
  POINT pt;
  if (!getCursorPos(pt)) // out of client rect
  {
    // GetSystemMetrics() call is slow, perform it once
    // Assumning that need to restart the game after settings change is not a problem
    static bool swap_buttons = GetSystemMetrics(SM_SWAPBUTTON) != 0;
    SHORT mlbState = GetAsyncKeyState(swap_buttons ? VK_RBUTTON : VK_LBUTTON);

    if ((mlbState & 0x8000) != 0) // pressed
      return false;
  }

  HWND hwnd = (HWND)win32_get_main_wnd();
  if (hwnd != GetForegroundWindow())
    return false;

  return true;
}
#endif

void WinMouseDevice::update()
{
#if _TARGET_PC_WIN

  if (!dgs_inpdev_rawinput_mouse_inited && relMode)
  {
    POINT center, curPos;
    center.x = (clip.r - clip.l) / 2;
    center.y = (clip.b - clip.t) / 2;

    mouse_api_GetCursorPosRel(&curPos, win32_get_main_wnd());

    int dx = curPos.x - center.x;
    int dy = curPos.y - center.y;

    raw_state_pnt.mouse.deltaX += dx;
    raw_state_pnt.mouse.deltaY += dy;
    state.mouse.deltaX = dx;
    state.mouse.deltaY = dy;

    raw_state_pnt.mouse.x = state.mouse.x = curPos.x;
    raw_state_pnt.mouse.y = state.mouse.y = curPos.y;

    if (client && (dx || dy))
      client->gmcMouseMove(this, dx, dy);

    ::mouse_api_SetCursorPosRel(win32_get_main_wnd(), &center);
  }

#endif
}

void WinMouseDevice::fetchLastState()
{
  // First of all coordinates are important, the rest can be added later if needed
  POINT pt;
  RECT r;
  overWnd = getCursorPos(pt, &r);
  state.mouse.x = pt.x;
  state.mouse.y = pt.y;
}


IWndProcComponent::RetCode WinMouseDevice::process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result)
{
#if !_TARGET_PC_WIN
  G_UNREFERENCED(hwnd);
  G_UNREFERENCED(result);
  G_UNREFERENCED(lParam);
#endif

#if _TARGET_PC_WIN
  if (relativeModeNextTime)
  {
    relativeModeNextTime = false;
    setRelativeMovementMode(true);
  }
#endif


#if _TARGET_PC_WIN
  if (msg == WM_INPUT || msg == WM_DAGOR_INPUT)
  {
    if (!dgs_inpdev_rawinput_mouse_inited)
      return PROCEED_OTHER_COMPONENTS;

    RAWINPUT ri{};
    if (msg == WM_INPUT)
    {
      UINT dwSize = sizeof(ri);
      // ri.data.mouse = (0, 0) if left-upper point of screen (NOT WINDOW) and (65536, 65536) if it's right-bottom
      if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &ri, &dwSize, sizeof(ri.header)) == (UINT)-1)
      {
        DEBUG_TRACE_INPDEV("WM:mouse: GetRawInputData failed, dwSize=%d", dwSize);
        return PROCEED_OTHER_COMPONENTS;
      }
    }
    else if (auto pri = windows::get_rid(); pri)
      ri = *pri;
    else
      return PROCEED_OTHER_COMPONENTS;

    if (ri.header.dwType != RIM_TYPEMOUSE)
    {
      DEBUG_TRACE_INPDEV("WM:mouse: skip type %d", ri.header.dwType);
      return PROCEED_OTHER_COMPONENTS;
    }
    if (!ri.header.hDevice && touchscreen_present && stg_pnt.touchScreen)
    {
      DEBUG_TRACE_INPDEV("WM:mouse: skip touch screen event", ri.header.dwType);
      return PROCEED_OTHER_COMPONENTS;
    }

    if ((ri.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE)
    {
      static const float vscr_sclx = GetSystemMetrics(SM_CXVIRTUALSCREEN) / 65535.0f;
      static const float vscr_scly = GetSystemMetrics(SM_CYVIRTUALSCREEN) / 65535.0f;
      static int last_abs_x = -1, last_abs_y = -1;
      int abs_x = ri.data.mouse.lLastX, abs_y = ri.data.mouse.lLastY;

      if (last_abs_x < 0)
        ri.data.mouse.lLastX = ri.data.mouse.lLastY = 0;
      else
        ri.data.mouse.lLastX = abs_x - last_abs_x, ri.data.mouse.lLastY = abs_y - last_abs_y;
      if ((ri.data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) == MOUSE_VIRTUAL_DESKTOP)
        ri.data.mouse.lLastX = ri.data.mouse.lLastX * vscr_sclx, ri.data.mouse.lLastY = ri.data.mouse.lLastY * vscr_scly;

      last_abs_x = abs_x, last_abs_y = abs_y;
    }

    DEBUG_TRACE_INPDEV("WM:mouse: mouse.usFlags=0x%x (%d,%d) %X %p", ri.data.mouse.usFlags, ri.data.mouse.lLastX, ri.data.mouse.lLastY,
      ri.data.mouse.usButtonFlags, client);
    if (stg_pnt.emuTouchScreenWithMouse)
    {
      POINT pt;
      RECT r;
      overWnd = getCursorPos(pt, &r);
      raw_state_pnt.mouse.x = state.mouse.x = pt.x;
      raw_state_pnt.mouse.y = state.mouse.y = pt.y;

      static bool touch_started[2] = {false, false};
      if ((ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) && !touch_started[0])
      {
        hid::gpcm_TouchBegan(state, this, client, pt.x, pt.y, 1, PointingRawState::Touch::TS_emuTouchScreen);
        touch_started[0] = true;
      }
      else if ((ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP) && touch_started[0])
      {
        hid::gpcm_TouchEnded(state, this, client, pt.x, pt.y, 1);
        touch_started[0] = false;
      }
      else if ((ri.data.mouse.lLastX || ri.data.mouse.lLastY) && touch_started[0])
      {
        hid::gpcm_TouchMoved(state, this, client, pt.x, pt.y, 1);
      }

      if ((ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) && !touch_started[1])
      {
        hid::gpcm_TouchBegan(state, this, client, pt.x, pt.y, 2, PointingRawState::Touch::TS_emuTouchScreen);
        touch_started[1] = true;
      }
      else if ((ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP) && touch_started[1])
      {
        hid::gpcm_TouchEnded(state, this, client, pt.x, pt.y, 2, true);
        touch_started[1] = false;
      }

      return PROCEED_DEF_WND_PROC;
    }

    if (ri.data.mouse.lLastX || ri.data.mouse.lLastY)
    {
      if (!stg_pnt.touchScreen || state.mouse.buttons)
      {
        raw_state_pnt.mouse.deltaX += ri.data.mouse.lLastX;
        raw_state_pnt.mouse.deltaY += ri.data.mouse.lLastY;
        lbmMotionDx += ri.data.mouse.lLastX;
        lbmMotionDy += ri.data.mouse.lLastY;
        lbmMotion = max(lbmMotion, max(abs(lbmMotionDx), abs(lbmMotionDy)));
      }

      POINT pt;
      RECT r;
      overWnd = getCursorPos(pt, &r);
      DEBUG_TRACE_INPDEV("mousePos: %d %d %d", pt.x, pt.y, overWnd);
      if (relMode && wrapCursorPos(pt, rectFromClip(clip)))
        deviceSetPosition(pt.x, pt.y);

      state.mouse.x = pt.x;
      state.mouse.y = pt.y;
      if (overWnd)
        clampStateCoord();

      raw_state_pnt.mouse.x = state.mouse.x;
      raw_state_pnt.mouse.y = state.mouse.y;

      if (client)
        client->gmcMouseMove(this, ri.data.mouse.lLastX, ri.data.mouse.lLastY);
    }

    int mouseMainButtonDown = areButtonsSwapped ? RI_MOUSE_BUTTON_2_DOWN : RI_MOUSE_BUTTON_1_DOWN;
    int mouseMainButtonUp = areButtonsSwapped ? RI_MOUSE_BUTTON_2_UP : RI_MOUSE_BUTTON_1_UP;
    int mouseSecondaryButtonDown = areButtonsSwapped ? RI_MOUSE_BUTTON_1_DOWN : RI_MOUSE_BUTTON_2_DOWN;
    int mouseSecondaryButtonUp = areButtonsSwapped ? RI_MOUSE_BUTTON_1_UP : RI_MOUSE_BUTTON_2_UP;

    if (ri.data.mouse.usButtonFlags)
    {
      if (ri.data.mouse.usButtonFlags & mouseSecondaryButtonDown)
        state.mouse.buttons |= 2;
      else if (ri.data.mouse.usButtonFlags & mouseSecondaryButtonUp)
        state.mouse.buttons &= ~2;

      if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN)
        state.mouse.buttons |= 4;
      else if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP)
        state.mouse.buttons &= ~4;

      if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN)
        state.mouse.buttons |= 8;
      else if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP)
        state.mouse.buttons &= ~8;

      if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN)
        state.mouse.buttons |= 16;
      else if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP)
        state.mouse.buttons &= ~16;

      if (ri.data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
      {
        state.mouse.deltaZ += (short)ri.data.mouse.usButtonData;
        raw_state_pnt.mouse.deltaZ += (short)ri.data.mouse.usButtonData;
      }

      if (!stg_pnt.touchScreen)
      {
        if (ri.data.mouse.usButtonFlags & mouseMainButtonDown)
          state.mouse.buttons |= 1;
        else if (ri.data.mouse.usButtonFlags & mouseMainButtonUp)
          state.mouse.buttons &= ~1;
      }

      raw_state_pnt.mouse.buttons = state.mouse.buttons;

      if (client)
      {
        if ((ri.data.mouse.usButtonFlags & mouseMainButtonDown) && overWnd)
          client->gmcMouseButtonDown(this, 0);
        else if (ri.data.mouse.usButtonFlags & mouseMainButtonUp)
          client->gmcMouseButtonUp(this, 0);

        if ((ri.data.mouse.usButtonFlags & mouseSecondaryButtonDown) && overWnd)
          client->gmcMouseButtonDown(this, 1);
        else if (ri.data.mouse.usButtonFlags & mouseSecondaryButtonUp)
          client->gmcMouseButtonUp(this, 1);

        if ((ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) && overWnd)
          client->gmcMouseButtonDown(this, 2);
        else if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP)
          client->gmcMouseButtonUp(this, 2);

        if ((ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) && overWnd)
          client->gmcMouseButtonDown(this, 3);
        else if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP)
          client->gmcMouseButtonUp(this, 3);

        if ((ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) && overWnd)
          client->gmcMouseButtonDown(this, 4);
        else if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP)
          client->gmcMouseButtonUp(this, 4);

        if (ri.data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
        {
          if (overWnd)
            client->gmcMouseWheel(this, (short)ri.data.mouse.usButtonData);

          if (((short)ri.data.mouse.usButtonData > 0) && overWnd)
          {
            client->gmcMouseButtonDown(this, 5);
            client->gmcMouseButtonUp(this, 5);
          }
          else if (((short)ri.data.mouse.usButtonData < 0) && overWnd)
          {
            client->gmcMouseButtonDown(this, 6);
            client->gmcMouseButtonUp(this, 6);
          }
        }
      }
    }
    return PROCEED_DEF_WND_PROC;
  }
  else if (msg == WM_TOUCH && stg_pnt.touchScreen)
  {
    static constexpr int MAX_TOUCHES = 32;
    int cInputs = LOWORD(wParam);
    static TOUCHINPUT pInputs[MAX_TOUCHES];
    if (cInputs > MAX_TOUCHES)
    {
      logwarn("Touches %d overflows the limit %d", cInputs, MAX_TOUCHES);
      cInputs = MAX_TOUCHES;
    }
    if (GetTouchInputInfo((HTOUCHINPUT)lParam, cInputs, pInputs, sizeof(TOUCHINPUT)))
      for (int i = 0; i < cInputs; i++)
      {
        intptr_t touch_id = pInputs[i].dwID + 1;
        POINT pt = {pInputs[i].x / 100, pInputs[i].y / 100};
        ::ScreenToClient((HWND)win32_get_main_wnd(), &pt);

        float mx = pt.x + (pInputs[i].x % 100) / 100.f, my = pt.y + (pInputs[i].y % 100) / 100.f;
        if (pInputs[i].dwFlags & TOUCHEVENTF_MOVE)
          hid::gpcm_TouchMoved(state, this, client, mx, my, touch_id);
        else if (pInputs[i].dwFlags & TOUCHEVENTF_DOWN)
          hid::gpcm_TouchBegan(state, this, client, mx, my, touch_id, PointingRawState::Touch::TS_touchScreen);
        else if (pInputs[i].dwFlags & TOUCHEVENTF_UP)
          hid::gpcm_TouchEnded(state, this, client, mx, my, touch_id);
      }

    CloseTouchInputHandle((HTOUCHINPUT)lParam);
    result = 0;
    return IMMEDIATE_RETURN;
  }
  else if (msg == WM_KILLFOCUS)
  {
    if (stg_pnt.emuTouchScreenWithMouse)
    {
      // pt is relative pos of mouse to left-top corner of window
      POINT pt;
      RECT r;
      overWnd = getCursorPos(pt, &r);
      raw_state_pnt.mouse.x = state.mouse.x = pt.x;
      raw_state_pnt.mouse.y = state.mouse.y = pt.y;
      hid::gpcm_TouchEnded(state, this, client, pt.x, pt.y, 1);
    }
    return PROCEED_OTHER_COMPONENTS;
  }
  else if (msg == WM_SETFOCUS)
  {
    return PROCEED_OTHER_COMPONENTS;
  }
  else if (msg == WM_ACTIVATE)
  {
    int fActive = LOWORD(wParam);

    if (fActive == WA_ACTIVE || fActive == WA_CLICKACTIVE)
      updateCursorClipping();
    else
      releaseCursorClipping();
  }
  else if (!dgs_inpdev_rawinput_mouse_inited)
  {
    if (touchscreen_present && stg_pnt.touchScreen)
      if ((GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) == MOUSEEVENTF_FROMTOUCH)
        return PROCEED_OTHER_COMPONENTS;

    if (msg == WM_MOUSEMOVE)
    {
      if (relMode)
        return PROCEED_DEF_WND_PROC;

      int x = (short)LOWORD(lParam);
      int y = (short)HIWORD(lParam);
      int dx = x - state.mouse.x;
      int dy = y - state.mouse.y;

      raw_state_pnt.mouse.deltaX += dx;
      raw_state_pnt.mouse.deltaY += dy;
      state.mouse.deltaX = dx;
      state.mouse.deltaY = dy;

      POINT pt;
      RECT r;
      overWnd = getCursorPos(pt, &r);
      if (relMode && wrapCursorPos(pt, rectFromClip(clip), 2, 20))
        deviceSetPosition(x = pt.x, y = pt.y);

      raw_state_pnt.mouse.x = state.mouse.x = x;
      raw_state_pnt.mouse.y = state.mouse.y = y;

      clampStateCoord();

      if (client && (dx || dy))
        client->gmcMouseMove(this, dx, dy);

      return PROCEED_DEF_WND_PROC;
    }
    if (msg == WM_LBUTTONDOWN)
    {
      onButtonDownWin(0, wParam, lParam);
      return PROCEED_DEF_WND_PROC;
    }
    if (msg == WM_RBUTTONDOWN)
    {
      onButtonDownWin(1, wParam, lParam);
      return PROCEED_DEF_WND_PROC;
    }
    if (msg == WM_MBUTTONDOWN)
    {
      onButtonDownWin(2, wParam, lParam);
      return PROCEED_DEF_WND_PROC;
    }
    if (msg == WM_LBUTTONUP)
    {
      onButtonUpWin(0, wParam, lParam);
      return PROCEED_DEF_WND_PROC;
    }
    if (msg == WM_RBUTTONUP)
    {
      onButtonUpWin(1, wParam, lParam);
      return PROCEED_DEF_WND_PROC;
    }
    if (msg == WM_MBUTTONUP)
    {
      onButtonUpWin(2, wParam, lParam);
      return PROCEED_DEF_WND_PROC;
    }
    if (msg == WM_MOUSEWHEEL)
    {
      raw_state_pnt.mouse.deltaZ += short(HIWORD(wParam)) * stg_pnt.zSens;
      if (client)
        client->gmcMouseWheel(this, short(HIWORD(wParam)));
      overWnd = true;
      return PROCEED_DEF_WND_PROC;
    }
  }
#else
  if (msg == GPCM_MouseMove || msg == GPCM_MouseBtnPress || msg == GPCM_MouseBtnRelease || msg == GPCM_MouseWheel)
  {
    int dx = 0, dy = 0;
#if _TARGET_PC_MACOSX
    if (ignoreNextDelta)
    {
      mouse_api_get_deltas(dx, dy);
      ignoreNextDelta = false;
    }
#endif
    mouse_api_get_deltas(dx, dy);

    if (msg == GPCM_MouseMove || dx || dy)
    {
      raw_state_pnt.mouse.deltaX += dx;
      raw_state_pnt.mouse.deltaY += dy;

      POINT pt;
      RECT r;
      bool curOverWnd = getCursorPos(pt, &r);
      r = rectFromClip(clip);

#if _TARGET_PC_LINUX
      r.left += linux_wrap_edge_distance;
      r.top += linux_wrap_edge_distance;
      r.right -= linux_wrap_edge_distance;
      r.bottom -= linux_wrap_edge_distance;
#endif

      if (relMode && wrapCursorPos(pt, r))
        deviceSetPosition(pt.x, pt.y);
      else
        overWnd = curOverWnd;

      state.mouse.x = pt.x;
      state.mouse.y = pt.y;
      if (overWnd)
        clampStateCoord();
      raw_state_pnt.mouse.x = state.mouse.x;
      raw_state_pnt.mouse.y = state.mouse.y;

      if (client)
        client->gmcMouseMove(this, dx, dy);
    }

    if (msg == GPCM_MouseBtnPress)
      state.mouse.buttons |= 1 << wParam;
    else if (msg == GPCM_MouseBtnRelease)
      state.mouse.buttons &= ~(1 << wParam);

    if (msg == GPCM_MouseWheel)
    {
      state.mouse.deltaZ += (short)wParam;
      raw_state_pnt.mouse.deltaZ += (short)wParam;
      accDz += wParam;
    }
    raw_state_pnt.mouse.buttons = state.mouse.buttons;

    if (client)
    {
      if (msg == GPCM_MouseBtnPress && overWnd)
        client->gmcMouseButtonDown(this, wParam);
      else if (msg == GPCM_MouseBtnRelease)
        client->gmcMouseButtonUp(this, wParam);
      else if (msg == GPCM_MouseWheel && (accDz >= 0 ? accDz : -accDz) >= 10 && overWnd)
      {
        client->gmcMouseWheel(this, accDz);
        accDz = 0;
      }
    }
    else if (!client && msg == GPCM_MouseWheel && (accDz >= 0 ? accDz : -accDz) >= 10)
      accDz = 0;
  }
  else if (msg == GPCM_Activate)
  {
    if (wParam == GPCMP1_Activate || wParam == GPCMP1_ClickActivate)
    {
      if (relMode)
        mouse_api_ClipCursorToAppWindow();
      mouse_api_hide_cursor(hidden);
    }
    else
    {
      mouse_api_UnclipCursor();
      mouse_api_hide_cursor(false);
    }
  }
#endif

  return PROCEED_OTHER_COMPONENTS;
}

#if _TARGET_PC_WIN

void WinMouseDevice::updateState(int wParam, int lParam)
{
  state.mouse.buttons = 0;
  if (wParam & MK_LBUTTON)
    state.mouse.buttons |= PointingRawState::Mouse::LBUTTON_BIT;
  if (wParam & MK_RBUTTON)
    state.mouse.buttons |= PointingRawState::Mouse::RBUTTON_BIT;
  if (wParam & MK_MBUTTON)
    state.mouse.buttons |= PointingRawState::Mouse::MBUTTON_BIT;
  state.mouse.x = (short)LOWORD(lParam);
  state.mouse.y = (short)HIWORD(lParam);

  raw_state_pnt.mouse.x = state.mouse.x;
  raw_state_pnt.mouse.y = state.mouse.y;
  raw_state_pnt.mouse.buttons = state.mouse.buttons;
}

void WinMouseDevice::onButtonDownWin(int btn_id, int wParam, int lParam)
{
  updateState(wParam, lParam);
  if (client)
    client->gmcMouseButtonDown(this, btn_id);
  setMouseCapture((HWND)win32_get_main_wnd());
  // Cursor clipping is shared between all processes in os
  overWnd = true;
  updateCursorClipping();
}

void WinMouseDevice::onButtonUpWin(int btn_id, int wParam, int lParam)
{
  updateState(wParam, lParam);
  if (client)
    client->gmcMouseButtonUp(this, btn_id);
  releaseMouseCapture();
}

#endif


void WinMouseDevice::setRelativeMovementMode(bool enable)
{
#if _TARGET_PC_WIN
  if (!enable)
    relativeModeNextTime = false;
  bool canClip = canClipCursor();
  if (enable && !canClip)
  {
    relativeModeNextTime = true;
    enable = false;
  }
#endif

  relMode = enable;
#if !_TARGET_PC_MACOSX
  if (relMode)
  {
    POINT center;
    center.x = (clip.r - clip.l) / 2;
    center.y = (clip.b - clip.t) / 2;
    mouse_api_SetCursorPosRel(win32_get_main_wnd(), &center);
  }
#endif
#if _TARGET_PC_WIN

  updateCursorClipping();

#else

  // On Linux and Mac this is not a clipping but a relative mode (Mac) or window focus (Linux)
  if (relMode)
    mouse_api_ClipCursorToAppWindow();
  else
    mouse_api_UnclipCursor();

#endif
}


void WinMouseDevice::setPosition(int x, int y)
{
  state.mouse.x = x;
  state.mouse.y = y;
  clampStateCoord();
  raw_state_pnt.mouse.x = state.mouse.x;
  raw_state_pnt.mouse.y = state.mouse.y;
  deviceSetPosition(state.mouse.x, state.mouse.y);

#if _TARGET_PC_MACOSX
  ignoreNextDelta = true;
#endif
  if (client)
    client->gmcMouseMove(this, 0, 0);
}

void WinMouseDevice::setClipRect(int l, int t, int r, int b)
{
  GenericMouseDevice::setClipRect(l, t, r, b);

#if _TARGET_PC_WIN
  // FIXME: this is acutally called to notify about the resolution change.
  // Probably we need it more explicit.
  updateCursorClipping();
#endif
}


void WinMouseDevice::setAlwaysClipToWindow(bool on)
{
  shouldAlwaysClipTopWindows = on;

#if _TARGET_PC_WIN
  updateCursorClipping();
#endif
}


bool WinMouseDevice::isPointerOverWindow()
{
  return overWnd || (relMode && (dgs_inpdev_rawinput_mouse_inited || !stg_pnt.touchScreen));
}


void WinMouseDevice::deviceSetPosition(int x, int y)
{
  POINT pt;
  pt.x = x;
  pt.y = y;
#if _TARGET_PC_WIN || _TARGET_PC_LINUX
  RECT r;
  ::mouse_api_GetClientRect(win32_get_main_wnd(), &r);
  int w = 1, h = 1;
  d3d::get_screen_size(w, h);
  if (w && h)
  {
    pt.x = pt.x * max<int>(r.right - r.left, 1) / w;
    pt.y = pt.y * max<int>(r.bottom - r.top, 1) / h;
  }
#endif
  bool prev_over = overWnd;
  ::mouse_api_SetCursorPosRel(win32_get_main_wnd(), &pt);
  overWnd = getCursorPos(pt);
  if (overWnd != prev_over)
    ::mouse_api_hide_cursor(overWnd);
  raw_state_pnt.mouse.x = state.mouse.x = pt.x;
  raw_state_pnt.mouse.y = state.mouse.y = pt.y;
}


#if _TARGET_PC_WIN
void WinMouseDevice::updateCursorClipping()
{
  bool windowIsActive = ((HWND)win32_get_main_wnd() == GetForegroundWindow());

  if (windowIsActive)
  {
    if (relMode)
    {
      RECT r;
      mouse_api_GetWindowRect(win32_get_main_wnd(), &r);
      r.left = (r.left + r.right) / 2;
      r.top = (r.top + r.bottom) / 2;
      r.right = r.left + 2;
      r.bottom = r.top + 2;
      mouse_api_ClipCursorToRect(r);
      cursorClipPerformed = true;
    }
    else if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE || shouldAlwaysClipTopWindows)
    {
      mouse_api_ClipCursorToAppWindow();
      cursorClipPerformed = true;
    }
    else if (cursorClipPerformed)
      releaseCursorClipping();
  }
  else
  {
    if (cursorClipPerformed)
      releaseCursorClipping();
  }
}

void WinMouseDevice::releaseCursorClipping()
{
  if (cursorClipPerformed)
  {
    mouse_api_UnclipCursor();
    cursorClipPerformed = false;
  }
}

#endif
