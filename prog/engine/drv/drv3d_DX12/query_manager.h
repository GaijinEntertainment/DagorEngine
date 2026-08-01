// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "pipeline.h"

#include <atomic>
#include <generic/dag_concurrentElementPool.h>
#include <generic/dag_objectPool.h>
#include <osApiWrappers/dag_critSec.h>
#include <generic/dag_reverseView.h>
#include <generic/dag_span.h>


namespace drv3d_dx12
{
constexpr uint32_t read_back_buffer_size = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
constexpr uint32_t heap_size = read_back_buffer_size / sizeof(uint64_t);
constexpr uint32_t pipeline_stats_heap_size = read_back_buffer_size / sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);

constexpr D3D12_QUERY_DATA_PIPELINE_STATISTICS &operator+=(D3D12_QUERY_DATA_PIPELINE_STATISTICS &lhs,
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

class Device;

template <class T>
class QueryBase
{
public:
  using ResultType = T;

protected:
  enum class State
  {
    ISSUED,
    FINALIZED,
  };
  std::atomic<State> state{State::FINALIZED};
  ResultType result{};

public:
  bool isIssued() const { return state.load(std::memory_order_acquire) == State::ISSUED; }
  bool isFinalized() const { return state.load(std::memory_order_acquire) == State::FINALIZED; }

  ResultType getValue() const { return result; }
};

class Query : public QueryBase<uint64_t>
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
  uint64_t packed = 0;

public:
  void setIssued() { state.store(State::ISSUED, std::memory_order_relaxed); }
  void update(uint64_t value)
  {
    result = value;
    state.store(State::FINALIZED, std::memory_order_release);
  }

  uint64_t getRaw() const { return packed; }
  uint64_t getIndex() const { return packed >> 2; }
  Qtype getType() const { return static_cast<Qtype>(packed & 3); } // last 2 bits
  bool hasReadBack() const
  {
    // The encoding relies on the low bit of Qtype: TIMESTAMP and VISIBILITY are read back, UNDEFINED and SURVEY are not.
    static_assert((static_cast<uint64_t>(Qtype::TIMESTAMP) & 1) == 1);
    static_assert((static_cast<uint64_t>(Qtype::VISIBILITY) & 1) == 1);
    static_assert((static_cast<uint64_t>(Qtype::UNDEFINED) & 1) == 0);
    static_assert((static_cast<uint64_t>(Qtype::SURVEY) & 1) == 0);
    return (packed & 1) == 1;
  }
  void setQueryIndexAndType(uint64_t query_index, Qtype type) { packed = (query_index << 2) | static_cast<uint64_t>(type); }
};

class PipelineStatsQuery : public QueryBase<D3D12_QUERY_DATA_PIPELINE_STATISTICS>
{
public:
  void setIssued()
  {
    resetValue();
    [[maybe_unused]] State prevState = state.exchange(State::ISSUED, std::memory_order_acq_rel);
    G_ASSERT(prevState == State::FINALIZED);
  }
  void setFinalized() { state.store(State::FINALIZED, std::memory_order_release); }

  void accumulate(ResultType value)
  {
    G_ASSERT(!isFinalized());
    result += value;
  }

  using QueryBase<D3D12_QUERY_DATA_PIPELINE_STATISTICS>::getValue;
  auto getValue(auto member) const { return result.*member; }

  void resetValue() { result = {}; }
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
    Query *querySlots[heap_size]{};

    uint32_t findFreeSlot() const
    {
      auto ref = eastl::find(eastl::begin(querySlots), eastl::end(querySlots), nullptr);
      return ref != eastl::end(querySlots) ? static_cast<uint32_t>(eastl::distance(eastl::begin(querySlots), ref)) : ~uint32_t(0);
    }
  };

  // Heaps are indexed lock-free by the render thread (getPredicateInfo / getQueryPtrFromId)
  // while other threads may create new ones, so storage must stay stable under growth; the
  // pool guarantees allocated elements never move. predicateGuard only serializes writers.
  enum class PredicateHeapId : uint32_t
  {
    Invalid = 0
  };
  using PredicateHeapPool = dag::ConcurrentElementPool<PredicateHeapId, HeapPredicate, 0, 8>;

  PredicateHeapPool predicateHeaps;
  WinCritSec predicateGuard;

  ObjectPool<Query> queryPool;
  ObjectPool<PipelineStatsQuery> pipelineStatsQueryPool;
  WinCritSec queryGuard;

  bool createPredicateHeapResources(HeapPredicate &heap, Device &device, ID3D12Device *dx_device);
  PredicateHeapId newPredicateHeap(Device &device, ID3D12Device *dx_device);

