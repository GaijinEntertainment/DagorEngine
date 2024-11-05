//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_inttypes.h>
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_resource.h>

class BaseTexture;

/** \defgroup RenderPassStructs
 * @{
 */

/// \brief Description of render target bind inside render pass
/// \details Fully defines operation that will happen with target at defined subpass slot
struct RenderPassBind
{
  /// \brief Index of render target in render targets array that will be used for this bind
  int32_t target;
  /// \brief Index of subpass where bind happens
  int32_t subpass;
  /// \brief Index of slot where target will be used
  int32_t slot;

  /// \brief defines what will happen with target used in binding
  RenderPassTargetAction action;
  /// \brief optional user barrier for generic(emulated) implementation
  ResourceBarrier dependencyBarrier;
};

/// \brief Early description of render target
/// \details Gives necessary info at render pass creation so render pass is compatible with targets of same type later on
struct RenderPassTargetDesc
{
  /// \brief Resource from which information is extracted, can be nullptr
  BaseTexture *templateResource;
  /// \brief Raw texture create flags, used if template resource is not provided
  unsigned texcf;
  /// \brief Must be set if template resource is empty and target will use memory aliased inside render pass
  bool aliased;
};

/// \brief Description of target that is used inside render pass
struct RenderPassTarget
{
  /// \brief Actual render target subresource reference
  RenderTarget resource;
  /// \brief Clear value that is used on clear action
  ResourceClearValue clearValue;
};

/// \brief Render pass resource description, used to create RenderPass object
struct RenderPassDesc
{
  /// \brief Name that is visible only in developer builds and inside graphics tools, if possible
  const char *debugName;
  /// \brief Total amount of targets inside render pass
  uint32_t targetCount;
  /// \brief Total amount of bindings for render pass
  uint32_t bindCount;

  /// \brief Array of targetCount elements, supplying early description of render target
  const RenderPassTargetDesc *targetsDesc;
  /// \brief Array of bindCount elements, describing all subpasses
  const RenderPassBind *binds;

  /// \brief Texture binding offset for shader subpass reads used on APIs without native render passes
  /// \details Generic(emulated) implementation will use registers starting from this offset, to bind input attachments.
  /// This must be properly handled inside shader code for generic implementation to work properly!
  uint32_t subpassBindingOffset;
};

/// \brief Area of render target where rendering will happen inside render pass
struct RenderPassArea
{
  uint32_t left;
  uint32_t top;
  uint32_t width;
  uint32_t height;
  float minZ;
  float maxZ;
};

/**@}*/

namespace d3d
{
//! opaque class that represents render pass
struct RenderPass;
/** \defgroup RenderPassD3D
 * @{
 */

/// \brief Creates render pass object
/// \details Render pass objects are intended to be created once (and ahead of time), used many times
/// \note No external sync required
/// \warning Do not run per frame/realtime!
/// \warning Avoid using at time sensitive places!
/// \warning Will assert-fail if rp_desc.bindCount is 0
/// \param rp_desc Description of render pass to be created
/// \result Pointer to opaque RenderPass object, may be nullptr if description is invalid
RenderPass *create_render_pass(const RenderPassDesc &rp_desc);
/// \brief Deletes render pass object
/// \note Sync with usage is required (must not delete object that is in use in current CPU frame)
/// \warning All usage to object becomes invalid right after method call
/// \param rp Object to be deleted
void delete_render_pass(RenderPass *rp);

/// \brief Begins render pass rendering
/// \details After this command, viewport is reset to area supplied
/// and subpass 0, described in render pass object, is started
/// \note Must be external synced (GPU lock required)
/// \warning When inside pass, all other GPU execution methods aside of Draw* are prohibited!
/// \warning Avoid writes/reads outside area, it is UB in general
/// \warning Will assert-fail if other render pass is already in process
/// \warning Backbuffer can't be used as target
/// \param rp Render pass resource to begin with
/// \param area Rendering area restriction
/// \param targets Array of targets that will be used in rendering
void begin_render_pass(RenderPass *rp, const RenderPassArea area, const RenderPassTarget *targets);
/// \brief Advances to next subpass
/// \details Increases subpass number and executes necessary synchronization as well as binding,
/// described for this subpass
/// \details Viewport is reset to render area on every call
/// \note Must be external synced (GPU lock required)
/// \warning Will assert-fail if there is no subpass to advance to
/// \warning Will assert-fail if called outside of render pass
void next_subpass();
/// \brief Ends render pass
/// \details Processes store&sync operations described in render pass
/// \details After this call, any non Draw operations are allowed and render targets are reset to backbuffer
/// \note Must be external synced (GPU lock required)
/// \warning Will assert-fail if subpass is not final
/// \warning Will assert-fail if called  outside of render pass
void end_render_pass();

/// When renderpass splits validation is enabled in Vulkan this
/// command tells that we actually want to load previous contents of attached color targets or depth
/// to render on top of it. Otherwise loading previous contents treated as renderpass split
/// and the validation fails (we want to avoid RP splits cause of performance impact on TBDR).
/// If it's known that the render target will be just fully redrawn (like in most postfx),
/// it's better to use d3d::clearview(CLEAR_DISCARD, ...) instead of this command.
#if DAGOR_DBGLEVEL > 0
void allow_render_pass_target_load();
#else
inline void allow_render_pass_target_load() {}
#endif

/** @}*/
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline RenderPass *create_render_pass(const RenderPassDesc &rp_desc) { return d3di.create_render_pass(rp_desc); }
inline void delete_render_pass(RenderPass *rp) { d3di.delete_render_pass(rp); }
inline void begin_render_pass(RenderPass *rp, const RenderPassArea area, const RenderPassTarget *targets)
{
  d3di.begin_render_pass(rp, area, targets);
}
inline void next_subpass() { d3di.next_subpass(); }
inline void end_render_pass() { d3di.end_render_pass(); }

#if DAGOR_DBGLEVEL > 0
inline void allow_render_pass_target_load()
{
  if (d3di.driverCode.is(d3d::vulkan))
    d3di.allow_render_pass_target_load();
}
#endif
} // namespace d3d
#endif
