// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <colorPipette/colorPipette.h>

#if _TARGET_PC_WIN && DAGOR_DBGLEVEL > 0

#include <atomic>
#include <osApiWrappers/dag_threads.h>
#include <util/dag_delayedAction.h>
#include <EASTL/unique_ptr.h>
#include <debug/dag_debug.h>
#include <stdio.h> // for snprinf

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


#define WINDOW_WIDTH    800
#define WINDOW_HEIGHT   800
#define PIXEL_GRID_SIZE 7
#define PIXEL_CELL_SIZE 20

static HWND colorpicker_hwnd;
static const COLORREF transparent_color = RGB(0xFE, 0x01, 0xFD); // kind of rare color
static int last_x = -1;
static int last_y = -1;
static bool picked_ok = false;
static bool termination_requested = false;
static bool wnd_class_registred = false;
static WNDCLASSEXA wnd_class;
static colorpipette::ColorPipetteCallback callback = nullptr;
static void *user_data = nullptr;

static LRESULT CALLBACK mouse_hook_proc(int nCode, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK keyboard_hook_proc(int nCode, WPARAM wParam, LPARAM lParam);
static void on_pick(bool picked, int r, int g, int b);


class PipetteThread final : public DaThread
{
public:
  PipetteThread() : DaThread("PipetteThread", DEFAULT_STACK_SZ, 0, WORKER_THREADS_AFFINITY_MASK) {}

  void execute() override
  {
    termination_requested = false;
    picked_ok = false;
    last_x = -1;
    last_y = -1;

    colorpicker_hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_LAYERED, "ColorPickerClass", "Color picker transparent window", WS_POPUP,
      CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, 0, NULL);

    if (colorpicker_hwnd == NULL)
    {
      logwarn("Color Pipette: Window creation failed");
      on_pick(false, 0, 0, 0);
      return;
    }

    SetLayeredWindowAttributes(colorpicker_hwnd, transparent_color, 0, LWA_COLORKEY);

    ShowWindow(colorpicker_hwnd, SW_SHOW);
    UpdateWindow(colorpicker_hwnd);

    HHOOK mouse_hook = SetWindowsHookExA(WH_MOUSE_LL, mouse_hook_proc, NULL, 0);
    HHOOK keyboard_hook = SetWindowsHookExA(WH_KEYBOARD_LL, keyboard_hook_proc, NULL, 0);
    SetTimer(colorpicker_hwnd, 1, 80, NULL);

    MSG msg = {0};
    while (GetMessage(&msg, colorpicker_hwnd, 0, 0) > 0)
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);

      if (isThreadTerminating())
        termination_requested = true;
    }

    UnhookWindowsHookEx(keyboard_hook);
    UnhookWindowsHookEx(mouse_hook);

    if (!picked_ok)
      on_pick(false, 0, 0, 0);
  }
};


static eastl::unique_ptr<PipetteThread> pipette_thread = nullptr;

struct PickDelayedAction : public DelayedAction
{
  colorpipette::ColorPipetteCallback callback;
  void *userData;
  bool picked;
  int r, g, b;
  void performAction() override
  {
    // main thread
    if (callback)
      callback(userData, picked, r, g, b);

    if (pipette_thread.get() != nullptr)
    {
      pipette_thread->terminate(true, -1);
      pipette_thread.reset();
    }
  }
};


static void on_pick(bool picked, int r, int g, int b)
{
  // another thread
  PickDelayedAction *act = new PickDelayedAction();
  act->userData = user_data;
  act->callback = callback;
  act->picked = picked;
  act->r = r;
  act->g = g;
  act->b = b;
  add_delayed_action(act);
}


static void capture_screen_pixels(int x, int y, int size_x, int size_y, COLORREF *colors)
{
  HDC hdc = GetDC(NULL);
  HDC hdcMem = CreateCompatibleDC(hdc);
  HBITMAP hBitmap = CreateCompatibleBitmap(hdc, size_x, size_y);
  HBITMAP hBitmapOld = (HBITMAP)SelectObject(hdcMem, hBitmap);
  BitBlt(hdcMem, 0, 0, size_x, size_y, hdc, x, y, SRCCOPY);

  for (int ix = 0; ix < size_x; ix++)
    for (int iy = 0; iy < size_y; iy++)
      colors[ix + iy * size_x] = GetPixel(hdcMem, iy, ix);

  SelectObject(hdcMem, hBitmapOld);
  DeleteObject(hBitmap);
  DeleteDC(hdcMem);
  ReleaseDC(NULL, hdc);
}


static void check_cursor_near_sceen_border(POINT cursor_pos, int threshold_px, bool &near_right_border, bool &near_bottom_border)
{
  HMONITOR hMonitor = MonitorFromPoint(cursor_pos, MONITOR_DEFAULTTONEAREST);
  MONITORINFOEX monitor_info;
  monitor_info.cbSize = sizeof(MONITORINFOEX);
  GetMonitorInfo(hMonitor, &monitor_info);

  near_right_border = (monitor_info.rcWork.right - cursor_pos.x) < threshold_px;
  near_bottom_border = (monitor_info.rcWork.bottom - cursor_pos.y) < threshold_px;
}


