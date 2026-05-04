// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <driver.h>
#include <EASTL/unordered_map.h>
#include "command_list_identifier.h"


namespace drv3d_dx12::debug
{
// Possible improvement:
// Have a table of "active" buffers in addition to the table of all buffers.
// This may be needed with big numbers of command lists that are tracked.
template <typename I, typename T>
class CommandListStorage
{
  eastl::unordered_map<I, T> table;

public:
  T &beginList(I cmd)
  {
    auto ref = table.find(cmd);
    if (end(table) == ref)
    {
      ref = table.try_emplace(cmd).first;
    }
    return ref->second;
  }

  template <typename... Is>
  T &beginList(I cmd, Is &&...is)
  {
    auto ref = table.find(cmd);
    if (end(table) == ref)
    {
      ref = table.try_emplace(cmd, eastl::forward<Is>(is)...).first;
    }
    return ref->second;
  }

  template <typename C>
  T &beginListWithCallback(I cmd, C clb)
  {
    auto ref = table.find(cmd);
    if (end(table) == ref)
    {
      ref = table.try_emplace(cmd, clb(cmd)).first;
    }
    return ref->second;
  }

  void endList(I) {}

  T &getList(I cmd) { return table[cmd]; }
  const T *getOptionalList(I cmd) const
  {
    auto ref = table.find(cmd);
    return table.end() != ref ? &ref->second : nullptr;
  }

  template <typename C>
  void visitAll(C clb)
  {
    for (auto &kvp : table)
    {
      clb(kvp.first, kvp.second);
    }
  }

  void reset() { table.clear(); }
};
} // namespace drv3d_dx12::debug