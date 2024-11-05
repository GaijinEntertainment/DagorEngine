//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

class BaseTexture;

namespace d3d
{

/**
 * @brief Opaque type representing a buffer used for updating resources with content,
 * such as streaming in texture data.
 */
class ResUpdateBuffer;

/**
 * @brief Allocates a update buffer to update the subregion described by
 * offset_x, offset_y, offset_z, width, height and depth.
 *
 * @details
 * - dest_base_texture can not be nullptr.
 * - dest_mip must be a valid mipmap level for dest_base_texture
 * - dest_slice must be a valid array index / cube face for dest_base_texture
 *   when it is a array, cube or cube array texture, otherwise has to be 0
 * - offset_x must be within the width of dest_base_texture of miplevel dest_mip and
 *   aligned to the texture format block size
 * - offset_y must be within the height of dest_base_texture of miplevel dest_mip and
 *   aligned to the texture format block size
 * - offset_z must be within the depth of dest_base_texture of miplevel dest_mip when
 *   the texture is a vol tex, otherwise has to be 0
 * - width plus offset_x must be within the width of dest_base_texture of miplevel dest_mip and
 *   aligned to the texture format block size
 * - height plus offset_y must be within the height of dest_base_texture of miplevel dest_mip and
 *   aligned to the texture format block size
 * - depth plus offset_z must be within the depth of dest_base_texture of miplevel dest_mip when
 *   the texture is a vol tex, otherwise has to be 1
 *
 * May return nullptr if either inputs violate the rules above, the driver can currently not provide
 * the memory required or the driver is unable to perform the needed copy operation on update.
 */
ResUpdateBuffer *allocate_update_buffer_for_tex_region(BaseTexture *dest_base_texture, unsigned dest_mip, unsigned dest_slice,
  unsigned offset_x, unsigned offset_y, unsigned offset_z, unsigned width, unsigned height, unsigned depth);

/**
 * @brief Allocates update buffer in system memory to be filled directly and then dispatched
 * to apply_tex_update_buffer.
 * @warning This method can fail if too much allocations happens in N-frame period.
 * Caller should retry after rendering frame on screen if this happens.
 *
 * @param dest_tex destination texture
 * @param dest_mip destination mip level
 * @param dest_slice destination slice
 * @return pointer to update buffer or nullptr if allocation failed
 */
ResUpdateBuffer *allocate_update_buffer_for_tex(BaseTexture *dest_tex, int dest_mip, int dest_slice);

/**
 * @brief Releases update buffer (clears pointer afterwards); skips any update, just releases data
 * @param rub update buffer to release
 */
void release_update_buffer(ResUpdateBuffer *&rub);

/**
 * @brief Returns data address to fill update data
 * @param rub update buffer
 * @return pointer to memory that should be filled with data
 */
char *get_update_buffer_addr_for_write(ResUpdateBuffer *rub);

/**
 * @brief Returns size of update buffer
 * @param rub update buffer
 * @return size of update buffer in bytes
 */
size_t get_update_buffer_size(ResUpdateBuffer *rub);

/**
 * @brief Returns pitch of update buffer (if applicable)
 * @param rub update buffer
 * @return pitch of update buffer
 */
size_t get_update_buffer_pitch(ResUpdateBuffer *rub);

/**
 * @brief Returns the pitch of one 2d image slice for volumetric textures.
 * @param rub update buffer
 * @return pitch of of a slice in the update buffer
 */
size_t get_update_buffer_slice_pitch(ResUpdateBuffer *rub);

/**
 * @brief applies update to target d3dres and releases update buffer (clears pointer afterwards)
 * @param src_rub update buffer to apply
 * @return true if update was successful, false if update failed
 */
bool update_texture_and_release_update_buffer(ResUpdateBuffer *&src_rub);

} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline ResUpdateBuffer *allocate_update_buffer_for_tex_region(BaseTexture *dest_base_texture, unsigned dest_mip, unsigned dest_slice,
  unsigned offset_x, unsigned offset_y, unsigned offset_z, unsigned width, unsigned height, unsigned depth)
{
  return d3di.allocate_update_buffer_for_tex_region(dest_base_texture, dest_mip, dest_slice, offset_x, offset_y, offset_z, width,
    height, depth);
}

inline ResUpdateBuffer *allocate_update_buffer_for_tex(BaseTexture *dest_tex, int dest_mip, int dest_slice)
{
  return d3di.allocate_update_buffer_for_tex(dest_tex, dest_mip, dest_slice);
}
inline void release_update_buffer(ResUpdateBuffer *&rub) { d3di.release_update_buffer(rub); }
inline char *get_update_buffer_addr_for_write(ResUpdateBuffer *rub) { return d3di.get_update_buffer_addr_for_write(rub); }
inline size_t get_update_buffer_size(ResUpdateBuffer *rub) { return d3di.get_update_buffer_size(rub); }
inline size_t get_update_buffer_pitch(ResUpdateBuffer *rub) { return d3di.get_update_buffer_pitch(rub); }
inline size_t get_update_buffer_slice_pitch(ResUpdateBuffer *rub) { return d3di.get_update_buffer_slice_pitch(rub); }
inline bool update_texture_and_release_update_buffer(ResUpdateBuffer *&src_rub)
{
  return d3di.update_texture_and_release_update_buffer(src_rub);
}
} // namespace d3d
#endif