public:
  uint64_t createPredicate(Device &device, ID3D12Device *dx_device);
  void deletePredicate(uint64_t packed_predicate_id);
  void shutdownPredicate(DeviceContext &ctx);
  PredicateInfo getPredicateInfo(Query *query);

  Query *newQuery();
  Query *getQueryPtrFromId(uint64_t packed_query_id);
  void deleteQuery(Query *query);
  void removeDeletedQueries(dag::ConstSpan<Query *> deleted_queries);

  PipelineStatsQuery *newPipelineStatsQuery();
  void removeDeletedPipelineStatsQueries(dag::ConstSpan<PipelineStatsQuery *> deleted_queries);

  void preRecovery();
  bool postRecovery(Device &device, ID3D12Device *dx_device);
};

class BackendQueryManager
{
private:
  template <class T, size_t HeapSize>
  struct QueryHeap
  {
    static constexpr size_t size = HeapSize;
    Bitset<HeapSize> freeMask = Bitset<HeapSize>().set();
    ComPtr<ID3D12QueryHeap> heap;
    ComPtr<ID3D12Resource> readBackBuffer;
    T *mappedMemory = nullptr;
  };

  struct TimestampFlushMapping
  {
    Query *target = nullptr;
    uint64_t *result = nullptr;
  };

  struct VisibilityFlushMapping
  {
    Query *target = nullptr;
    uint64_t *result = nullptr;
  };

  struct PipelineStatsFlushMapping
  {
    PipelineStatsQuery *target = nullptr;
    D3D12_QUERY_DATA_PIPELINE_STATISTICS *result = nullptr;
    ID3D12QueryHeap *heap = nullptr;
  };

  struct PipelineStatsQueryBackendState
  {
    PipelineStatsQuery *frontend = nullptr;
    bool suspended = false;
    uint64_t queryIndex = 0;
  };

  struct LazyPipelineStatsQueryBackendState
  {
    PipelineStatsQuery *frontend = nullptr;
    bool activated = false;
  };

  dag::Vector<QueryHeap<uint64_t, heap_size>> timestampHeaps;
  dag::Vector<QueryHeap<uint64_t, heap_size>> visibilityHeaps;
  dag::Vector<QueryHeap<D3D12_QUERY_DATA_PIPELINE_STATISTICS, pipeline_stats_heap_size>> pipelineStatsHeaps;

  dag::Vector<TimestampFlushMapping> timestampFlushes;
  dag::Vector<VisibilityFlushMapping> visibilityFlushes;
  dag::Vector<PipelineStatsFlushMapping> pipelineStatsFlushes;

  // Will be suspended on flush and resumed on next prepareCommandExecution (unless it has already completed by then)
  dag::Vector<PipelineStatsQueryBackendState> currentPipelineStatsQueries;
  // Finished either at the end of the frame or upon the user's request
  dag::Vector<PipelineStatsQuery *> finishedPipelineStatsQueries;

  dag::Vector<LazyPipelineStatsQueryBackendState> lazyPipelineStatsQueries;
  dag::Vector<PipelineStatsQuery *> pendingDeactivationLazyPipelineStatsQueries;

  void addInactiveLazyShares(PipelineStatsQuery *query, D3D12_QUERY_DATA_PIPELINE_STATISTICS *result, ID3D12QueryHeap *heap);

  static ComPtr<ID3D12QueryHeap> createQueryHeap(Device &device, D3D12_QUERY_HEAP_TYPE type, uint32_t count);
  static ComPtr<ID3D12Resource> createQueryReadBackBuffer(Device &device, void **mapped_memory);

