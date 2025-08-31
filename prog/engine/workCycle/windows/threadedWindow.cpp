// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <workCycle/threadedWindow.h>
#include <osApiWrappers/setProgGlobals.h>
#include <generic/dag_initOnDemand.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_lowLatency.h>
#include <EASTL/vector.h>
#include <perfMon/dag_statDrv.h>
#include "../../drv/drv3d_commonCode/drv_utils.h"


#define AVOID_FALSE_SHARING char DAG_CONCAT(cacheline_pad, __LINE__)[std::hardware_constructive_interference_size] = {}

static_assert(WM_DAGOR_USER == WM_USER);

namespace workcycle_internal::windows
{
extern eastl::pair<bool, intptr_t> main_wnd_proc(void *, unsigned, uintptr_t, intptr_t);
extern eastl::pair<bool, intptr_t> default_wnd_proc(void *, unsigned, uintptr_t, intptr_t);
} // namespace workcycle_internal::windows

namespace windows
{
class WindowThread final : public DaThread
{
  os_event_t winCreatedEvent;

  void execute() override;

  static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

public:
  static bool process_main_thread_messages(bool input_only);

  enum Status : int
  {
    RUNNING = 0,
    EXIT, // exit started from game
    CLOSE // the window is closed outside from game
  };

  WindowThread(void *hinst, const char *name, int show, void *icon, const char *title, Driver3dInitCallback *cb);
  ~WindowThread() override { os_event_destroy(&winCreatedEvent); }

  void Start()
  {
    start();
    os_event_wait(&winCreatedEvent, OS_WAIT_INFINITE);
  }

  DWORD threadId = 0;
  int status = -1;

  RenderWindowSettings settings{};
  RenderWindowParams params{};

  decltype(MSG::time) time{};

  union Msg
  {
    Msg() = default;
    Msg(const MSG &msg) : msg{msg} {}
    Msg(const RAWINPUT &ri) : ri{ri} {}

    MSG msg;
    RAWINPUT ri;
  };

  eastl::vector<Msg> message_queues[2];
  eastl::vector<Msg> *write_queue = nullptr;
  eastl::vector<Msg>::iterator tail = nullptr;

  AVOID_FALSE_SHARING;
  eastl::vector<Msg>::iterator head = nullptr;
  AVOID_FALSE_SHARING;
  int message_count = 0;
  AVOID_FALSE_SHARING;
  WinCritSec critSec;
  AVOID_FALSE_SHARING;

  void push_msg(const MSG &msg);
  void push_msg(const MSG &msg, const RAWINPUT &ri);

  MSG *get_msg();
  bool pop_msg();
  RAWINPUT *get_rid();
};

static InitOnDemand<WindowThread> window_thread;

void WindowThread::execute()
{
  threadId = GetCurrentThreadId();

  bool success = set_render_window_params(params, settings);
  os_event_set(&winCreatedEvent);
  if (!success)
    return;

  debug("Window has been created on thread=0x%08x", threadId);

  interlocked_release_store(status, WindowThread::RUNNING);

  while (interlocked_acquire_load(status) == RUNNING)
  {
    MSG msg;
    BOOL ret = GetMessageW(&msg, nullptr, 0, 0);
    if (ret == 0) // WM_QUIT
      break;

    if (ret == -1)
    {
      // handle the error and possibly exit
      break;
    }

    time = msg.time;

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  if (interlocked_acquire_load(status) != CLOSE)
    DestroyWindow((HWND)window_thread->params.hwnd);
}

LRESULT WindowThread::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (auto [handled, result] = workcycle_internal::windows::default_wnd_proc(hwnd, message, wParam, lParam); handled)
    return result;

  if (message == WM_DAGOR_CLOSED)
  {
    window_thread->push_msg(MSG{hwnd, WM_DAGOR_DESTROY, wParam, lParam, window_thread->time});
    interlocked_release_store(window_thread->status, WindowThread::CLOSE);
    while (interlocked_acquire_load(window_thread->message_count) > 1)
      Sleep(0);
    message = WM_CLOSE;
  }
  else if (message == WM_INPUT)
  {
    RAWINPUT ri{};
    UINT dwSize = sizeof(ri);
    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &ri, &dwSize, sizeof(ri.header)) != (UINT)-1)
      window_thread->push_msg(MSG{hwnd, WM_DAGOR_INPUT, wParam, lParam, window_thread->time}, ri);
  }
  else
    window_thread->push_msg(MSG{hwnd, message, wParam, lParam, window_thread->time});

  return DefWindowProcW(hwnd, message, wParam, lParam);
}

