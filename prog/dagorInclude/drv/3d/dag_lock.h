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
 * @brief The GpuAutoLock struct is a lock that automatically acquires and releases a GPU lock.
 *        It is used to protect critical sections of code that interact with the GPU.
 */
struct GpuAutoLock
{
  GpuAutoLock();
  ~GpuAutoLock();
};

/**
 * @brief The LoadingAutoLock struct is a non-exclusive lock that protects a thread from GPU reset.
 *        It is used to ensure that the GPU is not reset while a thread is performing loading operations.
 */
struct LoadingAutoLock
{
  LoadingAutoLock() { driver_command(Drv3dCommand::ACQUIRE_LOADING); }
  ~LoadingAutoLock() { driver_command(Drv3dCommand::RELEASE_LOADING); }
};

/**
 * @brief The GPUWorkloadSplit struct is used for scoped conditional GPU workload splitting.
 *        It allows splitting the GPU workload based on certain conditions.
 */
struct GPUWorkloadSplit
{
  bool needSplitAtEnd; /**< Flag indicating whether a split is needed at the end. */

  /**
   * @brief Constructs a GPUWorkloadSplit object.
   * @param do_split Flag indicating whether to split the GPU workload.
   * @param split_at_end Flag indicating whether to split at the end.
   * @param marker A marker used for splitting the workload.
   */
  GPUWorkloadSplit(bool do_split, bool split_at_end, const char *marker) : needSplitAtEnd(do_split && split_at_end)
  {
    if (!do_split)
      return;
    d3d::driver_command(Drv3dCommand::D3D_FLUSH, (void *)marker, (void *)0);
  }

  /**
   * @brief Destructs the GPUWorkloadSplit object.
   *        If a split is needed at the end, it performs the split.
   */
  ~GPUWorkloadSplit()
  {
    if (!needSplitAtEnd)
      return;

    d3d::driver_command(Drv3dCommand::D3D_FLUSH, (void *)"split_end", (void *)1);
  }
};

} // namespace d3d
