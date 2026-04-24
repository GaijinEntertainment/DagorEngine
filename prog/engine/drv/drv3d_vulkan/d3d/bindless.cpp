// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_driver.h>

#include "bindless.h"
#include "texture.h"
#include "device_context.h"
#include "globals.h"
#include "physical_device_set.h"
#include "validation.h"

using namespace drv3d_vulkan;

NO_UBSAN uint32_t d3d::register_bindless_sampler(d3d::SamplerHandle sampler)
{
  D3D_CONTRACT_ASSERTF_RETURN(Globals::VK::phy.hasBindless, 0, "Bindless resources are not supported on this hardware");
  return Globals::bindless.registerBindlessSampler(reinterpret_cast<SamplerResource *>(sampler));
}

uint32_t d3d::allocate_bindless_resource_range(D3DResourceType type, uint32_t count)
{
  D3D_CONTRACT_ASSERTF_RETURN(Globals::VK::phy.hasBindless, 0, "Bindless resources are not supported on this hardware");
  return Globals::bindless.allocateBindlessResourceRange(type, count);
}

uint32_t d3d::resize_bindless_resource_range(D3DResourceType type, uint32_t index, uint32_t current_count, uint32_t new_count)
{
  D3D_CONTRACT_ASSERTF_RETURN(Globals::VK::phy.hasBindless, 0, "Bindless resources are not supported on this hardware");
  return Globals::bindless.resizeBindlessResourceRange(type, index, current_count, new_count);
}

void d3d::free_bindless_resource_range(D3DResourceType type, uint32_t index, uint32_t count)
{
  D3D_CONTRACT_ASSERTF_RETURN(Globals::VK::phy.hasBindless, , "Bindless resources are not supported on this hardware");
  return Globals::bindless.queueFreeBindlessResourceRange(type, index, count);
}

void d3d::update_bindless_resource_range(D3DResourceType type, uint32_t index, const dag::ConstSpan<D3dResource *> &resources)
{
  D3D_CONTRACT_ASSERTF_RETURN(Globals::VK::phy.hasBindless, , "Bindless resources are not supported on this hardware");

  for (uint32_t i = 0; i < resources.size(); ++i)
  {
    if (resources[i])
    {
      D3D_CONTRACT_ASSERTF(resources[i]->getType() == type, "Bindless resource '%s' has wrong type for batch update",
        resources[i]->getName());
    }
  }

  drv3d_vulkan::Globals::ctx.updateBindlessResourceRange(type, index, resources);
}

bool d3d::update_bindless_resource(D3DResourceType, uint32_t index, D3dResource *res)
{
  D3D_CONTRACT_ASSERTF_RETURN(Globals::VK::phy.hasBindless, false, "Bindless resources are not supported on this hardware");
  return drv3d_vulkan::Globals::ctx.updateBindlessResource(index, res);
}

void d3d::update_bindless_resources_to_null(D3DResourceType type, uint32_t index, uint32_t count)
{
  D3D_CONTRACT_ASSERTF_RETURN(Globals::VK::phy.hasBindless, , "Bindless resources are not supported on this hardware");
  drv3d_vulkan::Globals::ctx.updateBindlessResourcesToNull(type, index, count);
}

uint32_t d3d::add_bindless_resource(D3DResourceType type, D3dResource *res)
{
  uint32_t index = d3d::allocate_bindless_resource_range(type, 1);
  d3d::update_bindless_resource(type, index, res);
  return index;
}

void d3d::add_bindless_resources(dag::StridedConstSpan<D3DResourceType> types, dag::StridedConstSpan<D3dResource *> resources,
  dag::StridedSpan<uint32_t> ids)
{
  D3D_CONTRACT_ASSERT(ids.size() != 0);
  D3D_CONTRACT_ASSERT(ids.stride_bytes() != 0);
  D3D_CONTRACT_ASSERT(types.size() == ids.size());
  D3D_CONTRACT_ASSERT(resources.size() == ids.size());
  // NOTE:
  // - resources.stride_bytes() may be 0, which means all are the same
  // - types.stride_bytes() may be 0, which means all are the same
  for (size_t i = 0; i < types.size(); ++i)
  {
    auto type = types[i];
    auto id = ids[i] = d3d::allocate_bindless_resource_range(type, 1);
    auto res = resources[i];
    if (res)
    {
      d3d::update_bindless_resource(type, id, res);
    }
    else
    {
      d3d::update_bindless_resources_to_null(type, id, 1);
    }
  }
}
