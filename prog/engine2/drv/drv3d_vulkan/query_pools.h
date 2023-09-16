// various query pools
#pragma once
#include "driver.h"
#include <generic/dag_objectPool.h>

namespace drv3d_vulkan
{

class Buffer;
struct CmdBeginSurvey;
struct CmdEndSurvey;
struct CmdInsertTimesampQuery;

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

struct TimestampQueryPool : QueryPool
{
  std::atomic<uint64_t> values[QueryPool::POOL_SIZE] = {};
};

struct TimestampQueryRef
{
  TimestampQueryPool *pool = nullptr;
  uint32_t index = 0;
};

struct TimestampQuery
{
  enum class State
  {
    ISSUED,
    FINALIZED,
  };
  State state = State::FINALIZED;
  TimestampQueryRef timestampHandle;
  uint64_t result = 0;
};

class TimestampQueryManager
{
  eastl::vector<eastl::unique_ptr<TimestampQueryPool>> queryPools;
  ObjectPool<TimestampQuery> objPool;
  WinCritSec guard;

public:
  TimestampQueryManager() = default;
  TimestampQueryManager(const TimestampQueryManager &) = delete;

  TimestampQuery *allocate();
  void release(TimestampQuery *query);

  CmdInsertTimesampQuery issue(TimestampQuery *query);
  bool isReady(TimestampQuery *query);

  void shutdownPools();

  // execution context called methods
private:
  // keep reset list in query manager to avoid resets without writes
  eastl::vector<TimestampQueryRef> resetList;

public:
  void write(const TimestampQueryRef &ref, VulkanCommandBufferHandle frame_cmds);
  void writeResetsAndQueueReadbacks(VulkanCommandBufferHandle cmd_b);
  bool writtenThisFrame() { return resetList.size() > 0; }
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

} // namespace drv3d_vulkan
