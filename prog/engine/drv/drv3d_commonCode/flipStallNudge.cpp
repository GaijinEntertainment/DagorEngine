// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "flipStallNudge.h"

#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_threads.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <generic/dag_align.h>
#include <EASTL/algorithm.h>
#include <EASTL/unique_ptr.h>

#include <debug/dag_log.h>

namespace
{

constexpr int ACTIVATION_WINDOW_USEC = 1000000;
constexpr int SLOW_DETECT_USEC = 300000;
constexpr int SLOW_DETECT_FRAMES = 10;
constexpr int MIN_DETECT_USEC = 4000;
constexpr int MAX_DETECT_USEC = 50000;
constexpr int MAX_FRAME_SAMPLE_USEC = 250000;
constexpr int HOLD_USEC = 1000000;
constexpr UINT HEARTBEAT_PERIOD_MS = 10;
constexpr DWORD PUMP_WAKE_MS = 100;

constexpr UINT MSG_SHOW = WM_APP + 1;
constexpr UINT MSG_HIDE = WM_APP + 2;
constexpr UINT_PTR HIDE_TIMER_ID = 1;
constexpr UINT_PTR HEARTBEAT_TIMER_ID = 2;

class NudgeThread final : public DaThread
{
  HWND window;
  HANDLE readyEvent;
  bool heartbeat;

public:
  explicit NudgeThread(bool heartbeat) :
    DaThread{"FlipNudge"}, window{nullptr}, readyEvent{CreateEventW(nullptr, TRUE, FALSE, nullptr)}, heartbeat{heartbeat}
  {}
  ~NudgeThread() override { cleanup(); }

  void execute() override;

  bool awaitStartup();
  void cleanup();

  bool initialized() const { return readyEvent != nullptr; }
  HWND getNudgeWindow() const { return window; }
};

struct NudgeSetup
{
  bool enabled = false;
  bool heartbeat = false;
  HWND window = nullptr;
  eastl::unique_ptr<NudgeThread> thread;
};

struct NudgeState
{
  HWND gameWindow = nullptr;
  bool recoveryLogged = false;
  bool stalledSinceFrame = false;
  bool wasActive = false;
  bool lastFrameActive = false;
  int64_t activatedAt = 0;
  int64_t lastFrameAt = 0;
  int64_t shownAt = 0;
  int64_t holdUntil = 0;
  float frameUsec = 16000.f;
};

NudgeSetup setup;
NudgeState nudge;
WinCritSec nudge_cs;

int64_t usec_since(int64_t ticks) { return ref_time_delta_to_usec(ref_time_ticks() - ticks); }

bool in_activation_window() { return usec_since(nudge.activatedAt) < ACTIVATION_WINDOW_USEC; }

int get_fast_detect_usec() { return eastl::clamp(int(nudge.frameUsec * 2), MIN_DETECT_USEC, MAX_DETECT_USEC); }
int get_slow_detect_usec() { return eastl::max(int(nudge.frameUsec * SLOW_DETECT_FRAMES), SLOW_DETECT_USEC); }

DWORD get_detect_ms(bool active, DWORD timeout_ms)
{
  if (!active)
    return timeout_ms;
  int detectUsec = get_slow_detect_usec();
  if (in_activation_window())
    detectUsec = get_fast_detect_usec();
  return dag::divide_align_up(detectUsec, 1000);
}

void track_activation(bool active, int64_t now)
{
  if (active && !nudge.wasActive)
    nudge.activatedAt = now;
  nudge.wasActive = active;
}

void show_nudge(DWORD stalled_ms, const char *what)
{
  if (!nudge.gameWindow || WindowMode::FULLSCREEN_EXCLUSIVE == dgs_get_window_mode())
    return;
  POINT origin = {0, 0};
  if (!ClientToScreen(nudge.gameWindow, &origin))
    return;
  const int64_t now = ref_time_ticks();
  if (!nudge.holdUntil || now >= nudge.holdUntil)
  {
    nudge.shownAt = now;
    nudge.recoveryLogged = false;
    logwarn("flip nudge: %s stalled for %u ms, showing the nudge window at %d,%d", what, stalled_ms, origin.x, origin.y);
  }
  nudge.holdUntil = rel_ref_time_ticks(now, HOLD_USEC);
  nudge.stalledSinceFrame = true;
  PostMessageW(setup.window, MSG_SHOW, WPARAM(origin.x), LPARAM(origin.y));
}

void log_recovery(const char *what)
{
  if (!nudge.holdUntil || nudge.recoveryLogged)
    return;
  logwarn("flip nudge: %s resumed %d us after showing the nudge window", what, int(usec_since(nudge.shownAt)));
  nudge.recoveryLogged = true;
}

void check_heartbeat()
{
  WinAutoLock lock{nudge_cs};
  const bool active = interlocked_relaxed_load(::dgs_app_active);
  if (!nudge.lastFrameAt || !active || !in_activation_window())
    return;
  const int64_t sinceFrameUsec = usec_since(nudge.lastFrameAt);
  if (sinceFrameUsec > get_fast_detect_usec())
    show_nudge(DWORD(sinceFrameUsec / 1000), "frame heartbeat");
}

LRESULT CALLBACK nudge_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg)
  {
    case MSG_SHOW:
      SetWindowPos(hwnd, HWND_TOPMOST, int(wparam), int(lparam), 1, 1, SWP_NOACTIVATE | SWP_SHOWWINDOW);
      SetTimer(hwnd, HIDE_TIMER_ID, HOLD_USEC / 1000, nullptr);
      return 0;
    case WM_TIMER:
      if (HEARTBEAT_TIMER_ID == wparam)
      {
        check_heartbeat();
        return 0;
      }
      [[fallthrough]];
    case MSG_HIDE:
      KillTimer(hwnd, HIDE_TIMER_ID);
      if (IsWindowVisible(hwnd))
      {
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);
        logdbg("flip nudge: hiding the nudge window");
      }
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void NudgeThread::execute()
{
  WNDCLASSEXW wc = {sizeof(wc)};
  wc.lpfnWndProc = nudge_wnd_proc;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.hbrBackground = HBRUSH(GetStockObject(BLACK_BRUSH));
  wc.lpszClassName = L"dagorFlipNudge";
  RegisterClassExW(&wc);
  window = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, wc.lpszClassName, L"", WS_POPUP, 0, 0, 1, 1, nullptr,
    nullptr, wc.hInstance, nullptr);
  SetEvent(readyEvent);
  if (window)
  {
    if (heartbeat)
      SetTimer(window, HEARTBEAT_TIMER_ID, HEARTBEAT_PERIOD_MS, nullptr);
    while (!isThreadTerminating())
    {
      MsgWaitForMultipleObjects(0, nullptr, FALSE, PUMP_WAKE_MS, QS_ALLINPUT);
      MSG msg;
      while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        DispatchMessageW(&msg);
    }
    DestroyWindow(window);
  }
  UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