  template <D3D12_QUERY_HEAP_TYPE type>
  auto getQuerySlot(Device &device)
  {
    auto createHeapResources =
      [&device]<class T, size_t HeapSize>(
        const dag::Vector<QueryHeap<T, HeapSize>> &) -> eastl::tuple<ComPtr<ID3D12QueryHeap>, ComPtr<ID3D12Resource>, T *> {
      ComPtr<ID3D12QueryHeap> heap = createQueryHeap(device, type, HeapSize);
      if (!heap)
      {
        return {};
      }

      void *mappedMemory = nullptr;
      ComPtr<ID3D12Resource> readBackBuffer = createQueryReadBackBuffer(device, &mappedMemory);
      if (!readBackBuffer)
      {
        return {};
      }

      return {eastl::move(heap), eastl::move(readBackBuffer), reinterpret_cast<T *>(mappedMemory)};
    };

    auto findOrAllocateSlot =
      [=]<class T, size_t HeapSize>(
        dag::Vector<QueryHeap<T, HeapSize>> &heaps) -> eastl::tuple<QueryHeap<T, HeapSize> *, uint32_t, uint64_t> {
      uint32_t slotIndex;
      uint64_t queryIndex;
      // find free position in heaps vector
      auto heap = eastl::find_if(begin(heaps), end(heaps), [&](auto &heap) {
        slotIndex = heap.freeMask.find_first_and_reset();
        return slotIndex != heap.freeMask.kSize;
      });
      if (heap != end(heaps)) [[likely]]
      {
        auto heapIndex = eastl::distance(begin(heaps), heap);
        queryIndex = HeapSize * heapIndex + slotIndex;
      }
      else
      {
        // need to create new heap
        auto [newHeap, readBackBuffer, mappedMemory] = createHeapResources(heaps);
        if (!newHeap)
        {
          return {};
        }
        slotIndex = 0;
        queryIndex = HeapSize * heaps.size();
        heap = &heaps.push_back({
          .heap = eastl::move(newHeap),
          .readBackBuffer = eastl::move(readBackBuffer),
          .mappedMemory = mappedMemory,
        });
        heap->freeMask.reset(0);
      }

      return {heap, slotIndex, queryIndex};
    };

    if constexpr (type == D3D12_QUERY_HEAP_TYPE_TIMESTAMP)
    {
      return findOrAllocateSlot(timestampHeaps);
    }
    else if constexpr (type == D3D12_QUERY_HEAP_TYPE_OCCLUSION)
    {
      return findOrAllocateSlot(visibilityHeaps);
    }
    else if constexpr (type == D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS)
    {
      return findOrAllocateSlot(pipelineStatsHeaps);
    }
    else
    {
      static_assert(false, "Unsupported query heap type");
    }
  }

public:
  void makeTimeStampQuery(Query *query, Device &device, ID3D12GraphicsCommandList *cmd);

  void makeVisibilityQuery(Query *query, Device &device, ID3D12GraphicsCommandList *cmd);
  void endVisibilityQuery(Query *query, ID3D12GraphicsCommandList *cmd);

  void makePipelineStatsQuery(PipelineStatsQuery *query, Device &device, ID3D12GraphicsCommandList *cmd);
  void endPipelineStatsQuery(PipelineStatsQuery *query, ID3D12GraphicsCommandList *cmd);

  void pushLazyPipelineStatsQuery(PipelineStatsQuery *query);
  void activateTopLazyPipelineStatsQuery(Device &device, ID3D12GraphicsCommandList *cmd);
  void popLazyPipelineStatsQuery(PipelineStatsQuery *query);
  void deactivatePendingLazyPipelineStatsQueries(ID3D12GraphicsCommandList *cmd);
  void accumulateInactiveLazyQueries(ID3D12GraphicsCommandList *cmd, uint64_t primitives);

  // Suspend all active pipeline statistics queries before the command list is closed
  void suspendActiveQueries(ID3D12GraphicsCommandList *cmd);
  void resumePipelineStatsQueries(ID3D12GraphicsCommandList *cmd, Device &device);
  // Finish all active pipeline statistics queries
  void finishActivePipelineStatsQueries();

  void cancelQuery(Query *query);
  void cancelPipelineStatsQuery(PipelineStatsQuery *query);

  void resolve(ID3D12GraphicsCommandList *cmd);
  void flush();
  void shutdown(Device &device);
};

