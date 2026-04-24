// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "pipeline.h"

#include <atomic>
#include <generic/dag_objectPool.h>


namespace drv3d_dx12
{
inline constexpr uint32_t heap_size = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT / sizeof(uint64_t);
class Device;

class Query
{
public:
  enum class Qtype
  {
    UNDEFINED,
    TIMESTAMP,
    SURVEY,
    VISIBILITY
  };

private:
  uint64_t result = 0;
  uint64_t id = 0;
  enum class State
  {
    ISSUED,
    FINALIZED,
  };
  std::atomic<State> state{State::FINALIZED};

public:
  void setIssued() { state.store(State::ISSUED, std::memory_order_relaxed); }
  void update(uint64_t value)
  {
    result = value;
    state.store(State::FINALIZED, std::memory_order_seq_cst);
  }
  bool isFinalized() const { return state.load(std::memory_order_seq_cst) == State::FINALIZED; }
  bool isIssued() const { return state.load(std::memory_order_seq_cst) == State::ISSUED; }
  uint64_t getValue() const { return result; }
  uint64_t getId() const { return id; }
  uint64_t getIndex() const { return id >> 2; }
  Qtype getType() const { return static_cast<Qtype>(id & 3); } // last 2 bits
  bool hasReadBack() const { return (id % 2 == 1); }
  void setId(uint64_t _id, Qtype type) { id = (_id << 2) | static_cast<uint64_t>(type); }
};

struct PredicateInfo
{
  ID3D12QueryHeap *heap = nullptr;
  BufferReference buffer;
  uint32_t index = 0;
  uint32_t bufferOffset() const { return index * sizeof(uint64_t); }
};

class FrontendQueryManager
{
  struct HeapPredicate
  {
    ComPtr<ID3D12QueryHeap> heap;
    BufferState buffer;
    Query *qArr[heap_size]{};

    bool hasAnyFree() const
    {
      return eastl::end(qArr) != eastl::find_if(eastl::begin(qArr), eastl::end(qArr), [](const auto p) { return nullptr == p; });
    }

    uint32_t addQuery(Query *qry)
    {
      auto ref = eastl::find_if(eastl::begin(qArr), eastl::end(qArr), [](const auto p) { return nullptr == p; });
      G_ASSERT(ref != eastl::end(qArr));
      if (ref != eastl::end(qArr))
      {
        *ref = qry;
        return eastl::distance(eastl::begin(qArr), ref);
      }
      return ~uint32_t(0);
    }

    bool posFree(uint32_t pos) const { return pos < heap_size ? (qArr[pos] == nullptr) : false; }
  };

  dag::Vector<HeapPredicate> predicateHeaps;
  WinCritSec predicateGuard;

  ObjectPool<Query> queryPool;
  WinCritSec queryGuard;

  dag::Vector<HeapPredicate>::iterator newPredicateHeap(Device &device, ID3D12Device *dx_device);

public:
  uint64_t createPredicate(Device &device, ID3D12Device *dx_device);
  void deletePredicate(uint64_t id);
  void shutdownPredicate(DeviceContext &ctx);
  PredicateInfo getPredicateInfo(Query *query);

  Query *newQuery();
  Query *getQueryPtrFromId(uint64_t id);
  void deleteQuery(Query *query_ptr);
  void removeDeletedQueries(const dag::Vector<Query *> &deletedQueries);

  void preRecovery();
  bool postRecovery(Device &device, ID3D12Device *dx_device);
};

class BackendQueryManager
{
private:
  struct HeapTimeStampVisibility
  {
    ComPtr<ID3D12QueryHeap> heap;
    Bitset<heap_size> freeMask = Bitset<heap_size>().set();
    ComPtr<ID3D12Resource> readBackBuffer;
    uint64_t *mappedMemory = nullptr;
  };

  struct QueryFlushMapping
  {
    Query *target = nullptr;
    uint64_t *result = nullptr;
  };

  struct VisibilityFlushMapping
  {
    Query *target = nullptr;
    uint64_t *result = nullptr;
    ID3D12QueryHeap *heap = nullptr;
    uint32_t heapIndex = 0;
  };

  dag::Vector<HeapTimeStampVisibility> timeStampHeaps;
  dag::Vector<HeapTimeStampVisibility> visibilityHeaps;
  dag::Vector<QueryFlushMapping> tsFlushes;
  dag::Vector<VisibilityFlushMapping> visFlushes;

