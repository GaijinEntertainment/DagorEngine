// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unordered_map.h>

#include <driver.h>


namespace drv3d_dx12::debug
{
// Possible improvement:
// Have a table of "active" buffers in addition to the table of all buffers.
// This may be needed with big numbers of command lists that are tracked.
template <typename T>
class CommandListStorage
{
  eastl::unordered_map<ID3D12GraphicsCommandList *, T> table;

public:
  T &beginList(ID3D12GraphicsCommandList *cmd)
  {
    auto ref = table.find(cmd);
    if (end(table) == ref)
    {
      ref = table.try_emplace(cmd).first;
    }
    return ref->second;
  }

  template <typename... Is>
  T &beginList(ID3D12GraphicsCommandList *cmd, Is &&...is)
  {
    auto ref = table.find(cmd);
    if (end(table) == ref)
    {
      ref = table.try_emplace(cmd, eastl::forward<Is>(is)...).first;
    }
    return ref->second;
  }

  template <typename C>
  const T &beginListWithCallback(ID3D12GraphicsCommandList *cmd, C clb)
  {
    auto ref = table.find(cmd);
    if (end(table) == ref)
    {
      ref = table.try_emplace(cmd, clb(cmd)).first;
    }
    return ref->second;
  }

  void endList(ID3D12GraphicsCommandList *) {}

  T &getList(ID3D12GraphicsCommandList *cmd) { return table[cmd]; }

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