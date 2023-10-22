#include <render/heatHazeRenderer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_tex3d.h>
#include <math/dag_color.h>
#include <gameRes/dag_gameResSystem.h>
#include <perfMon/dag_statDrv.h>

static int fx_render_modeVarId = -1;
static int haze_noise_texVarId = -1;
static int haze_paramsVarId = -1;
static int haze_target_widthVarId = -1;
static int haze_target_heightVarId = -1;
static int global_frame_block_id = -1;
static int texsz_consts_id = -1;
static int apply_haze_passVarId = -1;
static int source_texVarId = -1;
static int haze_source_tex_uv_transformVarId = -1;
static int haze_scene_depth_texVarId = -1;
static int haze_scene_depth_tex_lodVarId = -1;
static int rendering_distortion_colorVarId = -1;
static int inv_distortion_resolutionVarId = -1;

static inline Color4 calc_texsz_consts(int w, int h)
{
  return Color4(0.5f, -0.5f, 0.5f + HALF_TEXEL_OFSF / w, 0.5f + HALF_TEXEL_OFSF / h);
}

HeatHazeRenderer::HeatHazeRenderer(int haze_resolution_divisor) : hazeNoiseTexId(BAD_TEXTUREID) { setUp(haze_resolution_divisor); }

HeatHazeRenderer::~HeatHazeRenderer() { tearDown(); }

void HeatHazeRenderer::setUp(int haze_resolution_divisor)
{
  G_ASSERT(haze_resolution_divisor > 0);
  hazeResolutionDivisor = haze_resolution_divisor;

  haze_paramsVarId = ::get_shader_variable_id("haze_params", true);
  haze_target_widthVarId = ::get_shader_variable_id("haze_target_width", true);
  haze_target_heightVarId = ::get_shader_variable_id("haze_target_height", true);
  haze_noise_texVarId = ::get_shader_variable_id("haze_noise_tex", true);
  fx_render_modeVarId = ::get_shader_variable_id("fx_render_mode", true);
  texsz_consts_id = ::get_shader_variable_id("texsz_consts", true);
  apply_haze_passVarId = ::get_shader_variable_id("apply_haze_pass", true);
  source_texVarId = ::get_shader_variable_id("source_tex", true);
  haze_source_tex_uv_transformVarId = ::get_shader_variable_id("haze_source_tex_uv_transform", true);
  haze_scene_depth_texVarId = ::get_shader_variable_id("haze_scene_depth_tex", true);
  haze_scene_depth_tex_lodVarId = ::get_shader_variable_id("haze_scene_depth_tex_lod", true);
  rendering_distortion_colorVarId = ::get_shader_variable_id("rendering_distortion_color", true);
  inv_distortion_resolutionVarId = ::get_shader_variable_id("inv_distortion_resolution", true);

  if (!VariableMap::isGlobVariablePresent(fx_render_modeVarId))
    fx_render_modeVarId = -1;

  global_frame_block_id = ShaderGlobal::getBlockId("global_frame");

  int haze_render_mode_id_id = ::get_shader_variable_id("haze_render_mode_id", true);
  hazeFxId = ShaderGlobal::get_int_fast(haze_render_mode_id_id);

  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_FUNC);
  state.zFunc = CMPF_ALWAYS;
  zFuncAlwaysStateId = shaders::overrides::create(state);

  state = shaders::OverrideState();
  state.set(shaders::OverrideState::Z_TEST_DISABLE);
  state.set(shaders::OverrideState::Z_WRITE_DISABLE);
  state.set(shaders::OverrideState::BLEND_OP);
  state.blendOp = BLENDOP_MIN;
  zDisabledBlendMinStateId = shaders::overrides::create(state);

  tearDown();

  const DataBlock *hazeBlk = ::dgs_get_game_params()->getBlockByNameEx("haze");
  hazeNoiseScale = Point2(hazeBlk->getReal("noiseScale", 0.135f), hazeBlk->getReal("distortionScale", 0.1f));
  hazeLuminanceScale = hazeBlk->getPoint2("luminanceScale", Point2(4.0f, 0.1f));

  d3d::GpuAutoLock gpuLock;

  hazeNoiseTexId = ::get_tex_gameres("haze_noise");
  G_ASSERT(hazeNoiseTexId != BAD_TEXTUREID);

  if (isHazeAppliedManual())
  {
    hazeFxRenderer = eastl::make_unique<PostFxRenderer>();
    hazeFxRenderer->init("apply_haze");
  }
}

