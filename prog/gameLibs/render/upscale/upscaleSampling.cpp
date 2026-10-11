// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/upscale/upscaleSampling.h>

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_viewScissor.h>
#include <osApiWrappers/dag_spinlock.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/upscale_sampling_weights.hlsli>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>
#include <math/integer/dag_IPoint2.h>

namespace
{

static const float UPSCALE_WEIGHTS_DATA[UPSCALE_WEIGHTS_COUNT * 4] = {
  1,
  1,
  1,
  1,
  0,
  0,
  0,
  1,
  0,
  0,
  1,
  0,
  0,
  0,
  1,
  1,
  0,
  1,
  0,
  0,
  0,
  1,
  0,
  1,
  0,
  1,
  1,
  0,
  0,
  1,
  1,
  1,

  1,
  0,
  0,
  0,
  1,
  0,
  0,
  1,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  1,
  1,
  1,
  0,
  0,
  1,
  1,
  0,
  1,
  1,
  1,
  1,
  0,
  1,
  1,
  1,
  1,
};

}; // anonymous namespace
static struct UpsampleTextureSingleTone
{
  UniqueBufWithShaderVar upscaleWeightsBuffer;
  uint32_t upscale_weights_buffer_counter;
  OSSpinlock upscale_weights_buffer_lock;
  PostFxRenderer upscaleRenderer;
  eastl::unique_ptr<ComputeShaderElement> upscaleRendererCS;
  void addRef()
  {
    OSSpinlockScopedLock lock(upscale_weights_buffer_lock);
    if (upscale_weights_buffer_counter++ == 0)
    {
      upscaleWeightsBuffer =
        UniqueBufWithShaderVar(dag::buffers::create_persistent_cb(UPSCALE_WEIGHTS_COUNT, "upscale_sampling_weights", RESTAG_POSTFX),
          "upscale_sampling_weights");
      upload_weights();
      upscaleRenderer.init("upscale_sampling");
      upscaleRendererCS.reset(new_compute_shader("upscale_sampling_cs", true));
    }
  }
  void delRef()
  {
    OSSpinlockScopedLock lock(upscale_weights_buffer_lock);
    if (--upscale_weights_buffer_counter == 0)
    {
      upscaleWeightsBuffer.close();
      upscaleRenderer.clear();
      upscaleRendererCS.reset();
    }
  }
  void reset()
  {
    OSSpinlockScopedLock lock(upscale_weights_buffer_lock);

    if (upscale_weights_buffer_counter)
      upload_weights();
  }
  void upload_weights()
  {
    if (!upscaleWeightsBuffer.getBuf())
      return;
    upscaleWeightsBuffer.getBuf()->updateData(0, sizeof(UPSCALE_WEIGHTS_DATA), (void *)UPSCALE_WEIGHTS_DATA,
      VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  }

} upsample_single;

static void upscale_weights_after_reset_device(bool) { upsample_single.reset(); }

REGISTER_D3D_AFTER_RESET_FUNC(upscale_weights_after_reset_device);

UpscaleSamplingRenderer::UpscaleSamplingRenderer() { upsample_single.addRef(); }

UpscaleSamplingRenderer::~UpscaleSamplingRenderer() { upsample_single.delRef(); }

uint32_t UpscaleSamplingRenderer::getTexFlags() const
{
  const uint32_t fmt = TEXFMT_R8UI;
  bool hasComputeSupport = upsample_single.upscaleRendererCS && d3d::should_use_compute_for_image_processing({fmt});
  return fmt | (hasComputeSupport ? TEXCF_UNORDERED : TEXCF_RTARGET);
}

void UpscaleSamplingRenderer::render(BaseTexture *target, const IPoint2 &resolution, const IPoint2 &gbuffer_offset) const
{
  TIME_D3D_PROFILE(upscale_sampling_tex);

  SCOPE_RESET_SHADER_BLOCKS;

  TextureInfo ti;
  target->getinfo(ti);

  static int upscale_gbuffer_offsetVarId = ::get_shader_variable_id("upscale_gbuffer_offset", true);
  static int downsampled_checkerboard_depth_tex_samplerstateVarId =
    ::get_shader_variable_id("downsampled_checkerboard_depth_tex_samplerstate", true);
  ShaderGlobal::set_int4(upscale_gbuffer_offsetVarId, gbuffer_offset.x, gbuffer_offset.y, 0, 0);

  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
  smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
  ShaderGlobal::set_sampler(downsampled_checkerboard_depth_tex_samplerstateVarId, d3d::request_sampler(smpInfo));

  if (ti.cflg & TEXCF_UNORDERED)
  {
    d3d::set_rwtex(STAGE_CS, 0, target, 0, 0);
    upsample_single.upscaleRendererCS->dispatchThreads(resolution.x / 2 + 1, resolution.y / 2 + 1, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  }
  else
  {
    SCOPE_RENDER_TARGET;

    d3d::set_render_target({}, DepthAccess::RW, {{target, 0, 0}});
    d3d::setview(0, 0, resolution.x, resolution.y, 0, 1);
    d3d::setscissor(0, 0, resolution.x, resolution.y);
    upsample_single.upscaleRenderer.render();
  }
  ShaderGlobal::set_sampler(downsampled_checkerboard_depth_tex_samplerstateVarId, d3d::INVALID_SAMPLER_HANDLE);
}

UpscaleSamplingTex::UpscaleSamplingTex(uint32_t w, uint32_t h, const char *)
{
  upscaleSamplingTexVarId = get_shader_variable_id("upscale_sampling_tex", true);
  upscaleTexRTPool = RTargetPool::get(w, h, renderer.getTexFlags(), 1);
}

void UpscaleSamplingTex::render(float goffset_x, float goffset_y)
{
  if (upscaleTex == nullptr)
  {
    upscaleTex = upscaleTexRTPool->acquire();
    ShaderGlobal::set_texture(upscaleSamplingTexVarId, upscaleTex->getTexId());
  }

  TextureInfo ti;
  upscaleTex->getTex2D()->getinfo(ti);
  renderer.render(upscaleTex->getTex2D(), IPoint2(ti.w, ti.h), IPoint2(goffset_x, goffset_y));
  d3d::resource_barrier({upscaleTex->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
}

void UpscaleSamplingTex::releaseRTs()
{
  upscaleTex = nullptr;
  ShaderGlobal::set_texture(upscaleSamplingTexVarId, BAD_TEXTUREID);
}
