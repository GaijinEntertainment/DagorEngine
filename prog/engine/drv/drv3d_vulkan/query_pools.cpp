#include "query_pools.h"
#include "device.h"
#include <EASTL/sort.h>

using namespace drv3d_vulkan;

// FIXME: ensure guard is placed correctly in SurveyQueryManager,
// as it looks like context commands can be executed out of sync
// FIXME: remove poolIndex/surveyId lookup copy-paste

void SurveyQueryManager::resetSurveyStateImpl(uint32_t name)
{
  uint32_t poolIndex = name / QueryPool::POOL_SIZE;
  uint32_t surveyId = name % QueryPool::POOL_SIZE;

  surveyPool[poolIndex].validMask &= ~(QueryPool::BlockMask_t(1) << surveyId);
}

void SurveyQueryManager::createSurveyPoolImpl()
{
  WinAutoLock lock(surveyPoolGuard);
  SurveyQueryPool pool;

  Device &device = get_device();
  VulkanDevice &vkDev = device.getVkDevice();

  VkQueryPoolCreateInfo qpci = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
  qpci.queryType = VK_QUERY_TYPE_OCCLUSION;
  qpci.queryCount = SurveyQueryPool::POOL_SIZE;
  VULKAN_EXIT_ON_FAIL(vkDev.vkCreateQueryPool(vkDev.get(), &qpci, NULL, ptr(pool.pool)));

  pool.dataStore = device.createBuffer(sizeof(uint32_t) * SurveyQueryPool::POOL_SIZE, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER, 1,
    BufferMemoryFlags::NONE);
  device.setBufName(pool.dataStore, "survey query pool");
  surveyPool.push_back(pool);
}

CmdBeginSurvey SurveyQueryManager::start(uint32_t name)
{
  ++openSurveyScopeCount;

  uint32_t poolIndex = name / QueryPool::POOL_SIZE;
  uint32_t surveyId = name % QueryPool::POOL_SIZE;

  surveyPool[poolIndex].validMask |= QueryPool::BlockMask_t(1) << surveyId;
  return CmdBeginSurvey{surveyPool[poolIndex].pool, surveyId};
}

CmdEndSurvey SurveyQueryManager::end(uint32_t name)
{
  --openSurveyScopeCount;

  uint32_t poolIndex = name / QueryPool::POOL_SIZE;
  uint32_t surveyId = name % QueryPool::POOL_SIZE;

  return CmdEndSurvey //
    {surveyPool[poolIndex].pool, surveyId, surveyPool[poolIndex].dataStore};
}

ConditionalRenderingState SurveyQueryManager::startCondRender(uint32_t name) const
{
  uint32_t poolIndex = name / QueryPool::POOL_SIZE;
  uint32_t surveyId = name % QueryPool::POOL_SIZE;

  auto *buf = surveyPool[poolIndex].dataStore;

  return ConditionalRenderingState //
    {BufferRef(buf, sizeof(uint32_t)), surveyId * sizeof(uint32_t)};
}

void SurveyQueryManager::remove(uint32_t name)
{
  WinAutoLock lock(surveyPoolGuard);
  surveyPoolAllocMask[name / QueryPool::POOL_SIZE] &= ~(QueryPool::BlockMask_t(1) << (name % QueryPool::POOL_SIZE));
}

uint32_t SurveyQueryManager::add()
{
  for (int i = 0; i < surveyPoolAllocMask.size(); ++i)
  {
    QueryPool::BlockMask_t &allocMask = surveyPoolAllocMask[i];
    if (!~allocMask) // each bit is one slot
      continue;

    for (int j = 0; j < QueryPool::POOL_SIZE; ++j)
    {
      const QueryPool::BlockMask_t bit = QueryPool::BlockMask_t(1) << j;
      if (allocMask & bit)
        continue;

      allocMask |= bit;
      uint32_t name = i * QueryPool::POOL_SIZE + j;
      resetSurveyStateImpl(name);
      return name;
    }
  }

  uint32_t pool = surveyPoolAllocMask.size();
  surveyPoolAllocMask.push_back(1);
  createSurveyPoolImpl();

  return pool * QueryPool::POOL_SIZE;
}

bool SurveyQueryManager::getResult(uint32_t name)
{
  if (name == -1)
    return true;

  uint32_t poolIndex = name / QueryPool::POOL_SIZE;
  uint32_t surveyId = name % QueryPool::POOL_SIZE;

  VulkanDevice &vkDev = get_device().getVkDevice();

  WinAutoLock lock(surveyPoolGuard);
  if (surveyPool[poolIndex].validMask & (QueryPool::BlockMask_t(1) << surveyId))
  {
    uint32_t result = 0;
    VULKAN_EXIT_ON_FAIL(vkDev.vkGetQueryPoolResults(vkDev.get(), surveyPool[poolIndex].pool, surveyId, 1, sizeof(result), &result, 0,
      VK_QUERY_RESULT_WAIT_BIT));
    return result != 0;
  }
  else
    return true;
}

void SurveyQueryManager::shutdownDataBuffers()
{
  // delete the buffers here and later the pools
  DeviceContext &ctx = get_device().getContext();

  for (auto &&pool : surveyPool)
    ctx.destroyBuffer(pool.dataStore);
}

