// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "queries.h"
#include "device_context_cmd.h"
#include "globals.h"
#include "backend.h"
#include "frame_info.h"
#include "timelines.h"
#include <EASTL/sort.h>
#include "device_context.h"
#include "timeline_latency.h"
#include "wrapped_command_buffer.h"
#include "vulkan_allocation_callbacks.h"

using namespace drv3d_vulkan;

uint64_t QueryManager::getResult(QueryId query_id, uint64_t fail_code, uint64_t fallback_code)
{
  size_t wRingIdx = decodeWorkItemRingIdx(query_id);
  uintptr_t qIdx = decodeQueryIndex(query_id);
  // read directly from frontend, as we are under frontend lock
  size_t cuWId = Frontend::replay->id;

  const QueryBlock &qb = blocks[wRingIdx];
  if (!TimelineLatency::isReplayCompleted(qb.work_item_id, cuWId))
    return fail_code;
  qb.verifyCompleted(); // sanity check

  // asking result for already overwritten query (old one), return fallback_code
  if (qb.count <= qIdx)
    return fallback_code;
  G_ASSERTF(!qb.data.empty(), "vulkan: query block is empty when it should be filled");

  // we should be MT safe here due to completion requirements
  QueryIndex dataIdx = qb.dataIndexes[qIdx];
  // some data may be missing due to filters on non graphics queue, so just pretend it is comleted
  return dataIdx < qb.data.size() ? qb.data[dataIdx] : 1;
}

QueryBlock *QueryManager::onFrontFlush()
{
  if (scopeDepth > 0)
    D3D_ERROR("vulkan: work item ended with %u active queries", scopeDepth);

  // read directly from frontend, as we are under frontend lock
  size_t cuWId = Frontend::replay->id;
  size_t wRingIdx = covertWorkIdToRingIdx(cuWId);
  QueryBlock &qb = blocks[wRingIdx];
  G_ASSERTF(qb.work_item_id == cuWId,
    "vulkan: query readback block work item id should be setted up ahead of time. expected %u, got %u", cuWId, qb.work_item_id);
  qb.count = lastQueryIdx;

  // reset query index
  lastQueryIdx = 0;
  // setup next block ahead of time, to be MT safe at getResult
  ++cuWId;
  wRingIdx = covertWorkIdToRingIdx(cuWId);
  QueryBlock &nextRbb = blocks[wRingIdx];
  nextRbb.setCompleted(false);
  nextRbb.work_item_id = cuWId;
  nextRbb.count = 0;
  return &qb;
}

void QueryBlock::ensureSizesAndResetStatus(VkQueryType query_type)
{
  VulkanDevice &vkDev = Globals::VK::dev;
  if (allocatedCount >= count)
  {
    if (allocatedCount)
    {
      VULKAN_LOG_CALL(Backend::cb.wCmdResetQueryPool(pool, 0, allocatedCount));
      data.clear();
      dataIndexes.clear();
    }
    return;
  }
  TIME_PROFILE(vulkan_query_pool_realloc);
  allocatedCount = ((count / POOL_SIZE_STEP) + 1) * POOL_SIZE_STEP;
  data.reserve(allocatedCount);
  dataIndexes.reserve(allocatedCount);
  data.clear();
  dataIndexes.clear();

  if (!is_null(pool))
    VULKAN_LOG_CALL(vkDev.vkDestroyQueryPool(vkDev.get(), pool, VKALLOC(query_pool)));
  const VkQueryPoolCreateInfo qpci = //
    {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, nullptr, 0, query_type, allocatedCount, 0};
  VULKAN_EXIT_ON_FAIL(vkDev.vkCreateQueryPool(vkDev.get(), &qpci, VKALLOC(query_pool), ptr(pool)));
  VULKAN_LOG_CALL(Backend::cb.wCmdResetQueryPool(pool, 0, allocatedCount));
}

void QueryBlock::cleanup()
{
  VulkanDevice &vkDev = Globals::VK::dev;
  VULKAN_LOG_CALL(vkDev.vkDestroyQueryPool(vkDev.get(), pool, VKALLOC(query_pool)));
  pool = VulkanNullHandle();
  allocatedCount = 0;
  work_item_id = 0;
  data.clear();
  dataIndexes.clear();
}

void QueryBlock::fillDataFromPool()
{
  G_ASSERTF(dataIndexes.size() == count, "vulkan: counted %u GPU queries but index filled only for %u", count, dataIndexes.size());
  VulkanDevice &vkDev = Globals::VK::dev;

  if (data.size())
    VULKAN_EXIT_ON_FAIL(vkDev.vkGetQueryPoolResults(vkDev.get(), pool, 0, data.size(), sizeof(uint64_t) * data.size(), data.data(),
      sizeof(uint64_t), VK_QUERY_RESULT_64_BIT));
  setCompleted(true);
}

void QueryManager::shutdown()
{
  for (QueryBlock &i : blocks)
    i.cleanup();
  lastQueryIdx = 0;
  scopeDepth = 0;
}