inline uint64_t FrontendQueryManager::createPredicate(Device &device, ID3D12Device *dx_device)
{
  WinAutoLock lock(predicateGuard);
  uint32_t slotIndex = ~uint32_t(0);
  PredicateHeapId heapId = predicateHeaps.findIf([&slotIndex](const HeapPredicate &predicateHeap) {
    slotIndex = predicateHeap.findFreeSlot();
    return slotIndex != ~uint32_t(0);
  });
  if (heapId == PredicateHeapId::Invalid)
  {
    heapId = newPredicateHeap(device, dx_device);
    if (heapId == PredicateHeapId::Invalid)
    {
      return ~uint64_t(0);
    }
    slotIndex = 0;
  }
  Query *query = newQuery();
  predicateHeaps[heapId].querySlots[slotIndex] = query;
  uint32_t heapIndex = PredicateHeapPool::to_index(heapId);
  uint64_t queryIndex = heap_size * heapIndex + slotIndex;
  query->setQueryIndexAndType(queryIndex, Query::Qtype::SURVEY);
  return query->getRaw();
}

inline void FrontendQueryManager::deletePredicate(uint64_t packed_predicate_id)
{
  if (static_cast<Query::Qtype>(packed_predicate_id & 3) != Query::Qtype::SURVEY)
    return;
  uint64_t queryIndex = packed_predicate_id >> 2;
  uint32_t heapIndex = queryIndex / heap_size;
  uint32_t slotIndex = queryIndex % heap_size;
  WinAutoLock lock(predicateGuard);
  if (heapIndex >= predicateHeaps.totalElements())
    return;
  deleteQuery(eastl::exchange(predicateHeaps[PredicateHeapPool::from_index(heapIndex)].querySlots[slotIndex], nullptr));
}

inline PredicateInfo FrontendQueryManager::getPredicateInfo(Query *query)
{
  if (!query)
    return {};

  uint64_t queryIndex = query->getIndex();
  uint32_t heapIndex = queryIndex / heap_size;
  uint32_t slotIndex = queryIndex % heap_size;
  // Lock-free: pool storage is stable and heap/buffer are set before the id is handed out.
  auto &heap = predicateHeaps[PredicateHeapPool::from_index(heapIndex)];

  return {
    .heap = heap.heap.Get(),
    .buffer = heap.buffer,
    .index = slotIndex,
  };
}

inline Query *FrontendQueryManager::newQuery()
{
  WinAutoLock lock(queryGuard);
  return queryPool.allocate();
}

inline PipelineStatsQuery *FrontendQueryManager::newPipelineStatsQuery()
{
  WinAutoLock lock(queryGuard);
  return pipelineStatsQueryPool.allocate();
}

inline Query *FrontendQueryManager::getQueryPtrFromId(uint64_t packed_query_id)
{
  if (static_cast<Query::Qtype>(packed_query_id & 3) == Query::Qtype::SURVEY)
  {
    uint64_t queryIndex = packed_query_id >> 2;
    uint32_t heapIndex = queryIndex / heap_size;
    uint32_t slotIndex = queryIndex % heap_size;
    return predicateHeaps[PredicateHeapPool::from_index(heapIndex)].querySlots[slotIndex];
  }
  return nullptr;
}

inline void FrontendQueryManager::deleteQuery(Query *query)
{
  if (!query)
    return;

  WinAutoLock lock(queryGuard);
  queryPool.free(query);
}

inline void FrontendQueryManager::removeDeletedQueries(dag::ConstSpan<Query *> deleted_queries)
{
  WinAutoLock lock(queryGuard);
  for (auto query : deleted_queries)
  {
    if (query)
      queryPool.free(query);
  }
}

inline void FrontendQueryManager::removeDeletedPipelineStatsQueries(dag::ConstSpan<PipelineStatsQuery *> deleted_queries)
{
  WinAutoLock lock(queryGuard);
  for (auto query : deleted_queries)
  {
    if (query)
      pipelineStatsQueryPool.free(query);
  }
}

inline void FrontendQueryManager::preRecovery()
{
  queryPool.iterateAllocated([](auto ts) { ts->update(0); });
  pipelineStatsQueryPool.iterateAllocated([](auto ps) {
    ps->resetValue();
    ps->setFinalized();
  });
  for (auto &heap : predicateHeaps)
    heap.heap.Reset();
}

