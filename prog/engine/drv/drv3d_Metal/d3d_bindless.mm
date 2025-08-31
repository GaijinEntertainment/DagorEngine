// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_bindless.h>

#include "bindless.h"
#include "texture.h"
#include "render.h"
#include "drv_assert_defs.h"

using namespace drv3d_metal;

uint32_t d3d::allocate_bindless_resource_range(D3DResourceType type, uint32_t count)
{
  D3D_CONTRACT_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, 0, "Bindless resources are not supported on this hardware");
  return render.bindlessManager.allocateBindlessResourceRange(type, count);
}

uint32_t d3d::resize_bindless_resource_range(D3DResourceType type, uint32_t index, uint32_t current_count, uint32_t new_count)
{
  D3D_CONTRACT_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, 0, "Bindless resources are not supported on this hardware");
  return render.bindlessManager.resizeBindlessResourceRange(type, index, current_count, new_count);
}

void d3d::free_bindless_resource_range(D3DResourceType type, uint32_t index, uint32_t count)
{
  D3D_CONTRACT_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, , "Bindless resources are not supported on this hardware");
  render.bindlessManager.freeBindlessResourceRange(type, index, count);
}

bool d3d::update_bindless_resource(D3DResourceType range_type, uint32_t index, D3dResource *res)
{
  D3D_CONTRACT_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, false, "Bindless resources are not supported on this hardware");
  return render.updateBindlessResource(range_type, index, res);
}

void d3d::update_bindless_resources_to_null(D3DResourceType type, uint32_t index, uint32_t count)
{
  D3D_CONTRACT_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, , "Bindless resources are not supported on this hardware");
  render.updateBindlessResourcesToNull(type, index, count);
}

union SamplerConverter
{
  struct
  {
    int index;
    float bias;
  };
  d3d::SamplerHandle handle = d3d::SamplerHandle::Invalid;
};

uint32_t d3d::register_bindless_sampler(d3d::SamplerHandle sampler)
{
  D3D_CONTRACT_ASSERTF_RETURN(d3d::get_driver_desc().caps.hasBindless, 0, "Bindless resources are not supported on this hardware");

  SamplerConverter converter { .handle = sampler };
  return render.bindlessManager.registerBindlessSampler(converter.index - 1, converter.bias);
}
