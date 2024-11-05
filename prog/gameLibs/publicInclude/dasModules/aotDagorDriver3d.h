//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_rwResource.h>
#include <3d/dag_render.h>
#include <ecs/render/shaders.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_overrideStates.h>
#include <ecs/render/postfx_renderer.h>
#include <ecs/render/compute_shader.h>

extern bool grs_draw_wire;

MAKE_TYPE_FACTORY(Driver3dPerspective, Driver3dPerspective);
MAKE_TYPE_FACTORY(ShadersECS, ShadersECS);
MAKE_TYPE_FACTORY(OverrideState, shaders::OverrideState);
MAKE_TYPE_FACTORY(PostFxRenderer, PostFxRenderer);
MAKE_TYPE_FACTORY(ComputeShader, ComputeShader);

DAS_BIND_ENUM_CAST(d3d::SamplerHandle);
DAS_BASE_BIND_ENUM_FACTORY(d3d::SamplerHandle, "SamplerHandle");

namespace bind_dascript
{
inline void get_Driver3dPerspective(const das::TBlock<void, const das::TTemporary<const Driver3dPerspective>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  Driver3dPerspective persp;
  d3d::getpersp(persp);
  vec4f arg = das::cast<Driver3dPerspective *>::from(&persp);
  context->invoke(block, &arg, nullptr, at);
}

inline bool get_grs_draw_wire() { return grs_draw_wire; }

inline void shader_setStates(const ShadersECS &shader_ecs)
{
  if (!shader_ecs.shElem)
    return;
  shader_ecs.shElem->setStates();
}

inline void postfx_setStates(const PostFxRenderer &shader)
{
  if (auto elem = shader.getElem())
    elem->setStates();
}


inline void d3d_resource_barrier_tex(BaseTexture *tex, uint32_t flags, uint32_t sub_res_index, uint32_t sub_res_range)
{
  d3d::resource_barrier({tex, static_cast<ResourceBarrier>(flags), sub_res_index, sub_res_range});
}

inline void d3d_resource_barrier_buf(Sbuffer *buf, uint32_t flags)
{
  d3d::resource_barrier({buf, static_cast<ResourceBarrier>(flags)});
}

inline float d3d_get_vsync_refresh_rate()
{
  double refreshRate = 0;
  d3d::driver_command(Drv3dCommand::GET_VSYNC_REFRESH_RATE, &refreshRate);
  return refreshRate;
}

inline void d3d_stretch_rect(BaseTexture *src, BaseTexture *dst) { d3d::stretch_rect(src, dst); }

} // namespace bind_dascript
