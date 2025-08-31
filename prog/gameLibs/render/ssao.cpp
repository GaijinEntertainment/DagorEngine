// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix4.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_lockSbuffer.h>
#include <math/dag_TMatrix4D.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <perfMon/dag_statDrv.h>
#include <render/viewVecs.h>
#include <render/ssao.h>
#include <render/set_reprojection.h>
#include <drv/3d/dag_lock.h>
#include <poisson/poisson_buffer_helper.h>

#define PATTERN_SIZE 3
#define SAMPLE_COUNT 8

#define GLOBAL_VARS_LIST               \
  VAR(ssao_frame_no)                   \
  VAR(ssaoBlurVertical)                \
  VAR(ssao_poisson_sample_count)       \
  VAR(ssao_poisson_frame_count)        \
  VAR(ssao_poisson_all_sample_count)   \
  VAR(ssao_poisson_all_sample_count_f) \
  VAR(ssao_mode)                       \
  VAR(ssao_prev_tex)                   \
  VAR(ssao_tex)                        \
  VAR(ssao_prev_tex_samplerstate)      \
  VAR(ssao_tex_samplerstate)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

static int ssaoBlurTexelOffsetVarId = -1;

// enable or disable SSAO blur
bool g_ssao_blur = true;

SSAORenderer::SSAORenderer(int w, int h, int num_views, uint32_t flags, bool use_own_textures, const char *tag,
  bool use_own_reprojection_params) :
  tag(tag)
{
  aoWidth = w;
  aoHeight = h;
  useOwnTextures = use_own_textures;
  useOwnReprojectionParams = use_own_reprojection_params;

  if (useOwnTextures)
  {
    uint32_t format = ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(flags));
    ssaoRTPool = RTargetPool::get(aoWidth, aoHeight, format | TEXCF_RTARGET, 1);

    viewSpecific.forTheFirstN(num_views, [&](ViewSpecific &view, int view_ix) {
      G_UNUSED(view_ix);
      view.ssaoTex = ssaoRTPool->acquire();
    });

    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.border_color = d3d::BorderColor::Color::OpaqueWhite;
    ssao_prev_tex_samplerstateVarId.set_sampler(d3d::request_sampler(smpInfo));
    ssao_tex_samplerstateVarId.set_sampler(d3d::request_sampler(smpInfo));
  }
  ssaoBlurTexelOffsetVarId = get_shader_variable_id("texelOffset");

  aoRenderer.reset(create_postfx_renderer(ssao_sh_name));
  ssaoBlurRenderer.reset(create_postfx_renderer(blur_sh_name));

  if (!(flags & SSAO_SKIP_POISSON_POINTS_GENERATION))
  {
    if ((flags & SSAO_IMMEDIATE) != 0)
    {
      ShaderGlobal::set_int(ssao_modeVarId, 0);
      generatePoissionPoints(16, 1);
    }
    else
    {
      ShaderGlobal::set_int(ssao_modeVarId, 1);
      generatePoissionPoints(4, 8);
    }
  }

  reset();

  if (!(flags & SSAO_SKIP_RANDOM_PATTERN_GENERATION))
    initRandomPattern();
}

void SSAORenderer::initRandomPattern()
{
  String name(128, "%srandom_pattern_tex", tag);

  randomPatternTex.close();
  randomPatternTex = UniqueTexHolder(
    dag::create_tex(nullptr, PATTERN_SIZE * PATTERN_SIZE, SAMPLE_COUNT, TEXCF_RTARGET | TEXFMT_A16B16G16R16F, 1, name.data()),
    "random_pattern_tex");
  renderRandomPattern();
}

void SSAORenderer::renderRandomPattern()
{
  d3d::GpuAutoLock lock;
  SCOPE_RESET_SHADER_BLOCKS;
  Driver3dRenderTarget prevRt;
  d3d::get_render_target(prevRt);
  d3d::set_render_target(randomPatternTex.getTex2D(), 0);
  PostFxRenderer randomizer;
  randomizer.init("bent_cones_random_pattern");
  randomizer.render();
  d3d::set_render_target(prevRt);
}

