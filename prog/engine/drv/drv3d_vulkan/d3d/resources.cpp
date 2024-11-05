// Copyright (C) Gaijin Games KFT.  All rights reserved.

// common resource-like objects managment implementation
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_res.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_renderStates.h>
#include <texture.h>
#include <buffer.h>
#include "globals.h"
#include "external_resource_pools.h"
#include "shader/pipeline_description.h"
#include <validate_sbuf_flags.h>
#include "render_state_system.h"
#include "sampler_resource.h"
#include "sampler_cache.h"
#include "resourceActivationGeneric.h"

using namespace drv3d_vulkan;

TextureInterfaceBase *drv3d_vulkan::allocate_texture(int res_type, uint32_t cflg)
{
  return Globals::Res::tex.allocate(res_type, cflg);
}

GenericBufferInterface *drv3d_vulkan::allocate_buffer(uint32_t struct_size, uint32_t element_count, uint32_t flags, FormatStore format,
  bool managed, const char *stat_name)
{
  validate_sbuffer_flags(flags, stat_name);
  return Globals::Res::buf.allocate(struct_size, element_count, flags, format, managed, stat_name);
}

void drv3d_vulkan::free_texture(TextureInterfaceBase *texture) { Globals::Res::tex.free(texture); }

void drv3d_vulkan::free_buffer(GenericBufferInterface *buffer) { Globals::Res::buf.free(buffer); }

void d3d::reserve_res_entries(bool strict_max, int max_tex, int max_vs, int max_ps, int max_vdecl, int max_vb, int max_ib,
  int max_stblk)
{
  Globals::Res::tex.reserve(max_tex);
  Globals::Res::buf.reserve(max_vb + max_ib);
  G_UNUSED(strict_max);
  G_UNUSED(max_tex);
  G_UNUSED(max_vs);
  G_UNUSED(max_ps);
  G_UNUSED(max_vdecl);
  G_UNUSED(max_stblk);
}

void d3d::get_max_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  max_tex = Globals::Res::tex.capacity();
  max_vb = max_ib = 0;
  Globals::Res::buf.iterateAllocated([&](auto buffer) {
    int flags = buffer->getFlags();
    if ((flags & SBCF_BIND_MASK) == SBCF_BIND_INDEX)
      ++max_ib;
    else
      ++max_vb;
  });

  auto total = max_vb + max_ib;
  max_vb *= Globals::Res::buf.capacity();
  max_ib *= Globals::Res::buf.capacity();
  max_vb /= total;
  max_ib /= total;
  G_UNUSED(max_vs);
  G_UNUSED(max_ps);
  G_UNUSED(max_vdecl);
  G_UNUSED(max_stblk);
}

void d3d::get_cur_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  max_tex = Globals::Res::tex.size();
  max_vb = max_ib = 0;
  Globals::Res::buf.iterateAllocated([&](auto buffer) {
    int flags = buffer->getFlags();
    if ((flags & SBCF_BIND_MASK) == SBCF_BIND_INDEX)
      ++max_ib;
    else
      ++max_vb;
  });

  G_UNUSED(max_vs);
  G_UNUSED(max_ps);
  G_UNUSED(max_vdecl);
  G_UNUSED(max_stblk);
}

Vbuffer *d3d::create_vb(int size, int flg, const char *name)
{
  return allocate_buffer(1, size, flg | SBCF_BIND_VERTEX | SBCF_BIND_SHADER_RES, FormatStore(), true /*managed*/, name);
}

Ibuffer *d3d::create_ib(int size, int flg, const char *stat_name)
{
  return allocate_buffer(1, size, flg | SBCF_BIND_INDEX | SBCF_BIND_SHADER_RES, FormatStore(), true /*managed*/, stat_name);
}

Vbuffer *d3d::create_sbuffer(int struct_size, int elements, unsigned flags, unsigned format, const char *name)
{
  return allocate_buffer(struct_size, elements, flags, FormatStore::fromCreateFlags(format), true /*managed*/, name);
}

shaders::DriverRenderStateId d3d::create_render_state(const shaders::RenderState &state)
{
  return Globals::renderStates.registerState(Globals::ctx, state);
}

void d3d::clear_render_states()
{
  // do nothing
}

VDECL d3d::create_vdecl(VSDTYPE *vsd)
{
  drv3d_vulkan::InputLayout layout;
  layout.fromVdecl(vsd);
  return Globals::shaderProgramDatabase.registerInputLayout(Globals::ctx, layout).get();
}

void d3d::delete_vdecl(VDECL vdecl)
{
  (void)vdecl;
  // ignore delete request, we keep it as a 'optimization'
}

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
  // use VkSamplerCustomBorderColorCreateInfoEXT with customBorderColorWithoutFormat feature
  result.setBorder(BaseTex::remap_border_color(samplerInfo.border_color, true));

  result.setBias(samplerInfo.mip_map_bias);
  result.setAniso(clamp<float>(samplerInfo.anisotropic_max, 1, 16));

  return result;
}

d3d::SamplerHandle d3d::request_sampler(const d3d::SamplerInfo &sampler_info)
{
  SamplerState state = translate_d3d_samplerinfo_to_vulkan_samplerstate(sampler_info);
  SamplerResource *ret = Globals::samplers.getResource(state);

  return d3d::SamplerHandle(reinterpret_cast<uint64_t>(ret));
}

IMPLEMENT_D3D_RESOURCE_ACTIVATION_API_USING_GENERIC()
