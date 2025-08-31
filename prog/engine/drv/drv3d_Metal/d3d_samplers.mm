// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_driver.h>

#include "texture.h"
#include "render.h"

using namespace drv3d_metal;

drv3d_metal::SamplerState samplerInfoToSamplerState(const d3d::SamplerInfo &sampler_info)
{
  drv3d_metal::SamplerState state;

  state.border_color = sampler_info.border_color;
  state.lod = sampler_info.mip_map_bias;

  state.addrU = uint32_t(sampler_info.address_mode_u);
  state.addrV = uint32_t(sampler_info.address_mode_v);
  state.addrW = uint32_t(sampler_info.address_mode_w);

  state.anisotropy = clamp(uint32_t(sampler_info.anisotropic_max), 1u, 16u);
  state.texFilter = sampler_info.filter_mode;
  state.mipmap = sampler_info.mip_map_mode;

  return state;
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

d3d::SamplerHandle d3d::request_sampler(const d3d::SamplerInfo &sampler_info)
{
  SamplerConverter converter;
  converter.index = render.getSampler(samplerInfoToSamplerState(sampler_info)) + 1;
  converter.bias = sampler_info.mip_map_bias;
  return converter.handle;
}

void d3d::set_sampler(unsigned shader_stage, unsigned slot, d3d::SamplerHandle sampler)
{
  SamplerConverter converter { .handle = sampler };
  render.setSampler(shader_stage, slot, converter.index - 1, converter.bias);
}
