// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "processes.h"
#include "shCompiler.h"
#include "defer.h"
#include <debug/dag_assert.h>
#include <dag/dag_vector.h>
#include <EASTL/deque.h>
#include <EASTL/algorithm.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/basePath.h>

#include <atomic>

#if !_TARGET_PC_WIN
#error This file can only be used on windows
#endif

#include <windows.h>

namespace proc
{

// @TODO: add memory orders to cancelled. Beware, there are dances w/ shutdown, so acq_rel may not be automatically enough!

/* How this is done:
 * When the user calls perform, the function starts launching processes and collecting results.
 *
 * For this time, regular shutdown (inside a ^C handler) is not an option, so the shutdown is locked
 * for the duration of the compilation. Instead, the user can call proc::cancel from any other thread,
 * and the process loop will pick it up, drop all enqueued tasks and terminate ones running.
 *
 * This is implemented with an atomic flag (cancelled), which is read on each iteration of the loop,
 * and a windows event that gets signaled. When the loop waits for the next finished process, it also
 * waits on the event, so that cancellation wakes it up too, and there are no delays.
 *
 * When the loop goes into cancellation mode, either because a process failed of because it was told to do so,
 * it also awaits running processes, but now without the event and with a possible timeout.
 *
 * When the loop is done, it restores the ability of the app to shut down regularly.
 */

struct ProcessHandle
{
  PROCESS_INFORMATION pi;
  ProcessTask task;
};

struct ExecutionState
{
  int maxProcs = -1;

  eastl::deque<ProcessTask> tasks{};
  eastl::deque<ProcessHandle> processes{};

