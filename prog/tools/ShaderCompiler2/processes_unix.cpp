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
#include <climits>
#include <cstring>

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

namespace proc
{

// @TODO: add memory orders to cancelled. Beware, there are dances w/ shutdown, so acq_rel may not be automatically enough!

/* How this is done:
 *
 * The main idea is the same as in processes_win.cpp.
 *
 * The difference is that we can't use sync primitives in select, so a pipe takes the place of the event.
 * We also await not one process at a time, but always collect all zombies in a waitpid cycle.
 */

using Sigaction = struct sigaction;

struct ProcessHandle
{
  pid_t pid;
  ProcessTask task;
};

union Pipe
{
  struct
  {
    int read_fd, write_fd;
  };
  int fds[2];
};

static constexpr uint32_t CANCELLATION_TOKEN = 'stop';

struct ExecutionState
{
  int maxProcs = -1;

  eastl::deque<ProcessTask> tasks{};
  eastl::deque<ProcessHandle> processes{};

  std::atomic_bool cancelled{false};
  Pipe cancellationPipe{};

  sigset_t initialSigmask{};
  Sigaction sigchldAction{};
  pthread_t performerThreadHnd{};
};

static ExecutionState g_state{};

void init(int max_proc_count)
{
  G_ASSERT(is_main_thread());
  G_ASSERT(cpujobs::is_inited());

  g_state.maxProcs = max_proc_count;

  if (g_state.maxProcs < 0)
    g_state.maxProcs = cpujobs::get_physical_core_count();
  if (g_state.maxProcs < 2)
    g_state.maxProcs = 0;
  if (g_state.maxProcs > 24)
    g_state.maxProcs = 24;

  int res = pipe(g_state.cancellationPipe.fds);
  G_VERIFY(res == 0);

  // This makes it so spawned procs don't inherit the pipe fds
  fcntl(g_state.cancellationPipe.read_fd, F_SETFD, FD_CLOEXEC);
  fcntl(g_state.cancellationPipe.write_fd, F_SETFD, FD_CLOEXEC);

  // This moves the pipe into non-blocking mode, which allows to use it as an event
  int flags = fcntl(g_state.cancellationPipe.read_fd, F_GETFL, 0);
  fcntl(g_state.cancellationPipe.read_fd, F_SETFL, flags | O_NONBLOCK);

  // We use sigchld signals for waking from pselect, thus it has to be masked off at all other times
  sigset_t excludedSigmask;
  sigemptyset(&g_state.initialSigmask);
  sigemptyset(&excludedSigmask);
  sigaddset(&excludedSigmask, SIGCHLD);

  sigprocmask(SIG_SETMASK, &excludedSigmask, nullptr);

  // We set a sigchld handler so that this signal is not ignored and wakes us
  g_state.sigchldAction.sa_handler = +[](int) {
    sigaction(SIGCHLD, &g_state.sigchldAction, nullptr);

    // If picked up by sigint handler thread, forward
    if (!pthread_equal(g_state.performerThreadHnd, pthread_self()))
      pthread_kill(g_state.performerThreadHnd, SIGCHLD);
  };
  g_state.sigchldAction.sa_flags = SA_RESTART;

  sigaction(SIGCHLD, &g_state.sigchldAction, nullptr);
}

void deinit()
{
  G_ASSERT(is_main_thread());
  G_ASSERT(g_state.tasks.empty());
  G_ASSERT(g_state.processes.empty());
}


int is_multiproc() { return g_state.maxProcs > 0; }
int max_proc_count() { return g_state.maxProcs; }

void enqueue(ProcessTask &&task)
{
  G_ASSERT(is_main_thread());
  g_state.tasks.push_back(eastl::move(task));
}

static pid_t spawn_process(const ProcessTask &task)
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

  return pid;
}

static bool await_done_processes(bool listen_to_cancellation_event, long timeout_ms = -1)
{
  timespec ts{timeout_ms / 1000, (timeout_ms % 1000) * 1'000'000};
  timespec *selectTimeoutPtr = timeout_ms >= 0 ? &ts : nullptr;

  if (listen_to_cancellation_event)
  {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(g_state.cancellationPipe.read_fd, &fds);

    int eventCnt = pselect(1, &fds, nullptr, nullptr, selectTimeoutPtr, &g_state.initialSigmask);

    if (eventCnt == 0) // 0 is timeout, on signal it is -1 and errno == EINTR
      return false;
    else if (eventCnt > 0)
    {
      uint32_t token = 0;
      while (int bytesRead = read(g_state.cancellationPipe.read_fd, &token, sizeof(token)))
      {
        if (token == CANCELLATION_TOKEN)
        {
          G_ASSERT(g_state.cancelled.load(std::memory_order_relaxed));
          return false;
        }
      }
    }
  }
  else
  {
    int eventCnt = pselect(0, nullptr, nullptr, nullptr, selectTimeoutPtr, &g_state.initialSigmask);
    if (eventCnt == 0) // Timeout, it's -1 + EINTR on signal
      return false;
  }

  bool succeeded = true;

  for (;;)
  {
    int status;
    int res = waitpid(-1, &status, WNOHANG);

    if (res <= 0)
      break;

    ProcessTask task;
    for (auto it = g_state.processes.cbegin(); it != g_state.processes.cend(); ++it)
    {
      if (it->pid == res)
      {
        task = eastl::move(it->task);
        g_state.processes.erase(it);
        break;
      }
    }

    bool processSucceeded = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    if (!processSucceeded)
      task.cleanupOnFail();

    succeeded &= processSucceeded;
  }

  return succeeded;
}

bool execute()
{
  G_ASSERT(is_main_thread());
  g_state.performerThreadHnd = pthread_self();

  if (!shc::try_lock_shutdown())
    return false;

  // Reset when done
  DEFER_FUNC([] {
    g_state.cancelled.store(false);

    // Drain pipe just to be sure
    uint32_t token;
    while (read(g_state.cancellationPipe.read_fd, &token, sizeof(token)) > 0) {}
  });

  while (!g_state.tasks.empty())
  {
    while (g_state.processes.size() >= g_state.maxProcs)
    {
      bool allSucceeded = await_done_processes(true);
      if (!allSucceeded)
        goto cancel;
    }

    if (g_state.cancelled.load())
      goto cancel;

    ProcessHandle nextProc{-1, eastl::move(g_state.tasks.front())};
    g_state.tasks.pop_front();

    if ((nextProc.pid = spawn_process(nextProc.task)) == -1)
      goto cancel;

    g_state.processes.push_back(eastl::move(nextProc));
  }

  while (!g_state.processes.empty())
  {
    bool allSucceeded = await_done_processes(true);
    if (!allSucceeded)
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

  // See processes_win.cpp for the description of cancellation, it's the same here but in unix language.

  for (const auto &proc : g_state.processes)
    kill(proc.pid, SIGINT);

  int attempts = g_state.processes.size() * 3 / 2;
  while (!g_state.processes.empty() && ((attempts--) > 0))
    await_done_processes(false, 250);

  if (!g_state.processes.empty())
  {
    for (const auto &proc : g_state.processes)
      kill(proc.pid, SIGKILL);

    while (!g_state.processes.empty())
      await_done_processes(false);
  }

  shc::unlock_shutdown();
  return false;
}

void cancel()
{
  g_state.cancelled.store(true);
  int bytesWritten = write(g_state.cancellationPipe.write_fd, &CANCELLATION_TOKEN, sizeof(CANCELLATION_TOKEN));
  G_ASSERT(bytesWritten == sizeof(CANCELLATION_TOKEN));
}

} // namespace proc