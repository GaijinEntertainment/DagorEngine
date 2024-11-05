//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_inttypes.h>

class Sbuffer;

namespace d3d
{
/**
 * @brief Dispatches a mesh shader with the specified thread group dimensions.
 *
 * Max value for each direction is 64k, product of all dimensions can not exceed 2^22
 *
 * @param thread_group_x The number of thread groups in the X direction.
 * @param thread_group_y The number of thread groups in the Y direction.
 * @param thread_group_z The number of thread groups in the Z direction.
 */
void dispatch_mesh(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z);

/**
 * @brief Dispatches a mesh shader indirectly using the provided arguments.
 *
 * @param args The buffer containing the dispatch arguments. Buffer should contain structures of the following layout:
 * struct DispatchArgs
 * {
 *   uint32_t thread_group_x;
 *   uint32_t thread_group_y;
 *   uint32_t thread_group_z;
 * };
 * @param dispatch_count The number of dispatches to execute.
 * @param stride_bytes The stride between dispatch arguments in bytes.
 * @param byte_offset The byte offset into the buffer where the dispatch arguments start (default = 0).
 */
void dispatch_mesh_indirect(Sbuffer *args, uint32_t dispatch_count, uint32_t stride_bytes, uint32_t byte_offset = 0);

/**
 * @brief Dispatches a mesh shader indirectly using the provided arguments and a count buffer.
 *
 *  Variant of dispatch_mesh_indirect where 'dispatch_count' is read by the GPU from 'count' buffer at 'count_offset' (as uint32_t),
 *  this value can not exceed 'max_count'.
 *
 * @param args The buffer containing the dispatch arguments.
 * @param args_stride_bytes The stride between dispatch arguments in bytes.
 * @param args_byte_offset The byte offset into the buffer where the dispatch arguments start.
 * @param count The buffer containing the dispatch count.
 * @param count_byte_offset The byte offset into the count buffer where the dispatch count starts.
 * @param max_count The maximum value that the dispatch count can have.
 */
void dispatch_mesh_indirect_count(Sbuffer *args, uint32_t args_stride_bytes, uint32_t args_byte_offset, Sbuffer *count,
  uint32_t count_byte_offset, uint32_t max_count);
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline void dispatch_mesh(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z)
{
  d3di.dispatch_mesh(thread_group_x, thread_group_y, thread_group_z);
}
inline void dispatch_mesh_indirect(Sbuffer *args, uint32_t dispatch_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  d3di.dispatch_mesh_indirect(args, dispatch_count, stride_bytes, byte_offset);
}
inline void dispatch_mesh_indirect_count(Sbuffer *args, uint32_t args_stride_bytes, uint32_t args_byte_offset, Sbuffer *count,
  uint32_t count_byte_offset, uint32_t max_count)
{
  d3di.dispatch_mesh_indirect_count(args, args_stride_bytes, args_byte_offset, count, count_byte_offset, max_count);
}
} // namespace d3d
#endif
