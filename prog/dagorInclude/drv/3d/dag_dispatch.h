//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_inttypes.h>
#include <drv/3d/dag_consts.h>

class Sbuffer;

namespace d3d
{
/**
 * @brief Dispatches a compute shader with the specified thread group dimensions.
 *
 * @param thread_group_x The number of thread groups in the X dimension.
 * @param thread_group_y The number of thread groups in the Y dimension.
 * @param thread_group_z The number of thread groups in the Z dimension.
 * @param gpu_pipeline The GPU pipeline to use for dispatching (default: GpuPipeline::GRAPHICS).
 * @warning gpu_pipeline argument doesn't work currently.
 * @return True if the dispatch was successful, false otherwise.
 */
bool dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z,
  GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);

/**
 @brief Dispatches a compute shader indirectly using the specified argument buffer.
 *
 * @param args The argument buffer containing the dispatch parameters. The buffer must be created with the
 * d3d::buffers::create_indirect or d3d::buffers::create_ua_indirect methods and Indirect::Dispatch argument.
 * The buffer should contain the following data: uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z.
 * @param byte_offset The byte offset within the argument buffer (default: 0).
 * @param gpu_pipeline The GPU pipeline to use for dispatching (default: GpuPipeline::GRAPHICS).
 * @warning gpu_pipeline argument doesn't work currently.
 * @return True if the dispatch was successful, false otherwise.
 */
bool dispatch_indirect(Sbuffer *args, uint32_t byte_offset = 0, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline bool dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z, GpuPipeline gpu_pipeline)
{
  return d3di.dispatch(thread_group_x, thread_group_y, thread_group_z, gpu_pipeline);
}
inline bool dispatch_indirect(Sbuffer *args, uint32_t byte_offset, GpuPipeline gpu_pipeline)
{
  return d3di.dispatch_indirect(args, byte_offset, gpu_pipeline);
}
} // namespace d3d
#endif
