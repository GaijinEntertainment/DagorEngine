//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "gpuReadbackResult.h"

#include <EASTL/vector.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/unique_ptr.h>

#include <util/dag_globDef.h>

#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_info.h>
#include "3d/dag_sbufferIDHolder.h"
#include "3d/dag_ringCPUQueryLock.h"
#include "shaders/dag_computeShaders.h"
#include "perfMon/dag_statDrv.h"
#include <3d/dag_resPtr.h>


struct GpuReadbackQuerySystemDescription
{
  // Mandatory to set explicitly (defaults are invalid)
  int maxQueriesPerFrame = 0;
  int maxQueriesPerDispatch = 0;
  const char *computeShaderName = nullptr;
  const char *resultBufferName = nullptr;
  int resultBufferSlot = -1;
  int resultsOffsetSlot = -1;
  const char *inputBufferName = nullptr;

  // Optional to set explicitly (defaults are valid)
  int numInputsPerThreadGroup = 1;
  bool setNumInputsToShader = false;
  int numInputsSlot = -1;

  bool isValid() const
  {
    return maxQueriesPerFrame > 0 && computeShaderName && resultBufferName && resultBufferSlot >= 0 && inputBufferName &&
           numInputsPerThreadGroup > 0 && (!setNumInputsToShader || numInputsSlot >= 0);
  }
};

template <typename InputT, typename ResultT>
class GpuReadbackQuerySystem
{
public:
  explicit GpuReadbackQuerySystem(const GpuReadbackQuerySystemDescription &desc);

  GpuReadbackQuerySystem(const GpuReadbackQuerySystem &) = delete;
  GpuReadbackQuerySystem &operator=(const GpuReadbackQuerySystem &) = delete;
  GpuReadbackQuerySystem(GpuReadbackQuerySystem &&) = delete;
  GpuReadbackQuerySystem &operator=(GpuReadbackQuerySystem &&) = delete;

  bool isValid() const;
  void update();
  void beforeDeviceReset();
  void afterDeviceReset();
  int query(const InputT &input);
  GpuReadbackResultState getQueryResult(int query_id, ResultT &result);
  bool getQueryInput(int query_id, InputT &input);

private:
  enum class GpuReadbackQueryState
  {
    NEW,
    DISPATCHED,
    COMPLETED,
    FAILED,
    READ
  };

  struct GpuReadbackQuery
  {
    int id;
    int index;
    int frame;
    GpuReadbackQueryState state;
    InputT input;
    ResultT result;

    GpuReadbackQuery()
    {
      id = index = frame = -1;
      state = GpuReadbackQueryState::NEW;
      secure_zero_memory(&input, sizeof(input));
      secure_zero_memory(&result, sizeof(result));
    }

    GpuReadbackQuery(int _id, const InputT &_input) : GpuReadbackQuery()
    {
      id = _id;
      input = _input;
    }
  };

  GpuReadbackQuerySystemDescription querySystemDesc;
  eastl::unique_ptr<ComputeShaderElement> queryCs;
  RingCPUBufferLock resultRingBuffer;
  UniqueBufHolder inputBuffer;
  eastl::vector<InputT> inputs;
  ska::flat_hash_map<int, GpuReadbackQuery> queryMap;

  int nextQueryId = 0;
  int newQueriesSinceLastFrame = 0;
  bool isResettingDevice = false;

  void dispatchNewQueries();
  void completeQueries();
  void deleteReadQueries();

  void dispatch(Sbuffer *resultBuffer, uint32_t offset, uint32_t num_inputs);
};

template <typename InputT, typename ResultT>
GpuReadbackQuerySystem<InputT, ResultT>::GpuReadbackQuerySystem(const GpuReadbackQuerySystemDescription &desc)
{
  if (!desc.isValid())
  {
    logerr("GpuReadbackQuerySystem: Invalid desc passed to ctor.");
    return;
  }

  queryCs.reset(new_compute_shader(desc.computeShaderName, true));
  if (queryCs == nullptr)
  {
    logerr("GpuReadbackQuerySystem: Could not find compute shader.");
    return;
  }

  resultRingBuffer.init(sizeof(ResultT), desc.maxQueriesPerFrame, 3, desc.resultBufferName, SBCF_UA_STRUCTURED_READBACK, 0, false);

  const uint32_t inputBufferFlags = SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES | SBCF_DYNAMIC;
  inputBuffer = dag::create_sbuffer(sizeof(InputT), desc.maxQueriesPerFrame, inputBufferFlags, 0, desc.inputBufferName);

  inputs.resize(desc.maxQueriesPerFrame);

  querySystemDesc = desc;
}

template <typename InputT, typename ResultT>
bool GpuReadbackQuerySystem<InputT, ResultT>::isValid() const
{
  return queryCs != nullptr;
}

template <typename InputT, typename ResultT>
void GpuReadbackQuerySystem<InputT, ResultT>::update()
{
  deleteReadQueries();

  if (d3d::is_in_device_reset_now())
    return;
  dispatchNewQueries();
  completeQueries();
}

template <typename InputT, typename ResultT>
void GpuReadbackQuerySystem<InputT, ResultT>::beforeDeviceReset()
{
  isResettingDevice = true;
  for (auto &[_, query] : queryMap)
    if (query.state == GpuReadbackQueryState::NEW || query.state == GpuReadbackQueryState::DISPATCHED)
      query.state = GpuReadbackQueryState::FAILED;
}

template <typename InputT, typename ResultT>
void GpuReadbackQuerySystem<InputT, ResultT>::afterDeviceReset()
{
  isResettingDevice = false;
}

