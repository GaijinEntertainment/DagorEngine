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
  dag::FixedMoveOnlyFunction<64, void(void)> cleanupOnFail = [] {};
  bool isExternal = false;
};

// Can only be called from the main thread
void init(int max_proc_count);
void deinit();

int is_multiproc();
int max_proc_count();

void enqueue(ProcessTask &&task);

// Executes enqueued tasks with separate processes until all done or at least one erred or cancel was requested
// Can only be called from the main thread, is synchronous.
bool execute();

// Sends cancellation signal to current perform loop. Can be called from other threads.
void cancel();

} // namespace proc