void SSAORenderer::generatePoissionPoints(int num_samples, int num_frames)
{
  String name(128, "%s_ssao_poisson_samples", tag);
  generate_poission_points(poissonPoints, 1984183168U, num_samples * num_frames, name, "ssao_poisson_samples");

  ShaderGlobal::set_int(ssao_poisson_sample_countVarId, num_samples);
  ShaderGlobal::set_int(ssao_poisson_frame_countVarId, num_frames);
  ShaderGlobal::set_int(ssao_poisson_all_sample_countVarId, num_frames * num_samples);
  ShaderGlobal::set_real(ssao_poisson_all_sample_count_fVarId, num_frames * num_samples);
}

void SSAORenderer::reset()
{
  viewSpecific.forEach([&](ViewSpecific &view) {
    view.prevGlobTm = TMatrix4(0);
    view.prevProjTm = matrix_perspective_reverse(1, 1, 0.1, 1000);
    view.prevViewVecLT = Point4(0, 0, 0, 0);
    view.prevViewVecRT = Point4(0, 0, 0, 0);
    view.prevViewVecLB = Point4(0, 0, 0, 0);
    view.prevViewVecRB = Point4(0, 0, 0, 0);
    view.prevWorldPos = DPoint3(0, 0, 0);
    view.frameNo = 0;
  });

  if (randomPatternTex.getTex2D())
  {
    randomPatternTex.setVar();
    renderRandomPattern();
  }
}

