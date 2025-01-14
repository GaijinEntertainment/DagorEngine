//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_commands.h>

namespace d3d
{

/**
 * @brief Enables async pipeline compilation in its scope
 *        when allow_compute_pipelines is false, only graphics pipelines are async compiled (compute compiled as is)
 *        override parameter in constructor may be used to supply game logic for async pipeline enable/disable
 */
template <bool allow_compute_pipelines = true>
struct AutoPipelineAsyncCompile
{
  AutoPipelineAsyncCompile()
  {
    d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_BEGIN, nullptr, allow_compute_pipelines ? nullptr : (void *)-1);
  }
  AutoPipelineAsyncCompile(void *override_value)
  {
    d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_BEGIN, override_value,
      allow_compute_pipelines ? nullptr : (void *)-1);
  }
  ~AutoPipelineAsyncCompile() { d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_END); }
};

/**
 * @brief Disables async pipeline compilation in its scope
 */
struct AutoPipelineSyncCompile
{
  AutoPipelineSyncCompile() { d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_BEGIN, (void *)-1); }
  ~AutoPipelineSyncCompile() { d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_END); }
};

/**
 * @brief When feedback supported, enables async pipeline compilation and
 *        captures async compilation skip count into feedback_ptr in its scope
 *        needRetry field will be set if any pipeline was skipped in previous frames
 *        externalNeedRetry field will be set if any other (external) AsyncCompileFeedback in frame has needRetry set
 *        when feedback not supported, does nothing
 *        when allow_compute_pipelines is false, only graphics pipelines are async compiled (compute compiled as is)
 */
template <bool allow_compute_pipelines = true>
class AutoPipelineAsyncCompileFeedback
{
  bool needRetry;
  bool supported;
  bool externalNeedRetry;

public:
  bool isRetryNeeded() const { return needRetry; }
  bool isExternalRetryNeeded() const { return externalNeedRetry; }
  bool isSupported() const { return supported; }
  AutoPipelineAsyncCompileFeedback(uint32_t *feedback_ptr)
  {
    if (!feedback_ptr)
    {
      externalNeedRetry = false;
      needRetry = false;
      supported = false;
      return;
    }
    int cmdRet = d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILATION_FEEDBACK_BEGIN, (void *)feedback_ptr);
    // decode bitset
    externalNeedRetry = (cmdRet & 4) > 0;
    needRetry = (cmdRet & 2) > 0;
    supported = (cmdRet & 1) > 0;
    if (supported)
      d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_BEGIN, (void *)(uintptr_t)(0),
        allow_compute_pipelines ? nullptr : (void *)-1);
  }
  ~AutoPipelineAsyncCompileFeedback()
  {
    if (!supported)
      return;
    d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILATION_FEEDBACK_END);
    d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_END);
  }
};

} // namespace d3d
