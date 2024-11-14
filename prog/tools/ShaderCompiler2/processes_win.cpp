// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "processes_impl.h"
#include <debug/dag_assert.h>
#include <dag/dag_vector.h>
#include <EASTL/algorithm.h>
#include <osApiWrappers/basePath.h>

#include <cstring>

#if !_TARGET_PC_WIN
#error This file can only be used on windows
#endif

#include <windows.h>

namespace proc::internal
{
struct ProcessData : PROCESS_INFORMATION
{
  ProcessData(const PROCESS_INFORMATION &pi) : PROCESS_INFORMATION(pi) {}
};

struct ExtraStateData
{
  HANDLE cancellationEventHnd{};
};

void init_state(ExecutionState &state)
{
  state.extraData = new ExtraStateData{CreateEventEx(nullptr, nullptr, 0, SYNCHRONIZE | DELETE)};
}

void deinit_state(ExecutionState &state)
{
  CloseHandle(state.extraData->cancellationEventHnd);
  delete state.extraData;
}

void start_execution(ExecutionState &state) {}
void end_execution(ExecutionState &state) { ResetEvent(state.extraData->cancellationEventHnd); }

AwaitResult await_processes(ExecutionState &state, bool listen_to_cancellation_event, int timeout_ms)
{
  auto scrapeHandlesForWait = [&] {
    dag::Vector<HANDLE> handles{};
    handles.resize(state.processes.size());
    eastl::transform(state.processes.cbegin(), state.processes.cend(), handles.begin(),
      [](const ProcessHandle &hnd) { return hnd.processData->hProcess; });
    if (listen_to_cancellation_event)
      handles.push_back(state.extraData->cancellationEventHnd);
    return handles;
  };

  dag::Vector<HANDLE> winHandles = scrapeHandlesForWait();

  DWORD timeoutMsDw = timeout_ms == NO_TIMEOUT ? INFINITE : timeout_ms;

  DWORD waitRes = WaitForMultipleObjectsEx(winHandles.size(), winHandles.data(), false, timeoutMsDw, false);
  if (waitRes == WAIT_TIMEOUT)
    return AwaitResult::TIMEOUT;

  G_VERIFY(waitRes >= WAIT_OBJECT_0 && waitRes < WAIT_OBJECT_0 + winHandles.size());

  if (listen_to_cancellation_event && waitRes == winHandles.size() - 1) // Hit event
  {
    G_ASSERT(state.cancelled.load(std::memory_order_relaxed));
    return AwaitResult::CANCELLED_BY_USER;
  }

  size_t handleId = waitRes - WAIT_OBJECT_0;

  DWORD exitCode = 0;
  BOOL exitCodeSuccess = GetExitCodeProcess(winHandles[handleId], &exitCode);
  G_VERIFY(exitCodeSuccess);
  G_VERIFY(exitCode != STILL_ACTIVE);

  CloseHandle(state.processes[handleId].processData->hThread);
  CloseHandle(state.processes[handleId].processData->hProcess);
  delete state.processes[handleId].processData;

  if (exitCode != 0)
    state.processes[handleId].task.cleanupOnFail();

  state.processes.erase(state.processes.cbegin() + handleId);

  return exitCode == 0 ? AwaitResult::ALL_SUCCEEDED : AwaitResult::SOME_FAILED;
}

eastl::optional<ProcessHandle> spawn_process(ProcessTask &&task)
{
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;

  G_ASSERT(!task.argv.empty());
  eastl::string cmdline = task.argv[0];
  for (size_t i = 1; i < task.argv.size(); ++i)
  {
    cmdline += " ";
    cmdline += task.argv[i];
  }

  const char *cwd = nullptr;

  char cwdStorage[DAGOR_MAX_PATH];
  if (task.cwd.has_value())
  {
    if (is_path_abs(task.cwd->c_str()))
      strncpy(cwdStorage, task.cwd->c_str(), sizeof(cwdStorage));
    else
    {
      DWORD len = GetCurrentDirectory(sizeof(cwdStorage), cwdStorage);
      strncat_s(cwdStorage, sizeof(cwdStorage), "\\", 1);
      strncat_s(cwdStorage, sizeof(cwdStorage), task.cwd->c_str(), task.cwd->length());
    }
    dd_simplify_fname_c(cwdStorage);

    cwd = cwdStorage;
  }

  // External programs (non-dsc) are presumed to be able to handle ^C, fail w/ a non-zero exit code and cause loop exit this way.
  // This is needed, because not all programs handle CtrlBreak like we want them to, so a new proc groups leads to them ignoring our
  // ^C. On the other hand, clone dsc processes need to have a separate group, cause otherwise the signal crashes the parent terminal
  // instance.
  DWORD dwCreationFlags = task.isExternal ? 0 : CREATE_NEW_PROCESS_GROUP;
  if (!CreateProcessA(task.argv[0].c_str(), cmdline.data(), NULL, NULL, TRUE, dwCreationFlags, NULL, cwd, &si, &pi))
    return eastl::nullopt;

  ProcessHandle hnd;
  hnd.processData = new ProcessData{pi};
  hnd.task = eastl::move(task);
  return hnd;
}

void send_interrupt_signal_to_process(const ProcessHandle &process)
{
  GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, process.processData->dwProcessId);
}

void kill_process(const ProcessHandle &process) { TerminateProcess(process.processData->hProcess, 1); }
void fire_cancellation_event(ExecutionState &state) { SetEvent(state.extraData->cancellationEventHnd); }

} // namespace proc::internal
