#include <render/distortion.h>

#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <3d/dag_drv3d.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderMesh.h>
#include <fx/dag_hdrRender.h>

static inline Color4 calc_texsz_consts(int w, int h)
{
  return Color4(0.5f, -0.5f, 0.5f + HALF_TEXEL_OFSF / w, 0.5f + HALF_TEXEL_OFSF / h);
}


DistortionRenderer *distortionRenderer = NULL;

DistortionRenderer::DistortionRenderer(int screenWidth, int screenHeight, bool _msaaPs3)
{
  mistEnableVarId = -1;
  started = false;

  msaaPs3 = _msaaPs3;

  const DataBlock *distortionBlk = ::dgs_get_game_params()->getBlockByNameEx("distortion");

  ShaderGlobal::set_real(get_shader_variable_id("distortion_offset_scale"), distortionBlk->getReal("offsetScale", 1.f));


  if (!msaaPs3)
  {
    distortionTargetTex =
      d3d::create_tex(NULL, screenWidth, screenHeight, ::hdr_render_format | TEXCF_RTARGET, 1, "distortionTargetTex");
    G_ASSERT(distortionTargetTex);

    distortionTargetTexId = register_managed_tex("distortionTargetTex", distortionTargetTex);
  }
  else
  {
    distortionTargetTexId = BAD_TEXTUREID;
    distortionTargetTex = NULL;
  }
  distortionOffsetTexVarId = get_shader_variable_id("distortion_offset_tex");

  distortionFxRenderer = new PostFxRenderer;
  distortionFxRenderer->init("apply_distortion");
  distortionFxRenderer->getMat()->set_color4_param(::get_shader_variable_id("texsz_consts"),
    calc_texsz_consts(screenWidth, screenHeight));

  sourceTexVarId = ::get_shader_variable_id("source_tex");
  // mistEnableVarId=::get_shader_glob_var_id("mist_enable");
}

DistortionRenderer::~DistortionRenderer()
{
  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(distortionTargetTexId, distortionTargetTex);
  delete distortionFxRenderer;
}

void DistortionRenderer::startRenderDistortion(Texture *distortionOffsetTex)
{
  G_ASSERT(!started);
  started = true;
  // Render offset map masked by depth.

  d3d::get_render_target(prevRt);
  d3d::set_render_target(distortionOffsetTex, 0);
  d3d::set_backbuf_depth();
  d3d::clearview(CLEAR_TARGET, E3DCOLOR_MAKE(0x80, 0x80, 0, 0), 0.f, 0);

  /*float prevHdrOverbright = ShaderGlobal::get_real_fast(hdrOverbrightGlobVarId);
  ShaderGlobal::set_real(hdrOverbrightVarId, 1.f);
  ShaderGlobal::set_real(maxHdrOverbrightVarId, 1.f);*/
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  // ShaderGlobal::set_int_fast(mistEnableVarId, 0);
}


void DistortionRenderer::endRenderDistortion()
{
  G_ASSERT(started);
  started = false;

  /*ShaderGlobal::set_real(hdrOverbrightVarId, prevHdrOverbright);
  ShaderGlobal::set_real(maxHdrOverbrightVarId, ::hdr_max_overbright);*/
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  // ShaderGlobal::set_int_fast(mistEnableVarId, 1);

  d3d::set_render_target(prevRt);
}

void DistortionRenderer::applyDistortion(TEXTUREID sourceTexId, Texture *destTex, TEXTUREID distortionOffsetTexId)
{
  G_ASSERT(!started);

  d3d::get_render_target(prevRt);

  if (msaaPs3)
  {
    static int msaaId = ::get_shader_variable_id("msaa", true);
    ShaderGlobal::set_int(msaaId, 1);

    int wd, ht, l = 0, t = 0;
    d3d::get_target_size(wd, ht);
    static int target_sizeId = ::get_shader_variable_id("target_size", true);

    distortionFxRenderer->getMat()->set_color4_param(target_sizeId,
      Color4(float(l) / wd + (0.1f / wd), float(t) / ht + (0.1f / ht), float(1), float(1)));

    d3d::set_render_target(destTex, 0);

    ShaderGlobal::set_texture(sourceTexVarId, sourceTexId);
    ShaderGlobal::set_texture(distortionOffsetTexVarId, distortionOffsetTexId);

    distortionFxRenderer->render();

    ShaderGlobal::set_texture(sourceTexVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(distortionOffsetTexVarId, BAD_TEXTUREID);

    ShaderGlobal::set_int(msaaId, 0);

    d3d::set_render_target(prevRt);
    return;
  }

  d3d::set_render_target(distortionTargetTex, 0);

  ShaderGlobal::set_texture(sourceTexVarId, sourceTexId);
  ShaderGlobal::set_texture(distortionOffsetTexVarId, distortionOffsetTexId);

  distortionFxRenderer->render();

  ShaderGlobal::set_texture(sourceTexVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(distortionOffsetTexVarId, BAD_TEXTUREID);

  d3d::set_render_target(prevRt);
  d3d::stretch_rect(distortionTargetTex, destTex);
}
