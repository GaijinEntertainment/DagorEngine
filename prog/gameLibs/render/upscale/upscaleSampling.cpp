// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/upscale/upscaleSampling.h>

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_resetDevice.h>
#include <osApiWrappers/dag_spinlock.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/upscale_sampling_weights.hlsli>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>

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
  UniqueBufHolder upscaleWeightsBuffer;
  uint32_t upscale_weights_buffer_counter;
  OSSpinlock upscale_weights_buffer_lock;
  PostFxRenderer upscaleRenderer;
  eastl::unique_ptr<ComputeShaderElement> upscaleRendererCS;
  void addRef()
  {
    OSSpinlockScopedLock lock(upscale_weights_buffer_lock);
    if (upscale_weights_buffer_counter++ == 0)
    {
      upscaleWeightsBuffer = UniqueBufHolder(dag::buffers::create_persistent_cb(UPSCALE_WEIGHTS_COUNT, "upscale_sampling_weights"),
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

UpscaleSamplingTex::~UpscaleSamplingTex() { upsample_single.delRef(); }

UpscaleSamplingTex::UpscaleSamplingTex(uint32_t w, uint32_t h, const char *tag)
{
  String name(128, "%supscale_sampling_weights", tag);
  upsample_single.addRef();
  const uint32_t fmt = TEXFMT_R8UI;
  bool hasComputeSupport = upsample_single.upscaleRendererCS && d3d::should_use_compute_for_image_processing({fmt});

  name = String(128, "%supscale_sampling_tex", tag);

  uint32_t flags = fmt | (hasComputeSupport ? TEXCF_UNORDERED : TEXCF_RTARGET);
  upscaleSamplingTexVarId = get_shader_variable_id("upscale_sampling_tex", true);
  upscaleTexRTPool = RTargetPool::get(w, h, flags, 1);
}

void UpscaleSamplingTex::render(float goffset_x, float goffset_y)
{
  TIME_D3D_PROFILE(upscale_sampling_tex);

  SCOPE_RESET_SHADER_BLOCKS;

  if (upscaleTex == nullptr)
  {
    upscaleTex = upscaleTexRTPool->acquire();
    ShaderGlobal::set_texture(upscaleSamplingTexVarId, upscaleTex->getTexId());
  }

  TextureInfo ti;
  upscaleTex->getTex2D()->getinfo(ti);

  static int upscale_gbuffer_offsetVarId = ::get_shader_variable_id("upscale_gbuffer_offset", true);
  static int downsampled_checkerboard_depth_tex_samplerstateVarId =
    ::get_shader_variable_id("downsampled_checkerboard_depth_tex_samplerstate", true);
  ShaderGlobal::set_int4(upscale_gbuffer_offsetVarId, goffset_x, goffset_y, 0, 0);

  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
  smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
  ShaderGlobal::set_sampler(downsampled_checkerboard_depth_tex_samplerstateVarId, d3d::request_sampler(smpInfo));

  if (upscaleTex && !!(ti.cflg & TEXCF_UNORDERED))
  {
    d3d::set_rwtex(STAGE_CS, 0, upscaleTex->getTex2D(), 0, 0);
    upsample_single.upscaleRendererCS->dispatchThreads(ti.w, ti.h, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    d3d::resource_barrier({upscaleTex->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  }
  else
  {
    SCOPE_RENDER_TARGET;

    auto target = upscaleTex->getTex2D();

    d3d::set_render_target(target, 0);
    upsample_single.upscaleRenderer.render();
    d3d::resource_barrier({target, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  }
  ShaderGlobal::set_sampler(downsampled_checkerboard_depth_tex_samplerstateVarId, d3d::INVALID_SAMPLER_HANDLE);
}

void UpscaleSamplingTex::releaseRTs()
{
  upscaleTex = nullptr;
  ShaderGlobal::set_texture(upscaleSamplingTexVarId, BAD_TEXTUREID);
}
