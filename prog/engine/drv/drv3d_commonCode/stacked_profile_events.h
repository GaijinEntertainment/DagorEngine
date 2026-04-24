// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_objectPool.h>

#define PROFILE_USER_MARKERS_IN_REPLAY 0
#define PROFILE_COMMANDS_WITH_COUNT    0

class StackedProfileEvents
{
private:
  // only for dev builds with profiler
#if DA_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0
  da_profiler::desc_id_t lastChainDescId = -1;
  uint32_t lastChainCounter = 0;
  ObjectPool<da_profiler::ScopedEvent> storage;
  dag::Vector<da_profiler::ScopedEvent *> stack;

  enum class State
  {
    Finished,
    Started,
  } state = State::Finished;

  void push(da_profiler::desc_id_t desc_id) { stack.push_back(storage.allocate(desc_id)); }
  void pop()
  {
    if (stack.empty())
      return;
    storage.free(stack.back());
    stack.pop_back();
  }

public:
  void popChained()
  {
    if (lastChainDescId != -1)
    {
#if PROFILE_COMMANDS_WITH_COUNT
      da_profiler::add_short_string_tag_args(lastChainDescId, "count: %d", lastChainCounter);
#endif

      pop();
      lastChainDescId = -1;
      lastChainCounter = 0;
    }
  }
  void pushChained(da_profiler::desc_id_t desc_id)
  {
    if (state == State::Finished)
      return;

    if (lastChainDescId == desc_id)
    {
      ++lastChainCounter;
      return;
    }

    popChained();
    push(desc_id);
    lastChainDescId = desc_id;
    lastChainCounter = 1;
  }

  void start(da_profiler::desc_id_t desc_id)
  {
    if (state == State::Started)
      return;

    if (desc_id != -1)
      push(desc_id);

    state = State::Started;
  }

  void finish()
  {
#if PROFILE_USER_MARKERS_IN_REPLAY
    userMarkDepth = 0;
#endif
    lastChainDescId = -1;
    lastChainCounter = 0;
    while (!stack.empty())
      pop();
    state = State::Finished;
  }
#else
public:
  void pushChained(::da_profiler::desc_id_t) {}
  void popChained() {}

  void start(::da_profiler::desc_id_t) {}
  void finish() {}
#endif

private:
#if PROFILE_USER_MARKERS_IN_REPLAY && DA_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0

  int userMarkDepth = 0;

public:
  // both methods are triggered by user code, add a bit safety to not break profiler
  void pushInterruptChain(const char *user_dyn_marker)
  {
    if (state == State::Finished)
      return;

    da_profiler::desc_id_t desc_id = da_profiler::add_copy_description(nullptr, 0, da_profiler::Normal, user_dyn_marker);
    popChained();
    push(desc_id);
    ++userMarkDepth;
  }
  void popInterruptChain()
  {
    if (userMarkDepth == 0)
      return;
    popChained();
    pop();
    --userMarkDepth;
  }
#else
public:
  void popInterruptChain() {}
  void pushInterruptChain(const char *) {}
#endif
};