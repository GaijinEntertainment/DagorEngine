// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <atomic>

#include "query_pools.h"

// GPU timestamp query
// accumulates queries for work item and processes them at once in backend
// uses ring buffer to store results from backend, so if query result asked too late it will not give correct result
// empty markers are filtered via index remap
// reallocates pool/containers in fixed step size
// uses single sync point of frontend lock + work item index

namespace drv3d_vulkan
{

struct CmdInsertTimesampQuery;
// id is index + work item ring index combo
typedef uintptr_t TimestampQueryId;
typedef uint32_t TimestampQueryIndex;

struct TimestampQueryBlock
{
  size_t work_item_id;
  TimestampQueryIndex count;
  // raw data
  eastl::vector<uint64_t> data;
  // indexes to raw data as we skip markers that inserted without any work between them
  eastl::vector<TimestampQueryIndex> dataIndexes;

  static const int POOL_SIZE_STEP = 64;

  void ensureSizesAndResetStatus();
  void cleanup();
  void fillDataFromPool();

  TimestampQueryIndex allocatedCount;
  VulkanQueryPoolHandle pool;

#if DAGOR_DBGLEVEL > 0
  std::atomic<bool> completed{true};
  void verifyCompleted() const { G_ASSERTF(completed, "vulkan: non completed timestamp query block treated as completed!"); }
  void setCompleted(bool v) { completed = v; }
#else
  void verifyCompleted() const {}
  void setCompleted(bool) {}
#endif
};

class TimestampQueryManager
{
  static constexpr int id_total_bits = sizeof(TimestampQueryId) * 8;
  static constexpr int id_work_item_ring_idx_bits = 4;
  static constexpr int blocks_ring_size = 1 << id_work_item_ring_idx_bits;
  // use 32 bits on 64bit and (32-ring bits) on 32bit as TimestampQueryId must fit to uintptr_t
#if _TARGET_64BIT
  static constexpr TimestampQueryIndex max_query_idx = -1;
#else
  static constexpr int id_idx_bits = id_total_bits - id_work_item_ring_idx_bits;
  static constexpr TimestampQueryIndex max_query_idx = (1 << id_idx_bits) - 1;
#endif

  TimestampQueryBlock blocks[blocks_ring_size];
  TimestampQueryIndex lastQueryIdx = 0;

  static size_t decodeWorkItemRingIdx(TimestampQueryId id) { return id & (blocks_ring_size - 1); }
  static TimestampQueryIndex decodeQueryIndex(TimestampQueryId id) { return id >> id_work_item_ring_idx_bits; }

  static size_t covertWorkIdToRingIdx(size_t work_item_id) { return work_item_id % blocks_ring_size; }

public:
  TimestampQueryManager() = default;
  TimestampQueryManager(const TimestampQueryManager &) = delete;

  static TimestampQueryId encodeId(TimestampQueryIndex idx, size_t work_item_id)
  {
    return (idx << id_work_item_ring_idx_bits) | covertWorkIdToRingIdx(work_item_id);
  }

  TimestampQueryIndex allocate()
  {
    G_ASSERTF(lastQueryIdx < max_query_idx, "vulkan: too much (%u) queries in single work item", lastQueryIdx);
    return lastQueryIdx++;
  }
  uint64_t getResult(TimestampQueryId query_id);
  void onFrontFlush();

  void shutdown();
};

} // namespace drv3d_vulkan