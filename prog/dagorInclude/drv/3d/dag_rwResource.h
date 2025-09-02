//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_inttypes.h>

class BaseTexture;
class Sbuffer;

namespace d3d
{
/**
 * @brief Set the read/write (UAV) texture to slot
 *
 * @param shader_stage shader stage (VS, PS, CS)
 * @param slot slot index
 * @param tex texture to set as UAV resoruce
 * @param face face index for cubemaps, 3D textures and texture arrays
 * @param mip_level mip level
 * @param as_uint if true then texture will be viewed as uint in UAV. The texture type should be 32-bit format.
 * https://msdn.microsoft.com/en-us/library/windows/desktop/ff728749(v=vs.85).aspx
 * @return true if success, false otherwise
 */
bool set_rwtex(uint32_t shader_stage, uint32_t slot, BaseTexture *tex, uint32_t face, uint32_t mip_level, bool as_uint = false);

/**
 * @brief Clear UAV texture with integer values
 *
 * @param tex texture to clear
 * @param val clear value
 * @param face face index for cubemaps, 3D textures and texture arrays
 * @param mip_level mip level
 * @return true if success, false otherwise
 */
bool clear_rwtexi(BaseTexture *tex, const uint32_t val[4], uint32_t face, uint32_t mip_level);

/**
 * @brief Clear UAV texture with float values
 *
 * @param tex texture to clear
 * @param val clear value
 * @param face face index for cubemaps, 3D textures and texture arrays
 * @param mip_level mip level
 * @return true if success, false otherwise
 */
bool clear_rwtexf(BaseTexture *tex, const float val[4], uint32_t face, uint32_t mip_level);

/**
 * @brief Zero out UAV buffer as uint. Distinction between types is needed for DX.
 *
 * @param buf buffer to clear
 * @return true if success, false otherwise
 */
bool zero_rwbufi(Sbuffer *buf);

} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline bool set_rwtex(uint32_t shader_stage, uint32_t slot, BaseTexture *tex, uint32_t face, uint32_t mip, bool as_uint)
{
  return d3di.set_rwtex(shader_stage, slot, tex, face, mip, as_uint);
}
inline bool clear_rwtexi(BaseTexture *tex, const uint32_t val[4], uint32_t face, uint32_t mip)
{
  return d3di.clear_rwtexi(tex, val, face, mip);
}
inline bool clear_rwtexf(BaseTexture *tex, const float val[4], uint32_t face, uint32_t mip)
{
  return d3di.clear_rwtexf(tex, val, face, mip);
}

inline bool zero_rwbufi(Sbuffer *buffer) { return d3di.zero_rwbufi(buffer); }
} // namespace d3d
#endif
