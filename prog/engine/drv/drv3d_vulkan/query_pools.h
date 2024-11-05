// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// various query pools

#include <generic/dag_objectPool.h>
#include <osApiWrappers/dag_critSec.h>

#include "driver.h"

namespace drv3d_vulkan
{

class Buffer;
struct CmdBeginSurvey;
struct CmdEndSurvey;

struct ConditionalRenderingState;

struct QueryPool
{
  typedef uint64_t BlockMask_t;
  VulkanQueryPoolHandle pool;
  // if bit is set then it was used if not then it was allocated but never used for any rendering
  BlockMask_t validMask = 0;

  BlockMask_t toBit(uint32_t index)
  {
    G_ASSERT(index < POOL_SIZE);
    return 1ULL << index;
  }

  void markUnused(uint32_t index) { validMask &= ~toBit(index); }

  void markUsed(uint32_t index) { validMask |= toBit(index); }

  bool isFree(uint32_t index) { return (validMask & toBit(index)) == 0; }

  static constexpr int POOL_SIZE = sizeof(BlockMask_t) * 8;
};

struct SurveyQueryPool : QueryPool
{
  Buffer *dataStore = nullptr;
};

class SurveyQueryManager
{
  eastl::vector<SurveyQueryPool> surveyPool;
  eastl::vector<SurveyQueryPool::BlockMask_t> surveyPoolAllocMask;
  WinCritSec surveyPoolGuard;

  uint32_t openSurveyScopeCount = 0;

  void resetSurveyStateImpl(uint32_t name);
  void createSurveyPoolImpl();

public:
  SurveyQueryManager() = default;
  SurveyQueryManager(const SurveyQueryManager &) = delete;

  CmdBeginSurvey start(uint32_t name);
  CmdEndSurvey end(uint32_t name);
  ConditionalRenderingState startCondRender(uint32_t name) const;
  void remove(uint32_t name);
  uint32_t add();

  // Returns true if any pixel was recorded to the query
  // Returns true if name is invalid
  bool getResult(uint32_t name);


  void shutdownDataBuffers();
  void shutdownPools();

  bool anyScopesActive() const { return openSurveyScopeCount > 0; }
};

class RaytraceBLASCompactionSizeQueryPool
{
  VulkanQueryPoolHandle pool;

public:
  RaytraceBLASCompactionSizeQueryPool() = default;
  RaytraceBLASCompactionSizeQueryPool(const RaytraceBLASCompactionSizeQueryPool &) = delete;

  VulkanQueryPoolHandle getPool();
  void shutdownPools();
};

} // namespace drv3d_vulkan
