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
  bool hasReadBack() const { return (packed % 2 == 1); }
  void setQueryIndexAndType(uint64_t query_index, Qtype type) { packed = (query_index << 2) | static_cast<uint64_t>(type); }
};

class PipelineStatsQuery : public QueryBase<D3D12_QUERY_DATA_PIPELINE_STATISTICS>
{
public:
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
    Query *qArr[heap_size]{};

    bool hasAnyFree() const
    {
      return eastl::end(qArr) != eastl::find_if(eastl::begin(qArr), eastl::end(qArr), [](const auto p) { return nullptr == p; });
    }

    bool posFree(uint32_t pos) const { return pos < heap_size ? (qArr[pos] == nullptr) : false; }

    uint32_t addQuery(Query *qry);
  };

  dag::Vector<HeapPredicate> predicateHeaps;
  WinCritSec predicateGuard;

  ObjectPool<PipelineStatsQuery> pipelineStatsQueryPool;
  ObjectPool<Query> queryPool;
  WinCritSec queryGuard;

  dag::Vector<HeapPredicate>::iterator newPredicateHeap(Device &device, ID3D12Device *dx_device);

public:
  uint64_t createPredicate(Device &device, ID3D12Device *dx_device);
  void deletePredicate(uint64_t packedPredicateId);
  void shutdownPredicate(DeviceContext &ctx);
  PredicateInfo getPredicateInfo(Query *query);

  Query *newQuery();
  PipelineStatsQuery *newPipelineStatsQuery();
  Query *getQueryPtrFromId(uint64_t packedQueryId);
  void deleteQuery(Query *query_ptr);
  void removeDeletedQueries(dag::ConstSpan<Query *> deleted_queries);
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

  dag::Vector<QueryHeap<uint64_t, heap_size>> timeStampHeaps;
  dag::Vector<QueryHeap<uint64_t, heap_size>> visibilityHeaps;
  dag::Vector<QueryHeap<D3D12_QUERY_DATA_PIPELINE_STATISTICS, pipeline_stats_heap_size>> pipelineStatsHeaps;

  dag::Vector<QueryFlushMapping> tsFlushes;
  dag::Vector<VisibilityFlushMapping> visFlushes;
  dag::Vector<PipelineStatsFlushMapping> pipelineStatsFlushes;

  // Will be suspended on flush and resumed on next prepareCommandExecution (unless it has already completed by then)
  dag::Vector<PipelineStatsQueryBackendState> currentPipelineStatsQueries;
  // Finished either at the end of the frame or upon the user's request
  dag::Vector<PipelineStatsQuery *> finishedPipelineStatsQueries;

  template <D3D12_QUERY_HEAP_TYPE type>
  auto getQuerySlot(ID3D12Device *device)
  {
    auto createQueryHeap =
      [=]<class T, size_t HeapSize>(
        const dag::Vector<QueryHeap<T, HeapSize>> &) -> eastl::tuple<ComPtr<ID3D12QueryHeap>, ComPtr<ID3D12Resource>, T *> {
      D3D12_QUERY_HEAP_DESC heapDesc{
        .Type = type,
        .Count = HeapSize,
        .NodeMask = 0,
      };
      ComPtr<ID3D12QueryHeap> heap;
      if (!DX12_CHECK_OK(device->CreateQueryHeap(&heapDesc, COM_ARGS(&heap))))
      {
        return {};
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
      ComPtr<ID3D12Resource> readBackBuffer;
      if (!DX12_CHECK_OK(device->CreateCommittedResource(&bufferHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, COM_ARGS(&readBackBuffer))))
      {
        return {};
      }

      D3D12_RANGE range{};
      void *mappedMemory;
      if (!DX12_CHECK_OK(readBackBuffer->Map(0, &range, &mappedMemory)))
      {
        return {};
      }

      return {heap, readBackBuffer, reinterpret_cast<T *>(mappedMemory)};
    };

    auto getQuerySlot = [=]<class T, size_t HeapSize>(
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
        auto [newHeap, readBackBuffer, mappedMemory] = createQueryHeap(heaps);
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
      return getQuerySlot(timeStampHeaps);
    }
    else if constexpr (type == D3D12_QUERY_HEAP_TYPE_OCCLUSION)
    {
      return getQuerySlot(visibilityHeaps);
    }
    else if constexpr (type == D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS)
    {
      return getQuerySlot(pipelineStatsHeaps);
    }
    else
    {
      static_assert(false, "Unsupported query heap type");
    }
  }

public:
  void makeTimeStampQuery(Query *query_ptr, ID3D12Device *device, ID3D12GraphicsCommandList *cmd);
  void makeVisibilityQuery(Query *query_ptr, ID3D12Device *device, ID3D12GraphicsCommandList *cmd);
  void endVisibilityQuery(Query *query_ptr, ID3D12GraphicsCommandList *cmd);
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

inline uint32_t FrontendQueryManager::HeapPredicate::addQuery(Query *qry)
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

inline uint64_t FrontendQueryManager::createPredicate(Device &device, ID3D12Device *dx_device)
{
  WinAutoLock lock(predicateGuard);
  auto heap = eastl::find_if(begin(predicateHeaps), end(predicateHeaps),
    [](const FrontendQueryManager::HeapPredicate &heap) { return heap.hasAnyFree(); });
  if (heap == end(predicateHeaps))
  {
    heap = newPredicateHeap(device, dx_device);
    if (!heap)
    {
      return ~uint64_t(0);
    }
  }
  Query *q = newQuery();
  uint32_t slotIndex = heap->addQuery(q);
  uint32_t heapIndex = eastl::distance(eastl::begin(predicateHeaps), heap);
  uint64_t queryIndex = heap_size * heapIndex + slotIndex;
  q->setQueryIndexAndType(queryIndex, Query::Qtype::SURVEY);
  return q->getRaw();
}

inline void FrontendQueryManager::deletePredicate(uint64_t packedPredicateId)
{
  WinAutoLock lock(predicateGuard);
  uint64_t queryIndex = packedPredicateId >> 2;
  uint32_t heapIndex = queryIndex / heap_size;
  uint32_t slotIndex = queryIndex % heap_size;
  auto &heap = predicateHeaps[heapIndex];
  heap.qArr[slotIndex] = nullptr;
  deleteQuery(getQueryPtrFromId(packedPredicateId));
}

inline PredicateInfo FrontendQueryManager::getPredicateInfo(Query *query)
{
  if (!query)
    return {};

  uint64_t queryIndex = query->getIndex();
  uint32_t heapIndex = queryIndex / heap_size;
  uint32_t slotIndex = queryIndex % heap_size;
  auto &heap = predicateHeaps[heapIndex];

  return {
    .heap = heap.heap.Get(),
    .buffer = heap.buffer,
    .index = slotIndex,
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

inline Query *FrontendQueryManager::getQueryPtrFromId(uint64_t packedQueryId)
{
  if (static_cast<Query::Qtype>(packedQueryId & 3) == Query::Qtype::SURVEY)
  {
    uint64_t queryIndex = packedQueryId >> 2;
    uint32_t heapIndex = queryIndex / heap_size;
    uint32_t slotIndex = queryIndex % heap_size;
    return predicateHeaps[heapIndex].qArr[slotIndex];
  }
  return nullptr;
}

inline void FrontendQueryManager::deleteQuery(Query *query_ptr) { queryPool.free(query_ptr); }

inline void FrontendQueryManager::removeDeletedQueries(dag::ConstSpan<Query *> deleted_queries)
{
  WinAutoLock l(queryGuard);
  for (auto query : deleted_queries)
  {
    queryPool.free(query);
  }
}

inline void FrontendQueryManager::removeDeletedPipelineStatsQueries(dag::ConstSpan<PipelineStatsQuery *> deleted_queries)
{
  WinAutoLock l(queryGuard);
  for (auto query : deleted_queries)
  {
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

inline void BackendQueryManager::makeTimeStampQuery(Query *query_ptr, ID3D12Device *device, ID3D12GraphicsCommandList *cmd)
{
  auto [heap, slotIndex, queryIndex] = getQuerySlot<D3D12_QUERY_HEAP_TYPE_TIMESTAMP>(device);
  if (!heap) [[unlikely]]
  {
    G_ASSERT_FAIL("DX12: unable to create timestamp query heap"); // almost impossible
    return;
  }

  query_ptr->setQueryIndexAndType(queryIndex, Query::Qtype::TIMESTAMP);
  cmd->EndQuery(heap->heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, slotIndex);
  tsFlushes.push_back({
    .target = query_ptr,
    .result = heap->mappedMemory + slotIndex,
  });
}

inline void BackendQueryManager::makeVisibilityQuery(Query *query_ptr, ID3D12Device *device, ID3D12GraphicsCommandList *cmd)
{
  auto [heap, slotIndex, queryIndex] = getQuerySlot<D3D12_QUERY_HEAP_TYPE_OCCLUSION>(device);
  if (!heap) [[unlikely]]
  {
    // almost impossible
    G_ASSERT_FAIL("DX12: unable to create visibility query heap");
    return;
  }

  query_ptr->setQueryIndexAndType(queryIndex, Query::Qtype::VISIBILITY);
  cmd->BeginQuery(heap->heap.Get(), D3D12_QUERY_TYPE_OCCLUSION, slotIndex);
  visFlushes.push_back({
    .target = query_ptr,
    .result = heap->mappedMemory + slotIndex,
    .heap = heap->heap.Get(),
  });
}

inline void BackendQueryManager::endVisibilityQuery(Query *target, ID3D12GraphicsCommandList *cmd)
{
  uint64_t queryIndex = target->getIndex();
  uint32_t heapIndex = queryIndex / heap_size;
  uint32_t slotIndex = queryIndex % heap_size;
  cmd->EndQuery(visibilityHeaps[heapIndex].heap.Get(), D3D12_QUERY_TYPE_OCCLUSION, slotIndex);
}

inline void BackendQueryManager::makePipelineStatsQuery(PipelineStatsQuery *query_ptr, ID3D12Device *device,
  ID3D12GraphicsCommandList *cmd)
{
  G_ASSERT(!query_ptr->isFinalized());
  G_ASSERT(eastl::find_if(currentPipelineStatsQueries.begin(), currentPipelineStatsQueries.end(),
             [query_ptr](const auto &state) { return state.frontend == query_ptr; }) == currentPipelineStatsQueries.end());

  auto [heap, slotIndex, queryIndex] = getQuerySlot<D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS>(device);
  if (!heap) [[unlikely]]
  {
    // almost impossible
    G_ASSERT_FAIL("DX12: unable to create pipeline statistics query heap");
    return;
  }

  cmd->BeginQuery(heap->heap.Get(), D3D12_QUERY_TYPE_PIPELINE_STATISTICS, slotIndex);
  pipelineStatsFlushes.push_back({
    .target = query_ptr,
    .result = heap->mappedMemory + slotIndex,
    .heap = heap->heap.Get(),
  });
  currentPipelineStatsQueries.push_back({
    .frontend = query_ptr,
    .suspended = false,
    .queryIndex = queryIndex,
  });
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

  uint64_t queryIndex = it->queryIndex;
  uint32_t heapIndex = queryIndex / pipeline_stats_heap_size;
  uint32_t slotIndex = queryIndex % pipeline_stats_heap_size;
  cmd->EndQuery(pipelineStatsHeaps[heapIndex].heap.Get(), D3D12_QUERY_TYPE_PIPELINE_STATISTICS, slotIndex);

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
  for (auto &[_, suspended, queryIndex] : currentPipelineStatsQueries)
  {
    G_ASSERT(suspended == false); // sanity check
    uint32_t heapIndex = queryIndex / pipeline_stats_heap_size;
    uint32_t slotIndex = queryIndex % pipeline_stats_heap_size;
    cmd->EndQuery(pipelineStatsHeaps[heapIndex].heap.Get(), D3D12_QUERY_TYPE_PIPELINE_STATISTICS, slotIndex);
    suspended = true;
  }
}

inline void BackendQueryManager::resumePipelineStatsQueries(ID3D12GraphicsCommandList *cmd, ID3D12Device *device)
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

    suspended = false;
    ++it;
  }
}

inline void BackendQueryManager::finishActivePipelineStatsQueries()
{
  for (auto &[frontend, suspended, _] : currentPipelineStatsQueries)
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
  auto heapResolve = [target](auto type, auto &heaps) {
    for (auto &heap : heaps)
    {
      for (auto [start, end] : heap.freeMask.invertedRanges())
      {
        target->ResolveQueryData(heap.heap.Get(), type, start, end - start, heap.readBackBuffer.Get(),
          start * sizeof(*heap.mappedMemory));
      }
    }
  };

  heapResolve(D3D12_QUERY_TYPE_TIMESTAMP, timeStampHeaps);
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

  syncReadback(timeStampHeaps);
  for (auto &flush : tsFlushes)
  {
    flush.target->update(*flush.result);
  }
  resetSlots(timeStampHeaps);

  syncReadback(visibilityHeaps);
  for (auto &flush : visFlushes)
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

  tsFlushes.clear();
  visFlushes.clear();
  pipelineStatsFlushes.clear();
  finishedPipelineStatsQueries.clear();
}

inline void BackendQueryManager::shutdown()
{
  tsFlushes.clear();
  visFlushes.clear();
  pipelineStatsFlushes.clear();
  finishedPipelineStatsQueries.clear();
  currentPipelineStatsQueries.clear();
  timeStampHeaps.clear();
  visibilityHeaps.clear();
  pipelineStatsHeaps.clear();
}

} // namespace drv3d_dx12
