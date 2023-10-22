#pragma once

namespace drv3d_dx12
{
namespace debug
{
namespace call_stack
{
namespace null
{
struct CommandData
{};

class ExecutionContextDataStore
{
public:
  CommandData getCommandData() const { return {}; }
  void setCommandData(const CommandData &, const char *) {}
  const char *getLastCommandName() const { return ""; }
};

class Generator
{
public:
  void configure(const DataBlock *) {}
  CommandData generateCommandData() const { return {}; }
};

class Reporter
{
public:
  void report(const CommandData &) {}

  void append(String &, const char *, const CommandData &) {}
  eastl::string_view resolve(const CommandData &) { return {}; }
};
} // namespace null
} // namespace call_stack
} // namespace debug
} // namespace drv3d_dx12