static void update_window_position(bool force_redraw = false)
{
  if (termination_requested)
  {
    PostMessage(colorpicker_hwnd, WM_CLOSE, 0, 0);
    return;
  }

  static int last_call_msec = 0;
  int now_msec = get_time_msec();
  if (abs(now_msec - last_call_msec) < 15)
    return;
  last_call_msec = now_msec;

  POINT cursor_pos;
  GetCursorPos(&cursor_pos);
  if (cursor_pos.x != last_x || cursor_pos.y != last_y)
  {
    last_x = cursor_pos.x;
    last_y = cursor_pos.y;
    SetWindowPos(colorpicker_hwnd, NULL, cursor_pos.x - WINDOW_WIDTH / 2, cursor_pos.y - WINDOW_HEIGHT / 2, 0, 0,
      SWP_NOSIZE | SWP_NOZORDER);
    InvalidateRect(colorpicker_hwnd, NULL, TRUE);
  }
  else if (force_redraw)
  {
    InvalidateRect(colorpicker_hwnd, NULL, TRUE);
  }
}


static LRESULT CALLBACK colorpicker_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_ERASEBKGND: return 1;

    case WM_TIMER:
    case WM_ACTIVATE: update_window_position(true); break;

    case WM_PAINT:
    {
      COLORREF colors_around_cursor[PIXEL_GRID_SIZE][PIXEL_GRID_SIZE];

      POINT cursor_pos;
      GetCursorPos(&cursor_pos);
      capture_screen_pixels(cursor_pos.x - PIXEL_GRID_SIZE / 2, cursor_pos.y - PIXEL_GRID_SIZE / 2, PIXEL_GRID_SIZE, PIXEL_GRID_SIZE,
        (COLORREF *)colors_around_cursor);

      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      // fill with transparent color
      HBRUSH brush = CreateSolidBrush(transparent_color);
      RECT rect;
      GetClientRect(hwnd, &rect);
      FillRect(hdc, &rect, brush);
      DeleteObject(brush);


      bool is_cursor_near_right_border = false;
      bool is_cursor_near_bottom_border = false;
      check_cursor_near_sceen_border(cursor_pos, 260, is_cursor_near_right_border, is_cursor_near_bottom_border);

      int offsetX = is_cursor_near_right_border ? WINDOW_WIDTH / 2 - PIXEL_CELL_SIZE * PIXEL_GRID_SIZE - 40 : WINDOW_WIDTH / 2 + 40;
      int offsetY = is_cursor_near_bottom_border ? WINDOW_HEIGHT / 2 - PIXEL_CELL_SIZE * PIXEL_GRID_SIZE - 40 : WINDOW_HEIGHT / 2 + 40;

      for (int x = 0; x < PIXEL_GRID_SIZE; x++)
        for (int y = 0; y < PIXEL_GRID_SIZE; y++)
        {
          brush = CreateSolidBrush(colors_around_cursor[x][y]);
          RECT rect;
          rect.left = offsetX + x * PIXEL_CELL_SIZE;
          rect.top = offsetY + y * PIXEL_CELL_SIZE;
          rect.right = rect.left + PIXEL_CELL_SIZE;
          rect.bottom = rect.top + PIXEL_CELL_SIZE;
          FillRect(hdc, &rect, brush);
          DeleteObject(brush);
        }

      // draw grid using gray color for pen
      HPEN pen = CreatePen(PS_SOLID, 1, RGB(0x80, 0x80, 0x80));
      SelectObject(hdc, pen);
      for (int x = 0; x <= PIXEL_GRID_SIZE; x++)
        for (int y = 0; y <= PIXEL_GRID_SIZE; y++)
        {
          MoveToEx(hdc, offsetX + x * PIXEL_CELL_SIZE, offsetY, NULL);
          LineTo(hdc, offsetX + x * PIXEL_CELL_SIZE, offsetY + PIXEL_GRID_SIZE * PIXEL_CELL_SIZE);
          MoveToEx(hdc, offsetX, offsetY + y * PIXEL_CELL_SIZE, NULL);
          LineTo(hdc, offsetX + PIXEL_GRID_SIZE * PIXEL_CELL_SIZE, offsetY + y * PIXEL_CELL_SIZE);
        }
      DeleteObject(pen);

      // bold rect around center pixel
      pen = CreatePen(PS_SOLID, 3, RGB(0x80, 0x80, 0x80));
      SelectObject(hdc, pen);
      MoveToEx(hdc, offsetX + (PIXEL_GRID_SIZE / 2) * PIXEL_CELL_SIZE, offsetY + (PIXEL_GRID_SIZE / 2) * PIXEL_CELL_SIZE, NULL);
      LineTo(hdc, offsetX + (PIXEL_GRID_SIZE / 2) * PIXEL_CELL_SIZE, offsetY + (PIXEL_GRID_SIZE / 2 + 1) * PIXEL_CELL_SIZE);
      LineTo(hdc, offsetX + (PIXEL_GRID_SIZE / 2 + 1) * PIXEL_CELL_SIZE, offsetY + (PIXEL_GRID_SIZE / 2 + 1) * PIXEL_CELL_SIZE);
      LineTo(hdc, offsetX + (PIXEL_GRID_SIZE / 2 + 1) * PIXEL_CELL_SIZE, offsetY + (PIXEL_GRID_SIZE / 2) * PIXEL_CELL_SIZE);
      LineTo(hdc, offsetX + (PIXEL_GRID_SIZE / 2) * PIXEL_CELL_SIZE, offsetY + (PIXEL_GRID_SIZE / 2) * PIXEL_CELL_SIZE);
      DeleteObject(pen);

      // print color values of center pixel
      COLORREF color = colors_around_cursor[PIXEL_GRID_SIZE / 2][PIXEL_GRID_SIZE / 2];
      char buf[64] = {0};
      snprintf(buf, sizeof(buf), "R=%d G=%d B=%d", GetRValue(color), GetGValue(color), GetBValue(color));
      int len = strlen(buf);
      int textPosX = offsetX + 5;
      int textPosY = offsetY - 25;

      SetBkMode(hdc, TRANSPARENT);
      SetTextColor(hdc, RGB(0, 0, 0));
      for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
          if (x || y)
            TextOutA(hdc, textPosX + x, textPosY + y, buf, len);

      SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
      TextOutA(hdc, textPosX, textPosY, buf, len);

      EndPaint(hwnd, &ps);
    }
    break;

    case WM_DESTROY:
      PostQuitMessage(0);
      printf("Destroy window\n");
      break;

    default: return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}