void SurveyQueryManager::shutdownPools()
{
  VulkanDevice &vkDev = get_device().getVkDevice();
  for (auto &&pool : surveyPool)
  {
    VULKAN_LOG_CALL(vkDev.vkDestroyQueryPool(vkDev.get(), pool.pool, nullptr));
  }
  surveyPool.clear();
}

TimestampQuery *TimestampQueryManager::allocate()
{
  WinAutoLock lock(guard);

  return objPool.allocate();
}

void TimestampQueryManager::release(TimestampQuery *query)
{
  WinAutoLock lock(guard);

  if (TimestampQuery::State::ISSUED == query->state)
    query->timestampHandle.pool->markUnused(query->timestampHandle.index);

  objPool.free(query);
}

bool TimestampQueryManager::isReady(TimestampQuery *query)
{
  if (TimestampQuery::State::ISSUED == query->state)
  {
    uint64_t value = query->timestampHandle.pool->values[query->timestampHandle.index].load();
    if (value != 0)
    {
      query->result = value;
      query->state = TimestampQuery::State::FINALIZED;

      WinAutoLock lock(guard);
      query->timestampHandle.pool->markUnused(query->timestampHandle.index);
      return true;
    }
    return false;
  }
  return true;
}

CmdInsertTimesampQuery TimestampQueryManager::issue(TimestampQuery *query)
{
  TimestampQueryRef queryRef;
  WinAutoLock lock(guard);
  auto poolRef = eastl::find_if(begin(queryPools), end(queryPools),
    [=](const eastl::unique_ptr<TimestampQueryPool> &pool) //
    {
      // each bit indicates if its allocated, so if we flip it and its 0
      // all slots are allocated
      return 0 != ~pool->validMask;
    });

  // no empty pool, create a new one
  if (poolRef == end(queryPools))
    poolRef = queryPools.emplace(end(queryPools), new TimestampQueryPool);

  queryRef.pool = poolRef->get();
  for (queryRef.index = 0; queryRef.index < TimestampQueryPool::POOL_SIZE; ++queryRef.index)
  {
    if (!queryRef.pool->isFree(queryRef.index))
      continue;

    queryRef.pool->markUsed(queryRef.index);
    // has to be set to 0 or user will mistakenly use old values resulting in wrong time stamp results
    // FIXME: we can read old result if it was not finished before we used index again, not written to it but readed
    queryRef.pool->values[queryRef.index].store(0);
    break;
  }

  // be aware if bitmagic are wrong
  G_ASSERTF(queryRef.index < TimestampQueryPool::POOL_SIZE,
    "vulkan: timestamps: can't add to pool with free slots. validMask %llX asked index %u", queryRef.pool->validMask, queryRef.index);

  query->state = TimestampQuery::State::ISSUED;
  query->timestampHandle = queryRef;
  CmdInsertTimesampQuery result{queryRef};
  return result;
}

void TimestampQueryManager::write(const TimestampQueryRef &ref, VulkanCommandBufferHandle frame_cmds)
{
  VulkanDevice &vkDev = get_device().getVkDevice();

  // lazy allocate pool
  if (is_null(ref.pool->pool))
  {
    const VkQueryPoolCreateInfo qpci = //
      {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, nullptr, 0, VK_QUERY_TYPE_TIMESTAMP, TimestampQueryPool::POOL_SIZE, 0};

    VULKAN_EXIT_ON_FAIL(vkDev.vkCreateQueryPool(vkDev.get(), &qpci, nullptr, ptr(ref.pool->pool)));
  }

  VULKAN_LOG_CALL(vkDev.vkCmdWriteTimestamp(frame_cmds, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, ref.pool->pool, ref.index));
  resetList.push_back(ref);
}

void TimestampQueryManager::writeResetsAndQueueReadbacks(VulkanCommandBufferHandle cmd_b)
{
  eastl::sort(begin(resetList), end(resetList),
    [](const TimestampQueryRef &l, const TimestampQueryRef &r) //
    {
      if (l.pool < r.pool)
        return true;
      if (l.pool > r.pool)
        return false;
      return l.index < r.index;
    });

  auto at = begin(resetList);
  const auto stop = end(resetList);

  Device &device = get_device();
  ContextBackend &back = device.getContext().getBackend();
  VulkanDevice &vkDev = device.getVkDevice();

  FrameInfo &frame = back.contextState.frame.get();
  // this tries to group contiguous pool elements together to reduce api calls
  while (at != stop)
  {
    frame.pendingTimestamps.push_back(*at);
    auto start = at;
    auto lastAt = at;
    for (++at; at != stop; ++at)
    {
      if (start->pool != at->pool)
        break;
      if (lastAt->index + 1 != at->index)
        break;

      frame.pendingTimestamps.push_back(*at);
      lastAt = at;
    }

    // needs to be this way, at may be same as stop, pointing behind the last valid entry
    auto count = (lastAt->index - start->index) + 1;
    VULKAN_LOG_CALL(vkDev.vkCmdResetQueryPool(cmd_b, start->pool->pool, start->index, count));
  }

  resetList.clear();
}

void TimestampQueryManager::shutdownPools()
{
  VulkanDevice &vkDev = get_device().getVkDevice();
  for (auto &&pool : queryPools)
    if (!is_null(pool->pool))
      VULKAN_LOG_CALL(vkDev.vkDestroyQueryPool(vkDev.get(), pool->pool, nullptr));
  queryPools.clear();
}