bool NudgeThread::awaitStartup()
{
  if (initialized())
    return WAIT_OBJECT_0 == WaitForSingleObject(readyEvent, INFINITE);
  return false;
}

void NudgeThread::cleanup()
{
  if (readyEvent)
    CloseHandle(eastl::exchange(readyEvent, nullptr));
}

} // namespace

void init_flip_nudge()
{
  if (setup.thread)
    return;
  const DataBlock *directx = dgs_get_settings() ? dgs_get_settings()->getBlockByNameEx("directx") : nullptr;
  setup.enabled = !directx || directx->getBool("flipStallNudge", true);
  setup.heartbeat = setup.enabled && (!directx || directx->getBool("flipStallNudgeHeartbeat", true));
  if (!setup.enabled)
  {
    logdbg("flip nudge: disabled by directx/flipStallNudge");
    return;
  }
  if (!setup.heartbeat)
    logdbg("flip nudge: frame heartbeat disabled by directx/flipStallNudgeHeartbeat");

  auto thread = eastl::make_unique<NudgeThread>(setup.heartbeat);
  if (!thread->initialized())
  {
    logwarn("flip nudge: failed to init the nudge window thread");
    setup.enabled = false;
    return;
  }
  if (!thread->start())
  {
    logwarn("flip nudge: failed to start the nudge window thread");
    setup.enabled = false;
    return;
  }
  if (!thread->awaitStartup() || !thread->getNudgeWindow())
  {
    logwarn("flip nudge: failed to create the nudge window");
    thread->terminate(true);
    setup.enabled = false;
    return;
  }
  setup.window = thread->getNudgeWindow();
  setup.thread = eastl::move(thread);
}

void flip_nudge_on_frame(HWND window)
{
  if (!setup.enabled)
    return;
  WinAutoLock lock{nudge_cs};
  nudge.gameWindow = window;
  const bool active = interlocked_relaxed_load(::dgs_app_active);
  const int64_t now = ref_time_ticks();
  track_activation(active, now);
  if (active && nudge.lastFrameActive && !nudge.stalledSinceFrame)
  {
    const int64_t sinceLastFrame = eastl::min<int64_t>(ref_time_delta_to_usec(now - nudge.lastFrameAt), MAX_FRAME_SAMPLE_USEC);
    if (sinceLastFrame > 0)
      nudge.frameUsec += (sinceLastFrame - nudge.frameUsec) * 0.1f;
  }
  if (nudge.stalledSinceFrame)
    log_recovery("frame");
  nudge.stalledSinceFrame = false;
  nudge.lastFrameActive = active;
  nudge.lastFrameAt = now;
  if (!active && nudge.holdUntil)
  {
    PostMessageW(setup.window, MSG_HIDE, 0, 0);
    nudge.holdUntil = 0;
  }
}

DWORD wait_with_flip_nudge(HANDLE handle, DWORD timeout_ms, const char *what)
{
  if (!setup.enabled)
    return WaitForSingleObjectEx(handle, timeout_ms, FALSE);

  const bool active = interlocked_relaxed_load(::dgs_app_active);
  DWORD detectMs;
  {
    WinAutoLock lock{nudge_cs};
    track_activation(active, ref_time_ticks());
    detectMs = get_detect_ms(active, timeout_ms);
  }

  DWORD result = WaitForSingleObjectEx(handle, eastl::min(detectMs, timeout_ms), FALSE);

  // If frames before the timeout are slow (loading or generally very slow system)
  // and we are not in the activation window, we just do the regular timed-out wait
  // so as not to ddos the user with the window switch
  if (WAIT_TIMEOUT == result && active && detectMs <= timeout_ms)
  {
    {
      WinAutoLock lock{nudge_cs};
      show_nudge(detectMs, what);
    }
    if (detectMs < timeout_ms)
      result = WaitForSingleObjectEx(handle, timeout_ms - detectMs, FALSE);
  }
  if (WAIT_OBJECT_0 == result)
  {
    WinAutoLock lock{nudge_cs};
    log_recovery(what);
  }
  return result;
}

void shutdown_flip_nudge()
{
  if (setup.thread)
  {
    PostMessageW(setup.window, WM_NULL, 0, 0);
    setup.thread->terminate(true);
  }
  setup = NudgeSetup{};
  WinAutoLock lock{nudge_cs};
  nudge = NudgeState{};
}
