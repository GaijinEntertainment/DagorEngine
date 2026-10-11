// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix4.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_viewScissor.h>
#include <3d/dag_lockSbuffer.h>
#include <math/dag_TMatrix4D.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <perfMon/dag_statDrv.h>
#include <render/viewVecs.h>
#include "ssao_public.h"
#include <render/set_reprojection.h>
#include <drv/3d/dag_lock.h>
#include <poisson/poisson_buffer_helper.h>

#define GLOBAL_VARS_LIST               \
  VAR(ssao_frame_no)                   \
  VAR(ssaoBlurVertical)                \
  VAR(ssao_poisson_sample_count)       \
  VAR(ssao_poisson_frame_count)        \
  VAR(ssao_poisson_all_sample_count)   \
  VAR(ssao_poisson_all_sample_count_f) \
  VAR(ssao_mode)                       \
  VAR(ssao_half_hfov_tan)              \
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

static constexpr const char *ssao_sh_name = "ssao";
static constexpr const char *blur_sh_name = "ssao_blur";

namespace ssao_pass
{

static ao_common::ReprojectionState make_reset_reprojection_state()
{
  ao_common::ReprojectionState state;
  state.prevProjTm = matrix_perspective_reverse(1, 1, 0.1, 1000);
  return state;
}

static void generate_poisson_points(UniqueBufWithShaderVar &poisson_points, const char *tag, int num_samples, int num_frames)
{
  String name(128, "%s_ssao_poisson_samples", tag);
  ::generate_poisson_points(poisson_points, 1984183168U, num_samples * num_frames, name, "ssao_poisson_samples", Point2(2, 2),
    Point2(-1, -1));

  ShaderGlobal::set_int(ssao_poisson_sample_countVarId, num_samples);
  ShaderGlobal::set_int(ssao_poisson_frame_countVarId, num_frames);
  ShaderGlobal::set_int(ssao_poisson_all_sample_countVarId, num_frames * num_samples);
  ShaderGlobal::set_float(ssao_poisson_all_sample_count_fVarId, num_frames * num_samples);
}

RenderResources create_render_resources(uint32_t flags, const char *poisson_tag)
{
  RenderResources res;
  ssaoBlurTexelOffsetVarId = get_shader_variable_id("texelOffset");

  res.aoRenderer.reset(create_postfx_renderer(ssao_sh_name));
  res.blurRenderer.reset(create_postfx_renderer(blur_sh_name));

  if (!(flags & SSAO_SKIP_POISSON_POINTS_GENERATION))
  {
    if ((flags & SSAO_IMMEDIATE) != 0)
    {
      ShaderGlobal::set_int(ssao_modeVarId, SSAO_MODE_IMMEDIATE);
      generate_poisson_points(res.poissonPoints, poisson_tag, 16, 1);
    }
    else
    {
      ShaderGlobal::set_int(ssao_modeVarId, SSAO_MODE_TEMPORAL);
      generate_poisson_points(res.poissonPoints, poisson_tag, 4, 8);
    }
  }

  return res;
}

void resolve(const RenderResources &res, ao_common::FrameState &frame_state, ao_common::ReprojectionState *reprojection,
  const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 *world_pos, SubFrameSample sub_sample, bool is_main_view)
{
  TIME_D3D_PROFILE(SSAO_total)

  if (reprojection)
  {
    set_reprojection(view_tm, proj_tm, reprojection->prevProjTm, reprojection->prevWorldPos, reprojection->prevGlobTm,
      reprojection->prevViewVecLT, reprojection->prevViewVecRT, reprojection->prevViewVecLB, reprojection->prevViewVecRB, world_pos);
  }

  ShaderGlobal::set_float4(ssao_frame_noVarId, frame_state.frameNo, (frame_state.frameNo & 7) / 8.0f, (frame_state.frameNo % 9) / 9.0f,
    (frame_state.frameNo % 11) / 11.0f);

  if (is_main_view && (sub_sample == SubFrameSample::Single || sub_sample == SubFrameSample::Last))
  {
    frame_state.frameNo++;
    frame_state.frameNo &= ((1 << 22) - 1);
  }

  ShaderGlobal::set_float(ssao_half_hfov_tanVarId, 1.0f / max(proj_tm.m[0][0], 1e-6f));

  TIME_D3D_PROFILE(SSAO_render)
  res.aoRenderer->render();
}

static void blur_pass(const RenderResources &res, int blur_variant, int width, int height)
{
  TIME_D3D_PROFILE(blurSSAO_pass);

  Color4 texelOffset(0.5, 0.5, 1.f / width, 1.f / height);
  res.blurRenderer->getMat()->set_color4_param(ssaoBlurTexelOffsetVarId, texelOffset);
  res.blurRenderer->getMat()->set_int_param(ssaoBlurVerticalVarId.get_var_id(), blur_variant);
  res.blurRenderer->render();
}

void two_pass_blur(const RenderResources &res, uint32_t frame_no, BaseTexture *ssao_tex, BaseTexture *tmp_tex, int width, int height)
{
  SCOPE_RENDER_TARGET;

  d3d::resource_barrier({ssao_tex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  if (g_ssao_blur)
  {
    const int order = frame_no & 1; // alternates per frame, hidden by reprojection

    // Phase 1: horizontal. ssao_tex -> tmp_tex.
    d3d::set_render_target({}, DepthAccess::RW, {{tmp_tex, 0, 0}});
    ShaderGlobal::set_texture(ssao_texVarId, ssao_tex);
    blur_pass(res, order, width, height);
    d3d::resource_barrier({tmp_tex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

    // Phase 2: vertical. tmp_tex -> ssao_tex.
    d3d::set_render_target({}, DepthAccess::RW, {{ssao_tex, 0, 0}});
    ShaderGlobal::set_texture(ssao_texVarId, tmp_tex);
    blur_pass(res, 1 - order, width, height);
    d3d::resource_barrier({ssao_tex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  ShaderGlobal::set_texture(ssao_texVarId, nullptr);
}

} // namespace ssao_pass

SSAORenderer::SSAORenderer(int w, int h, int num_views, uint32_t flags, bool use_own_textures, const char *tag)
{
  aoWidth = w;
  aoHeight = h;
  useOwnTextures = use_own_textures;

  if (useOwnTextures)
  {
    uint32_t format = ao_common::creation_flags_to_format(ao_common::consider_shader_assumes(flags));
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

  resources = ssao_pass::create_render_resources(flags, tag);

  resetImpl();
}

void SSAORenderer::reset() { resetImpl(); }

void SSAORenderer::resetImpl()
{
  viewSpecific.forEach([&](ViewSpecific &view) {
    view.frameState = ao_common::FrameState{};
    view.reprojection = ssao_pass::make_reset_reprojection_state();
  });
}

SSAORenderer::~SSAORenderer()
{
  ShaderGlobal::set_texture(ssao_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(ssao_prev_texVarId, BAD_TEXTUREID);
  if (useOwnTextures)
  {
    viewSpecific.forEach([&](ViewSpecific &view) { view.ssaoTex = nullptr; });
  }
}

void SSAORenderer::renderSSAO(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *ssao_tex, BaseTexture *prev_ssao_tex,
  const DPoint3 *world_pos, SubFrameSample sub_sample, bool clear_rt)
{
  // SSAO Renderer can work in two modes - using it's own textures or external ones. Mode is set in constructor.
  G_ASSERT(useOwnTextures || (ssao_tex && prev_ssao_tex));
  SCOPE_RENDER_TARGET;

  BaseTexture *prevSsaoTex = prev_ssao_tex;
  BaseTexture *ssaoTex = ssao_tex;
  RTarget::Ptr prevSsaoTarget;
  if (useOwnTextures)
  {
    prevSsaoTarget = viewSpecific->ssaoTex;
    viewSpecific->ssaoTex = ssaoRTPool->acquire();
    prevSsaoTex = prevSsaoTarget.get()->getBaseTex();
    ssaoTex = viewSpecific->ssaoTex.get()->getBaseTex();
  }

  ShaderGlobal::set_texture(ssao_prev_texVarId, prevSsaoTex);
  d3d::set_render_target({}, DepthAccess::RW, {{ssaoTex, 0, 0}});

  if (clear_rt)
    d3d::clearview(DISCARD_ALL, 0xFFFFFFFF, 1.0, 0);

  ssao_pass::resolve(resources, viewSpecific->frameState, &viewSpecific->reprojection, view_tm, proj_tm, world_pos, sub_sample,
    /*is_main_view*/ true);

  if (useOwnTextures)
    prevSsaoTarget = nullptr;
  else
    ShaderGlobal::set_texture(ssao_prev_texVarId, nullptr);
}

void SSAORenderer::applySSAOBlur(BaseTexture *ssao_tex, BaseTexture *tmp_tex)
{
  G_ASSERT(useOwnTextures || (ssao_tex && tmp_tex));

  BaseTexture *ssaoTex = useOwnTextures ? viewSpecific->ssaoTex.get()->getBaseTex() : ssao_tex;

  RTarget::Ptr tmpSsao;
  BaseTexture *tmpTex = tmp_tex;
  if (useOwnTextures && g_ssao_blur)
  {
    tmpSsao = ssaoRTPool->acquire();
    tmpTex = tmpSsao.get()->getBaseTex();
  }

  ssao_pass::two_pass_blur(resources, viewSpecific->frameState.frameNo, ssaoTex, tmpTex, aoWidth, aoHeight);

  if (useOwnTextures)
  {
    tmpSsao = nullptr;
    // Restore state
    ShaderGlobal::set_texture(ssao_texVarId, ssaoTex);
  }
}

void SSAORenderer::render(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *, BaseTexture *ssao_tex,
  BaseTexture *prev_ssao_tex, BaseTexture *tmp_tex, const DPoint3 *world_pos, SubFrameSample sub_sample)
{
  renderSSAO(view_tm, proj_tm, ssao_tex, prev_ssao_tex, world_pos, sub_sample, true);
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
