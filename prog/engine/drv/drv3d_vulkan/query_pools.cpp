// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "query_pools.h"
#include <EASTL/sort.h>
#include "globals.h"
#include "vulkan_device.h"
#include "buffer_resource.h"
#include "device_context.h"
#include "backend.h"
#include "vulkan_allocation_callbacks.h"

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

  VulkanDevice &vkDev = Globals::VK::dev;

  VkQueryPoolCreateInfo qpci = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
  qpci.queryType = VK_QUERY_TYPE_OCCLUSION;
  qpci.queryCount = SurveyQueryPool::POOL_SIZE;
  VULKAN_EXIT_ON_FAIL(vkDev.vkCreateQueryPool(vkDev.get(), &qpci, VKALLOC(query_pool), ptr(pool.pool)));

  pool.dataStore = Buffer::create(sizeof(uint32_t) * SurveyQueryPool::POOL_SIZE, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER, 1,
    BufferMemoryFlags::NONE);
  Globals::Dbg::naming.setBufName(pool.dataStore, "survey query pool");
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

  VulkanDevice &vkDev = Globals::VK::dev;

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

void SurveyQueryManager::onDeviceReset()
{
  VulkanDevice &vkDev = Globals::VK::dev;
  for (auto &&pool : surveyPool)
    VULKAN_LOG_CALL(vkDev.vkDestroyQueryPool(vkDev.get(), pool.pool, VKALLOC(query_pool)));
}

void SurveyQueryManager::afterDeviceReset()
{
  VulkanDevice &vkDev = Globals::VK::dev;

  for (auto &&pool : surveyPool)
  {
    VkQueryPoolCreateInfo qpci = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    qpci.queryType = VK_QUERY_TYPE_OCCLUSION;
    qpci.queryCount = SurveyQueryPool::POOL_SIZE;
    VULKAN_EXIT_ON_FAIL(vkDev.vkCreateQueryPool(vkDev.get(), &qpci, VKALLOC(query_pool), ptr(pool.pool)));
  }
}

void SurveyQueryManager::shutdownDataBuffers()
{
  // delete the buffers here and later the pools
  DeviceContext &ctx = Globals::ctx;

  for (auto &&pool : surveyPool)
    ctx.destroyBuffer(pool.dataStore);
}

void SurveyQueryManager::shutdownPools()
{
  VulkanDevice &vkDev = Globals::VK::dev;
  for (auto &&pool : surveyPool)
  {
    VULKAN_LOG_CALL(vkDev.vkDestroyQueryPool(vkDev.get(), pool.pool, VKALLOC(query_pool)));
  }
  surveyPool.clear();
}

VulkanQueryPoolHandle RaytraceBLASCompactionSizeQueryPool::getPool()
{
  // lazy allocate
#if VULKAN_HAS_RAYTRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
  if (is_null(pool))
  {
    const VkQueryPoolCreateInfo qpci = //
      {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, nullptr, 0, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, 1, 0};

    VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateQueryPool(Globals::VK::dev.get(), &qpci, VKALLOC(query_pool), ptr(pool)));
  }
#else
  DAG_FATAL("vulkan: RaytraceBLASCompactionSizeQueryPool used when RT is not available");
#endif
  return pool;
}

void RaytraceBLASCompactionSizeQueryPool::shutdownPools()
{
  if (!is_null(pool))
    VULKAN_LOG_CALL(Globals::VK::dev.vkDestroyQueryPool(Globals::VK::dev.get(), pool, VKALLOC(query_pool)));
}
