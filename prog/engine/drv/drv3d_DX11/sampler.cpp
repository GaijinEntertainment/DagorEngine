#include "sampler.h"

using namespace drv3d_dx11;

SamplerState::Key SamplerStateObject::makeKey(const d3d::SamplerInfo &sampler_info)
{
  SamplerState::Key key;
  memset(&key, 0, sizeof(key));

  key.borderColor = sampler_info.border_color;
  key.lodBias = sampler_info.mip_map_bias;

  key.addrU = uint32_t(sampler_info.address_mode_u);
  key.addrV = uint32_t(sampler_info.address_mode_v);
  key.addrW = uint32_t(sampler_info.address_mode_w);

  key.anisotropyLevel = uint32_t(sampler_info.anisotropic_max);
  key.texFilter = uint32_t(sampler_info.filter_mode);
  key.mipFilter = uint32_t(sampler_info.mip_map_mode);

  return key;
}


void remove_sampler_from_states(SamplerStateObject *smp)
{
  ResAutoLock resLock; // Thredsafe state.resources access

  TextureFetchState &state = g_render_state.texFetchState;

  // Remove references to deleted sampler from all states to avoid confusion with invalid pointers
  for (auto &stage : state.resources)
  {
    for (auto &resource : stage.resources)
    {
      if (resource.sampler == smp)
      {
        resource.setSampler(nullptr);
      }
    }
  }
}

d3d::SamplerHandle d3d::create_sampler(const d3d::SamplerInfo &sampler_info)
{
  auto *samplerObject = new SamplerStateObject(sampler_info);
  return reinterpret_cast<SamplerHandle>(samplerObject);
}

void d3d::destroy_sampler(d3d::SamplerHandle sampler)
{
  auto *samplerObject = reinterpret_cast<SamplerStateObject *>(sampler);

  remove_sampler_from_states(samplerObject);

  delete samplerObject;
}