void WindowThread::push_msg(const MSG &msg)
{
  WinAutoLock lock(critSec);
  write_queue->emplace_back(msg);
  interlocked_increment(message_count);
}

void WindowThread::push_msg(const MSG &msg, const RAWINPUT &ri)
{
  WinAutoLock lock(critSec);
  write_queue->emplace_back(msg);
  write_queue->emplace_back(ri);
  interlocked_increment(message_count);
}

MSG *WindowThread::get_msg()
{
  if (head == tail)
    if (!pop_msg())
      if (head == tail)
        return nullptr;
  return &head->msg;
}

bool WindowThread::pop_msg()
{
  if (head != tail)
  {
    switch (head->msg.message)
    {
      case WM_DAGOR_INPUT:
        head += 2; //
        break;
      default:
        head++; //
        break;
    }

    auto count = interlocked_decrement(message_count);
    G_ASSERT(count >= 0);
    return count != 0;
  }

  if (interlocked_acquire_load(message_count) == 0)
    return false;

  {
    WinAutoLock lock(critSec);
    if (write_queue == &message_queues[0])
    {
      write_queue = &message_queues[1];
      head = message_queues[0].begin();
      tail = message_queues[0].end();
    }
    else
    {
      write_queue = &message_queues[0];
      head = message_queues[1].begin();
      tail = message_queues[1].end();
    }
    write_queue->clear();
  }

  return head != tail;
}

RAWINPUT *WindowThread::get_rid()
{
  if (MSG *msg = get_msg(); msg != nullptr && (msg->message == WM_INPUT || msg->message == WM_DAGOR_INPUT))
    return &(head + 1)->ri;

  return nullptr;
}

void *create_threaded_window(void *hinst, const char *name, int show, void *icon, const char *title, Driver3dInitCallback *cb)
{
  window_thread.demandInit(hinst, name, show, icon, title, cb);
  window_thread->Start();

  return window_thread->params.hwnd;
}

void shutdown_threaded_window()
{
  if (window_thread)
  {
    int prevStatus = interlocked_compare_exchange(window_thread->status, WindowThread::EXIT, WindowThread::RUNNING);
    if (prevStatus == WindowThread::RUNNING)
    {
      PostThreadMessageW(window_thread->threadId, WM_NULL, 0, 0);

      win32_set_main_wnd(nullptr);
      if (d3d::is_inited())
        d3d::window_destroyed(window_thread->params.hwnd);
    }

    window_thread->terminate(true);
    window_thread.demandDestroy();
  }
}

bool WindowThread::process_main_thread_messages(bool)
{
  TIME_PROFILE(process_main_thread_messages);

  MSG *msg = window_thread->get_msg();
  bool were_events = msg != nullptr;

  for (; msg != nullptr; msg = window_thread->get_msg())
  {
    if (intptr_t result; !::perform_wnd_proc_components(msg->hwnd, msg->message, msg->wParam, msg->lParam, result))
      workcycle_internal::windows::main_wnd_proc(msg->hwnd, msg->message, msg->wParam, msg->lParam);

    if (!window_thread->pop_msg())
      break;
  }

  return were_events;
}

WindowThread::WindowThread(void *hinst, const char *name, int show, void *icon, const char *title, Driver3dInitCallback *cb) :
  DaThread("WindowThread", 1ULL << 20, 0, WORKER_THREADS_AFFINITY_MASK) // For Fraps and other 3rd parties that likes to hook d3d
                                                                        // present
{
  os_event_create(&winCreatedEvent);

  params.hinst = hinst;
  params.wcname = name;
  params.ncmdshow = show;
  params.icon = icon;
  params.title = title;
  params.mainProc = &WndProc;

  get_render_window_settings(settings, cb);

  write_queue = &message_queues[0];
}

bool process_main_thread_messages(bool input_only, bool &out_ret_val)
{
  if (!window_thread)
    return false;
  out_ret_val = WindowThread::process_main_thread_messages(input_only);
  return true;
}
RAWINPUT *get_rid()
{
  G_ASSERT(window_thread);
  return window_thread->get_rid();
};

unsigned long get_thread_id() { return window_thread ? window_thread->threadId : 0; }

} // namespace windows
