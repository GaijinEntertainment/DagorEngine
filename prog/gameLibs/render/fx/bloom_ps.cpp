#include <3d/dag_drv3d.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_adjpow2.h>
#include <shaders/dag_shaders.h>
#include <render/fx/bloom_ps.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_overrideStates.h>

#define GLOBAL_VARS_BLOOM   \
  VAR(should_write_log_lum) \
  VAR(bloom_mul)            \
  VAR(blur_src_tex)         \
  VAR(blur_src_tex_size)    \
  VAR(blur_pixel_offset)    \
  VAR(bloom_upsample_params)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_BLOOM
#undef VAR


void BloomPS::close()
{
  bloomMips.close();
  bloomLastMip.close();
  downsampledFrame.close();
  bloomLastMipStartMip = 100;
}

void BloomPS::getLumaResolution(int &w, int &h)
{
  w = width >> 4;
  h = height >> 4;
}

void BloomPS::init(int w, int h)
{
  close();
  uint32_t fmtflags = TEXCF_RTARGET | TEXFMT_R11G11B10F;
  width = w;
  height = h;
  const int gaussHalfKernel = 7; // we perform 15x15 gauss on higher mips
  mipsCount = clamp((int)get_log2i(min(w / 4, h / 4) / gaussHalfKernel), 1, (int)MAX_MIPS);
  debug("bloom inited with %d mips", mipsCount);
  downsampledFrame = dag::create_tex(NULL, width / 2, height / 2, fmtflags, 2, "downsampled_frame");
  d3d::resource_barrier({downsampledFrame.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  downsampledFrame.getTex2D()->texaddr(TEXADDR_CLAMP);
  bloomMips = dag::create_tex(NULL, width / 4, height / 4, fmtflags, mipsCount, "bloom_tex");
  bloomMips.getTex2D()->texaddr(TEXADDR_BORDER);
  ;

  downsample_first.init("frame_bloom_downsample");
  downsample_13.init("bloom_downsample_hq");
  downsample_4.init("bloom_downsample_lq");
  upSample.init("bloom_upsample");
  blur_4.init("bloom_blur_4");
#define VAR(a) a##VarId = get_shader_variable_id(#a);
  GLOBAL_VARS_BLOOM
#undef VAR
}


void BloomPS::downsample()
{
  TIME_D3D_PROFILE(downsample);
  SCOPE_RENDER_TARGET;
  {
    d3d::set_render_target(downsampledFrame.getTex2D(), 0);
    ShaderGlobal::set_color4(blur_src_tex_sizeVarId, 1.0f / width, 1.0f / height, 0, 0);
    downsample_first.render();
  }
  d3d::resource_barrier({downsampledFrame.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});
}

static void blur_bloom_mip(int tex_mip, ManagedTex &tex, ManagedTex &interm_texture,
  int interm_texture_mip, // target_mip-bloom_last_mip_start_mip
  const PostFxRenderer &blur)
{
  TextureInfo info;
  const int levels = tex.getTex2D()->level_count();
  G_ASSERTF((uint32_t)tex_mip < levels, "%d < %d", tex_mip, levels);
  G_ASSERTF((uint32_t)interm_texture_mip < interm_texture.getTex2D()->level_count(), "%d < %d", interm_texture_mip,
    interm_texture.getTex2D()->level_count());
  tex_mip = min(levels - 1, tex_mip);
  tex.getTex2D()->getinfo(info, tex_mip);
  const int w = info.w, h = info.h;

  interm_texture.getTex2D()->getinfo(info, interm_texture_mip);
  G_ASSERTF(info.w == w && info.h == h, "%dx%d (%d) != %dx%d (mip = %d)", w, h, tex_mip, info.w, info.h, interm_texture_mip);
  d3d::set_render_target(interm_texture.getTex2D(), interm_texture_mip);
  interm_texture.getTex2D()->texmiplevel(interm_texture_mip, interm_texture_mip);
  ShaderGlobal::set_texture(blur_src_texVarId, tex);
  ShaderGlobal::set_color4(blur_pixel_offsetVarId, 1.0f / w, 0, 0, 0);
  tex.getTex2D()->texmiplevel(tex_mip, tex_mip);

  blur.render();

  d3d::set_render_target(tex.getTex2D(), tex_mip);
  d3d::resource_barrier({interm_texture.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, unsigned(interm_texture_mip), 1});
  ShaderGlobal::set_texture(blur_src_texVarId, interm_texture.getTexId());
  ShaderGlobal::set_color4(blur_pixel_offsetVarId, 0, 1.0f / h, 0, 0);

  blur.render();

  interm_texture.getTex2D()->texmiplevel(-1, -1);
  tex.getTex2D()->texmiplevel(-1, -1);
}

static void ensure_mip(int mip, UniqueTex &bloom_last_mip, int &bloom_last_mip_start_mip, int mips_count, int width, int height)
{
  if (bloom_last_mip.getTex2D() && bloom_last_mip_start_mip <= mip)
    return;
  bloom_last_mip_start_mip = mip;
  const int w = (width / 4) >> mip, h = (height / 4) >> mip;
  bloom_last_mip.close();
  bloom_last_mip =
    dag::create_tex(NULL, w, h, TEXCF_RTARGET | TEXFMT_R11G11B10F, mips_count - bloom_last_mip_start_mip, "gauss_bloom_mip");
  bloom_last_mip.getTex2D()->texaddr(TEXADDR_BORDER);
}

void BloomPS::perform(BloomAdaptationParams *adaptation_params, bool force_hq_bloom)
{
  const bool isWorking = on && settings.mul > 0.f;
  if (!isWorking)
  {
    ShaderGlobal::set_real(bloom_mulVarId, 0);
    ShaderGlobal::set_texture(bloomMips.getVarId(), BAD_TEXTUREID);
    return;
  }

  TIME_D3D_PROFILE(bloom);
  SCOPE_RENDER_TARGET;

  TIME_D3D_PROFILE(downsample_mips);
  {
    d3d::set_render_target(bloomMips.getTex2D(), 0);
    downsampledFrame.getTex2D()->texmiplevel(0, 0);
    TIME_D3D_PROFILE(firstDownsample);

    const bool writeLuma = adaptation_params && !adaptation_params->isFixedExposure && adaptation_params->lumaTex;
    ShaderGlobal::set_int(should_write_log_lumVarId, writeLuma ? 1 : 0);
    if (writeLuma)
      d3d::set_rwtex(STAGE_PS, 7, adaptation_params->lumaTex, 0, 0);

    ShaderGlobal::set_texture(blur_src_texVarId, downsampledFrame);
    ShaderGlobal::set_color4(blur_src_tex_sizeVarId, 1.0f / (width / 2), 1.0f / (height / 2), settings.threshold, 0);
    downsample_13.render();
    ShaderGlobal::set_int(should_write_log_lumVarId, 0);
    if (writeLuma)
    {
      d3d::set_rwtex(STAGE_PS, 7, nullptr, 0, 0);
      d3d::resource_barrier({adaptation_params->lumaTex, RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    }
  }

  {
    TIME_D3D_PROFILE(downsampleMips);
    for (int i = 0; i < mipsCount; ++i)
    {
      if (i > 0)
      {
        d3d::resource_barrier({bloomMips.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, unsigned(i - 1), 1});
        d3d::set_render_target(bloomMips.getTex2D(), i);
        bloomMips.getTex2D()->texmiplevel(i - 1, i - 1);
        ShaderGlobal::set_texture(blur_src_texVarId, bloomMips);
        ShaderGlobal::set_color4(blur_src_tex_sizeVarId, 1.0f / ((width / 2) >> i), 1.0f / ((height / 2) >> i), i, 0);
        downsample_4.render();
      }

      if (i >= mipsCount / 2 || (settings.highQuality || force_hq_bloom))
      {
        bloomMips.getTex2D()->texmiplevel(i, i);
        d3d::resource_barrier({bloomMips.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, unsigned(i), 1});
        ensure_mip(i, bloomLastMip, bloomLastMipStartMip, mipsCount, width, height);
        bloomLastMip.getTex2D()->texaddr(TEXADDR_BORDER);
        blur_bloom_mip(i, bloomMips, bloomLastMip, i - bloomLastMipStartMip, blur_4);
      }
    }
    G_UNUSED(force_hq_bloom);
  }

  {
    TIME_D3D_PROFILE(upsample_mips);
    uint8_t bf = real2uchar(settings.upSample);
    for (int i = mipsCount - 1; i > 0; --i)
    {
      d3d::resource_barrier({bloomMips.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, unsigned(i), 1});
      int w = (width / 2) >> i, h = (height / 2) >> i;
      d3d::set_render_target(bloomMips.getTex2D(), i - 1);
      bloomMips.getTex2D()->texmiplevel(i, i);
      ShaderGlobal::set_texture(blur_src_texVarId, bloomMips);

      d3d::set_blend_factor(E3DCOLOR(bf, bf, bf, bf));
      ShaderGlobal::set_color4(bloom_upsample_paramsVarId, settings.radius / w, settings.radius / h, 1. / w, 1. / h);
      upSample.render();
    }
  }
  bloomMips.getTex2D()->texmiplevel(-1, -1);
  d3d::resource_barrier({bloomMips.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});
  bloomMips.setVar();
  ShaderGlobal::set_real(bloom_mulVarId, settings.mul);
  downsampledFrame.getTex2D()->texmiplevel(-1, -1);
}