void HeatHazeRenderer::tearDown()
{
  ShaderGlobal::set_texture(haze_noise_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(haze_scene_depth_texVarId, BAD_TEXTUREID);

  if (hazeNoiseTexId != BAD_TEXTUREID)
    ::release_managed_tex(hazeNoiseTexId);
  hazeNoiseTexId = BAD_TEXTUREID;
}

void HeatHazeRenderer::renderHazeParticles(Texture *haze_depth, Texture *haze_offset, TEXTUREID depth_tex_id, int depth_tex_lod,
  RenderHazeParticlesCallback render_haze_particles, RenderCustomHazeCallback render_custom_haze,
  RenderCustomHazeCallback render_ri_haze)
{
  if (!areShadersValid())
    return;

  TIME_D3D_PROFILE(renderHazeParticles);

  SCOPE_RENDER_TARGET;

  TextureInfo hazeOffsetInfo;
  haze_offset->getinfo(hazeOffsetInfo);
  d3d::set_render_target({haze_depth, 0, 0}, DepthAccess::RW, {{haze_offset, 0, 0}});

  ShaderGlobal::set_color4(inv_distortion_resolutionVarId, 1.0f / hazeOffsetInfo.w, 1.0f / hazeOffsetInfo.h);
  ShaderGlobal::set_int(rendering_distortion_colorVarId, 0);
  ShaderGlobal::set_texture(haze_scene_depth_texVarId, depth_tex_id);
  ShaderGlobal::set_real(haze_scene_depth_tex_lodVarId, depth_tex_lod);

  if (render_custom_haze)
    render_custom_haze();

  ShaderGlobal::setBlock(global_frame_block_id, ShaderGlobal::LAYER_FRAME);
  shaders::overrides::set(zFuncAlwaysStateId);

  if (render_ri_haze)
    render_ri_haze();

  if (fx_render_modeVarId >= 0)
    ShaderGlobal::set_int(fx_render_modeVarId, hazeFxId);
  render_haze_particles();
  if (fx_render_modeVarId >= 0)
    ShaderGlobal::set_int(fx_render_modeVarId, 0);
  ShaderGlobal::setBlock(global_frame_block_id, ShaderGlobal::LAYER_FRAME);
  shaders::overrides::reset();
}

void HeatHazeRenderer::renderColorHaze(Texture *haze_color, RenderHazeParticlesCallback render_haze_particles,
  RenderCustomHazeCallback render_custom_haze, RenderCustomHazeCallback render_ri_haze)
{
  SCOPE_RENDER_TARGET;

  d3d::set_render_target({nullptr, 0, 0}, DepthAccess::RW, {{haze_color, 0, 0}});

  ShaderGlobal::set_int(rendering_distortion_colorVarId, 1);

  if (render_custom_haze)
    render_custom_haze();

  ShaderGlobal::setBlock(global_frame_block_id, ShaderGlobal::LAYER_FRAME);
  shaders::overrides::set(zDisabledBlendMinStateId);

  if (render_ri_haze)
    render_ri_haze();

  if (fx_render_modeVarId >= 0)
    ShaderGlobal::set_int(fx_render_modeVarId, hazeFxId);
  render_haze_particles();
  if (fx_render_modeVarId >= 0)
    ShaderGlobal::set_int(fx_render_modeVarId, 0);
  ShaderGlobal::setBlock(global_frame_block_id, ShaderGlobal::LAYER_FRAME);
  shaders::overrides::reset();
}

void HeatHazeRenderer::applyHaze(double total_time, Texture *back_buffer, const RectInt *back_buffer_area, TEXTUREID back_buffer_id,
  TEXTUREID resolve_depth_tex_id, Texture *haze_temp, TEXTUREID haze_temp_id, const IPoint2 &back_buffer_resolution)
{
  TIME_D3D_PROFILE(apply_haze);

  ShaderGlobal::set_texture(haze_noise_texVarId, hazeNoiseTexId);
  ShaderGlobal::set_texture(haze_scene_depth_texVarId, resolve_depth_tex_id);
  ShaderGlobal::set_color4(haze_paramsVarId, Color4(sin(total_time), fmod(total_time * 6, 2048), hazeNoiseScale.x, hazeNoiseScale.y));
  ShaderGlobal::set_real(haze_target_widthVarId, back_buffer_resolution.x);
  ShaderGlobal::set_real(haze_target_heightVarId, back_buffer_resolution.y);

  if (hazeFxRenderer)
  {
    SCOPE_RENDER_TARGET;

    G_ASSERT(haze_temp && haze_temp_id != BAD_TEXTUREID);

    enum
    {
      APPLY_HAZE_DISTORT,
      APPLY_HAZE_DOWNSCALE
    };

    hazeFxRenderer->getMat()->set_color4_param(texsz_consts_id, calc_texsz_consts(back_buffer_resolution.x, back_buffer_resolution.y));

    Point4 uvt = Point4(1, 1, 0, 0);
    if (back_buffer_area)
      uvt = Point4(float(back_buffer_area->right - back_buffer_area->left) / back_buffer_resolution.x,
        float(back_buffer_area->bottom - back_buffer_area->top) / back_buffer_resolution.y,
        float(back_buffer_area->left) / back_buffer_resolution.x, float(back_buffer_area->top) / back_buffer_resolution.y);

    d3d::set_render_target(haze_temp, 0);
    back_buffer->texfilter(TEXFILTER_POINT);
    ShaderGlobal::set_texture(source_texVarId, back_buffer_id);
    ShaderGlobal::set_color4(haze_source_tex_uv_transformVarId, uvt);
    d3d::set_depth(nullptr, DepthAccess::RW);

    ShaderGlobal::set_color4(haze_paramsVarId, 0.5f / back_buffer_resolution.x, 0.5f / back_buffer_resolution.y, hazeLuminanceScale.x,
      hazeLuminanceScale.y);
    ShaderGlobal::set_int(apply_haze_passVarId, APPLY_HAZE_DOWNSCALE);
    hazeFxRenderer->render();

    back_buffer->texfilter(TEXFILTER_DEFAULT);
    d3d::set_render_target(back_buffer, 0);
    d3d::set_depth(NULL, DepthAccess::RW);
    if (back_buffer_area)
    {
      auto ba = back_buffer_area;
      d3d::setview(ba->left, ba->top, ba->right - ba->left, ba->bottom - ba->top, 0, 1);
      d3d::setscissor(ba->left, ba->top, ba->right - ba->left, ba->bottom - ba->top);
    }

    haze_temp->texaddr(TEXADDR_CLAMP);
    haze_temp->texfilter(TEXFILTER_SMOOTH);
    ShaderGlobal::set_texture(source_texVarId, haze_temp_id);

    ShaderGlobal::set_color4(haze_paramsVarId,
      Color4(sin(total_time), fmod(total_time * 6, 2048), hazeNoiseScale.x, hazeNoiseScale.y));
    ShaderGlobal::set_int(apply_haze_passVarId, APPLY_HAZE_DISTORT);
    hazeFxRenderer->render();
  }
}

void HeatHazeRenderer::render(double total_time, const RenderTargets &targets, const IPoint2 &back_buffer_resolution,
  int depth_tex_lod, RenderHazeParticlesCallback render_haze_particles, RenderCustomHazeCallback render_custom_haze,
  RenderCustomHazeCallback render_ri_haze)
{
  if (!areShadersValid())
    return;

  if (!hazeFxRenderer)
    clearTargets(targets.hazeColor, targets.hazeOffset, targets.hazeDepth);

  renderHazeParticles(targets.hazeDepth, targets.hazeOffset, targets.depthId, depth_tex_lod, render_haze_particles, render_custom_haze,
    render_ri_haze);

  d3d::resource_barrier({targets.hazeOffset, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  d3d::resource_barrier({targets.hazeDepth, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  // Render second pass. If there is support for colored haze, render haze color
  if (targets.hazeColor)
  {
    renderColorHaze(targets.hazeColor, render_haze_particles, render_custom_haze, render_ri_haze);
    d3d::resource_barrier({targets.hazeColor, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  applyHaze(total_time, targets.backBuffer, targets.backBufferArea, targets.backBufferId, targets.resolvedDepthId, targets.hazeTemp,
    targets.hazeTempId, back_buffer_resolution);

  if (hazeFxRenderer)
    clearTargets(targets.hazeColor, targets.hazeOffset, targets.hazeDepth);
}

void HeatHazeRenderer::clearTargets(Texture *haze_color, Texture *haze_offset, Texture *haze_depth)
{
  TIME_D3D_PROFILE(renderHazeClear);

  SCOPE_RENDER_TARGET;

  if (haze_color)
  {
    d3d::set_render_target(haze_color, 0);
    d3d::clearview(CLEAR_TARGET, E3DCOLOR_MAKE(0xFF, 0xFF, 0xFF, 0xFF), 0.f, 0);
    d3d::resource_barrier({haze_color, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  d3d::set_render_target(haze_offset, 0);
  d3d::set_depth(haze_depth, DepthAccess::RW);
  d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, E3DCOLOR_MAKE(0x00, 0x00, 0x00, 0x00), 0.f, 0);
  d3d::resource_barrier({haze_offset, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  d3d::resource_barrier({haze_depth, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}

bool HeatHazeRenderer::isHazeAppliedManual() const
{
  int haze_apply_is_manual_id = ::get_shader_variable_id("haze_apply_is_manual", true);
  return ShaderGlobal::get_int_fast(haze_apply_is_manual_id) != 0;
}

bool HeatHazeRenderer::areShadersValid()
{
  return VariableMap::isGlobVariablePresent(haze_paramsVarId) && VariableMap::isGlobVariablePresent(haze_target_widthVarId) &&
         VariableMap::isGlobVariablePresent(haze_target_heightVarId) && VariableMap::isGlobVariablePresent(haze_noise_texVarId) &&
         VariableMap::isGlobVariablePresent(haze_scene_depth_texVarId);
}
