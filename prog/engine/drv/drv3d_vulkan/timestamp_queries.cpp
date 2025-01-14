// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "timestamp_queries.h"
#include "device_context_cmd.h"
#include "globals.h"
#include "backend.h"
#include "frame_info.h"
#include "timelines.h"
#include <EASTL/sort.h>
#include "device_context.h"
#include "timeline_latency.h"
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

uint64_t TimestampQueryManager::getResult(TimestampQueryId query_id)
{
  size_t wRingIdx = decodeWorkItemRingIdx(query_id);
  uintptr_t qIdx = decodeQueryIndex(query_id);
  // read directly from frontend, as we are under frontend lock
  size_t cuWId = Frontend::replay->id;

  const TimestampQueryBlock &qb = blocks[wRingIdx];
  if (!TimelineLatency::isReplayCompleted(qb.work_item_id, cuWId))
    return 0;
  qb.verifyCompleted(); // sanity check

  // asking result for already overwritten query (old one), return fake "OK" value
  if (qb.count <= qIdx)
    return 1;
  G_ASSERTF(!qb.data.empty(), "vulkan: timestamp query block is empty when it should be filled");

  // we should be MT safe here due to completion requirements
  TimestampQueryIndex dataIdx = qb.dataIndexes[qIdx];
  // some data may be missing due to filters on non graphics queue, so just pretend it is comleted
  return dataIdx < qb.data.size() ? qb.data[dataIdx] : 1;
}

void TimestampQueryManager::onFrontFlush()
{
  // read directly from frontend, as we are under frontend lock
  size_t cuWId = Frontend::replay->id;
  size_t wRingIdx = covertWorkIdToRingIdx(cuWId);
  TimestampQueryBlock &qb = blocks[wRingIdx];
  G_ASSERTF(qb.work_item_id == cuWId, "vulkan: timestamp readback block work item id should be setted up ahead of time");
  qb.count = lastQueryIdx;

  Frontend::replay->timestampQueryBlock = &qb;

  // reset query index
  lastQueryIdx = 0;
  // setup next block ahead of time, to be MT safe at getResult
  ++cuWId;
  wRingIdx = covertWorkIdToRingIdx(cuWId);
  TimestampQueryBlock &nextRbb = blocks[wRingIdx];
  nextRbb.setCompleted(false);
  nextRbb.work_item_id = cuWId;
  nextRbb.count = 0;
}

void TimestampQueryBlock::ensureSizesAndResetStatus()
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
  TIME_PROFILE(vulkan_timestamp_query_pool_realloc);
  allocatedCount = ((count / POOL_SIZE_STEP) + 1) * POOL_SIZE_STEP;
  data.reserve(allocatedCount);
  dataIndexes.reserve(allocatedCount);
  data.clear();
  dataIndexes.clear();

  if (!is_null(pool))
    VULKAN_LOG_CALL(vkDev.vkDestroyQueryPool(vkDev.get(), pool, nullptr));
  const VkQueryPoolCreateInfo qpci = //
    {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, nullptr, 0, VK_QUERY_TYPE_TIMESTAMP, allocatedCount, 0};
  VULKAN_EXIT_ON_FAIL(vkDev.vkCreateQueryPool(vkDev.get(), &qpci, nullptr, ptr(pool)));
  VULKAN_LOG_CALL(Backend::cb.wCmdResetQueryPool(pool, 0, allocatedCount));
}

void TimestampQueryBlock::cleanup()
{
  VulkanDevice &vkDev = Globals::VK::dev;
  VULKAN_LOG_CALL(vkDev.vkDestroyQueryPool(vkDev.get(), pool, nullptr));
  pool = VulkanNullHandle();
  allocatedCount = 0;
  work_item_id = 0;
  data.clear();
  dataIndexes.clear();
}

void TimestampQueryBlock::fillDataFromPool()
{
  G_ASSERTF(dataIndexes.size() == count, "vulkan: counted %u GPU timestamps but index filled only for %u", count, dataIndexes.size());
  VulkanDevice &vkDev = Globals::VK::dev;

  if (data.size())
    VULKAN_EXIT_ON_FAIL(vkDev.vkGetQueryPoolResults(vkDev.get(), pool, 0, data.size(), sizeof(uint64_t) * data.size(), data.data(),
      sizeof(uint64_t), VK_QUERY_RESULT_64_BIT));
  setCompleted(true);
}

void TimestampQueryManager::shutdown()
{
  for (TimestampQueryBlock &i : blocks)
    i.cleanup();
  lastQueryIdx = 0;
}
