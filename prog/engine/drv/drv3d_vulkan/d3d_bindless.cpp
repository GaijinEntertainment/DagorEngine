#include <3d/dag_drv3d.h>

#include "device.h"
#include "bindless.h"
#include "texture.h"

using namespace drv3d_vulkan;

uint32_t d3d::register_bindless_sampler(BaseTexture *tex)
{
  G_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, 0, "Bindless resources are not supported on this hardware");
  BaseTex *texture = cast_to_texture_base(tex);
  return drv3d_vulkan::get_device().bindlessManager.registerBindlessSampler(texture);
}

uint32_t d3d::allocate_bindless_resource_range(uint32_t resource_type, uint32_t count)
{
  G_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, 0, "Bindless resources are not supported on this hardware");
  return drv3d_vulkan::get_device().bindlessManager.allocateBindlessResourceRange(resource_type, count);
}

uint32_t d3d::resize_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t current_count, uint32_t new_count)
{
  G_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, 0, "Bindless resources are not supported on this hardware");
  return drv3d_vulkan::get_device().bindlessManager.resizeBindlessResourceRange(resource_type, index, current_count, new_count);
}

void d3d::free_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t count)
{
  G_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, , "Bindless resources are not supported on this hardware");
  return drv3d_vulkan::get_device().bindlessManager.freeBindlessResourceRange(resource_type, index, count);
}

void d3d::update_bindless_resource(uint32_t index, D3dResource *res)
{
  G_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, , "Bindless resources are not supported on this hardware");
  drv3d_vulkan::get_device().getContext().updateBindlessResource(index, res);
}

void d3d::update_bindless_resources_to_null(uint32_t resource_type, uint32_t index, uint32_t count)
{
  G_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, , "Bindless resources are not supported on this hardware");
  drv3d_vulkan::get_device().getContext().updateBindlessResourcesToNull(resource_type, index, count);
}