inline void BackendQueryManager::makeTimeStampQuery(Query *query, Device &device, ID3D12GraphicsCommandList *cmd)
{
  auto [heap, slotIndex, queryIndex] = getQuerySlot<D3D12_QUERY_HEAP_TYPE_TIMESTAMP>(device);
  if (!heap) [[unlikely]]
  {
    G_ASSERT_FAIL("DX12: unable to create timestamp query heap"); // almost impossible
    // drop the stale type/index so nothing mistakes this query for one owning a slot,
    // and finalize with a zero result so pollers waiting on it terminate
    query->setQueryIndexAndType(0, Query::Qtype::UNDEFINED);
    query->update(0);
    return;
  }

  query->setQueryIndexAndType(queryIndex, Query::Qtype::TIMESTAMP);
  cmd->EndQuery(heap->heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, slotIndex);
  timestampFlushes.push_back({
    .target = query,
    .result = heap->mappedMemory + slotIndex,
  });
}

inline void BackendQueryManager::makeVisibilityQuery(Query *query, Device &device, ID3D12GraphicsCommandList *cmd)
{
  auto [heap, slotIndex, queryIndex] = getQuerySlot<D3D12_QUERY_HEAP_TYPE_OCCLUSION>(device);
  if (!heap) [[unlikely]]
  {
    // almost impossible
    G_ASSERT_FAIL("DX12: unable to create visibility query heap");
    // drop any stale index so endVisibilityQuery skips this query, and
    // finalize with a zero result so pollers waiting on it terminate
    query->setQueryIndexAndType(0, Query::Qtype::UNDEFINED);
    query->update(0);
    return;
  }

  query->setQueryIndexAndType(queryIndex, Query::Qtype::VISIBILITY);
  cmd->BeginQuery(heap->heap.Get(), D3D12_QUERY_TYPE_OCCLUSION, slotIndex);
  visibilityFlushes.push_back({
    .target = query,
    .result = heap->mappedMemory + slotIndex,
  });
}

inline void BackendQueryManager::endVisibilityQuery(Query *query, ID3D12GraphicsCommandList *cmd)
{
  // Begin may have failed to allocate a slot; do not end a query on a slot this object does not own
  if (query->getType() != Query::Qtype::VISIBILITY)
    return;
  uint64_t queryIndex = query->getIndex();
  uint32_t heapIndex = queryIndex / heap_size;
  uint32_t slotIndex = queryIndex % heap_size;
  cmd->EndQuery(visibilityHeaps[heapIndex].heap.Get(), D3D12_QUERY_TYPE_OCCLUSION, slotIndex);
}

inline void BackendQueryManager::makePipelineStatsQuery(PipelineStatsQuery *query, Device &device, ID3D12GraphicsCommandList *cmd)
{
  G_ASSERT(!query->isFinalized());
  G_ASSERT(eastl::find_if(currentPipelineStatsQueries.begin(), currentPipelineStatsQueries.end(),
             [query](const auto &state) { return state.frontend == query; }) == currentPipelineStatsQueries.end());

  auto [heap, slotIndex, queryIndex] = getQuerySlot<D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS>(device);
  if (!heap) [[unlikely]]
  {
    // almost impossible
    G_ASSERT_FAIL("DX12: unable to create pipeline statistics query heap");
    // a lazy query stays on the stack and is finalized at frame end even when inactive;
    // a direct query would otherwise stay ISSUED forever, so finalize it for pollers to terminate
    if (eastl::find_if(lazyPipelineStatsQueries.begin(), lazyPipelineStatsQueries.end(),
          [query](const auto &lazy) { return lazy.frontend == query; }) == lazyPipelineStatsQueries.end())
      query->setFinalized();
    return;
  }

  cmd->BeginQuery(heap->heap.Get(), D3D12_QUERY_TYPE_PIPELINE_STATISTICS, slotIndex);
  pipelineStatsFlushes.push_back({
    .target = query,
    .result = heap->mappedMemory + slotIndex,
    .heap = heap->heap.Get(),
  });
  currentPipelineStatsQueries.push_back({
    .frontend = query,
    .suspended = false,
    .queryIndex = queryIndex,
  });
}

