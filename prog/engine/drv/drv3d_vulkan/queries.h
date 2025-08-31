// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <atomic>

#include "query_pools.h"

// GPU query wrapper
// accumulates queries for work item and processes them at once in backend
// uses ring buffer to store results from backend, so if query result asked too late it will not give correct result
// empty queries are filtered via index remap
// reallocates pool/containers in fixed step size
// uses single sync point of frontend lock + work item index

namespace drv3d_vulkan
{

// id is index + work item ring index combo
typedef uintptr_t QueryId;
typedef uint32_t QueryIndex;

struct QueryBlock
{
  size_t work_item_id;
  QueryIndex count;
  // raw data
  dag::Vector<uint64_t> data;
  // indexes to raw data as we can skip some queries that inserted without any work between them
  dag::Vector<QueryIndex> dataIndexes;

  static const int POOL_SIZE_STEP = 64;

  void ensureSizesAndResetStatus(VkQueryType query_type);
  void cleanup();
  void fillDataFromPool();

  QueryIndex allocatedCount;
  VulkanQueryPoolHandle pool;

#if DAGOR_DBGLEVEL > 0
  std::atomic<bool> completed{true};
  void verifyCompleted() const { G_ASSERTF(completed, "vulkan: non completed query block treated as completed!"); }
  void setCompleted(bool v) { completed = v; }
#else
  void verifyCompleted() const {}
  void setCompleted(bool) {}
#endif
};

class QueryManager
{
  static constexpr int id_total_bits = sizeof(QueryId) * 8;
  static constexpr int id_work_item_ring_idx_bits = 5;
  static constexpr int blocks_ring_size = 1 << id_work_item_ring_idx_bits;
  // to keep "0" as special, null/invalid query
  static constexpr int id_bias = 1;
  // use 32 bits on 64bit and (32-ring bits) on 32bit as QueryId must fit to uintptr_t
#if _TARGET_64BIT
  static constexpr QueryIndex max_query_idx = -1;
#else
  static constexpr int id_idx_bits = id_total_bits - id_work_item_ring_idx_bits;
  static constexpr QueryIndex max_query_idx = (1 << id_idx_bits) - 1;
#endif

  QueryBlock blocks[blocks_ring_size] = {};
  QueryIndex lastQueryIdx = 0;
  QueryIndex scopeDepth = 0;

  static size_t decodeWorkItemRingIdx(QueryId id) { return (id - id_bias) & (blocks_ring_size - 1); }
  static size_t covertWorkIdToRingIdx(size_t work_item_id) { return work_item_id % blocks_ring_size; }

public:
  QueryManager() = default;
  QueryManager(const QueryManager &) = delete;

  static QueryId encodeId(QueryIndex idx, size_t work_item_id)
  {
    return ((idx << id_work_item_ring_idx_bits) | covertWorkIdToRingIdx(work_item_id)) + id_bias;
  }
  static QueryIndex decodeQueryIndex(QueryId id) { return (id - id_bias) >> id_work_item_ring_idx_bits; }
  bool verifyQueryBelongsToWorkItem(QueryId id, size_t work_item_id)
  {
    return covertWorkIdToRingIdx(work_item_id) == decodeWorkItemRingIdx(id);
  }


  QueryIndex allocate()
  {
    G_ASSERTF(lastQueryIdx < max_query_idx, "vulkan: too much (%u) queries in single work item", lastQueryIdx);
    return lastQueryIdx++;
  }
  uint64_t getResult(QueryId query_id, uint64_t fail_code, uint64_t fallback_code);
  QueryBlock *onFrontFlush();
  bool activeScope() { return scopeDepth > 0; }
  void activateScope() { ++scopeDepth; }
  void deactivateScope() { --scopeDepth; }

  void shutdown();
};

} // namespace drv3d_vulkan