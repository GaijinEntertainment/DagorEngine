// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_bindless.h>

#include "bindless.h"
#include "texture.h"
#include "render.h"

using namespace drv3d_metal;

uint32_t d3d::allocate_bindless_resource_range(uint32_t resource_type, uint32_t count)
{
  G_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, 0, "Bindless resources are not supported on this hardware");
  return render.bindlessManager.allocateBindlessResourceRange(resource_type, count);
}

uint32_t d3d::resize_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t current_count, uint32_t new_count)
{
  G_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, 0, "Bindless resources are not supported on this hardware");
  return render.bindlessManager.resizeBindlessResourceRange(resource_type, index, current_count, new_count);
}

void d3d::free_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t count)
{
  G_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, , "Bindless resources are not supported on this hardware");
  render.bindlessManager.freeBindlessResourceRange(resource_type, index, count);
}

void d3d::update_bindless_resource(uint32_t index, D3dResource *res)
{
  G_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, , "Bindless resources are not supported on this hardware");
  render.updateBindlessResource(index, res);
}

void d3d::update_bindless_resources_to_null(uint32_t resource_type, uint32_t index, uint32_t count)
{
  G_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, , "Bindless resources are not supported on this hardware");
  render.updateBindlessResourcesToNull(resource_type, index, count);
}


uint32_t d3d::register_bindless_sampler(BaseTexture *base_tex)
{
  G_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, 0, "Bindless resources are not supported on this hardware");

  auto tex = static_cast<drv3d_metal::Texture*>(base_tex);
  G_ASSERT(tex);
  drv3d_metal::Texture::getSampler(tex->sampler_state);
  G_ASSERT(tex->sampler_state.sampler);
  return render.bindlessManager.registerBindlessSampler(tex->sampler_state.sampler);
}

uint32_t d3d::register_bindless_sampler(d3d::SamplerHandle sampler)
{
  G_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, 0, "Bindless resources are not supported on this hardware");

  auto mtl_sampler = reinterpret_cast<id<MTLSamplerState>>(sampler);
  return render.bindlessManager.registerBindlessSampler(mtl_sampler);
}
