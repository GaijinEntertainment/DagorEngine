// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_driver.h>

#include "texture.h"
#include "render.h"

using namespace drv3d_metal;


drv3d_metal::Texture::SamplerState samplerInfoToSamplerState(const d3d::SamplerInfo &sampler_info)
{
  drv3d_metal::Texture::SamplerState state;

  state.border_color = sampler_info.border_color;
  state.lod = sampler_info.mip_map_bias;

  state.addrU = uint32_t(sampler_info.address_mode_u);
  state.addrV = uint32_t(sampler_info.address_mode_v);
  state.addrW = uint32_t(sampler_info.address_mode_w);

  state.anisotropy = clamp(uint32_t(sampler_info.anisotropic_max), 1u, 16u);
  state.texFilter = uint32_t(sampler_info.filter_mode);
  state.mipmap = uint32_t(sampler_info.mip_map_mode);

  return state;
}


d3d::SamplerHandle d3d::request_sampler(const d3d::SamplerInfo &sampler_info)
{
  auto samplerState = samplerInfoToSamplerState(sampler_info);
  drv3d_metal::Texture::getSampler(samplerState);

  return SamplerHandle(reinterpret_cast<uint64_t>(samplerState.sampler));
}

void d3d::set_sampler(unsigned shader_stage, unsigned slot, d3d::SamplerHandle sampler)
{
  auto mtl_sampler = reinterpret_cast<id<MTLSamplerState>>(sampler);
  render.setSampler(shader_stage, slot, mtl_sampler);
}