inline void BackendQueryManager::endPipelineStatsQuery(PipelineStatsQuery *query, ID3D12GraphicsCommandList *cmd)
{
  auto it = eastl::find_if(currentPipelineStatsQueries.begin(), currentPipelineStatsQueries.end(),
    [query](const auto &state) { return state.frontend == query; });
  if (it == currentPipelineStatsQueries.end())
  {
    return;
  }
  G_ASSERT(it->suspended == false); // sanity check

  uint64_t queryIndex = it->queryIndex;
  uint32_t heapIndex = queryIndex / pipeline_stats_heap_size;
  uint32_t slotIndex = queryIndex % pipeline_stats_heap_size;
  cmd->EndQuery(pipelineStatsHeaps[heapIndex].heap.Get(), D3D12_QUERY_TYPE_PIPELINE_STATISTICS, slotIndex);

  currentPipelineStatsQueries.erase_unsorted(it);

  finishedPipelineStatsQueries.push_back(query);
}

inline void BackendQueryManager::pushLazyPipelineStatsQuery(PipelineStatsQuery *q)
{
  lazyPipelineStatsQueries.push_back(LazyPipelineStatsQueryBackendState{
    .frontend = q,
    .activated = false,
  });
}

inline void BackendQueryManager::activateTopLazyPipelineStatsQuery(Device &device, ID3D12GraphicsCommandList *cmd)
{
  if (lazyPipelineStatsQueries.empty() || lazyPipelineStatsQueries.back().activated)
    return;

  deactivatePendingLazyPipelineStatsQueries(cmd);
  const auto flushesPrev = pipelineStatsFlushes.size();
  makePipelineStatsQuery(lazyPipelineStatsQueries.back().frontend, device, cmd);
  G_ASSERT_RETURN(flushesPrev < pipelineStatsFlushes.size(), ); // sanity check

  // Copy, addInactiveLazyShares may reallocate pipelineStatsFlushes
  const auto topFlush = pipelineStatsFlushes.back();

  lazyPipelineStatsQueries.back().activated = true;
  addInactiveLazyShares(topFlush.target, topFlush.result, topFlush.heap);
}

// Registers result sharing for all queries below `query` in the lazy stack until the first activated one,
// so they accumulate the values of `query` for the current command list segment.
inline void BackendQueryManager::addInactiveLazyShares(PipelineStatsQuery *query, D3D12_QUERY_DATA_PIPELINE_STATISTICS *result,
  ID3D12QueryHeap *heap)
{
  auto it = eastl::find_if(lazyPipelineStatsQueries.begin(), lazyPipelineStatsQueries.end(),
    [query](const auto &lazy) { return lazy.frontend == query; });
  if (it == lazyPipelineStatsQueries.end())
    return;
  G_ASSERT_RETURN(it->activated, );

  const auto stackSizeBelow = eastl::distance(lazyPipelineStatsQueries.begin(), it);
  for (auto [frontend, activated] : dag::ReverseView(make_span_const(lazyPipelineStatsQueries.data(), stackSizeBelow)))
  {
    if (activated)
      break;

    pipelineStatsFlushes.push_back({
      .target = frontend,
      .result = result,
      .heap = heap,
    });
  }
}

inline void BackendQueryManager::popLazyPipelineStatsQuery(PipelineStatsQuery *query)
{
  G_UNUSED(query);

  // Ignore the query if frame was already finished
  if (lazyPipelineStatsQueries.empty())
    return;

  D3D_CONTRACT_ASSERTF(lazyPipelineStatsQueries.back().frontend == query,
    "DX12: LIFO order of lazy pipeline stats queries is violated");

  auto top = lazyPipelineStatsQueries.back();
  lazyPipelineStatsQueries.pop_back();

  if (top.activated)
    pendingDeactivationLazyPipelineStatsQueries.push_back(top.frontend);
  else
    finishedPipelineStatsQueries.push_back(top.frontend);
}

inline void BackendQueryManager::deactivatePendingLazyPipelineStatsQueries(ID3D12GraphicsCommandList *cmd)
{
  for (auto query : pendingDeactivationLazyPipelineStatsQueries)
    endPipelineStatsQuery(query, cmd);
  pendingDeactivationLazyPipelineStatsQueries.clear();
}

