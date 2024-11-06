// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_log.h>
#include <EASTL/string_view.h>
#include <drv_returnAddrStore.h>


class DataBlock;
class String;

#if COMMANDS_STORE_RETURN_ADDRESS

#include <osApiWrappers/dag_stackHlp.h>
#include <EASTL/hash_map.h>
#include <EASTL/string.h>
#include <util/dag_string.h>

namespace drv3d_dx12::debug::call_stack::return_address
{
struct CommandData
{
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
  const char *getLastCommandName() const { return lastCommandName; }
};

class Generator
{
public:
  constexpr void configure(const DataBlock *) {}
  CommandData generateCommandData() const { return {ScopedReturnAddressStore::get_threadlocal_saved_address()}; }
};

class Reporter
{
  // cache of return address pointers to avoid looking it up over and over again
  eastl::hash_map<const void *, eastl::string> addressCache;

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

  eastl::string_view resolve(const CommandData &data) { return doResolve(data); }
};
} // namespace drv3d_dx12::debug::call_stack::return_address

#else

namespace drv3d_dx12::debug::call_stack::return_address
{
struct CommandData
{};

class ExecutionContextDataStore
{
public:
  constexpr CommandData getCommandData() const { return {}; }
  constexpr void setCommandData(const CommandData &, const char *) {}
};

class Generator
{
public:
  void configure(const DataBlock *)
  {
    logdbg("DX12: debug::call_stack::return_address using null implementation! No return addresses are available!");
  }
  constexpr CommandData generateCommandData() const { return {}; }
  constexpr const char *getLastCommandName() const { return ""; }
};

class Reporter
{
public:
  constexpr void report(const CommandData &) {}
  constexpr void append(String &, const char *, const CommandData &) {}
  constexpr eastl::string_view resolve(const CommandData &) { return {}; }
};
} // namespace drv3d_dx12::debug::call_stack::return_address

#endif
