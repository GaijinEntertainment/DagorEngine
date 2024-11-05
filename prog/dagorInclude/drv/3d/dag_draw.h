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
 * @brief Draw primitives.
 *
 * @param type The type of primitives to draw. One of PRIM_XXX enum.
 * @param start The starting index of the primitives.
 * @param numprim The number of primitives to draw.
 * @param num_instances The number of instances to draw.
 * @param start_instance The starting instance index.
 * @return True if the draw operation was successful, false otherwise.
 */
bool draw_base(int type, int start, int numprim, uint32_t num_instances, uint32_t start_instance);

/**
 * @brief Draw primitives with a single instance.
 *
 * @param type The type of primitives to draw. One of PRIM_XXX enum.
 * @param start The starting index of the primitives.
 * @param numprim The number of primitives to draw.
 * @return True if the draw operation was successful, false otherwise.
 */
inline bool draw(int type, int start, int numprim) { return draw_base(type, start, numprim, 1, 0); }

/**
 * @brief Draw primitives with multiple instances.
 *
 * @param type The type of primitives to draw. One of PRIM_XXX enum.
 * @param start The starting index of the primitives.
 * @param numprim The number of primitives to draw.
 * @param num_instances The number of instances to draw.
 * @param start_instance The starting instance index.
 * @return True if the draw operation was successful, false otherwise.
 */
inline bool draw_instanced(int type, int start, int numprim, uint32_t num_instances, uint32_t start_instance = 0)
{
  return draw_base(type, start, numprim, num_instances, start_instance);
}

/**
 * @brief Draw indexed primitives.
 *
 * @param type The type of primitives to draw. One of PRIM_XXX enum.
 * @param startind The starting index of the primitives.
 * @param numprim The number of primitives to draw.
 * @param base_vertex The base vertex index.
 * @param num_instances The number of instances to draw.
 * @param start_instance The starting instance index.
 * @return True if the draw operation was successful, false otherwise.
 */
bool drawind_base(int type, int startind, int numprim, int base_vertex, uint32_t num_instances, uint32_t start_instance);

/**
 * @brief Draw indexed primitives with multiple instances.
 *
 * @param type The type of primitives to draw. One of PRIM_XXX enum.
 * @param startind The starting index of the primitives.
 * @param numprim The number of primitives to draw.
 * @param base_vertex The base vertex index.
 * @param num_instances The number of instances to draw.
 * @param start_instance The starting instance index.
 * @return True if the draw operation was successful, false otherwise.
 */
inline bool drawind_instanced(int type, int startind, int numprim, int base_vertex, uint32_t num_instances,
  uint32_t start_instance = 0)
{
  return drawind_base(type, startind, numprim, base_vertex, num_instances, start_instance);
}

/**
 * @brief Draw indexed primitives with a single instance.
 *
 * @param type The type of primitives to draw. One of PRIM_XXX enum.
 * @param startind The starting index of the primitives.
 * @param numprim The number of primitives to draw.
 * @param base_vertex The base vertex index.
 * @return True if the draw operation was successful, false otherwise.
 */
inline bool drawind(int type, int startind, int numprim, int base_vertex)
{
  return drawind_base(type, startind, numprim, base_vertex, 1, 0);
}

/**
 * @brief Draw primitives from a user pointer (rather slow).
 *
 * @deprecated Remove this method. It uncontrollably allocates memory in driver.
 *
 * @param type The type of primitives to draw. One of PRIM_XXX enum.
 * @param numprim The number of primitives to draw.
 * @param ptr The pointer to the vertex data.
 * @param stride_bytes The stride between vertices in bytes.
 * @return True if the draw operation was successful, false otherwise.
 */
bool draw_up(int type, int numprim, const void *ptr, int stride_bytes);