inline void BackendQueryManager::accumulateInactiveLazyQueries(ID3D12GraphicsCommandList *cmd, uint64_t primitives)
{
  deactivatePendingLazyPipelineStatsQueries(cmd);
  for (auto [frontend, activated] : dag::ReverseView(lazyPipelineStatsQueries))
  {
    if (activated)
      break;
    frontend->accumulate({.CInvocations = primitives});
  }
}

inline void BackendQueryManager::suspendActiveQueries(ID3D12GraphicsCommandList *cmd)
{
  deactivatePendingLazyPipelineStatsQueries(cmd);
  for (auto &[_, suspended, queryIndex] : currentPipelineStatsQueries)
  {
    G_ASSERT(suspended == false); // sanity check
    uint32_t heapIndex = queryIndex / pipeline_stats_heap_size;
    uint32_t slotIndex = queryIndex % pipeline_stats_heap_size;
    cmd->EndQuery(pipelineStatsHeaps[heapIndex].heap.Get(), D3D12_QUERY_TYPE_PIPELINE_STATISTICS, slotIndex);
    suspended = true;
  }
}

inline void BackendQueryManager::resumePipelineStatsQueries(ID3D12GraphicsCommandList *cmd, Device &device)
{
  auto it = currentPipelineStatsQueries.begin();
  auto end = currentPipelineStatsQueries.end();
  while (it != end)
  {
    auto &[frontend, suspended, queryIndex] = *it;
    G_ASSERT(suspended == true); // sanity check

    QueryHeap<D3D12_QUERY_DATA_PIPELINE_STATISTICS, pipeline_stats_heap_size> *heap = nullptr;
    uint32_t slotIndex = 0;
    eastl::tie(heap, slotIndex, queryIndex) = getQuerySlot<D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS>(device);

    if (!heap) [[unlikely]]
    {
      G_ASSERT(false); // this should not happen, because flush happens before this call
      it->frontend->setFinalized();
      it = currentPipelineStatsQueries.erase_unsorted(it);
      end = currentPipelineStatsQueries.end();
      continue;
    }

    cmd->BeginQuery(heap->heap.Get(), D3D12_QUERY_TYPE_PIPELINE_STATISTICS, slotIndex);
    pipelineStatsFlushes.push_back({
      .target = frontend,
      .result = heap->mappedMemory + slotIndex,
      .heap = heap->heap.Get(),
    });
    // Inactive lazy queries shared the result of this query in the previous segment; that flush entry
    // does not cover the new segment, so the sharing has to be re-registered for the new slot.
    addInactiveLazyShares(frontend, heap->mappedMemory + slotIndex, heap->heap.Get());

    suspended = false;
    ++it;
  }
}

inline void BackendQueryManager::finishActivePipelineStatsQueries()
{
  for (auto [frontend, suspended, _] : currentPipelineStatsQueries)
  {
    G_ASSERT(suspended == true); // sanity check, must be suspended to be finished
    G_ASSERT(finishedPipelineStatsQueries.end() ==
             eastl::find(finishedPipelineStatsQueries.begin(), finishedPipelineStatsQueries.end(), frontend));
    finishedPipelineStatsQueries.push_back(frontend);
  }
  for (auto [frontend, activated] : lazyPipelineStatsQueries)
  {
    // active lazy queries are finished at the same time as non-lazy ones
    if (!activated)
      finishedPipelineStatsQueries.push_back(frontend);
  }
  // suspendActiveQueries must have deactivated all pending lazy queries before the frame ends;
  // entries here would mean endPipelineStatsQuery was never recorded for them.
  G_ASSERT(pendingDeactivationLazyPipelineStatsQueries.empty());
  lazyPipelineStatsQueries.clear();
  currentPipelineStatsQueries.clear();
  pendingDeactivationLazyPipelineStatsQueries.clear();
}

inline void BackendQueryManager::cancelQuery(Query *query)
{
  auto removeQuery = [query](auto &vec) {
    auto it = eastl::remove_if(vec.begin(), vec.end(), [query](const auto &flush) { return flush.target == query; });
    vec.erase(it, vec.end());
  };

  removeQuery(timestampFlushes);
  removeQuery(visibilityFlushes);
}