  std::atomic_bool cancelled{false};
  HANDLE cancellationEventHnd{};
};

static ExecutionState g_state{};

void init(int max_proc_count)
{
  G_ASSERT(is_main_thread());
  G_ASSERT(cpujobs::is_inited());

  g_state.maxProcs = max_proc_count;

  // @TODO: adapt logic to different platforms. Seems like unix schedulers don't deal as well w/ proc x thread overloads.
  if (g_state.maxProcs < 0)
    g_state.maxProcs = cpujobs::get_physical_core_count();
  if (g_state.maxProcs < 2)
    g_state.maxProcs = 0;
  if (g_state.maxProcs > 24) // very small benefit from 24+ processes, but noticeable memory consumption
    g_state.maxProcs = 24;

  g_state.cancellationEventHnd = CreateEventEx(nullptr, nullptr, 0, SYNCHRONIZE | DELETE);
}

void deinit()
{
  G_ASSERT(is_main_thread());
  G_ASSERT(g_state.tasks.empty());
  G_ASSERT(g_state.processes.empty());

  CloseHandle(g_state.cancellationEventHnd);
}

int is_multiproc() { return g_state.maxProcs > 0; }
int max_proc_count() { return g_state.maxProcs; }

void enqueue(ProcessTask &&task)
{
  G_ASSERT(is_main_thread());
  g_state.tasks.push_back(eastl::move(task));
}

static DWORD await_one_process(bool listen_to_cancellation_event, DWORD timeout_ms = INFINITE)
{
  auto scrapeHandlesForWait = [&] {
    dag::Vector<HANDLE> handles{};
    handles.resize(g_state.processes.size());
    eastl::transform(g_state.processes.cbegin(), g_state.processes.cend(), handles.begin(),
      [](const ProcessHandle &hnd) { return hnd.pi.hProcess; });
    if (listen_to_cancellation_event)
      handles.push_back(g_state.cancellationEventHnd);
    return handles;
  };

  dag::Vector<HANDLE> winHandles = scrapeHandlesForWait();

  DWORD waitRes = WaitForMultipleObjectsEx(winHandles.size(), winHandles.data(), false, timeout_ms, false);
  if (waitRes == WAIT_TIMEOUT)
    return -1;

  G_VERIFY(waitRes >= WAIT_OBJECT_0 && waitRes < WAIT_OBJECT_0 + winHandles.size());

  if (listen_to_cancellation_event && waitRes == winHandles.size() - 1) // Hit event
  {
    G_ASSERT(g_state.cancelled.load(std::memory_order_relaxed));
    return -1; // Same as if terminated with an error
  }

  size_t handleId = waitRes - WAIT_OBJECT_0;

  DWORD exitCode = 0;
  BOOL exitCodeSuccess = GetExitCodeProcess(winHandles[handleId], &exitCode);
  G_VERIFY(exitCodeSuccess);
  G_VERIFY(exitCode != STILL_ACTIVE);

  CloseHandle(g_state.processes[handleId].pi.hThread);
  CloseHandle(g_state.processes[handleId].pi.hProcess);

  if (exitCode != 0)
    g_state.processes[handleId].task.cleanupOnFail();

  g_state.processes.erase(g_state.processes.cbegin() + handleId);

  return exitCode;
}

bool execute()
{
  G_ASSERT(is_main_thread());

  if (!shc::try_lock_shutdown())
    return false;

  // Reset when done (this does not 100% support reentrability, see comment at return)
  DEFER_FUNC([] {
    g_state.cancelled.store(false);
    ResetEvent(g_state.cancellationEventHnd);
  });

  // Scheduling loop
  while (!g_state.tasks.empty())
  {
    while (g_state.processes.size() >= g_state.maxProcs)
    {
      DWORD exitCode = await_one_process(true);
      if (exitCode != 0)
        goto cancel;
    }

    if (g_state.cancelled.load())
      goto cancel;

    ProcessTask nextTask = eastl::move(g_state.tasks.front());
    g_state.tasks.pop_front();

    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    G_ASSERT(!nextTask.argv.empty());
    eastl::string cmdline = nextTask.argv[0];
    for (size_t i = 1; i < nextTask.argv.size(); ++i)
    {
      cmdline += " ";
      cmdline += nextTask.argv[i];
    }

    const char *cwd = nullptr;

    char cwdStorage[DAGOR_MAX_PATH];
    if (nextTask.cwd.has_value())
    {
      if (is_path_abs(nextTask.cwd->c_str()))
        strncpy(cwdStorage, nextTask.cwd->c_str(), sizeof(cwdStorage));
      else
      {
        DWORD len = GetCurrentDirectory(sizeof(cwdStorage), cwdStorage);
        strncat_s(cwdStorage, sizeof(cwdStorage), "\\", 1);
        strncat_s(cwdStorage, sizeof(cwdStorage), nextTask.cwd->c_str(), nextTask.cwd->length());
      }
      dd_simplify_fname_c(cwdStorage);

      cwd = cwdStorage;
    }

    // External programs (non-dsc) are presumed to be able to handle ^C, fail w/ a non-zero exit code and cause loop exit this way.
    // This is needed, because not all programs handle CtrlBreak like we want them to, so a new proc groups leads to them ignoring our
    // ^C. On the other hand, clone dsc processes need to have a separate group, cause otherwise the signal crashes the parent terminal
    // instance.
    DWORD dwCreationFlags = nextTask.isExternal ? 0 : CREATE_NEW_PROCESS_GROUP;
    if (!CreateProcessA(nextTask.argv[0].c_str(), cmdline.data(), NULL, NULL, TRUE, dwCreationFlags, NULL, cwd, &si, &pi))
      goto cancel;

    g_state.processes.push_back(ProcessHandle{pi, eastl::move(nextTask)});
  }

  while (!g_state.processes.empty())
  {
    DWORD exitCode = await_one_process(true);
    if (exitCode != 0)
      goto cancel;
  }

  // Re-read g_state.cancelled after unlocking shutdown.
  //
  // If try_enter_shutdown failed in sighandler -- then it was before unlock_shutdown in mo,
  // and preceding proc::cancel() hb the g_state.cancelled.load() and we read that we were cancelled.
  //
  // If try_enter_shutdown succeeded in sighandler -- the handler will perform the shutdown on it's own.
  //
  // @NOTE: this logic may break if we try to reenter proc::perform() instantly (not verified).
  // However, this should not happen. Currently we call it once per run,
  // so consider reentering proc::perform() not supported.
  shc::unlock_shutdown();
  return !g_state.cancelled.load();

cancel:
  g_state.tasks.clear();

  // First, try regular "signal"
  for (const auto &proc : g_state.processes)
    GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, proc.pi.dwProcessId);

  // Wait with timeout, leasing at most 250 * procs/2 additional time (the impl is a bit iffy, but enough in practice)
  int attempts = g_state.processes.size() * 3 / 2;
  while (!g_state.processes.empty() && ((attempts--) > 0))
    await_one_process(false, 250);

  // If some processes hung dead, terminate them with fire
  if (!g_state.processes.empty())
  {
    for (const auto &proc : g_state.processes)
      TerminateProcess(proc.pi.hProcess, 1);

    while (!g_state.processes.empty())
      await_one_process(false);
  }

  shc::unlock_shutdown();
  return false;
}

void cancel()
{
  g_state.cancelled.store(true);
  SetEvent(g_state.cancellationEventHnd);
}

} // namespace proc