/**
 * @brief Draw indexed primitives from a user pointer (rather slow).
 *
 * @deprecated Remove this method. It uncontrollably allocates memory in driver.
 *
 * @param type The type of primitives to draw. One of PRIM_XXX enum.
 * @param minvert The minimum vertex index.
 * @param numvert The number of vertices.
 * @param numprim The number of primitives to draw.
 * @param ind The pointer to the index data.
 * @param ptr The pointer to the vertex data.
 * @param stride_bytes The stride between vertices in bytes.
 * @return True if the draw operation was successful, false otherwise.
 */
bool drawind_up(int type, int minvert, int numvert, int numprim, const uint16_t *ind, const void *ptr, int stride_bytes);

/**
 * @brief Draw primitives using indirect parameters.
 *
 * @param type The type of primitives to draw. One of PRIM_XXX enum.
 * @param args The buffer containing the draw parameters. The buffer must contain DrawIndirectArgs structure.
 * @param byte_offset The byte offset into the buffer.
 * @return True if the draw operation was successful, false otherwise.
 */
bool draw_indirect(int type, Sbuffer *args, uint32_t byte_offset = 0);

/**
 * @brief Draw indexed primitives using indirect parameters.
 *
 * @param type The type of primitives to draw. One of PRIM_XXX enum.
 * @param args The buffer containing the draw parameters. The buffer must contain DrawIndexedIndirectArgs structure.
 * @param byte_offset The byte offset into the buffer.
 * @return True if the draw operation was successful, false otherwise.
 */
bool draw_indexed_indirect(int type, Sbuffer *args, uint32_t byte_offset = 0);

/**
 * @brief Draw multiple instances of primitives using indirect parameters.
 *
 * @param prim_type The type of primitives to draw. One of PRIM_XXX enum.
 * @param args The buffer containing the draw parameters. The buffer must contain DrawIndirectArgs structures.
 * @param draw_count The number of draw calls.
 * @param stride_bytes The stride between draw parameters in bytes.
 * @param byte_offset The byte offset into the buffer.
 * @return True if the draw operation was successful, false otherwise.
 */
bool multi_draw_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset = 0);

/**
 * @brief Draw multiple instances of indexed primitives using indirect parameters.
 *
 * @param prim_type The type of primitives to draw. One of PRIM_XXX enum.
 * @param args The buffer containing the draw parameters. The buffer must contain DrawIndexedIndirectArgs structures.
 * @param draw_count The number of draw calls.
 * @param stride_bytes The stride between draw parameters in bytes.
 * @param byte_offset The byte offset into the buffer.
 * @return True if the draw operation was successful, false otherwise.
 */
bool multi_draw_indexed_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset = 0);
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline bool draw_base(int type, int start, int numprim, uint32_t num_instances, uint32_t start_instance)
{
  return d3di.draw_base(type, start, numprim, num_instances, start_instance);
}
inline bool drawind_base(int type, int startind, int numprim, int base_vertex, uint32_t num_instances, uint32_t start_instance)
{
  return d3di.drawind_base(type, startind, numprim, base_vertex, num_instances, start_instance);
}
inline bool draw_up(int type, int numprim, const void *ptr, int stride_bytes)
{
  return d3di.draw_up(type, numprim, ptr, stride_bytes);
}
inline bool drawind_up(int type, int minvert, int numvert, int numprim, const uint16_t *ind, const void *ptr, int stride_bytes)
{
  return d3di.drawind_up(type, minvert, numvert, numprim, ind, ptr, stride_bytes);
}
inline bool draw_indirect(int type, Sbuffer *args, uint32_t byte_offset) { return d3di.draw_indirect(type, args, byte_offset); }
inline bool draw_indexed_indirect(int type, Sbuffer *args, uint32_t byte_offset)
{
  return d3di.draw_indexed_indirect(type, args, byte_offset);
}
inline bool multi_draw_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  return d3di.multi_draw_indirect(prim_type, args, draw_count, stride_bytes, byte_offset);
}
inline bool multi_draw_indexed_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  return d3di.multi_draw_indexed_indirect(prim_type, args, draw_count, stride_bytes, byte_offset);
}
} // namespace d3d
#endif
