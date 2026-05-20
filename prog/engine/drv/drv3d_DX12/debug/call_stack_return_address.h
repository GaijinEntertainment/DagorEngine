// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "call_stack_ext.h"
#include <debug/dag_log.h>
#include <drv_returnAddrStore.h>
#include <EASTL/string_view.h>

class DataBlock;
class String;

#include <osApiWrappers/dag_stackHlp.h>
#include <EASTL/hash_map.h>
#include <EASTL/string.h>
#include <util/dag_string.h>


namespace drv3d_dx12::debug::call_stack::return_address
{
struct CommandData
{
#if DX12_HAS_CALLSTACK_EXT
  ext::Message msg;
#endif
  const void *returnAddress;
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
    G_ASSERTF(data.returnAddress,
      "%s had no returnAddress stored.\n"
      "Possible issues: Cmd wasn't created with the make_command function. \n"
      "STORE_RETURN_ADDRESS() call is missing on the d3d interface.",
      name);
  }
  void setCommandData(const null::CommandData &, const char *) {}
  const char *getLastCommandName() const { return lastCommandName; }
};

class Generator
{
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
  CommandData generateCommandData(uint32_t frame_index)
  {
    G_UNUSED(frame_index);
    return {
#if DX12_HAS_CALLSTACK_EXT
      extGenerator.captureExtMessage(frame_index),
#endif
      ScopedReturnAddressStore::get_threadlocal_saved_address()};
  }
};

class Reporter
{
  // cache of return address pointers to avoid looking it up over and over again
  eastl::hash_map<const void *, eastl::string> addressCache;
#if DX12_HAS_CALLSTACK_EXT
  ext::Resolver extResolver;
#endif

  const eastl::string &doResolve(const CommandData &data)
  {
    auto ref = addressCache.find(data.returnAddress);
    if (end(addressCache) == ref)
    {
      char strBuf[4096];
      auto name = stackhlp_get_call_stack(strBuf, sizeof(strBuf) - 1, &data.returnAddress, 1);
      strBuf[sizeof(strBuf) - 1] = '\0';
      ref = addressCache.emplace(data.returnAddress, name).first;
    }
    return ref->second;
  }

public:
  void report(const CommandData &data)
  {
    if (!data.returnAddress)
    {
      return;
    }
    logdbg(doResolve(data).c_str());
  }

  void append(String &buffer, const char *prefix, const CommandData &data)
  {
    if (!data.returnAddress)
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
} // namespace drv3d_dx12::debug::call_stack::return_address