SSAORenderer::~SSAORenderer()
{
  ShaderGlobal::set_texture(ssao_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(ssao_prev_texVarId, BAD_TEXTUREID);
  if (useOwnTextures)
  {
    viewSpecific.forEach([&](ViewSpecific &view) { view.ssaoTex = nullptr; });
  }
  poissonPoints.close();
  clearRandomPattern();
}

void SSAORenderer::renderSSAO(BaseTexture *depth_to_use, const ManagedTex &ssaoTex, const ManagedTex &prevSsaoTex, bool clear_rt)
{
  G_UNUSED(depth_to_use);
  ShaderGlobal::set_texture(ssao_prev_texVarId, prevSsaoTex);
  d3d::set_render_target(ssaoTex.getTex2D(), 0);

  if (clear_rt)
    d3d::clearview(CLEAR_DISCARD, 0xFFFFFFFF, 1.0, 0);

  aoRenderer->render();
}

void SSAORenderer::updateViewSpecific(const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 *world_pos)
{
  set_reprojection(view_tm, proj_tm, viewSpecific->prevProjTm, viewSpecific->prevWorldPos, viewSpecific->prevGlobTm,
    viewSpecific->prevViewVecLT, viewSpecific->prevViewVecRT, viewSpecific->prevViewVecLB, viewSpecific->prevViewVecRB, world_pos);
}

void SSAORenderer::updateFrameNo()
{
  ShaderGlobal::set_color4(ssao_frame_noVarId, viewSpecific->frameNo, (viewSpecific->frameNo & 7) / 8.0f,
    (viewSpecific->frameNo % 9) / 9.0f, (viewSpecific->frameNo % 11) / 11.0f);
  viewSpecific->frameNo++;
  viewSpecific->frameNo &= ((1 << 22) - 1);
}

void SSAORenderer::applyBlur(const ManagedTex &ssaoTex, const ManagedTex &tmpTex)
{
  Color4 texelOffset(0.5, 0.5, 1.f / aoWidth, 1.f / aoHeight);

  ssaoBlurRenderer->getMat()->set_color4_param(ssaoBlurTexelOffsetVarId, texelOffset);

  // Phase 1. horizontal. ssao -> blurredSsaoTex
  d3d::set_render_target(tmpTex.getTex2D(), 0);

  ShaderGlobal::set_texture(ssao_texVarId, ssaoTex);
  const int order = viewSpecific->frameNo & 1; // we change order of blur each odd frame. that's hide it's unsepratable nature, with a
                                               // help of reprojection
  ssaoBlurRenderer->getMat()->set_int_param(ssaoBlurVerticalVarId.get_var_id(), order);
  ssaoBlurRenderer->render();

  d3d::resource_barrier({tmpTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  // Phase 2. vertical. blurredSsaoTex -> shadowBufferTex
  d3d::set_render_target(ssaoTex.getTex2D(), 0);
  ssaoBlurRenderer->getMat()->set_int_param(ssaoBlurVerticalVarId.get_var_id(), 1 - order);
  ShaderGlobal::set_texture(ssao_texVarId, tmpTex);
  ssaoBlurRenderer->render();
}

void SSAORenderer::renderSSAO(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *depth_tex_to_use,
  const ManagedTex *ssao_tex, const ManagedTex *prev_ssao_tex, const DPoint3 *world_pos, SubFrameSample sub_sample, bool clear_rt)
{
  // SSAO Renderer can work in two modes - using it's own textures or external ones. Mode is set in constructor.
  G_ASSERT(useOwnTextures || (ssao_tex && prev_ssao_tex));
  TIME_D3D_PROFILE(SSAO_total)
  SCOPE_RENDER_TARGET;

  G_ASSERT(depth_tex_to_use);

  if (useOwnReprojectionParams)
    updateViewSpecific(view_tm, proj_tm, world_pos);

  if (sub_sample == SubFrameSample::Single || sub_sample == SubFrameSample::First)
  {
    updateFrameNo();
  }

  const ManagedTex *prevSsaoTex = prev_ssao_tex;
  const ManagedTex *ssaoTex = ssao_tex;
  RTarget::Ptr prevSsaoTarget;
  if (useOwnTextures)
  {
    prevSsaoTarget = viewSpecific->ssaoTex;
    viewSpecific->ssaoTex = ssaoRTPool->acquire();
    prevSsaoTex = prevSsaoTarget.get();
    ssaoTex = viewSpecific->ssaoTex.get();
  }

  if (randomPatternTex.getTex2D())
    randomPatternTex.setVar();

  {
    TIME_D3D_PROFILE(SSAO_render)
    renderSSAO(depth_tex_to_use, *ssaoTex, *prevSsaoTex, clear_rt);
  }

  if (useOwnTextures)
  {
    prevSsaoTarget = nullptr;
  }
}

void SSAORenderer::applySSAOBlur(const ManagedTex *ssao_tex, const ManagedTex *tmp_tex)
{
  G_ASSERT(useOwnTextures || (ssao_tex && tmp_tex));

  const ManagedTex *ssaoTex = useOwnTextures ? viewSpecific->ssaoTex.get() : ssao_tex;
  SCOPE_RENDER_TARGET;

  d3d::resource_barrier({ssaoTex->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  if (g_ssao_blur)
  {
    TIME_D3D_PROFILE(blurSSAO);
    RTarget::Ptr tmpSsao;
    const ManagedTex *tmpTex = tmp_tex;
    if (useOwnTextures)
    {
      tmpSsao = ssaoRTPool->acquire();
      tmpTex = tmpSsao.get();
    }
    applyBlur(*ssaoTex, *tmpTex);
    if (useOwnTextures)
      tmpSsao = nullptr;
    d3d::resource_barrier({ssaoTex->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  // Restore state
  ShaderGlobal::set_texture(ssao_texVarId, *ssaoTex);
}

void SSAORenderer::render(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *depth_tex_to_use, const ManagedTex *ssao_tex,
  const ManagedTex *prev_ssao_tex, const ManagedTex *tmp_tex, const DPoint3 *world_pos, SubFrameSample sub_sample)
{
  renderSSAO(view_tm, proj_tm, depth_tex_to_use, ssao_tex, prev_ssao_tex, world_pos, sub_sample, true);
  applySSAOBlur(ssao_tex, tmp_tex);
}

Texture *SSAORenderer::getSSAOTex()
{
  G_ASSERT(viewSpecific->ssaoTex);
  return viewSpecific->ssaoTex->getTex2D();
}

TEXTUREID SSAORenderer::getSSAOTexId()
{
  G_ASSERT(viewSpecific->ssaoTex);
  return viewSpecific->ssaoTex->getTexId();
};

void SSAORenderer::setCurrentView(int view) { viewSpecific.setCurrentView(view); }
#undef GLOBAL_VARS_LIST
