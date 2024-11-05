// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string_view.h>


class DataBlock;
class String;

namespace drv3d_dx12::debug::call_stack::null
{
struct CommandData
{};

class ExecutionContextDataStore
{
public:
  constexpr CommandData getCommandData() const { return {}; }
  constexpr void setCommandData(const CommandData &, const char *) {}
  constexpr const char *getLastCommandName() const { return ""; }
};

class Generator
{
public:
  constexpr void configure(const DataBlock *) {}
  constexpr CommandData generateCommandData() const { return {}; }
};

class Reporter
{
public:
  constexpr void report(const CommandData &) {}

  constexpr void append(String &, const char *, const CommandData &) {}
  constexpr eastl::string_view resolve(const CommandData &) { return {}; }
};
} // namespace drv3d_dx12::debug::call_stack::null
