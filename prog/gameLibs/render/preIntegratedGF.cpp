// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/string.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_decl.h>
#include <render/preIntegratedGF.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>

#define GLOBAL_VARS_LIST VAR(preintegrated_envi_frame)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

TexPtr create_preintegrated_fresnel_GGX(const char *name, uint32_t preintegrate_type)
{
  uint32_t prefmt = TEXFMT_G16R16;
#if _TARGET_C1 | _TARGET_C2

#else
  const bool tryU16Format = true;
#endif

  // PS4 shader is hardwired to either u16 or f16 component output
  if (preintegrate_type & PREINTEGRATE_SPECULAR_DIFFUSE)
  {
    if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16) & d3d::USAGE_RTARGET) && tryU16Format)
      prefmt = TEXFMT_A16B16G16R16;
    else if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & d3d::USAGE_RTARGET))
      prefmt = TEXFMT_A16B16G16R16F;
    else if ((d3d::get_texformat_usage(TEXFMT_A2B10G10R10) & d3d::USAGE_RTARGET)) // 10 bits is a little bit not enough bits, but
                                                                                  // performance is higher. 11bit float is definetly
                                                                                  // not enough bits
      prefmt = TEXFMT_A2B10G10R10;
    else
      prefmt = TEXFMT_DEFAULT;
    if (!(preintegrate_type & PREINTEGRATE_QUALITY_MAX) && prefmt != TEXFMT_A2B10G10R10)
      if ((d3d::get_texformat_usage(TEXFMT_A2B10G10R10) & d3d::USAGE_RTARGET)) // 10 bits is a little bit not enough bits, but
                                                                               // performance is higher. 11bit float is definetly not
                                                                               // enough bits
        prefmt = TEXFMT_A2B10G10R10;
  }
  else
  {
    if ((d3d::get_texformat_usage(TEXFMT_G16R16) & d3d::USAGE_RTARGET) && tryU16Format)
      prefmt = TEXFMT_G16R16;
    else if ((d3d::get_texformat_usage(TEXFMT_G16R16F) & d3d::USAGE_RTARGET))
      prefmt = TEXFMT_G16R16F;
    else
      prefmt = TEXFMT_DEFAULT;
  }

  TexPtr preIntegratedGF = dag::create_tex(nullptr, 128, 32, prefmt | TEXCF_RTARGET, 1, name);
  preIntegratedGF->disableSampler();
  eastl::string samplerName(eastl::string::CtorSprintf{}, "%s_samplerstate", name);
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  ShaderGlobal::set_sampler(get_shader_variable_id(samplerName.c_str(), true), d3d::request_sampler(smpInfo));
  return preIntegratedGF;
}

void render_preintegrated_fresnel_GGX(Texture *preIntegratedGF, PostFxRenderer *preintegrate_envi, uint32_t frame, uint32_t frames)
{
  d3d::GpuAutoLock gpuLock;
  SCOPE_RENDER_TARGET;

  d3d::set_render_target(preIntegratedGF, 0);
  ShaderGlobal::set_int(preintegrated_envi_frameVarId, frames <= 1 ? -1 : int(frame));
  // d3d::clearview( CLEAR_TARGET, 0xffffffff, 0.f, 0 );

  // use external postfx renderer or recreate it inplace every call
  if (!preintegrate_envi)
  {
    PostFxRenderer preintegrateEnviInplace;
    preintegrateEnviInplace.init("preintegrateEnvi");
    preintegrateEnviInplace.render();
  }
  else
    preintegrate_envi->render();
  d3d::resource_barrier({preIntegratedGF, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}

TexPtr render_preintegrated_fresnel_GGX(const char *name, uint32_t preintegrate_type, PostFxRenderer *preintegrate_envi)
{
  TexPtr ret = create_preintegrated_fresnel_GGX(name, preintegrate_type);
  if (!ret)
    return ret;
  render_preintegrated_fresnel_GGX(ret.get(), preintegrate_envi, 0, 1);
  return ret;
}

MultiFramePGF::MultiFramePGF(uint32_t preintegrate_type, uint32_t _frames, const char *name, const char *shaderName)
{
  init(preintegrate_type, _frames, name, shaderName);
}

void MultiFramePGF::close()
{
  shader.clear();
  preIntegratedGF.close();
}

void MultiFramePGF::init(uint32_t preintegrate_type, uint32_t _frames, const char *name, const char *shaderName)
{
  close();
  if (!VariableMap::isVariablePresent(preintegrated_envi_frameVarId))
    _frames = 1;
  frames = _frames;
  preIntegratedGF = create_preintegrated_fresnel_GGX(name, preintegrate_type);
  shader.init(shaderName);
  reset();
}

void MultiFramePGF::reset()
{
  if (frames > 1)
  {
    TextureInfo ti;
    preIntegratedGF.getBaseTex()->getinfo(ti);
    frames = (d3d::get_texformat_usage(ti.cflg & TEXFMT_MASK) & d3d::USAGE_BLEND) ? frames : 1;
  }
  frame = 0;
  render_preintegrated_fresnel_GGX(preIntegratedGF.getBaseTex(), &shader, frame++, frames);
}

void MultiFramePGF::updateFrame()
{
  if (!preIntegratedGF)
    init();
  if (frame < frames)
    render_preintegrated_fresnel_GGX(preIntegratedGF.getBaseTex(), &shader, frame++, frames);
}
