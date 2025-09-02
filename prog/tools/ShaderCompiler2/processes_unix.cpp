// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "processes_impl.h"
#include <debug/dag_assert.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_globDef.h>

#include <climits>

#if !_TARGET_PC || _TARGET_PC_WIN
#error This file can only be used on unix systems
#endif

#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <spawn.h>
#include <fcntl.h>
#include <time.h>

namespace proc::internal
{
using Sigaction = struct sigaction;

union Pipe
{
  struct
  {
    int read_fd, write_fd;
  };
  int fds[2];
};

static constexpr uint32_t CANCELLATION_TOKEN = 'stop';

struct ProcessData
{
  pid_t pid;
  Pipe outputPipe;
};

struct ExtraStateData
{
  Pipe cancellationPipe{};
  sigset_t initialSigmask{};
  dag::Vector<Pipe> outputPipePool{};
};

// This has to be a separate static var, cause sigchld handler uses it
struct SigchldContext
{
  Sigaction sigchldAction{};
  pthread_t performerThreadHnd{};
};

static SigchldContext g_sigchld_ctx{};

void init_state(ExecutionState &state)
{
  state.extraData = new ExtraStateData;

  int res = pipe(state.extraData->cancellationPipe.fds);
  G_VERIFY(res == 0);

  // This makes it so spawned procs don't inherit the pipe fds
  fcntl(state.extraData->cancellationPipe.read_fd, F_SETFD, FD_CLOEXEC);
  fcntl(state.extraData->cancellationPipe.write_fd, F_SETFD, FD_CLOEXEC);

  // This moves the pipe into non-blocking mode, which allows to use it as an event
  int flags = fcntl(state.extraData->cancellationPipe.read_fd, F_GETFL, 0);
  fcntl(state.extraData->cancellationPipe.read_fd, F_SETFL, flags | O_NONBLOCK);

  // We use sigchld signals for waking from pselect, thus it has to be masked off at all other times
  sigset_t excludedSigmask;
  sigemptyset(&state.extraData->initialSigmask);
  sigemptyset(&excludedSigmask);
  sigaddset(&excludedSigmask, SIGCHLD);

  sigprocmask(SIG_SETMASK, &excludedSigmask, nullptr);

  // We set a sigchld handler so that this signal is not ignored and wakes us
  g_sigchld_ctx.sigchldAction.sa_handler = +[](int) {
    sigaction(SIGCHLD, &g_sigchld_ctx.sigchldAction, nullptr);

    // If picked up by sigint handler thread, forward
    if (!pthread_equal(g_sigchld_ctx.performerThreadHnd, pthread_self()))
      pthread_kill(g_sigchld_ctx.performerThreadHnd, SIGCHLD);
  };
  g_sigchld_ctx.sigchldAction.sa_flags = SA_RESTART;

  sigaction(SIGCHLD, &g_sigchld_ctx.sigchldAction, nullptr);
}

void deinit_state(ExecutionState &state)
{
  for (const auto &pipe : state.extraData->outputPipePool)
  {
    close(pipe.read_fd);
    close(pipe.write_fd);
  }
  delete state.extraData;
}

void start_execution(ExecutionState &state) { g_sigchld_ctx.performerThreadHnd = pthread_self(); }

void end_execution(ExecutionState &state)
{
  // Drain pipe just to be sure
  uint32_t token;
  while (read(state.extraData->cancellationPipe.read_fd, &token, sizeof(token)) > 0) {}
}

AwaitResult await_processes(ExecutionState &state, bool listen_to_cancellation_event, int timeout_ms)
{
  timespec ts{timeout_ms / 1000, (timeout_ms % 1000) * 1'000'000};
  timespec *selectTimeoutPtr = timeout_ms == NO_TIMEOUT ? nullptr : &ts;

  ExtraStateData *extra = state.extraData;

  fd_set fds;
  FD_ZERO(&fds);
  for (const auto &hnd : state.processes)
    FD_SET(hnd.processData->outputPipe.read_fd, &fds);

  if (listen_to_cancellation_event)
  {
    FD_SET(extra->cancellationPipe.read_fd, &fds);
    pselect(state.processes.size() + 1, &fds, nullptr, nullptr, selectTimeoutPtr, &extra->initialSigmask);

    if (FD_ISSET(extra->cancellationPipe.read_fd, &fds))
    {
      uint32_t token = 0;
      int bytesRead;
      while ((bytesRead = read(extra->cancellationPipe.read_fd, &token, sizeof(token))) > 0)
      {
        if (token == CANCELLATION_TOKEN)
        {
          G_ASSERT(state.cancelled.load(dag::memory_order_relaxed));
          return AwaitResult::CANCELLED_BY_USER;
        }
      }
    }
  }
  else
  {
    pselect(state.processes.size(), &fds, nullptr, nullptr, selectTimeoutPtr, &extra->initialSigmask);
  }

  for (auto &hnd : state.processes)
  {
    if (FD_ISSET(hnd.processData->outputPipe.read_fd, &fds))
      serve_process_output(state, hnd);
  }

  bool succeeded = true;
  bool collectedProcs = false;

  for (;;)
  {
    int status;
    int res = waitpid(-1, &status, WNOHANG);

    if (res <= 0)
      break;

    collectedProcs = true;

    ProcessTask task;
    for (auto it = state.processes.cbegin(); it != state.processes.cend(); ++it)
    {
      if (it->processData->pid == res)
      {
        serve_process_output(state, *it);
        state.sinkPool[eastl::to_underlying(it->sink)].free = true;
        state.extraData->outputPipePool.push_back(eastl::exchange(it->processData->outputPipe, Pipe{}));

        task = eastl::move(it->task);
        delete it->processData;
        state.processes.erase(it);
        break;
      }
    }

    bool processSucceeded = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    if (processSucceeded)
      task.onSuccess();
    else
      task.onFail();

    succeeded &= processSucceeded;
  }

  if (collectedProcs)
    return succeeded ? AwaitResult::ALL_SUCCEEDED : AwaitResult::SOME_FAILED;
  else
    return AwaitResult::TIMEOUT;
}

static Pipe get_output_pipe(ExecutionState &state)
{
  if (!state.extraData->outputPipePool.empty())
  {
    Pipe pipe = state.extraData->outputPipePool.back();
    state.extraData->outputPipePool.pop_back();
    return pipe;
  }

  Pipe outputPipe{};

  G_VERIFY(pipe(outputPipe.fds) == 0);

  // Children should only inherit the write end
  fcntl(outputPipe.read_fd, F_SETFD, FD_CLOEXEC);

  // This moves the pipe into non-blocking mode
  int flags = fcntl(outputPipe.read_fd, F_GETFL, 0);
  fcntl(outputPipe.read_fd, F_SETFL, flags | O_NONBLOCK);

  return outputPipe;
}

eastl::optional<ProcessHandle> spawn_process(ExecutionState &state, ProcessTask &&task)
{
  dag::Vector<const char *> argv{};
  for (const eastl::string &arg : task.argv)
    argv.push_back(arg.c_str());
  argv.push_back(nullptr);

  const char *cwd = task.cwd.has_value() ? task.cwd->c_str() : nullptr;

  Pipe outputPipe = get_output_pipe(state);

  pid_t pid = fork();
  if (pid == 0) // Code in child process
  {
    if (cwd)
    {
      if (chdir(cwd) != 0)
        _exit(2);
    }

    dup2(outputPipe.write_fd, STDOUT_FILENO);
    dup2(outputPipe.write_fd, STDERR_FILENO);
    close(outputPipe.write_fd);

    _exit(execvp(task.argv[0].c_str(), (char *const *)argv.data()));
  }
  else if (pid == -1)
  {
    return eastl::nullopt;
  }

  ProcessHandle hnd;
  hnd.task = eastl::move(task);
  hnd.processData = new ProcessData{pid, outputPipe};
  return hnd;
}

void serve_process_output(ExecutionState &state, ProcessHandle &hnd)
{
  auto &sink = state.sinkPool[eastl::to_underlying(hnd.sink)];
  constexpr int READ_CHUNK_SIZE = 128;

  size_t before = sink.buffer.size();

  for (;;)
  {
    sink.buffer.resize(sink.buffer.size() + READ_CHUNK_SIZE);
    int bytesRead;
    if (
      (bytesRead = read(hnd.processData->outputPipe.read_fd, sink.buffer.end() - READ_CHUNK_SIZE, READ_CHUNK_SIZE)) < READ_CHUNK_SIZE)
    {
      sink.buffer.resize(sink.buffer.size() - READ_CHUNK_SIZE + max(bytesRead, 0));
      break;
    }
  }

  size_t after = sink.buffer.size();

  sink.lastTs = get_time_msec();
  hnd.hasCommunicated = true;
}

void send_interrupt_signal_to_process(const ProcessHandle &process) { kill(process.processData->pid, SIGINT); }
void kill_process(const ProcessHandle &process) { kill(process.processData->pid, SIGKILL); }

void fire_cancellation_event(ExecutionState &state)
{
  int bytesWritten = write(state.extraData->cancellationPipe.write_fd, &CANCELLATION_TOKEN, sizeof(CANCELLATION_TOKEN));
  G_ASSERT(bytesWritten == sizeof(CANCELLATION_TOKEN));
}

} // namespace proc::internal
