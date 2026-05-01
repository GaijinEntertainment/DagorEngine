// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "pipeline.h"

#include <atomic>
#include <generic/dag_objectPool.h>
#include <osApiWrappers/dag_critSec.h>

namespace drv3d_dx12
{
constexpr uint32_t read_back_buffer_size = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
constexpr uint32_t heap_size = read_back_buffer_size / sizeof(uint64_t);
constexpr uint32_t pipeline_stats_heap_size = read_back_buffer_size / sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);

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

inline D3D12_QUERY_DATA_PIPELINE_STATISTICS &operator+=(D3D12_QUERY_DATA_PIPELINE_STATISTICS &lhs,
  const D3D12_QUERY_DATA_PIPELINE_STATISTICS &rhs)
{
  lhs.IAVertices += rhs.IAVertices;
  lhs.IAPrimitives += rhs.IAPrimitives;
  lhs.VSInvocations += rhs.VSInvocations;
  lhs.GSInvocations += rhs.GSInvocations;
  lhs.GSPrimitives += rhs.GSPrimitives;
  lhs.CInvocations += rhs.CInvocations;
  lhs.CPrimitives += rhs.CPrimitives;
  lhs.PSInvocations += rhs.PSInvocations;
  lhs.HSInvocations += rhs.HSInvocations;
  lhs.DSInvocations += rhs.DSInvocations;
  lhs.CSInvocations += rhs.CSInvocations;
  return lhs;
}

class PipelineStatsQuery
{
public:
  using ResultType = D3D12_QUERY_DATA_PIPELINE_STATISTICS;

  void setIssued()
  {
    resetValue();
    G_ASSERT(state.exchange(State::ISSUED, std::memory_order_acq_rel) == State::FINALIZED);
  }
  void setFinalized() { state.store(State::FINALIZED, std::memory_order_release); }

  void accumulate(ResultType value)
  {
    G_ASSERT(!isFinalized());
    result += value;
  }

  bool isIssued() const { return state.load(std::memory_order_acquire) == State::ISSUED; }
  bool isFinalized() const { return state.load(std::memory_order_acquire) == State::FINALIZED; }

  ResultType getValue() const { return result; }
  auto getValue(auto member) const { return result.*member; }

  void resetValue() { result = {}; }

private:
  enum class State
  {
    ISSUED,
    FINALIZED,
  };

  std::atomic<State> state{State::FINALIZED};
  ResultType result{};
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

  ObjectPool<PipelineStatsQuery> pipelineStatsQueryPool;
  ObjectPool<Query> queryPool;
  WinCritSec queryGuard;

  dag::Vector<HeapPredicate>::iterator newPredicateHeap(Device &device, ID3D12Device *dx_device);

public:
  uint64_t createPredicate(Device &device, ID3D12Device *dx_device);
  void deletePredicate(uint64_t id);
  void shutdownPredicate(DeviceContext &ctx);
  PredicateInfo getPredicateInfo(Query *query);

  Query *newQuery();
  PipelineStatsQuery *newPipelineStatsQuery();
  Query *getQueryPtrFromId(uint64_t id);
  void deleteQuery(Query *query_ptr);
  void removeDeletedQueries(dag::ConstSpan<Query *> deleted_queries);
  void removeDeletedPipelineStatsQueries(dag::ConstSpan<PipelineStatsQuery *> deleted_queries);

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

  struct HeapPipelineStatistics
  {
    ComPtr<ID3D12QueryHeap> heap;
    Bitset<pipeline_stats_heap_size> freeMask = Bitset<pipeline_stats_heap_size>().set();
    ComPtr<ID3D12Resource> readBackBuffer;
    D3D12_QUERY_DATA_PIPELINE_STATISTICS *mappedMemory = nullptr;
  };

  struct PipelineStatsFlushMapping
  {
    PipelineStatsQuery *target = nullptr;
    D3D12_QUERY_DATA_PIPELINE_STATISTICS *result = nullptr;
    ID3D12QueryHeap *heap = nullptr;
    uint32_t heapIndex = 0;
  };

  dag::Vector<HeapTimeStampVisibility> timeStampHeaps;
  dag::Vector<HeapTimeStampVisibility> visibilityHeaps;
  dag::Vector<HeapPipelineStatistics> pipelineStatsHeaps;
  dag::Vector<QueryFlushMapping> tsFlushes;
  dag::Vector<VisibilityFlushMapping> visFlushes;
  dag::Vector<PipelineStatsFlushMapping> pipelineStatsFlushes;

