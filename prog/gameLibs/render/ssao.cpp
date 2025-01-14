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

SSAORenderer::SSAORenderer(int w, int h, int num_views, uint32_t flags, bool use_own_textures, const char *tag) : tag(tag)
{
  aoWidth = w;
  aoHeight = h;
  useOwnTextures = use_own_textures;

  if (useOwnTextures)
  {
    uint32_t format = ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(flags));
    viewSpecific.forTheFirstN(num_views, [&](ViewSpecific &view, int view_ix) {
      for (int i = 0; i < (ssao_prev_texVarId >= 0 ? 3 : 2); ++i)
      {
        String name(128, "%sssao_tex_%d_%d", tag, view_ix, i);
        view.ssaoTex[i] = dag::create_tex(nullptr, aoWidth, aoHeight, format | TEXCF_RTARGET, 1, name.c_str());
        view.ssaoTex[i]->disableSampler();
      }
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
  randomPatternTex.getTex2D()->texfilter(TEXFILTER_POINT);
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
    view.currentSSAOid = 0;
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
    viewSpecific.forEach([&](ViewSpecific &view) {
      for (auto &tex : view.ssaoTex)
        tex.close();
    });
  }
  poissonPoints.close();
  clearRandomPattern();
}

void SSAORenderer::renderSSAO(BaseTexture *depth_to_use, const ManagedTex &ssaoTex, const ManagedTex &prevSsaoTex)
{
  G_UNUSED(depth_to_use);
  ShaderGlobal::set_texture(ssao_prev_texVarId, prevSsaoTex);
  d3d::set_render_target(ssaoTex.getTex2D(), 0);
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

void SSAORenderer::render(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *depth_tex_to_use, const ManagedTex *ssao_tex,
  const ManagedTex *prev_ssao_tex, const ManagedTex *tmp_tex, const DPoint3 *world_pos, SubFrameSample sub_sample)
{
  // SSAO Renderer can work in two modes - using it's own textures or external ones. Mode is set in constructor.
  G_ASSERT(useOwnTextures || (ssao_tex && prev_ssao_tex && tmp_tex));
  TIME_D3D_PROFILE(SSAO_total)
  Driver3dRenderTarget prevRt;
  d3d::get_render_target(prevRt);

  G_ASSERT(depth_tex_to_use);

  updateViewSpecific(view_tm, proj_tm, world_pos);
  if (sub_sample == SubFrameSample::Single || sub_sample == SubFrameSample::First)
  {
    viewSpecific->currentSSAOid = 1 - viewSpecific->currentSSAOid; // 0 or 1
    updateFrameNo();
  }

  const int ownBlurTexId = ssao_prev_texVarId >= 0 ? 2 : (1 - viewSpecific->currentSSAOid);
  const ManagedTex &ssaoTex = ssao_tex ? *ssao_tex : viewSpecific->ssaoTex[viewSpecific->currentSSAOid];
  const ManagedTex &prevSsaoTex = prev_ssao_tex ? *prev_ssao_tex : viewSpecific->ssaoTex[1 - viewSpecific->currentSSAOid];
  const ManagedTex &tmpTex = tmp_tex ? *tmp_tex : viewSpecific->ssaoTex[ownBlurTexId];

  if (randomPatternTex.getTex2D())
    randomPatternTex.setVar();

  {
    TIME_D3D_PROFILE(SSAO_render)
    renderSSAO(depth_tex_to_use, ssaoTex, prevSsaoTex);
  }

  d3d::resource_barrier({ssaoTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  if (g_ssao_blur)
  {
    TIME_D3D_PROFILE(blurSSAO)
    applyBlur(ssaoTex, tmpTex);

    d3d::resource_barrier({ssaoTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  // Restore state
  d3d::set_render_target(prevRt);
  ShaderGlobal::set_texture(ssao_texVarId, ssaoTex);
}

Texture *SSAORenderer::getSSAOTex() { return viewSpecific->ssaoTex[viewSpecific->currentSSAOid].getTex2D(); }

TEXTUREID SSAORenderer::getSSAOTexId() { return viewSpecific->ssaoTex[viewSpecific->currentSSAOid].getTexId(); };

void SSAORenderer::setCurrentView(int view) { viewSpecific.setCurrentView(view); }
#undef GLOBAL_VARS_LIST
