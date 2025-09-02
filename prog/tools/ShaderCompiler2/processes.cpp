// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "processes.h"
#include "processes_impl.h"
#include "shCompiler.h"
#include "defer.h"
#include <generic/dag_sort.h>
#include <debug/dag_assert.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_cpuJobs.h>

namespace proc
{

// @TODO: add memory orders to cancelled. Beware, there are dances w/ shutdown, so acq_rel may not be automatically enough!

/* How this is done:
 *
 * When the user calls perform, the function starts launching processes and collecting results.
 *
 * For this time, regular shutdown (inside a ^C handler) is not an option, so the shutdown is locked
 * for the duration of the compilation. Instead, the user can call proc::cancel from any other thread,
 * and the process loop will pick it up, drop all enqueued tasks and terminate ones running.
 *
 * This is implemented with an atomic flag (cancelled), which is read on each iteration of the loop,
 * and an os event/pipe that gets signaled. When the loop waits for the next finished process, it also
 * waits on the event, so that cancellation wakes it up too, and there are no delays.
 *
 * When the loop goes into cancellation mode, either because a process failed of because it was told to do so,
 * it also awaits running processes, but now without the event and with a possible timeout.
 *
 * When the loop is done, it restores the ability of the app to shut down regularly.
 */

static internal::ExecutionState g_state{};

static void parse_output(internal::OutputSink &sink)
{
  const bool shouldPrintSep = sink.messageSep != MANAGED_MESSAGE_SEPARATOR;

  eastl::string_view bufView{sink.buffer.data(), sink.buffer.size()};
  size_t begin = 0;
  for (size_t end = bufView.find(sink.messageSep); end != eastl::string_view::npos;
       begin = end + 1, end = begin >= bufView.size() ? eastl::string_view::npos : bufView.find(sink.messageSep, begin))
  {
    auto message = bufView.substr(begin, end - begin + (shouldPrintSep ? 1 : 0));
    printf("%.*s", int(message.size()), message.data());
    fflush(stdout);
  }

  if (sink.free)
    sink.buffer.clear(); // Drop unfinished message remnants
  else
    sink.buffer.erase(sink.buffer.begin(), sink.buffer.begin() + begin);
}

static void marshall_output(internal::ExecutionState &state)
{
  dag::Vector<internal::OutputSink *> sinks{};
  sinks.reserve(state.sinkPool.size());
  for (auto &s : state.sinkPool)
  {
    if (!s.buffer.empty() && s.lastTs > 0)
      sinks.push_back(&s);
  }

  fast_sort(sinks, [](const auto *s1, const auto *s2) { return s1->lastTs < s2->lastTs; });
  for (auto *s : sinks)
  {
    parse_output(*s);
    s->lastTs = -1;
  }
}

static void serve_output(internal::ExecutionState &state)
{
  for (auto &proc : g_state.processes)
    internal::serve_process_output(state, proc);
  marshall_output(state);
}

void init(int max_proc_count, int should_cancel_on_fail)
{
  G_ASSERT(is_main_thread());
  G_ASSERT(cpujobs::is_inited());

  g_state.maxProcs = max_proc_count;
  g_state.shouldCancelOnProcFail = should_cancel_on_fail;

  // @TODO: adapt logic to different platforms. Seems like unix schedulers don't deal as well w/ proc x thread overloads.
  if (g_state.maxProcs < 0)
    g_state.maxProcs = cpujobs::get_physical_core_count();
  if (g_state.maxProcs < 2)
    g_state.maxProcs = 0;
  if (g_state.maxProcs > shc::max_allowed_process_count())
    g_state.maxProcs = shc::max_allowed_process_count();

  internal::init_state(g_state);
}

void deinit()
{
  G_ASSERT(g_state.tasks.empty());
  G_ASSERT(g_state.processes.empty());

  internal::deinit_state(g_state);
}

int is_multiproc() { return g_state.maxProcs > 0; }
int max_proc_count() { return g_state.maxProcs; }

void enqueue(ProcessTask &&task) { g_state.tasks.push_back(eastl::move(task)); }

static auto start_process(internal::ExecutionState &state, ProcessTask &&task)
{
  return internal::spawn_process(state, eastl::move(task)).transform([&state](auto &&hnd) {
    for (auto &s : state.sinkPool)
    {
      if (s.free)
      {
        s.free = false;
        hnd.sink = internal::SinkHandle(eastl::distance(state.sinkPool.begin(), &s));
        break;
      }
    }

    if (hnd.sink == internal::SinkHandle::INVALID)
    {
      hnd.sink = internal::SinkHandle(state.sinkPool.size());
      state.sinkPool.emplace_back();
    }

    {
      auto &sink = state.sinkPool[eastl::to_underlying(hnd.sink)];
      sink.free = false;
      sink.messageSep = hnd.task.isExternal ? '\n' : MANAGED_MESSAGE_SEPARATOR;
    }

    return eastl::move(hnd);
  });
}

bool execute()
{
  if (!shc::try_lock_shutdown())
    return false;

  internal::start_execution(g_state);

  // Reset when done (this does not 100% support reentrability, see comment at return)
  DEFER([] {
    g_state.cancelled.store(false);
    internal::end_execution(g_state);
  });

  bool failedWithoutCancellation = false;

  // When awaiting a process, decide if the fail should cause cancellation.
  // If failed but should not cancel, we instead drop tasks and set the flag
  auto awaitProcsAndDecideContinuation = [&] {
    internal::AwaitResult res = internal::await_processes(g_state, true, 50);
    serve_output(g_state);

    if (res == internal::AwaitResult::SOME_FAILED)
    {
      if (g_state.shouldCancelOnProcFail)
        return false;
      else
      {
        // With -cjWait we await all running procs, but drop queue
        failedWithoutCancellation = true;
        g_state.tasks.clear();
      }
    }
    else if (res == internal::AwaitResult::CANCELLED_BY_USER)
    {
      return false;
    }

    // ALL_SUCCEEDED or TIMEOUT
    return true;
  };

  // Scheduling loop
  while (!g_state.tasks.empty())
  {
    while (g_state.processes.size() >= g_state.maxProcs)
    {
      if (!awaitProcsAndDecideContinuation())
        goto cancel;
      else if (failedWithoutCancellation)
        goto final_wait;
    }

    if (g_state.cancelled.load())
      goto cancel;

    ProcessTask nextTask = eastl::move(g_state.tasks.front());
    g_state.tasks.pop_front();

    auto nextProcMaybe = start_process(g_state, eastl::move(nextTask));
    if (!nextProcMaybe)
      goto cancel;

    g_state.processes.push_back(eastl::move(*nextProcMaybe));
  }

final_wait:
  while (!g_state.processes.empty())
  {
    if (!awaitProcsAndDecideContinuation())
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
  return !failedWithoutCancellation && !g_state.cancelled.load();

cancel:
  g_state.tasks.clear();

  auto sendInterruptSafe = [](auto &proc) {
    if (proc.hasCommunicated)
    {
      internal::send_interrupt_signal_to_process(proc);
      proc.hasBeenSignalled = true;
    }
  };

  // First, try regular signals
  for (auto &proc : g_state.processes)
    sendInterruptSafe(proc);

  int timeouts = 0;
  constexpr int MAX_TIMEOUTS = 50;
  while (!g_state.processes.empty())
  {
    internal::AwaitResult res = internal::await_processes(g_state, false, 50);
    serve_output(g_state);

    // If some processes hung or have not started in the prev signal sends, send more signals
    for (auto &proc : g_state.processes)
    {
      if (res == internal::AwaitResult::TIMEOUT || !proc.hasBeenSignalled)
        sendInterruptSafe(proc);
    }

    if (res == internal::AwaitResult::TIMEOUT)
    {
      ++timeouts;
      if (timeouts > MAX_TIMEOUTS)
        break;
    }
  }

  // If some processes hung dead, terminate them with fire
  for (const auto &proc : g_state.processes)
    internal::kill_process(proc);
  while (!g_state.processes.empty())
  {
    internal::await_processes(g_state, false, 50);
    serve_output(g_state);
  }

  shc::unlock_shutdown();
  return false;
}

void cancel()
{
  g_state.cancelled.store(true);
  internal::fire_cancellation_event(g_state);
}

} // namespace proc
