// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "processes.h"
#include <EASTL/unique_ptr.h>
#include <EASTL/optional.h>
#include <EASTL/deque.h>
#include <atomic>

namespace proc
{
namespace internal
{
// Opaque types specified by the platform specific code
struct ProcessData;
struct ExtraStateData;

struct ProcessHandle
{
  ProcessTask task;
  ProcessData *processData = nullptr;
};

struct ExecutionState
{
  int maxProcs = -1;
  bool shouldCancelOnProcFail = true;

  eastl::deque<ProcessTask> tasks{};
  eastl::deque<ProcessHandle> processes{};

  std::atomic_bool cancelled{false};

  ExtraStateData *extraData = nullptr;
};

enum class AwaitResult
{
  ALL_SUCCEEDED,
  SOME_FAILED,
  CANCELLED_BY_USER,
  TIMEOUT
};

constexpr int NO_TIMEOUT = -1;

void init_state(ExecutionState &state);
void deinit_state(ExecutionState &state);

void start_execution(ExecutionState &state);
void end_execution(ExecutionState &state);

AwaitResult await_processes(ExecutionState &state, bool listen_to_cancellation_event, int timeout_ms = NO_TIMEOUT);
eastl::optional<ProcessHandle> spawn_process(ProcessTask &&task);
void send_interrupt_signal_to_process(const ProcessHandle &process);
void kill_process(const ProcessHandle &process);

void fire_cancellation_event(ExecutionState &state);
} // namespace internal
} // namespace proc
