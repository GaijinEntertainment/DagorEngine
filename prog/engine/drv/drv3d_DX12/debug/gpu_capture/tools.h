// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/variant.h>


namespace drv3d_dx12::debug
{
namespace gpu_capture
{
struct Issues
{
  /// Pix version 2501.30 breaks external heaps as soon the DLL is loaded and it can not be undone, so we can not use the feature
  bool brokenExistingHeaps : 1 = false;
};
} // namespace gpu_capture

namespace detail
{
// Note that Ts are tried to be constructed in Ts order, so final catch all should be last.
// Note NoT is the default state and represents the state when no of the Ts where constructed.
template <typename NoT, typename... Ts>
class ToolSet
{
public:
  using StorageType = eastl::variant<NoT, Ts...>;
  using DefaultType = NoT;

  template <typename... Is>
  static bool connect(StorageType &store, Is &&...is)
  {
    for (auto connector : {try_to_connect<Ts, Is...>...})
    {
      if (connector(is..., store))
      {
        return true;
      }
    }
    store = DefaultType{};
    return false;
  }

private:
  template <typename T, typename... Is>
  static bool try_to_connect(Is &&...is, StorageType &store)
  {
    return T::connect(is..., store);
  }
};
} // namespace detail
} // namespace drv3d_dx12::debug
