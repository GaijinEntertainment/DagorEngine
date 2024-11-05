//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_inttypes.h>
#include <generic/dag_span.h>
#include <drv/3d/dag_resource.h>
#include <drv/3d/dag_decl.h>
#include <math/dag_e3dColor.h>

class BaseTexture;
typedef BaseTexture Texture;

/**
 * @enum DepthAccess
 * @brief Specifies the access mode for a depth attachment.
 */
enum class DepthAccess
{
  /// For regular depth attachment.
  RW,

  /**
   * For read-only depth attachment which can also be sampled as a texture in the same shader.
   * IF YOU DON'T NEED TO SAMPLE THE DEPTH, USE z_write=false WITH DepthAccess::RW INSTEAD.
   * Using this state will cause HiZ decompression on some hardware and
   * split of renderpass with flush of tile data into memory in a TBR.
   */
  SampledRO
};

/**
 * @struct RenderTarget
 * @brief Represents a render target to set it into slot.
 */
struct RenderTarget
{
  BaseTexture *tex;   ///< Texture to set as render target.
  uint32_t mip_level; ///< Mip level to set as render target.
  uint32_t layer;     ///< Layer to set as render target.
};

namespace d3d
{
/**
 * @brief Copy the current render target to a texture. It is useful to get a backbuffer content on such drivers as Metal.
 *
 * @param to_tex Texture to copy the current render target to.
 * @return true if the operation was successful, false otherwise.
 */
bool copy_from_current_render_target(BaseTexture *to_tex);

/**
 * @brief Clear the render target. Perform full clear on the RT according to its' creation flag
 *
 * @param rt Render target to clear.
 * @param clear_val Clear value.
 * @return true if the operation was successful, false otherwise.
 */
bool clear_rt(const RenderTarget &rt, const ResourceClearValue &clear_val = {});

/**
 * @brief Set the default render target (backbuffer) as a single current render target.
 * @return true if the operation was successful, false otherwise.
 */
bool set_render_target();

/**
 * @brief Set the depth texture target.
 *
 * @param tex Texture to set as depth target. NULL means NO depth.
 * @param access Access mode for the depth attachment.
 * @return true if the operation was successful, false otherwise.
 */
bool set_depth(BaseTexture *tex, DepthAccess access);

/**
 * @brief Set the depth texture target.
 *
 * @param tex Texture to set as depth target. NULL means NO depth.
 * @param layer Layer of the tex to set as depth target.
 * @param access Access mode for the depth attachment.
 * @return true if the operation was successful, false otherwise.
 */
bool set_depth(BaseTexture *tex, int layer, DepthAccess access);

/**
 * @brief Sets the render target for rendering.
 *
 * @warning if texture is depth texture format, it is the same as call set_depth() DON'T USE THIS BEHAVIOR!!!
 * @deprecated Use set_render_target(RenderTarget depth, DepthAccess depth_access, dag::ConstSpan<RenderTarget> colors) instead.
 *
 * @param rt_index The index of the render target. The maximum number of render targets is defined by Driver3dRenderTarget::MAX_SIMRT.
 * @param texture A pointer to the BaseTexture object representing the render target.
 * @param fc The face of the texture (for cube textures and texture arrays). If face is RENDER_TO_WHOLE_ARRAY, then the whole Texture
 * Array/Volume Tex will be set as render target. This is to be used with geom shader (and Metal allows with vertex shader).
 * @param level The level of the render target.
 * @return True if the render target was set successfully, false otherwise.
 */
bool set_render_target(int rt_index, BaseTexture *, int fc, int level);

/**
 * @brief Sets the render target for rendering.
 *
 * @warning if texture is depth texture format, it is the same as call set_depth() DON'T USE THIS BEHAVIOR!!!
 * @deprecated Use set_render_target(RenderTarget depth, DepthAccess depth_access, dag::ConstSpan<RenderTarget> colors) instead.
 *
 * @param rt_index The index of the render target. The maximum number of render targets is defined by Driver3dRenderTarget::MAX_SIMRT.
 * @param texture A pointer to the BaseTexture object representing the render target.
 * @param level The level of the render target.
 * @return True if the render target was set successfully, false otherwise.
 */
bool set_render_target(int rt_index, BaseTexture *, int level);

/**
 * @brief Sets the render target for rendering. All other render targets will be set to nullptr.
 *
 * @warning if texture is depth texture format, it is the same as call set_depth() DON'T USE THIS BEHAVIOR!!!
 * @deprecated Use set_render_target(RenderTarget depth, DepthAccess depth_access, dag::ConstSpan<RenderTarget> colors) instead.
 *
 * @param texture A pointer to the BaseTexture object representing the render target.
 * @param level The level of the render target.
 * @return True if the render target was set successfully, false otherwise.
 */
inline bool set_render_target(BaseTexture *t, int level) { return set_render_target() && set_render_target(0, t, level); }

/**
 * @brief Sets the render target for rendering. All other render targets will be set to nullptr.
 *
 * @warning if texture is depth texture format, it is the same as call set_depth() DON'T USE THIS BEHAVIOR!!!
 * @deprecated Use set_render_target(RenderTarget depth, DepthAccess depth_access, dag::ConstSpan<RenderTarget> colors) instead.
 *
 * @param t A pointer to the BaseTexture object representing the render target.
 * @param fc The face of the texture (for cube textures and texture arrays). If face is RENDER_TO_WHOLE_ARRAY, then the whole Texture
 * Array/Volume Tex will be set as render target. This is to be used with geom shader (and Metal allows with vertex shader).
 * @param level The level of the render target.
 * @return True if the render target was set successfully, false otherwise.
 */
inline bool set_render_target(BaseTexture *t, int fc, int level) { return set_render_target() && set_render_target(0, t, fc, level); }

/**
 * @brief Sets the render target for rendering. All other render targets will be set to nullptr.
 *
 * @param depth The depth render target.
 * @param depth_access The access mode for the depth attachment.
 * @param colors The color render targets.
 * @return True if the render target was set successfully, false otherwise.
 */
inline void set_render_target(RenderTarget depth, DepthAccess depth_access, dag::ConstSpan<RenderTarget> colors)
{
  int i = 0;
  for (; i < colors.size() && i < Driver3dRenderTarget::MAX_SIMRT; ++i)
    set_render_target(i, colors[i].tex, colors[i].mip_level);
  for (; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
    set_render_target(i, nullptr, 0);

  set_depth(depth.tex, depth_access);
}

/**
 * @brief Sets the render target for rendering. All other render targets will be set to nullptr.
 *
 * @param depth The depth render target.
 * @param depth_access The access mode for the depth attachment.
 * @param colors The color render targets.
 * @return True if the render target was set successfully, false otherwise.
 */
inline void set_render_target(RenderTarget depth, DepthAccess depth_access, const std::initializer_list<RenderTarget> colors)
{
  set_render_target(depth, depth_access, dag::ConstSpan<RenderTarget>(colors.begin(), colors.end() - colors.begin()));
}

/**
 * @brief Get the render target object
 *
 * @deprecated Don't use it since the method relies on a global state.
 *
 * @param out_rt The render target object to fill.
 */
void get_render_target(Driver3dRenderTarget &out_rt);

/**
 * @brief Set the render target object
 *
 * @param rt The render target object to set. Usually, it is obtained by get_render_target(Driver3dRenderTarget &out_rt).
 * @return true if the operation was successful, false otherwise.
 */
bool set_render_target(const Driver3dRenderTarget &rt);

/**
 * @brief Get the size of the render target.
 *
 * @deprecated If you need to use the method, it seems that you are doing something wrong.
 *
 * @param w The width of the render target.
 * @param h The height of the render target.
 * @return true if the operation was successful, false otherwise.
 */
bool get_target_size(int &w, int &h);

/**
 * @brief Get the size of the render target texture.
 *
 * @deprecated If you need to use the method, it seems that you are doing something wrong.
 *
 * @param w The width of the render target texture.
 * @param h The height of the render target texture.
 * @param rt_tex The render target texture. If nullptr, the size of the backbuffer will be written to w and h.
 * @param lev The level of the render target texture.
 * @return true if the operation was successful, false otherwise.
 */
bool get_render_target_size(int &w, int &h, BaseTexture *rt_tex, int lev = 0);

/// clear all view
/**
 * @brief Clear the view. What is view will be explained in the params.
 *
 * @param what The view to clear. It can be one of the CLEAR_*** enum. It will clears either color, depth, or stencil buffer.
 * @param c The color to clear the view with.
 * @param z The depth to clear the view with.
 * @param stencil The stencil to clear the view with.
 * @return true if the operation was successful, false otherwise.
 */
bool clearview(int what, E3DCOLOR c, float z, uint32_t stencil);

/**
 * @brief Get the size of the screen (backbuffer).
 *
 * @warning The size of the screen can be different from the size of the framebuffer.
 *
 * @param w The width of the screen.
 * @param h The height of the screen.
 */
void get_screen_size(int &w, int &h);

/**
 * @brief Get the backbuffer texture.
 *
 * Backbuffer is only valid while the GPU is acquired, and can be recreated in between.
 *
 * @return The backbuffer texture.
 */
Texture *get_backbuffer_tex();

/**
 * @brief Get the secondary backbuffer texture.
 *
 * Secondary backbuffer is only valid while the GPU is acquired, and can be recreated in between.
 * It exists only for Xbox. Mostly used as a backbuffer with a higher resolution to draw UI.
 *
 * @return The secondary backbuffer texture.
 */
Texture *get_secondary_backbuffer_tex();

} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline bool copy_from_current_render_target(BaseTexture *to_tex) { return d3di.copy_from_current_render_target(to_tex); }
inline bool clear_rt(const RenderTarget &rt, const ResourceClearValue &clear_val) { return d3di.clear_rt(rt, clear_val); }
inline bool set_render_target() { return d3di.set_render_target(); }
inline bool set_depth(BaseTexture *tex, DepthAccess access) { return d3di.set_depth(tex, access); }
inline bool set_depth(BaseTexture *tex, int layer, DepthAccess access) { return d3di.set_depth(tex, layer, access); }
inline bool set_render_target(int rt_index, BaseTexture *t, int fc, int level)
{
  return d3di.set_render_target(rt_index, t, fc, level);
}
inline bool set_render_target(int rt_index, BaseTexture *t, int level) { return d3di.set_render_target(rt_index, t, level); }
inline void get_render_target(Driver3dRenderTarget &out_rt) { return d3di.get_render_target(out_rt); }
inline bool set_render_target(const Driver3dRenderTarget &rt) { return d3di.set_render_target(rt); }
inline bool get_target_size(int &w, int &h) { return d3di.get_target_size(w, h); }
inline bool get_render_target_size(int &w, int &h, BaseTexture *rt_tex, int lev)
{
  return d3di.get_render_target_size(w, h, rt_tex, lev);
}
inline bool clearview(int what, E3DCOLOR c, float z, uint32_t stencil) { return d3di.clearview(what, c, z, stencil); }
inline void get_screen_size(int &w, int &h) { d3di.get_screen_size(w, h); }
inline Texture *get_backbuffer_tex() { return d3di.get_backbuffer_tex(); }
inline Texture *get_secondary_backbuffer_tex() { return d3di.get_secondary_backbuffer_tex(); }
} // namespace d3d
#endif
