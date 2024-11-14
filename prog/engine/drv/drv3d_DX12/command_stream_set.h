// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "versioned_com_ptr.h"
#include "d3d12_error_handling.h"

#include <dag/dag_vector.h>


namespace drv3d_dx12
{
template <typename>
struct GetSmartPointerOfCommandBufferPointer;

template <typename... As>
struct GetSmartPointerOfCommandBufferPointer<VersionedPtr<As...>>
{
  using Type = VersionedComPtr<As...>;
};

template <typename CommandListResultType, D3D12_COMMAND_LIST_TYPE CommandListTypeName,
  typename CommandListStoreType = typename GetSmartPointerOfCommandBufferPointer<CommandListResultType>::Type>
struct CommandStreamSet
{
  ComPtr<ID3D12CommandAllocator> pool;
  dag::Vector<CommandListStoreType> lists;
  uint32_t listsInUse = 0;

  void init(ID3D12Device *device) { DX12_CHECK_RESULT(device->CreateCommandAllocator(CommandListTypeName, COM_ARGS(&pool))); }
  CommandListResultType allocateList(ID3D12Device *device)
  {
    CommandListResultType result = {};
    if (!pool)
    {
      return result;
    }
    if (listsInUse < lists.size())
    {
      result = lists[listsInUse++];
      result->Reset(pool.Get(), nullptr);
    }
    else
    {
      CommandListStoreType newList;
      if (newList.autoQuery([=](auto uuid, auto ptr) //
            { return DX12_DEBUG_OK(device->CreateCommandList(0, CommandListTypeName, pool.Get(), nullptr, uuid, ptr)); }))
      {
        lists.push_back(eastl::move(newList));
        result = lists[listsInUse++];
      }
      else
      {
        D3D_ERROR("DX12: Unable to allocate new command list");
        // can only happen when all CreateCommandList failed and this can only happen when the device was reset.
      }
    }
    return result;
  }
  void frameReset()
  {
    DX12_CHECK_RESULT(pool->Reset());
    listsInUse = 0;
  }
  void shutdown()
  {
    listsInUse = 0;
    lists.clear();
    pool.Reset();
  }
};
} // namespace drv3d_dx12
