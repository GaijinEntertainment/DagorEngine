// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_fixedMoveOnlyFunction.h>
#include <dag/dag_vector.h>
#include <EASTL/string.h>
#include <EASTL/optional.h>

namespace proc
{

struct ProcessTask
{
  dag::Vector<eastl::string> argv;
  eastl::optional<eastl::string> cwd;
  dag::FixedMoveOnlyFunction<64, void(void)> onSuccess = [] {}, onFail = [] {};
  bool isExternal = false;
};

inline constexpr char MANAGED_MESSAGE_SEPARATOR = '\x1E';

// Can only be called from the main thread and must happen-before all other calls
void init(int max_proc_count, int should_cancel_on_fail);
// 'init' and all calls to 'enqueue' and 'execute' mush happen-before 'deinit'
void deinit();

int is_multiproc();
int max_proc_count();

void enqueue(ProcessTask &&task);

// Executes enqueued tasks with separate processes until all done or at least one erred or cancel was requested
// Is synchronous. Must happen-before with all calls to 'enqueue' and 'execute'.
bool execute();

// Sends cancellation signal to current 'execute' loop. Can be called asynchronously with all other functions.
void cancel();

} // namespace proc