inline void BackendQueryManager::cancelPipelineStatsQuery(PipelineStatsQuery *query)
{
  // Multiple invocations for same query are possible due to lazy queries accumulation
  pipelineStatsFlushes.erase(eastl::remove_if(pipelineStatsFlushes.begin(), pipelineStatsFlushes.end(),
                               [query](const auto &flush) { return flush.target == query; }),
    pipelineStatsFlushes.end());

  // By design, each vector does not contain duplicate pointers, so (remove_if, erase) idiom is ambiguous

  if (auto itCurrent = eastl::find_if(currentPipelineStatsQueries.begin(), currentPipelineStatsQueries.end(),
        [query](const auto &current) { return current.frontend == query; });
      itCurrent != currentPipelineStatsQueries.end())
  {
    currentPipelineStatsQueries.erase_unsorted(itCurrent);
  }

  if (auto itFinished = eastl::find(finishedPipelineStatsQueries.begin(), finishedPipelineStatsQueries.end(), query);
      itFinished != finishedPipelineStatsQueries.end())
  {
    finishedPipelineStatsQueries.erase_unsorted(itFinished);
  }

  if (auto itLazy = eastl::find_if(lazyPipelineStatsQueries.begin(), lazyPipelineStatsQueries.end(),
        [query](const auto &lazy) { return lazy.frontend == query; });
      itLazy != lazyPipelineStatsQueries.end())
  {
    lazyPipelineStatsQueries.erase(itLazy);
  }

  if (auto itPending =
        eastl::find(pendingDeactivationLazyPipelineStatsQueries.begin(), pendingDeactivationLazyPipelineStatsQueries.end(), query);
      itPending != pendingDeactivationLazyPipelineStatsQueries.end())
  {
    pendingDeactivationLazyPipelineStatsQueries.erase_unsorted(itPending);
  }
}

inline void BackendQueryManager::resolve(ID3D12GraphicsCommandList *cmd)
{
  auto heapResolve = [cmd](auto type, auto &heaps) {
    for (auto &heap : heaps)
    {
      for (auto [start, end] : heap.freeMask.invertedRanges())
      {
        cmd->ResolveQueryData(heap.heap.Get(), type, start, end - start, heap.readBackBuffer.Get(),
          start * sizeof(*heap.mappedMemory));
      }
    }
  };

  heapResolve(D3D12_QUERY_TYPE_TIMESTAMP, timestampHeaps);
  heapResolve(D3D12_QUERY_TYPE_OCCLUSION, visibilityHeaps);
  heapResolve(D3D12_QUERY_TYPE_PIPELINE_STATISTICS, pipelineStatsHeaps);
}

inline void BackendQueryManager::flush()
{
  // Invalidate CPU caches so GPU-written data is visible before reading results.
  auto syncReadback = [](auto &heaps) {
    constexpr D3D12_RANGE fullRange{0, read_back_buffer_size}, emptyRange{};
#if _TARGET_XBOX
    void *pData = nullptr;
    void **ppData = &pData;
#else
    constexpr void **ppData = nullptr;
#endif
    for (auto &heap : heaps)
    {
      if (DX12_CHECK_OK(heap.readBackBuffer->Map(0, &fullRange, ppData)))
        heap.readBackBuffer->Unmap(0, &emptyRange);
    }
  };
  // Release heap slots only after all results are consumed to prevent
  // newly allocated slots from overwriting mappedMemory still in the flush lists.
  auto resetSlots = [](auto &heaps) {
    for (auto &heap : heaps)
      heap.freeMask.set();
  };

  syncReadback(timestampHeaps);
  for (auto &flush : timestampFlushes)
  {
    flush.target->update(*flush.result);
  }
  resetSlots(timestampHeaps);

  syncReadback(visibilityHeaps);
  for (auto &flush : visibilityFlushes)
  {
    flush.target->update(*flush.result);
  }
  resetSlots(visibilityHeaps);

  syncReadback(pipelineStatsHeaps);
  for (auto &flush : pipelineStatsFlushes)
  {
    flush.target->accumulate(*flush.result);
  }
  for (auto &finished : finishedPipelineStatsQueries)
  {
    finished->setFinalized();
  }
  resetSlots(pipelineStatsHeaps);

  timestampFlushes.clear();
  visibilityFlushes.clear();
  pipelineStatsFlushes.clear();
  finishedPipelineStatsQueries.clear();
}

} // namespace drv3d_dx12
