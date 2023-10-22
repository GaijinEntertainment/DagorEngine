#pragma once

#include "call_stack_null.h"
#include "call_stack_return_address.h"
#include "call_stack_full_stack.h"

#include <ioSys/dag_dataBlock.h>

namespace drv3d_dx12
{
namespace debug
{
namespace call_stack
{
namespace selectable
{
struct CommandData
{
  enum class ActiveMember
  {
    AS_NULL,
    AS_RETURN_ADDRESS,
    AS_FULL_STACK
  } activeMember;
  union
  {
    ::drv3d_dx12::debug::call_stack::null::CommandData asNull;
    ::drv3d_dx12::debug::call_stack::return_address::CommandData asReturnAddress;
    ::drv3d_dx12::debug::call_stack::full_stack::CommandData asFullStack;
  };

  static CommandData make(::drv3d_dx12::debug::call_stack::null::CommandData)
  {
    CommandData result{};
    result.activeMember = ActiveMember::AS_NULL;
    return result;
  }
  static CommandData make(const ::drv3d_dx12::debug::call_stack::return_address::CommandData &data)
  {
    CommandData result{};
    result.activeMember = ActiveMember::AS_RETURN_ADDRESS;
    result.asReturnAddress = data;
    return result;
  }
  static CommandData make(const ::drv3d_dx12::debug::call_stack::full_stack::CommandData &data)
  {
    CommandData result{};
    result.activeMember = ActiveMember::AS_FULL_STACK;
    result.asFullStack = data;
    return result;
  }
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

class Generator : protected ::drv3d_dx12::debug::call_stack::null::Generator,
                  protected ::drv3d_dx12::debug::call_stack::return_address::Generator,
                  protected ::drv3d_dx12::debug::call_stack::full_stack::Generator
{
  using NullBaseType = ::drv3d_dx12::debug::call_stack::null::Generator;
  using ReturnAddressBaseType = ::drv3d_dx12::debug::call_stack::return_address::Generator;
  using FullStackBaseType = ::drv3d_dx12::debug::call_stack::full_stack::Generator;

  CommandData::ActiveMember generatorMode = CommandData::ActiveMember::AS_NULL;

public:
  void configure(const DataBlock *api_config)
  {
    struct ModeNameTableEntry
    {
      const char *name;
      CommandData::ActiveMember mode;
    };

    static const ModeNameTableEntry modes[] =                             //
      {{"null", CommandData::ActiveMember::AS_NULL},                      //
        {"return_address", CommandData::ActiveMember::AS_RETURN_ADDRESS}, //
        {"full_stack", CommandData::ActiveMember::AS_FULL_STACK}};
    // return_address is the default mode, it is almost free to use and available
    const auto defaultMode = &modes[1];

    const char *modeName = "";
    const char *blockName = "";
    if (api_config)
    {
      blockName = api_config->getBlockName();

      auto debugBlock = api_config->getBlockByNameEx("debug");
      if (debugBlock)
      {
        auto callStackBlock = debugBlock->getBlockByNameEx("call_stack");
        if (callStackBlock)
        {
          modeName = callStackBlock->getStr("mode", defaultMode->name);
        }
      }
    }
    auto ref = eastl::find_if(eastl::begin(modes), eastl::end(modes), [modeName](auto &e) { return 0 == strcmp(modeName, e.name); });
    if (eastl::end(modes) != ref)
    {
      generatorMode = ref->mode;
      debug("DX12: debug::call_stack::selectable using mode '%s'", ref->name);
    }
    else
    {
      debug("DX12: debug::call_stack::selectable invalid mode '%s' from %s/debug/call_stack/mode:t=, using default %s", modeName,
        blockName, defaultMode->name);
      generatorMode = defaultMode->mode;
    }

    switch (generatorMode)
    {
      case CommandData::ActiveMember::AS_NULL: NullBaseType::configure(api_config); break;
      case CommandData::ActiveMember::AS_RETURN_ADDRESS: ReturnAddressBaseType::configure(api_config); break;
      case CommandData::ActiveMember::AS_FULL_STACK: FullStackBaseType::configure(api_config); break;
    }
  }
  CommandData generateCommandData()
  {
    switch (generatorMode)
    {
      case CommandData::ActiveMember::AS_NULL: return CommandData::make(NullBaseType::generateCommandData());
      case CommandData::ActiveMember::AS_RETURN_ADDRESS: return CommandData::make(ReturnAddressBaseType::generateCommandData());
      case CommandData::ActiveMember::AS_FULL_STACK: return CommandData::make(FullStackBaseType::generateCommandData());
    }
    return {};
  }
};

class Reporter : protected ::drv3d_dx12::debug::call_stack::null::Reporter,
                 protected ::drv3d_dx12::debug::call_stack::return_address::Reporter,
                 protected ::drv3d_dx12::debug::call_stack::full_stack::Reporter
{
  using NullBaseType = ::drv3d_dx12::debug::call_stack::null::Reporter;
  using ReturnAddressBaseType = ::drv3d_dx12::debug::call_stack::return_address::Reporter;
  using FullStackBaseType = ::drv3d_dx12::debug::call_stack::full_stack::Reporter;

public:
  void report(const CommandData &data)
  {
    switch (data.activeMember)
    {
      case CommandData::ActiveMember::AS_NULL: NullBaseType::report(data.asNull); break;
      case CommandData::ActiveMember::AS_RETURN_ADDRESS: ReturnAddressBaseType::report(data.asReturnAddress); break;
      case CommandData::ActiveMember::AS_FULL_STACK: FullStackBaseType::report(data.asFullStack); break;
    }
  }

  void append(String &buffer, const char *prefix, const CommandData &data)
  {
    switch (data.activeMember)
    {
      case CommandData::ActiveMember::AS_NULL: NullBaseType::append(buffer, prefix, data.asNull); break;
      case CommandData::ActiveMember::AS_RETURN_ADDRESS: ReturnAddressBaseType::append(buffer, prefix, data.asReturnAddress); break;
      case CommandData::ActiveMember::AS_FULL_STACK: FullStackBaseType::append(buffer, prefix, data.asFullStack); break;
    }
  }

  eastl::string_view resolve(const CommandData &data)
  {
    switch (data.activeMember)
    {
      case CommandData::ActiveMember::AS_NULL: return NullBaseType::resolve(data.asNull); break;
      case CommandData::ActiveMember::AS_RETURN_ADDRESS: return ReturnAddressBaseType::resolve(data.asReturnAddress); break;
      case CommandData::ActiveMember::AS_FULL_STACK: return FullStackBaseType::resolve(data.asFullStack); break;
    }
    return {};
  }
};
} // namespace selectable
} // namespace call_stack
} // namespace debug
} // namespace drv3d_dx12