#pragma once

#include "call_stack.h"

namespace drv3d_dx12::debug::break_point
{
namespace core
{
class Controller : public call_stack::ExecutionContextDataStore, public debug::call_stack::Reporter
{
  eastl::vector<eastl::string> breakPoints;

  using BaseType = call_stack::ExecutionContextDataStore;
  bool isBreakPoint(const call_stack::CommandData &call_stack_info)
  {
    if (breakPoints.empty())
    {
      return false;
    }
    auto resolved = resolve(call_stack_info);
    for (auto &breakPoint : breakPoints)
    {
      auto ref = eastl::search(begin(resolved), end(resolved), begin(breakPoint), end(breakPoint));
      if (ref != end(resolved))
      {
        return true;
      }
    }
    return false;
  }

public:
  void breakNow() { __debugbreak(); }
  void setCommandData(const call_stack::CommandData &call_stack_info, const char *name)
  {
    BaseType::setCommandData(call_stack_info, name);
    if (isBreakPoint(call_stack_info))
    {
      breakNow();
    }
  }
  void addBreakPointString(eastl::string_view text) { breakPoints.emplace_back(begin(text), end(text)); }
  void removeBreakPointString(eastl::string_view text)
  {
    // The comparison operator implementations for string_view of eastl are completely broken, they have variants with
    // decay_t<string_view> with will incorrectly decay and result in operators with the same signature and mangled
    // name and so collide. So to avoid this, we have to cast it to string_view here.
    auto newEnd = eastl::remove_if(begin(breakPoints), end(breakPoints), [text](auto &bp) { return text == eastl::string_view{bp}; });
    breakPoints.erase(newEnd, end(breakPoints));
  }
};
} // namespace core
namespace null
{
class Controller : public call_stack::ExecutionContextDataStore, public debug::call_stack::Reporter
{
  using BaseType = call_stack::ExecutionContextDataStore;

public:
  void breakNow() {}
  void addBreakPointString(eastl::string_view) {}
  void removeBreakPointString(eastl::string_view) {}
};
} // namespace null

#if DAGOR_DBGLEVEL > 0
using Controller = ::drv3d_dx12::debug::break_point::core::Controller;
#else
using Controller = ::drv3d_dx12::debug::break_point::null::Controller;
#endif
} // namespace drv3d_dx12::debug::break_point