template <typename InputT, typename ResultT>
int GpuReadbackQuerySystem<InputT, ResultT>::query(const InputT &input)
{
  if (newQueriesSinceLastFrame >= querySystemDesc.maxQueriesPerFrame || isResettingDevice)
    return -1;

  GpuReadbackQuery query(nextQueryId, input);
  queryMap.emplace(query.id, query);

  newQueriesSinceLastFrame++;
  nextQueryId = (nextQueryId + 1) % (INT_MAX - 1);

  return query.id;
}

template <typename InputT, typename ResultT>
GpuReadbackResultState GpuReadbackQuerySystem<InputT, ResultT>::getQueryResult(int query_id, ResultT &result)
{
  auto queryIt = queryMap.find(query_id);
  if (queryIt == queryMap.end())
    return GpuReadbackResultState::ID_NOT_FOUND;

  GpuReadbackQuery &query = queryIt->second;
  if (query.state == GpuReadbackQueryState::COMPLETED)
  {
    result = query.result;
    query.state = GpuReadbackQueryState::READ;
    return GpuReadbackResultState::SUCCEEDED;
  }
  if (query.state == GpuReadbackQueryState::FAILED)
  {
    query.state = GpuReadbackQueryState::READ;
    return GpuReadbackResultState::FAILED;
  }
  return GpuReadbackResultState::IN_PROGRESS;

  return GpuReadbackResultState::ID_NOT_FOUND;
}

template <typename InputT, typename ResultT>
bool GpuReadbackQuerySystem<InputT, ResultT>::getQueryInput(int query_id, InputT &input)
{
  auto queryIt = queryMap.find(query_id);
  if (queryIt != queryMap.end())
  {
    input = queryIt->second.input;
    return true;
  }
  return false;
}

template <typename InputT, typename ResultT>
void GpuReadbackQuerySystem<InputT, ResultT>::dispatch(Sbuffer *resultBuffer, uint32_t offset, uint32_t num_inputs)
{
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, querySystemDesc.resultBufferSlot, VALUE), resultBuffer);
  alignas(16) int offsetForShader[] = {static_cast<int>(offset), 0, 0, 0};
  d3d::set_const(STAGE_CS, querySystemDesc.resultsOffsetSlot, offsetForShader, 1);

  if (querySystemDesc.setNumInputsToShader)
  {
    alignas(16) int numInputsForShader[4]{static_cast<int>(num_inputs), 0, 0, 0};
    d3d::set_const(STAGE_CS, querySystemDesc.numInputsSlot, (const float *)&numInputsForShader, 1);
  }

  const int numThreadGroups = (num_inputs + querySystemDesc.numInputsPerThreadGroup - 1) / querySystemDesc.numInputsPerThreadGroup;
  queryCs->dispatch(numThreadGroups, 1, 1);
  d3d::resource_barrier({resultBuffer, RB_NONE});
}


template <typename InputT, typename ResultT>
void GpuReadbackQuerySystem<InputT, ResultT>::dispatchNewQueries()
{
  TIME_PROFILE(dispatchNewQueries);
  int frame = 0;
  Sbuffer *resultBuffer = (Sbuffer *)resultRingBuffer.getNewTarget(frame);
  if (!resultBuffer)
    return;

  uint32_t numInputs = 0;

  for (auto &[_, query] : queryMap)
  {
    if (query.state == GpuReadbackQueryState::NEW)
    {
      query.index = numInputs;
      query.frame = frame;
      query.state = GpuReadbackQueryState::DISPATCHED;
      inputs[numInputs++] = query.input;
    }
  }


  if (numInputs > 0)
  {
    G_ASSERT(numInputs <= querySystemDesc.maxQueriesPerFrame);

    TIME_D3D_PROFILE_NAME(gpu_readback_query, querySystemDesc.computeShaderName);

    inputBuffer.getBuf()->updateData(0, (uint32_t)(numInputs * sizeof(inputs[0])), inputs.data(), VBLOCK_DISCARD | VBLOCK_WRITEONLY);
    for (uint32_t offset = 0; offset < numInputs; offset += querySystemDesc.maxQueriesPerDispatch)
      dispatch(resultBuffer, offset, min(static_cast<unsigned int>(querySystemDesc.maxQueriesPerDispatch), numInputs - offset));
  }

  resultRingBuffer.startCPUCopy();
  newQueriesSinceLastFrame = 0;
}

template <typename InputT, typename ResultT>
void GpuReadbackQuerySystem<InputT, ResultT>::completeQueries()
{
  TIME_PROFILE(completeQueries);
  int stride;
  uint32_t frame;
  while (ResultT *data = reinterpret_cast<ResultT *>(resultRingBuffer.lock(stride, frame, false)))
  {
    for (auto &[_, query] : queryMap)
    {
      if (query.state == GpuReadbackQueryState::DISPATCHED)
      {
        if (query.frame == frame)
        {
          query.result = data[query.index];
          query.state = GpuReadbackQueryState::COMPLETED;
        }
        else if (query.frame < frame)
        {
          logwarn("GpuReadbackQuerySystem: resultRingBuffer.lock skipped frame: %d", query.frame);
          query.state = GpuReadbackQueryState::FAILED;
        }
      }
    }
    resultRingBuffer.unlock();
  }
}

template <typename InputT, typename ResultT>
void GpuReadbackQuerySystem<InputT, ResultT>::deleteReadQueries()
{
  TIME_PROFILE(deleteReadQueries);

  for (auto it = queryMap.begin(); it != queryMap.end();)
  {
    if (it->second.state == GpuReadbackQueryState::READ)
      it = queryMap.erase(it);
    else
      ++it;
  }
}
