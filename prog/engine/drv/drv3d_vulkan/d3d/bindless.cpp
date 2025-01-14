// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_driver.h>

#include "bindless.h"
#include "texture.h"
#include "device_context.h"
#include "globals.h"
#include "physical_device_set.h"

using namespace drv3d_vulkan;

uint32_t d3d::register_bindless_sampler(BaseTexture *tex)
{
  G_ASSERTF_RETURN(Globals::VK::phy.hasBindless, 0, "Bindless resources are not supported on this hardware");
  BaseTex *texture = cast_to_texture_base(tex);
  return Globals::bindless.registerBindlessSampler(texture);
}

uint32_t d3d::register_bindless_sampler(d3d::SamplerHandle sampler)
{
  G_ASSERTF_RETURN(Globals::VK::phy.hasBindless, 0, "Bindless resources are not supported on this hardware");
  return Globals::bindless.registerBindlessSampler(reinterpret_cast<SamplerResource *>(sampler));
}

uint32_t d3d::allocate_bindless_resource_range(uint32_t resource_type, uint32_t count)
{
  G_ASSERTF_RETURN(Globals::VK::phy.hasBindless, 0, "Bindless resources are not supported on this hardware");
  return Globals::bindless.allocateBindlessResourceRange(resource_type, count);
}

uint32_t d3d::resize_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t current_count, uint32_t new_count)
{
  G_ASSERTF_RETURN(Globals::VK::phy.hasBindless, 0, "Bindless resources are not supported on this hardware");
  return Globals::bindless.resizeBindlessResourceRange(resource_type, index, current_count, new_count);
}

void d3d::free_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t count)
{
  G_ASSERTF_RETURN(Globals::VK::phy.hasBindless, , "Bindless resources are not supported on this hardware");
  return Globals::bindless.freeBindlessResourceRange(resource_type, index, count);
}

void d3d::update_bindless_resource(uint32_t index, D3dResource *res)
{
  G_ASSERTF_RETURN(Globals::VK::phy.hasBindless, , "Bindless resources are not supported on this hardware");
  drv3d_vulkan::Globals::ctx.updateBindlessResource(index, res);
}

void d3d::update_bindless_resources_to_null(uint32_t resource_type, uint32_t index, uint32_t count)
{
  G_ASSERTF_RETURN(Globals::VK::phy.hasBindless, , "Bindless resources are not supported on this hardware");
  drv3d_vulkan::Globals::ctx.updateBindlessResourcesToNull(resource_type, index, count);
}