  struct PipelineStatsQueryBackendState
  {
    PipelineStatsQuery *frontend = nullptr;
    bool suspended = false;
    uint64_t index = 0;
  };
  // Will be suspended on flush and resumed on next prepareCommandExecution (unless it has already completed by then)
  dag::Vector<PipelineStatsQueryBackendState> currentPipelineStatsQueries;
  // Finished either at the end of the frame or upon the user's request
  dag::Vector<PipelineStatsQuery *> finishedPipelineStatsQueries;

  HeapTimeStampVisibility *newTimeStampVisibilityHeap(D3D12_QUERY_HEAP_TYPE type, ID3D12Device *device);
  HeapPipelineStatistics *newPipelineStatisticsHeap(ID3D12Device *device);
  template <typename T>
  void heapResolve(ID3D12GraphicsCommandList *target, D3D12_QUERY_TYPE type, T &heap);

public:
  void makeVisibilityQuery(Query *query_ptr, ID3D12Device *device, ID3D12GraphicsCommandList *cmd);
  void endVisibilityQuery(Query *query_ptr, ID3D12GraphicsCommandList *cmd);
  void makeTimeStampQuery(Query *query_ptr, ID3D12Device *device, ID3D12GraphicsCommandList *cmd);
  void makePipelineStatsQuery(PipelineStatsQuery *query_ptr, ID3D12Device *device, ID3D12GraphicsCommandList *cmd);
  void endPipelineStatsQuery(PipelineStatsQuery *query_ptr, ID3D12GraphicsCommandList *cmd);
  void cancelQuery(Query *query_ptr);
  void cancelPipelineStatsQuery(PipelineStatsQuery *query_ptr);
  void resolve(ID3D12GraphicsCommandList *target);
  // Suspend all active pipeline statistics queries before the command list is closed
  void suspendActiveQueries(ID3D12GraphicsCommandList *cmd);
  // Resume all suspended pipeline statistics queries in prepareCommandExecution
  void resumePipelineStatsQueries(ID3D12GraphicsCommandList *cmd, ID3D12Device *device);
  // Finish all active pipeline statistics queries
  void finishActivePipelineStatsQueries();
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

inline PipelineStatsQuery *FrontendQueryManager::newPipelineStatsQuery()
{
  WinAutoLock lock(queryGuard);
  PipelineStatsQuery *q = pipelineStatsQueryPool.allocate();
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

inline void FrontendQueryManager::removeDeletedQueries(dag::ConstSpan<Query *> deleted_queries)
{
  WinAutoLock l(queryGuard);
  for (auto &&ts : deleted_queries)
  {
    queryPool.free(ts);
  }
}

inline void FrontendQueryManager::removeDeletedPipelineStatsQueries(dag::ConstSpan<PipelineStatsQuery *> deleted_queries)
{
  WinAutoLock l(queryGuard);
  for (auto &&dq : deleted_queries)
  {
    pipelineStatsQueryPool.free(dq);
  }
}

inline void FrontendQueryManager::preRecovery()
{
  queryPool.iterateAllocated([](auto ts) { ts->update(0); });
  pipelineStatsQueryPool.iterateAllocated([](auto ps) {
    ps->resetValue();
    ps->setFinalized();
  });
  for (auto &&heap : predicateHeaps)
    heap.heap.Reset();
}

inline BackendQueryManager::HeapTimeStampVisibility *BackendQueryManager::newTimeStampVisibilityHeap(D3D12_QUERY_HEAP_TYPE type,
  ID3D12Device *device)
{
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
    .Width = read_back_buffer_size,
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

template <typename T>
inline void BackendQueryManager::heapResolve(ID3D12GraphicsCommandList *target, D3D12_QUERY_TYPE type, T &heap)
{
  constexpr size_t heapSz = decltype(heap.freeMask)::kSize;
  size_t base = 0;

  for (size_t i : heap.freeMask)
  {
    if (base < i)
    {
      target->ResolveQueryData(heap.heap.Get(), type, base, i - base, heap.readBackBuffer.Get(), 0);
    }
    base = i + 1;
  }

  if (base < heapSz)
    target->ResolveQueryData(heap.heap.Get(), type, base, heapSz - base, heap.readBackBuffer.Get(), 0);
}

inline BackendQueryManager::HeapPipelineStatistics *BackendQueryManager::newPipelineStatisticsHeap(ID3D12Device *device)
{
  uint32_t READ_BACK_BUFFER_SIZE = pipeline_stats_heap_size * sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);
  D3D12_QUERY_HEAP_DESC heapDesc{
    .Type = D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS,
    .Count = pipeline_stats_heap_size,
    .NodeMask = 0,
  };
  HeapPipelineStatistics newHeap;
  if (!DX12_CHECK_OK(device->CreateQueryHeap(&heapDesc, COM_ARGS(&newHeap.heap))))
  {
    return nullptr;
  }

  D3D12_HEAP_PROPERTIES bufferHeapProps{
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
  if (!DX12_CHECK_OK(device->CreateCommittedResource(&bufferHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, COM_ARGS(&newHeap.readBackBuffer))))
  {
    return nullptr;
  }

  D3D12_RANGE range{};
  if (!DX12_CHECK_OK(newHeap.readBackBuffer->Map(0, &range, reinterpret_cast<void **>(&newHeap.mappedMemory))))
  {
    return nullptr;
  }
  return &pipelineStatsHeaps.emplace_back(eastl::move(newHeap));
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
    // almost impossible
    G_ASSERT_FAIL("DX12: unable to create visibility query heap");
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
    // almost impossible
    G_ASSERT_FAIL("DX12: unable to create timestamp query heap");
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

inline void BackendQueryManager::makePipelineStatsQuery(PipelineStatsQuery *query_ptr, ID3D12Device *device,
  ID3D12GraphicsCommandList *cmd)
{
  HeapPipelineStatistics *ptr = nullptr;
  G_ASSERT(!query_ptr->isFinalized());
  G_ASSERT(eastl::find_if(currentPipelineStatsQueries.begin(), currentPipelineStatsQueries.end(),
             [query_ptr](const auto &state) { return state.frontend == query_ptr; }) == currentPipelineStatsQueries.end());

  auto ref = eastl::find_if(begin(pipelineStatsHeaps), end(pipelineStatsHeaps),
    [](const HeapPipelineStatistics &h) { return h.freeMask.any(); });
  if (ref == end(pipelineStatsHeaps))
  {
    ptr = newPipelineStatisticsHeap(device);
  }
  else
  {
    ptr = &*ref;
  }
  if (!ptr)
  {
    // almost impossible
    G_ASSERT_FAIL("DX12: unable to create pipeline statistics query heap");
    return;
  }

  uint32_t slotIndex = ptr->freeMask.find_first_and_reset();
  cmd->BeginQuery(ptr->heap.Get(), D3D12_QUERY_TYPE_PIPELINE_STATISTICS, slotIndex);
  auto heap_num = ptr - pipelineStatsHeaps.data();
  uint64_t id = pipeline_stats_heap_size * heap_num + slotIndex;

  pipelineStatsFlushes.push_back({
    .target = query_ptr,
    .result = ptr->mappedMemory + slotIndex,
    .heap = ptr->heap.Get(),
    .heapIndex = slotIndex,
  });

  currentPipelineStatsQueries.push_back({query_ptr, false, id});
}

inline void BackendQueryManager::endPipelineStatsQuery(PipelineStatsQuery *target, ID3D12GraphicsCommandList *cmd)
{
  auto &active = currentPipelineStatsQueries;
  auto it = eastl::find_if(active.begin(), active.end(), [target](const auto &state) { return state.frontend == target; });
  if (it == active.end())
  {
    return;
  }
  G_ASSERT(it->suspended == false); // sanity check

  auto index = it->index;
  uint32_t heapNum = index / pipeline_stats_heap_size;
  uint32_t slotNum = index % pipeline_stats_heap_size;
  cmd->EndQuery(pipelineStatsHeaps[heapNum].heap.Get(), D3D12_QUERY_TYPE_PIPELINE_STATISTICS, slotNum);

  active.erase_unsorted(it);

  finishedPipelineStatsQueries.push_back(target);
}

inline void BackendQueryManager::cancelQuery(Query *query_ptr)
{
  auto removeQuery = [query_ptr](auto &vec) {
    auto it = eastl::remove_if(vec.begin(), vec.end(), [query_ptr](const auto &flush) { return flush.target == query_ptr; });
    vec.erase(it, vec.end());
  };

  removeQuery(tsFlushes);
  removeQuery(visFlushes);
}

inline void BackendQueryManager::cancelPipelineStatsQuery(PipelineStatsQuery *query_ptr)
{
  // By design, each vector does not contain duplicate pointers, so (remove_if, erase) idiom is ambiguous

  if (auto itFlush = eastl::find_if(pipelineStatsFlushes.begin(), pipelineStatsFlushes.end(),
        [query_ptr](const auto &flush) { return flush.target == query_ptr; });
      itFlush != pipelineStatsFlushes.end())
  {
    pipelineStatsFlushes.erase_unsorted(itFlush);
  }

  if (auto itCurrent = eastl::find_if(currentPipelineStatsQueries.begin(), currentPipelineStatsQueries.end(),
        [query_ptr](const auto &current) { return current.frontend == query_ptr; });
      itCurrent != currentPipelineStatsQueries.end())
  {
    currentPipelineStatsQueries.erase_unsorted(itCurrent);
  }

  if (auto itFinished = eastl::find(finishedPipelineStatsQueries.begin(), finishedPipelineStatsQueries.end(), query_ptr);
      itFinished != finishedPipelineStatsQueries.end())
  {
    finishedPipelineStatsQueries.erase_unsorted(itFinished);
  }
}

inline void BackendQueryManager::suspendActiveQueries(ID3D12GraphicsCommandList *cmd)
{
  for (auto &[frontend, suspended, index] : currentPipelineStatsQueries)
  {
    G_ASSERT(suspended == false); // sanity check
    uint32_t heapNum = index / pipeline_stats_heap_size;
    uint32_t slotNum = index % pipeline_stats_heap_size;
    cmd->EndQuery(pipelineStatsHeaps[heapNum].heap.Get(), D3D12_QUERY_TYPE_PIPELINE_STATISTICS, slotNum);
    suspended = true;
  }
}

inline void BackendQueryManager::resumePipelineStatsQueries(ID3D12GraphicsCommandList *cmd, ID3D12Device *device)
{
  auto it = currentPipelineStatsQueries.begin();
  auto end = currentPipelineStatsQueries.end();
  for (; it != end; ++it)
  {
    auto &[frontend, suspended, index] = *it;
    auto ref = eastl::find_if(pipelineStatsHeaps.begin(), pipelineStatsHeaps.end(),
      [](const HeapPipelineStatistics &h) { return h.freeMask.any(); });
    G_ASSERT(suspended == true); // sanity check

    HeapPipelineStatistics *ptr = nullptr;
    if (ref == pipelineStatsHeaps.end())
    {
      ptr = newPipelineStatisticsHeap(device);
    }
    else
    {
      ptr = &*ref;
    }
    if (!ptr)
    {
      G_ASSERT(false); // this should not happen, because flush happens before this call
      it->frontend->setFinalized();
      *it = *--end;
      currentPipelineStatsQueries.pop_back();
      continue;
    }

    uint32_t slotIndex = ptr->freeMask.find_first_and_reset();
    cmd->BeginQuery(ptr->heap.Get(), D3D12_QUERY_TYPE_PIPELINE_STATISTICS, slotIndex);
    auto heap_num = ptr - pipelineStatsHeaps.data();
    index = pipeline_stats_heap_size * heap_num + slotIndex;

    pipelineStatsFlushes.push_back({
      .target = frontend,
      .result = ptr->mappedMemory + slotIndex,
      .heap = ptr->heap.Get(),
      .heapIndex = slotIndex,
    });

    suspended = false;
  }
}

inline void BackendQueryManager::finishActivePipelineStatsQueries()
{
  for (auto &&[frontend, suspended, index] : currentPipelineStatsQueries)
  {
    G_ASSERT(suspended == true); // sanity check, must be suspended to be finished
    G_ASSERT(finishedPipelineStatsQueries.end() ==
             eastl::find(finishedPipelineStatsQueries.begin(), finishedPipelineStatsQueries.end(), frontend));
    finishedPipelineStatsQueries.push_back(frontend);
  }
  currentPipelineStatsQueries.clear();
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
  for (auto &heap : pipelineStatsHeaps)
  {
    heapResolve(target, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, heap);
  }
}

inline void BackendQueryManager::flush()
{
  D3D12_RANGE emptyRange{};
  D3D12_RANGE fullRange{0, read_back_buffer_size};
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
  D3D12_RANGE pipelineStatsFullRange{0, pipeline_stats_heap_size * sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS)};
  for (auto &&heap : pipelineStatsHeaps)
  {
    void *data = nullptr;
    if (DX12_CHECK_OK(heap.readBackBuffer->Map(0, &pipelineStatsFullRange, &data)))
    {
      heap.readBackBuffer->Unmap(0, &emptyRange);
    }
    heap.freeMask.set();
  }

  for (auto &&flush : pipelineStatsFlushes)
  {
    flush.target->accumulate(*flush.result);
  }
  for (auto &&finished : finishedPipelineStatsQueries)
  {
    finished->setFinalized();
  }

  visFlushes.clear();
  tsFlushes.clear();
  pipelineStatsFlushes.clear();
  finishedPipelineStatsQueries.clear();
}

inline void BackendQueryManager::shutdown()
{
  visFlushes.clear();
  tsFlushes.clear();
  pipelineStatsFlushes.clear();
  finishedPipelineStatsQueries.clear();
  currentPipelineStatsQueries.clear();
  visibilityHeaps.clear();
  timeStampHeaps.clear();
  pipelineStatsHeaps.clear();
}

} // namespace drv3d_dx12
