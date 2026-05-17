// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "call_stack_ext.h"
#include <EASTL/array.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <EASTL/string.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_stackHlp.h>
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
#if DX12_HAS_CALLSTACK_EXT
  ext::Message msg;
#endif
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
  void setCommandData(const null::CommandData &, const char *) {}
  const char *getLastCommandName() const { return lastCommandName; }
};

class Generator
{
  WinCritSec mutex;
  eastl::hash_set<CallStack, CallStackHasher> callstacks;
#if DX12_HAS_CALLSTACK_EXT
  ext::Generator extGenerator;
#endif

public:
  void configure(const DataBlock *api_config)
  {
    G_UNUSED(api_config);
#if DX12_HAS_CALLSTACK_EXT
    extGenerator.configure(api_config);
#endif
  }
  const CallStack *captureCallStack(uint32_t offset)
  {
    CallStack callstack;
    stackhlp_fill_stack(callstack.data(), callstack.size(), offset);

    WinAutoLock lock{mutex};
    auto ref = callstacks.insert(callstack).first;
    return &*ref;
  }

public:
  CommandData generateCommandData(uint32_t frame_index)
  {
    G_UNUSED(frame_index);
    return {
#if DX12_HAS_CALLSTACK_EXT
      extGenerator.captureExtMessage(frame_index),
#endif
      captureCallStack(2)};
  }
};

class Reporter
{
  eastl::hash_map<const CallStack *, eastl::string> callStackCache;
#if DX12_HAS_CALLSTACK_EXT
  ext::Resolver extResolver;
#endif

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

  void append(String &, const char *, const null::CommandData &) {}

  eastl::string_view resolve(const CommandData &data) { return doResolve(data); }

#if DX12_HAS_CALLSTACK_EXT
  const char *extMessage(const CommandData &data) { return extResolver.extMessage(data.msg); }
#endif
};
} // namespace drv3d_dx12::debug::call_stack::full_stack
