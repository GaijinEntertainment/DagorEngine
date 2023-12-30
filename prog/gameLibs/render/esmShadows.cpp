#include <render/esmShadows.h>

#include <perfMon/dag_statDrv.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_tex3d.h>
#include <shaders/dag_shaders.h>

static int gauss_directionVarId = -1;
static int esm_sliceVarId = -1;
static int esm_blur_srcVarId = -1;

void EsmShadows::init(int w, int h, int slices, float esm_exp)
{
  int fmt = TEXFMT_G16R16F | TEXCF_RTARGET;
  esmShadowArray = dag::create_array_tex(w, h, slices, fmt, 1, "esm_shadows");
  esmShadowArray->texbordercolor(1);
  esmShadowArray->texaddr(TEXADDR_CLAMP);
  esmShadowBlurTmp = dag::create_tex(NULL, w, h, fmt, 1, "esm_temp");
  esmShadowBlurTmp->texaddr(TEXADDR_CLAMP);
  esmBlurRenderer.init("esm_blur");

  float esmKExp = pow(2.0f, esm_exp);

  gauss_directionVarId = get_shader_variable_id("gauss_direction");
  esm_sliceVarId = get_shader_variable_id("esm_slice");
  esm_blur_srcVarId = get_shader_variable_id("esm_blur_src");
  ShaderGlobal::set_color4(get_shader_variable_id("esm_params"), w, h, esm_exp, esmKExp);

  initEsmShadowsStateId();
  esmDepthShader.init("esm_depth", nullptr, 0, "esm_depth", false);
}

void EsmShadows::close()
{
  esmShadowArray.close();
  esmShadowBlurTmp.close();
}

void EsmShadows::beginRenderSlice(int slice_id)
{
  d3d::set_render_target(0, esmShadowArray.getArrayTex(), slice_id, 0);
  d3d::clearview(CLEAR_TARGET, E3DCOLOR(255, 255, 255), 0.0f, 0);
  shaders::overrides::set(esmShadowsStateId);
  currentSlice = slice_id;
}

void EsmShadows::endRenderSlice()
{
  shaders::overrides::reset();
  blur(currentSlice);
  currentSlice = -1;
}

void EsmShadows::blur(int slice)
{
  TIME_D3D_PROFILE(esm_blur);

  ShaderGlobal::set_int(esm_sliceVarId, slice);

  ShaderGlobal::set_int(gauss_directionVarId, 0);
  ShaderGlobal::set_texture(esm_blur_srcVarId, esmShadowArray);
  d3d::resource_barrier({esmShadowArray.getArrayTex(), RB_RO_SRV | RB_STAGE_PIXEL, (unsigned)slice, 1});
  d3d::set_render_target(esmShadowBlurTmp.getTex2D(), 0);
  esmBlurRenderer.render();

  ShaderGlobal::set_int(gauss_directionVarId, 1);
  ShaderGlobal::set_texture(esm_blur_srcVarId, esmShadowBlurTmp);
  d3d::resource_barrier({esmShadowBlurTmp.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  d3d::set_render_target(0, esmShadowArray.getArrayTex(), slice, 0);
  esmBlurRenderer.render();

  d3d::resource_barrier({esmShadowArray.getArrayTex(), RB_RO_SRV | RB_STAGE_PIXEL, (unsigned)slice, 1});
}

void EsmShadows::initEsmShadowsStateId()
{
  shaders::OverrideState state;
  state.set(shaders::OverrideState::CULL_NONE); // We render max and min depth simultaneously from one view direction,
                                                // it requires face culling to be disabled.
  state.set(shaders::OverrideState::Z_CLAMP_ENABLED);
  state.set(shaders::OverrideState::BLEND_OP);
  state.set(shaders::OverrideState::BLEND_SRC_DEST);
  state.blendOp = BLENDOP_MIN;
  state.sblend = BLEND_ONE;
  state.dblend = BLEND_ONE;
  esmShadowsStateId = shaders::overrides::create(state);
}