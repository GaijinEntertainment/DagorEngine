// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "processes_impl.h"
#include <debug/dag_assert.h>
#include <dag/dag_vector.h>
#include <EASTL/algorithm.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/basePath.h>

#include <cstring>

#if !_TARGET_PC_WIN
#error This file can only be used on windows
#endif

#include <windows.h>

namespace proc::internal
{

struct Pipe
{
  HANDLE read, write;

  operator bool() const { return read && write; }
};

struct ProcessData : PROCESS_INFORMATION
{
  Pipe outputPipe;

  ProcessData(const PROCESS_INFORMATION &pi, Pipe output_pipe) : PROCESS_INFORMATION(pi), outputPipe{output_pipe} {}
};

struct ExtraStateData
{
  HANDLE cancellationEventHnd{};
  dag::Vector<Pipe> outputPipePool{};
};

void init_state(ExecutionState &state) { state.extraData = new ExtraStateData{CreateEvent(nullptr, FALSE, FALSE, nullptr)}; }

void deinit_state(ExecutionState &state)
{
  CloseHandle(state.extraData->cancellationEventHnd);
  for (auto &pipe : state.extraData->outputPipePool)
  {
    CloseHandle(pipe.read);
    CloseHandle(pipe.write);
  }
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

  if (listen_to_cancellation_event && waitRes == winHandles.size() - 1) // Woken by event
  {
    G_ASSERT(state.cancelled.load(dag::memory_order_relaxed));
    return AwaitResult::CANCELLED_BY_USER;
  }

  size_t handleId = waitRes - WAIT_OBJECT_0;

  DWORD exitCode = 0;
  BOOL exitCodeSuccess = GetExitCodeProcess(winHandles[handleId], &exitCode);
  G_VERIFY(exitCodeSuccess);
  G_VERIFY(exitCode != STILL_ACTIVE);

  auto &proc = state.processes[handleId];
  serve_process_output(state, proc);

  state.sinkPool[eastl::to_underlying(proc.sink)].free = true;
  state.extraData->outputPipePool.push_back(eastl::exchange(proc.processData->outputPipe, Pipe{}));

  CloseHandle(proc.processData->hThread);
  CloseHandle(proc.processData->hProcess);
  delete proc.processData;

  if (exitCode == 0)
    proc.task.onSuccess();
  else
    proc.task.onFail();

  state.processes.erase(state.processes.cbegin() + handleId);

  if (listen_to_cancellation_event && state.cancelled.load()) // Recheck 'cancelled' after collecting proc in case event was also fired
    return AwaitResult::CANCELLED_BY_USER;
  else
    return exitCode == 0 ? AwaitResult::ALL_SUCCEEDED : AwaitResult::SOME_FAILED;
}

static Pipe get_output_pipe(ExecutionState &state)
{
  if (!state.extraData->outputPipePool.empty())
  {
    Pipe pipe = state.extraData->outputPipePool.back();
    state.extraData->outputPipePool.pop_back();
    return pipe;
  }

  Pipe pipe{};

  SECURITY_ATTRIBUTES saAttr{};
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = true;
  saAttr.lpSecurityDescriptor = nullptr;

  if (!CreatePipe(&pipe.read, &pipe.write, &saAttr, 0) || !SetHandleInformation(pipe.read, HANDLE_FLAG_INHERIT, 0))
    return {};

  return pipe;
}

eastl::optional<ProcessHandle> spawn_process(ExecutionState &state, ProcessTask &&task)
{
  Pipe pipe = get_output_pipe(state);
  if (!pipe)
    return eastl::nullopt;

  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.hStdOutput = pipe.write;
  si.hStdError = pipe.write;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
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
  hnd.processData = new ProcessData{pi, pipe};
  hnd.task = eastl::move(task);

  return hnd;
}

void serve_process_output(ExecutionState &state, ProcessHandle &hnd)
{
  DWORD bytesAvail = 0;
  G_VERIFY(PeekNamedPipe(hnd.processData->outputPipe.read, nullptr, 0, nullptr, &bytesAvail, nullptr));
  if (bytesAvail == 0)
    return;

  auto &sink = state.sinkPool[eastl::to_underlying(hnd.sink)];
  sink.buffer.resize(sink.buffer.size() + bytesAvail);

  DWORD bytesRead = 0;
  G_VERIFY(ReadFile(hnd.processData->outputPipe.read, sink.buffer.end() - bytesAvail, bytesAvail, &bytesRead, nullptr));
  G_VERIFY(bytesRead == bytesAvail);

  sink.lastTs = get_time_msec();
  hnd.hasCommunicated = true;
}

void send_interrupt_signal_to_process(const ProcessHandle &process)
{
  GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, process.processData->dwProcessId);
}

void kill_process(const ProcessHandle &process) { TerminateProcess(process.processData->hProcess, 1); }
void fire_cancellation_event(ExecutionState &state) { SetEvent(state.extraData->cancellationEventHnd); }

} // namespace proc::internal
