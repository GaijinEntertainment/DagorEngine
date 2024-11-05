// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_critSec.h>
#include <EASTL/hash_set.h>
#include <EASTL/hash_map.h>
#include <EASTL/array.h>
#include <EASTL/string.h>
#include <util/dag_string.h>


class DataBlock;

namespace drv3d_dx12::debug::call_stack::full_stack
{
inline constexpr uint32_t max_call_stack_depth = 32;
using CallStack = eastl::array<void *, max_call_stack_depth>;

struct CallStackHasher
{
  // very simple hashing, each pointer is the hash value and we simply combine all of them.
  inline size_t operator()(const CallStack &call_stack) const
  {
    size_t value = 0;
    for (auto &p : call_stack)
    {
      value ^= reinterpret_cast<size_t>(p) + 0x9e3779b9 + (value << 6) + (value >> 2);
    }

    return value;
  }
};

struct CommandData
{
  const CallStack *callStack;
};

class ExecutionContextDataStore
{
  const char *lastCommandName = nullptr;
  CommandData data{};

public:
  const CommandData &getCommandData() const { return data; }
  void setCommandData(const CommandData &update, const char *name)
  {
    data = update;
    lastCommandName = name;
  }
  const char *getLastCommandName() const { return lastCommandName; }
};

class Generator
{
  WinCritSec mutex;
  eastl::hash_set<CallStack, CallStackHasher> callstacks;

public:
  constexpr void configure(const DataBlock *) {}
  const CallStack *captureCallStack(uint32_t offset)
  {
    CallStack callstack;
    stackhlp_fill_stack(callstack.data(), callstack.size(), offset);

    WinAutoLock lock{mutex};
    auto ref = callstacks.insert(callstack).first;
    return &*ref;
  }

public:
  CommandData generateCommandData() { return {captureCallStack(2)}; }
};

class Reporter
{
  eastl::hash_map<const CallStack *, eastl::string> callStackCache;
  const eastl::string &doResolve(const CommandData &data)
  {
    auto ref = callStackCache.find(data.callStack);
    if (end(callStackCache) == ref)
    {
      char strBuf[4096];
      auto name = stackhlp_get_call_stack(strBuf, sizeof(strBuf) - 1, data.callStack->data(), data.callStack->size());
      strBuf[sizeof(strBuf) - 1] = '\0';
      ref = callStackCache.emplace(data.callStack, name).first;
    }
    return ref->second;
  }

public:
  void report(const CommandData &data)
  {
    if (!data.callStack)
    {
      return;
    }
    logdbg(doResolve(data).c_str());
  }

  void append(String &buffer, const char *prefix, const CommandData &data)
  {
    if (!data.callStack)
    {
      return;
    }
    buffer.append(prefix);
    auto &str = doResolve(data);
    buffer.append(str.data(), str.length());
  }

  eastl::string_view resolve(const CommandData &data) { return doResolve(data); }
};
} // namespace drv3d_dx12::debug::call_stack::full_stack
