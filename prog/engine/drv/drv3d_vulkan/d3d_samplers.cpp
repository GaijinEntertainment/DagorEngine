#include <3d/dag_drv3d.h>

#include "device.h"
#include "texture.h"

using namespace drv3d_vulkan;

// global fields accessor to reduce code footprint
namespace
{
struct LocalAccessor
{
  PipelineState &pipeState;
  Device &drvDev;
  VulkanDevice &dev;
  DeviceContext &ctx;
  ResourceManager &resources;

  LocalAccessor() :
    pipeState(get_device().getContext().getFrontend().pipelineState),
    drvDev(get_device()),
    dev(get_device().getVkDevice()),
    ctx(get_device().getContext()),
    resources(get_device().resources)
  {}
};
} // namespace

inline SamplerState translate_d3d_samplerinfo_to_vulkan_samplerstate(const d3d::SamplerInfo &samplerInfo)
{
  SamplerState result;
  result.setMip(translate_mip_filter_type_to_vulkan(int(samplerInfo.mip_map_mode)));
  result.setFilter(translate_filter_type_to_vulkan(int(samplerInfo.filter_mode)));
  result.setIsCompare(int(samplerInfo.filter_mode) == TEXFILTER_COMPARE);
  result.setW(translate_texture_address_mode_to_vulkan(int(samplerInfo.address_mode_w)));
  result.setV(translate_texture_address_mode_to_vulkan(int(samplerInfo.address_mode_v)));
  result.setU(translate_texture_address_mode_to_vulkan(int(samplerInfo.address_mode_u)));

  // TODO: no info about texture format (isSampledAsFloat)
  result.setBorder(BaseTex::remap_border_color(samplerInfo.border_color, false));

  result.setBias(samplerInfo.mip_map_bias);
  result.setAniso(clamp<float>(samplerInfo.anisotropic_max, 1, 16));

  return result;
}

d3d::SamplerHandle d3d::create_sampler(const d3d::SamplerInfo &sampler_info)
{
  LocalAccessor la;
  SharedGuardedObjectAutoLock lock(la.resources);

  SamplerState state = translate_d3d_samplerinfo_to_vulkan_samplerstate(sampler_info);
  SamplerResource *ret = la.resources.alloc<SamplerResource>({SamplerDescription(state)});
  return reinterpret_cast<d3d::SamplerHandle>(ret);
}

void d3d::destroy_sampler(d3d::SamplerHandle sampler)
{
  SamplerResource *samplerResource = reinterpret_cast<SamplerResource *>(sampler);

  // can happen from external thread by overall deletion rules, so must be processed in backend
  LocalAccessor la;
  la.ctx.destroySamplerResource(samplerResource);
}

void d3d::set_sampler(unsigned shader_stage, unsigned unit, d3d::SamplerHandle sampler)
{
  SamplerResource *samplerResource = reinterpret_cast<SamplerResource *>(sampler);

  LocalAccessor la;
  auto &resBinds = la.pipeState.getStageResourceBinds((ShaderStage)shader_stage);

  SRegister sReg(samplerResource);
  if (resBinds.set<StateFieldSRegisterSet, StateFieldSRegister::Indexed>({unit, sReg}))
    la.pipeState.markResourceBindDirty((ShaderStage)shader_stage);
}