static LRESULT CALLBACK mouse_hook_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
  if (nCode < 0)
    return CallNextHookEx(NULL, nCode, wParam, lParam);

  if (wParam == WM_MOUSEMOVE)
    update_window_position();

  if (wParam == WM_LBUTTONDBLCLK || wParam == WM_RBUTTONDBLCLK)
    return 1;

  if (wParam == WM_RBUTTONDOWN)
  {
    PostMessage(colorpicker_hwnd, WM_CLOSE, 0, 0);
    return 1;
  }
  else if (wParam == WM_LBUTTONDOWN)
  {
    POINT cursor_pos;
    GetCursorPos(&cursor_pos);
    COLORREF color;
    capture_screen_pixels(cursor_pos.x, cursor_pos.y, 1, 1, &color);
    picked_ok = true;
    on_pick(true, GetRValue(color), GetGValue(color), GetBValue(color));
    PostMessage(colorpicker_hwnd, WM_CLOSE, 0, 0);
    return 1;
  }

  return CallNextHookEx(NULL, nCode, wParam, lParam);
}


static LRESULT CALLBACK keyboard_hook_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
  if (nCode < 0)
    return CallNextHookEx(NULL, nCode, wParam, lParam);

  if (wParam == WM_KEYUP)
  {
    if (((KBDLLHOOKSTRUCT *)lParam)->vkCode == VK_ESCAPE)
    {
      PostMessage(colorpicker_hwnd, WM_CLOSE, 0, 0);
      return 1;
    }
  }

  if (wParam == WM_KEYDOWN)
    return 1;

  return CallNextHookEx(NULL, nCode, wParam, lParam);
}


namespace colorpipette
{
bool is_available_on_this_platform() { return true; }

bool start(ColorPipetteCallback callback_, void *user_data_)
{
  if (!is_available_on_this_platform() || is_active())
    return false;

  ::callback = callback_;
  ::user_data = user_data_;

  if (!wnd_class_registred)
  {
    wnd_class.cbSize = sizeof(WNDCLASSEX);
    wnd_class.style = 0;
    wnd_class.lpfnWndProc = colorpicker_wnd_proc;
    wnd_class.cbClsExtra = 0;
    wnd_class.cbWndExtra = 0;
    wnd_class.hInstance = 0;
    wnd_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    wnd_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wnd_class.lpszMenuName = NULL;
    wnd_class.lpszClassName = "ColorPickerClass";
    wnd_class.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExA(&wnd_class))
    {
      logwarn("Color Pipette: Window class registration failed");
      on_pick(false, 0, 0, 0);
      return false;
    }
    wnd_class_registred = true;
  }

  pipette_thread.reset(new PipetteThread());
  pipette_thread->start();

  return true;
}

bool is_active() { return pipette_thread.get() != nullptr; }

void terminate()
{
  if (pipette_thread.get() == nullptr)
    return;
  pipette_thread->terminate(true, -1);
  pipette_thread.reset();
}
} // namespace colorpipette

#else // not _TARGET_PC_WIN

namespace colorpipette
{
bool is_available_on_this_platform() { return false; }
bool start(ColorPipetteCallback, void *) { return false; }
bool is_active() { return false; }
void terminate() {}
} // namespace colorpipette

#endif