  HeapTimeStampVisibility *newTimeStampVisibilityHeap(D3D12_QUERY_HEAP_TYPE type, ID3D12Device *device);
  void heapResolve(ID3D12GraphicsCommandList *target, D3D12_QUERY_TYPE type, HeapTimeStampVisibility &heap);

public:
  void makeVisibilityQuery(Query *query_ptr, ID3D12Device *device, ID3D12GraphicsCommandList *cmd);
  void endVisibilityQuery(Query *query_ptr, ID3D12GraphicsCommandList *cmd);
  void makeTimeStampQuery(Query *query_ptr, ID3D12Device *device, ID3D12GraphicsCommandList *cmd);
  void cancelQuery(Query *query_ptr);
  void resolve(ID3D12GraphicsCommandList *target);
  void flush();
  void shutdown();
};

inline void FrontendQueryManager::deletePredicate(uint64_t id)
{
  WinAutoLock lock(predicateGuard);
  uint32_t heapNum = id / heap_size;
  uint32_t slotNum = id % heap_size;
  auto &heap = predicateHeaps[heapNum];
  heap.qArr[slotNum] = nullptr;
  deleteQuery(getQueryPtrFromId(id));
}

inline PredicateInfo FrontendQueryManager::getPredicateInfo(Query *query)
{
  if (!query)
    return {};

  uint64_t id = query->getIndex();
  uint32_t heapNum = id / heap_size;
  uint32_t slotNum = id % heap_size;
  auto &heap = predicateHeaps[heapNum];

  return {
    .heap = heap.heap.Get(),
    .buffer = heap.buffer,
    .index = slotNum,
  };
}

inline Query *FrontendQueryManager::newQuery()
{
  WinAutoLock lock(queryGuard);
  Query *q = queryPool.allocate();
  return q;
}

inline Query *FrontendQueryManager::getQueryPtrFromId(uint64_t id)
{
  if (static_cast<Query::Qtype>(id & 3) == Query::Qtype::SURVEY)
  {
    id = id >> 2;
    uint32_t heapNum = id / heap_size;
    uint32_t slotNum = id % heap_size;
    return predicateHeaps[heapNum].qArr[slotNum];
  }
  return nullptr;
}

inline void FrontendQueryManager::deleteQuery(Query *query_ptr) { queryPool.free(query_ptr); }

inline void FrontendQueryManager::removeDeletedQueries(const dag::Vector<Query *> &deletedQueries)
{
  WinAutoLock l(queryGuard);
  for (auto &&ts : deletedQueries)
  {
    queryPool.free(ts);
  }
}

inline void FrontendQueryManager::preRecovery()
{
  queryPool.iterateAllocated([](auto ts) { ts->update(0); });
  for (auto &&heap : predicateHeaps)
    heap.heap.Reset();
}

inline BackendQueryManager::HeapTimeStampVisibility *BackendQueryManager::newTimeStampVisibilityHeap(D3D12_QUERY_HEAP_TYPE type,
  ID3D12Device *device)
{
  uint32_t READ_BACK_BUFFER_SIZE = heap_size * sizeof(uint64_t);
  D3D12_QUERY_HEAP_DESC heapDesc{
    .Type = type,
    .Count = heap_size,
    .NodeMask = 0,
  };
  HeapTimeStampVisibility newHeap;
  if (!DX12_CHECK_OK(device->CreateQueryHeap(&heapDesc, COM_ARGS(&newHeap.heap))))
  {
    return nullptr;
  }

  D3D12_HEAP_PROPERTIES bufferHeap{
    .Type = D3D12_HEAP_TYPE_READBACK,
    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
    .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
    .CreationNodeMask = 0,
    .VisibleNodeMask = 0,
  };

  D3D12_RESOURCE_DESC bufferDesc{
    .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
    .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
    .Width = READ_BACK_BUFFER_SIZE,
    .Height = 1,
    .DepthOrArraySize = 1,
    .MipLevels = 1,
    .Format = DXGI_FORMAT_UNKNOWN,
    .SampleDesc{
      .Count = 1,
      .Quality = 0,
    },
    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
    .Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,
  };
  if (!DX12_CHECK_OK(device->CreateCommittedResource(&bufferHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, COM_ARGS(&newHeap.readBackBuffer))))
  {
    return nullptr;
  }

  D3D12_RANGE range{};
  if (!DX12_CHECK_OK(newHeap.readBackBuffer->Map(0, &range, reinterpret_cast<void **>(&newHeap.mappedMemory))))
  {
    return nullptr;
  }
  if (type == D3D12_QUERY_HEAP_TYPE_OCCLUSION)
  {
    return &visibilityHeaps.emplace_back(eastl::move(newHeap));
  }
  else
  {
    return &timeStampHeaps.emplace_back(eastl::move(newHeap));
  }
}

inline void BackendQueryManager::heapResolve(ID3D12GraphicsCommandList *target, D3D12_QUERY_TYPE type,
  BackendQueryManager::HeapTimeStampVisibility &heap)
{
  size_t base = 0;

  for (size_t i : heap.freeMask)
  {
    if (base < i)
    {
      target->ResolveQueryData(heap.heap.Get(), type, base, i - base, heap.readBackBuffer.Get(), 0);
    }
    base = i + 1;
  }

  if (base < heap_size)
    target->ResolveQueryData(heap.heap.Get(), type, base, heap_size - base, heap.readBackBuffer.Get(), 0);
}

inline void BackendQueryManager::makeVisibilityQuery(Query *query_ptr, ID3D12Device *device, ID3D12GraphicsCommandList *cmd)
{
  HeapTimeStampVisibility *ptr = nullptr;
  // find free position in heaps vector
  auto ref = eastl::find_if(begin(visibilityHeaps), end(visibilityHeaps),
    [](const HeapTimeStampVisibility &visibilityHeap) { return visibilityHeap.freeMask.any(); });
  if (ref == end(visibilityHeaps))
  {
    // need to create new heap
    ptr = newTimeStampVisibilityHeap(D3D12_QUERY_HEAP_TYPE_OCCLUSION, device);
  }
  else
  {
    ptr = &*ref;
  }
  if (!ptr)
  {
    return;
  }
  uint32_t slotIndex = ptr->freeMask.find_first_and_reset();
  cmd->BeginQuery(ptr->heap.Get(), D3D12_QUERY_TYPE_OCCLUSION, slotIndex);
  ptrdiff_t heap_num = ptr - visibilityHeaps.data();
  query_ptr->setId(heap_size * heap_num + slotIndex, Query::Qtype::VISIBILITY);

  visFlushes.push_back({
    .target = query_ptr,
    .result = ptr->mappedMemory + slotIndex,
    .heap = ptr->heap.Get(),
    .heapIndex = slotIndex,
  });
}

inline void BackendQueryManager::endVisibilityQuery(Query *target, ID3D12GraphicsCommandList *cmd)
{
  uint32_t heapNum = target->getIndex() / heap_size;
  uint32_t slotNum = target->getIndex() % heap_size;
  cmd->EndQuery(visibilityHeaps[heapNum].heap.Get(), D3D12_QUERY_TYPE_OCCLUSION, slotNum);
}

inline void BackendQueryManager::makeTimeStampQuery(Query *query_ptr, ID3D12Device *device, ID3D12GraphicsCommandList *cmd)
{
  HeapTimeStampVisibility *ptr = nullptr;
  // find free position in heaps vector
  auto ref = eastl::find_if(begin(timeStampHeaps), end(timeStampHeaps),
    [](const HeapTimeStampVisibility &timeStampHeap) { return timeStampHeap.freeMask.any(); });
  if (ref == end(timeStampHeaps))
  {
    // need to create new heap
    ptr = newTimeStampVisibilityHeap(D3D12_QUERY_HEAP_TYPE_TIMESTAMP, device);
  }
  else
  {
    ptr = &*ref;
  }
  if (!ptr)
  {
    return;
  }
  auto slotIndex = ptr->freeMask.find_first_and_reset();
  cmd->EndQuery(ptr->heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, slotIndex);
  auto heap_num = ptr - timeStampHeaps.data();
  query_ptr->setId(heap_size * heap_num + slotIndex, Query::Qtype::TIMESTAMP);

  tsFlushes.push_back({
    .target = query_ptr,
    .result = ptr->mappedMemory + slotIndex,
  });
}

inline void BackendQueryManager::cancelQuery(Query *query_ptr)
{
  auto removeQuery = [query_ptr](auto &vec) {
    for (auto it = vec.begin(), end = vec.end(); it != end;)
    {
      if (it->target == query_ptr)
      {
        *it = *--end;
        vec.pop_back();
        continue;
      }

      it++;
    }
  };

  removeQuery(tsFlushes);
  removeQuery(visFlushes);
}

inline void BackendQueryManager::resolve(ID3D12GraphicsCommandList *target)
{
  for (auto &heap : visibilityHeaps)
  {
    heapResolve(target, D3D12_QUERY_TYPE_OCCLUSION, heap);
  }
  for (auto &heap : timeStampHeaps)
  {
    heapResolve(target, D3D12_QUERY_TYPE_TIMESTAMP, heap);
  }
}

inline void BackendQueryManager::flush()
{
  D3D12_RANGE emptyRange{};
  D3D12_RANGE fullRange{0, heap_size * sizeof(uint64_t)};
  for (auto &&heap : timeStampHeaps)
  {
    void *data = nullptr;
    if (DX12_CHECK_OK(heap.readBackBuffer->Map(0, &fullRange, &data)))
    {
      heap.readBackBuffer->Unmap(0, &emptyRange);
    }
    heap.freeMask.set();
  }
  for (auto &&heap : visibilityHeaps)
  {
    void *data = nullptr;
    if (DX12_CHECK_OK(heap.readBackBuffer->Map(0, &fullRange, &data)))
    {
      heap.readBackBuffer->Unmap(0, &emptyRange);
    }
    heap.freeMask.set();
  }
  for (auto &&flush : tsFlushes)
  {
    flush.target->update(*flush.result);
  }
  for (auto &&flush : visFlushes)
  {
    flush.target->update(*flush.result);
  }
  visFlushes.clear();
  tsFlushes.clear();
}

inline void BackendQueryManager::shutdown()
{
  visFlushes.clear();
  tsFlushes.clear();
  visibilityHeaps.clear();
  timeStampHeaps.clear();
}

} // namespace drv3d_dx12