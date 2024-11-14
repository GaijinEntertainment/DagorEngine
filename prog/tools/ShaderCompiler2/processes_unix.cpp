// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "processes_impl.h"
#include <debug/dag_assert.h>

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
};

struct ExtraStateData
{
  Pipe cancellationPipe{};
  sigset_t initialSigmask{};
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

void deinit_state(ExecutionState &state) { delete state.extraData; }

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

  if (listen_to_cancellation_event)
  {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(extra->cancellationPipe.read_fd, &fds);

    int eventCnt = pselect(1, &fds, nullptr, nullptr, selectTimeoutPtr, &extra->initialSigmask);

    if (eventCnt == 0) // 0 is timeout, on signal it is -1 and errno == EINTR
      return AwaitResult::TIMEOUT;
    else if (eventCnt > 0)
    {
      uint32_t token = 0;
      while (int bytesRead = read(extra->cancellationPipe.read_fd, &token, sizeof(token)))
      {
        if (token == CANCELLATION_TOKEN)
        {
          G_ASSERT(state.cancelled.load(std::memory_order_relaxed));
          return AwaitResult::CANCELLED_BY_USER;
        }
      }
    }
  }
  else
  {
    int eventCnt = pselect(0, nullptr, nullptr, nullptr, selectTimeoutPtr, &extra->initialSigmask);
    if (eventCnt == 0) // Timeout, it's -1 + EINTR on signal
      return AwaitResult::TIMEOUT;
  }

  bool succeeded = true;

  for (;;)
  {
    int status;
    int res = waitpid(-1, &status, WNOHANG);

    if (res <= 0)
      break;

    ProcessTask task;
    for (auto it = state.processes.cbegin(); it != state.processes.cend(); ++it)
    {
      if (it->processData->pid == res)
      {
        task = eastl::move(it->task);
        delete it->processData;
        state.processes.erase(it);
        break;
      }
    }

    bool processSucceeded = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    if (!processSucceeded)
      task.cleanupOnFail();

    succeeded &= processSucceeded;
  }

  return succeeded ? AwaitResult::ALL_SUCCEEDED : AwaitResult::SOME_FAILED;
}

eastl::optional<ProcessHandle> spawn_process(ProcessTask &&task)
{
  dag::Vector<const char *> argv{};
  for (const eastl::string &arg : task.argv)
    argv.push_back(arg.c_str());
  argv.push_back(nullptr);

  const char *cwd = task.cwd.has_value() ? task.cwd->c_str() : nullptr;

  pid_t pid = vfork();
  if (pid == 0) // Code in child process
  {
    if (cwd)
    {
      if (chdir(cwd) != 0)
        exit(2);
    }

    execvp(task.argv[0].c_str(), (char *const *)argv.data());
    exit(0);
  }
  else if (pid == -1)
    return eastl::nullopt;

  ProcessHandle hnd;
  hnd.task = eastl::move(task);
  hnd.processData = new ProcessData{pid};
  return hnd;
}

void send_interrupt_signal_to_process(const ProcessHandle &process) { kill(process.processData->pid, SIGINT); }
void kill_process(const ProcessHandle &process) { kill(process.processData->pid, SIGKILL); }

void fire_cancellation_event(ExecutionState &state)
{
  int bytesWritten = write(state.extraData->cancellationPipe.write_fd, &CANCELLATION_TOKEN, sizeof(CANCELLATION_TOKEN));
  G_ASSERT(bytesWritten == sizeof(CANCELLATION_TOKEN));
}

} // namespace proc::internal
