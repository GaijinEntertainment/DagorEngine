// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "waterEffects.h"
#include "waterEffectsEvent.h"
#include <render/shipWakeFx.h>
#include <render/viewVecs.h>
#include <render/waterProjFx.h>
#include <render/foam/foamFx.h>
#include <render/resolution.h>
#include <render/rendererFeatures.h>
#include <render/renderEvent.h>
#include <main/water.h>
#include <main/level.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/updateStageRender.h>
#include <render/daBfg/ecs/frameGraphNode.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderBlock.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <render/fx/fx.h>
#include <math/dag_frustum.h>

static int texSizeVarId = -1;

static CONSOLE_FLOAT_VAL_MINMAX("render.waterfoam", minSpeed, 3.00f, 0, 1000);
static CONSOLE_FLOAT_VAL_MINMAX("render.waterfoam", maxSpeed, 12.00f, 0, 1000);
static CONSOLE_FLOAT_VAL_MINMAX("render.ship", waveScale, 5.00f, 0, 100);

extern uint32_t get_water_quality();
extern float get_mipmap_bias();

class WaterEffectsCTM final : public ecs::ComponentTypeManager
{
public:
  using component_type = WaterEffects;

  void create(void *d, ecs::EntityManager &, ecs::EntityId, const ecs::ComponentsMap &, ecs::component_index_t) override
  {
    int w, h;
    get_rendering_resolution(w, h);
    WaterEffects::Settings settings;
    settings.rtWidth = w;
    settings.rtHeight = h;
    const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
    settings.foamFxOn = (get_water_quality() >= 2) && graphicsBlk->getBool("useFoamFx", false); // FoamFx from high water quality
    settings.waterQuality = get_water_quality();
    settings.lodBias = get_mipmap_bias();
    *(ecs::PtrComponentType<WaterEffects>::ptr_type(d)) = new WaterEffects(settings);
  }

  void destroy(void *d) override { delete *(ecs::PtrComponentType<WaterEffects>::ptr_type(d)); }
};

ECS_REGISTER_MANAGED_TYPE(WaterEffects, nullptr, WaterEffectsCTM);
ECS_AUTO_REGISTER_COMPONENT(WaterEffects, "water_effects", nullptr, 0);

template <typename Callable>
static void get_water_ecs_query(Callable c);
template <typename Callable>
static void get_foam_params_ecs_query(Callable c);
template <typename Callable>
static void get_water_effects_ecs_query(ecs::EntityId, Callable c);

static void destroy_water_effects_entity()
{
  g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("water_effects")));
}

FFTWater *get_water()
{
  FFTWater *waterPtr = nullptr;
  get_water_ecs_query([&waterPtr](FFTWater &water) { waterPtr = &water; });
  return waterPtr;
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, EventLevelLoaded, ChangeRenderFeatures)
ECS_REQUIRE(const FFTWater &water)
static void attempt_to_enable_water_effects_es(const ecs::Event &)
{
  if (!renderer_has_feature(FeatureRenderFlags::RIPPLES) || !renderer_has_feature(FeatureRenderFlags::WAKE))
  {
    destroy_water_effects_entity();
    return;
  }
  if (g_entity_mgr->getSingletonEntity(ECS_HASH("water_effects")))
    return;
  g_entity_mgr->createEntityAsync("water_effects");
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void set_up_foam_tex_request_es(const ecs::Event &, const WaterEffects &water_effects, bool &use_foam_tex)
{
  use_foam_tex = water_effects.isUsingFoamFx();
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(const FFTWater &water)
static void disable_water_effects_es(const ecs::Event &) { destroy_water_effects_entity(); }

ECS_TAG(render)
ECS_ON_EVENT(AfterDeviceReset)
static void reset_water_effects_es(const ecs::Event &, WaterEffects &water_effects) { water_effects.reset(); }

ECS_TAG(render)
static void update_water_effects_es(const UpdateStageInfoBeforeRender &evt, WaterEffects &water_effects, bool &should_use_wfx_textures)
{
  should_use_wfx_textures = water_effects.shouldUseWfxTextures();
  if (evt.dt <= 0.0f)
    return;
  get_water_ecs_query([&](FFTWater &water) {
    TIME_D3D_PROFILE(water_effects);
    water_effects.update(evt.viewTm, evt.viewItm, evt.projTm, evt.globTm, evt.dt, water);
  });
}

static FoamFxParams prepare_foam_params(float foamfx__tile_uv_scale,
  float foamfx__distortion_scale,
  float foamfx__normal_scale,
  float foamfx__pattern_gamma,
  float foamfx__mask_gamma,
  float foamfx__gradient_gamma,
  float foamfx__underfoam_threshold,
  float foamfx__overfoam_threshold,
  float foamfx__underfoam_weight,
  float foamfx__overfoam_weight,
  const Point3 &foamfx__underfoam_color,
  const Point3 &foamfx__overfoam_color,
  float foamfx__reflectivity,
  const ecs::string &foamfx__tile_tex,
  const ecs::string &foamfx__gradient_tex)
{
  Point3 scale(foamfx__tile_uv_scale, foamfx__distortion_scale, foamfx__normal_scale);
  Point3 gamma(foamfx__pattern_gamma, foamfx__mask_gamma, foamfx__gradient_gamma);
  Point2 threshold(foamfx__underfoam_threshold, foamfx__overfoam_threshold);
  Point2 weight(foamfx__underfoam_weight, foamfx__overfoam_weight);
  FoamFxParams waterFoamFxParams;
  waterFoamFxParams.scale = scale;
  waterFoamFxParams.gamma = gamma;
  waterFoamFxParams.threshold = threshold;
  waterFoamFxParams.weight = weight;
  waterFoamFxParams.underfoamColor = foamfx__underfoam_color;
  waterFoamFxParams.overfoamColor = foamfx__overfoam_color;
  waterFoamFxParams.reflectivity = foamfx__reflectivity;
  waterFoamFxParams.tileTex = String(foamfx__tile_tex.c_str());
  waterFoamFxParams.gradientTex = String(foamfx__gradient_tex.c_str());
  return waterFoamFxParams;
}

ECS_TAG(render)
static void set_up_foam_params_es(const SetResolutionEvent &evt, WaterEffects &water_effects)
{
  water_effects.setResolution(evt.renderingResolution.x, evt.renderingResolution.y, get_mipmap_bias(), get_water_quality());
}

ECS_TAG(render)
static void change_effects_resolution_es(const SetFxQuality &, WaterEffects &water_effects)
{
  water_effects.effectsResolutionChanged();
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void set_up_foam_params_es(const ecs::Event &, WaterEffects &water_effects)
{
  get_foam_params_ecs_query(
    [&water_effects](float foamfx__tile_uv_scale, float foamfx__distortion_scale, float foamfx__normal_scale,
      float foamfx__pattern_gamma, float foamfx__mask_gamma, float foamfx__gradient_gamma, float foamfx__underfoam_threshold,
      float foamfx__overfoam_threshold, float foamfx__underfoam_weight, float foamfx__overfoam_weight,
      const Point3 &foamfx__underfoam_color, const Point3 &foamfx__overfoam_color, float foamfx__reflectivity,
      const ecs::string &foamfx__tile_tex, const ecs::string &foamfx__gradient_tex) {
      FoamFxParams waterFoamFxParams = prepare_foam_params(foamfx__tile_uv_scale, foamfx__distortion_scale, foamfx__normal_scale,
        foamfx__pattern_gamma, foamfx__mask_gamma, foamfx__gradient_gamma, foamfx__underfoam_threshold, foamfx__overfoam_threshold,
        foamfx__underfoam_weight, foamfx__overfoam_weight, foamfx__underfoam_color, foamfx__overfoam_color, foamfx__reflectivity,
        foamfx__tile_tex, foamfx__gradient_tex);
      water_effects.setFoamFxParams(waterFoamFxParams);
    });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void set_up_water_effect_nodes_es(const ecs::Event &,
  WaterEffects &water_effects,
  dabfg::NodeHandle &water_effects__init_res_node,
  dabfg::NodeHandle &water_effects__node)
{
  water_effects__init_res_node = makeWaterEffectsPrepareNode();
  water_effects__node = makeWaterEffectsNode(water_effects);
}

ECS_ON_EVENT(on_appear)
ECS_TRACK(foamfx__tile_uv_scale,
  foamfx__distortion_scale,
  foamfx__normal_scale,
  foamfx__pattern_gamma,
  foamfx__mask_gamma,
  foamfx__gradient_gamma,
  foamfx__underfoam_threshold,
  foamfx__overfoam_threshold,
  foamfx__underfoam_weight,
  foamfx__overfoam_weight,
  foamfx__underfoam_color,
  foamfx__overfoam_color,
  foamfx__reflectivity,
  foamfx__tile_tex,
  foamfx__gradient_tex)
static void water_foamfx_es(const ecs::Event &,
  float foamfx__tile_uv_scale,
  float foamfx__distortion_scale,
  float foamfx__normal_scale,
  float foamfx__pattern_gamma,
  float foamfx__mask_gamma,
  float foamfx__gradient_gamma,
  float foamfx__underfoam_threshold,
  float foamfx__overfoam_threshold,
  float foamfx__underfoam_weight,
  float foamfx__overfoam_weight,
  const Point3 &foamfx__underfoam_color,
  const Point3 &foamfx__overfoam_color,
  float foamfx__reflectivity,
  const ecs::string &foamfx__tile_tex,
  const ecs::string &foamfx__gradient_tex)
{
  FoamFxParams waterFoamFxParams = prepare_foam_params(foamfx__tile_uv_scale, foamfx__distortion_scale, foamfx__normal_scale,
    foamfx__pattern_gamma, foamfx__mask_gamma, foamfx__gradient_gamma, foamfx__underfoam_threshold, foamfx__overfoam_threshold,
    foamfx__underfoam_weight, foamfx__overfoam_weight, foamfx__underfoam_color, foamfx__overfoam_color, foamfx__reflectivity,
    foamfx__tile_tex, foamfx__gradient_tex);
  get_water_effects_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("water_effects")),
    [&waterFoamFxParams](WaterEffects &water_effects) { water_effects.setFoamFxParams(waterFoamFxParams); });
}

static void unload_texture(TEXTUREID tex_id) { ShaderGlobal::reset_from_vars_and_release_managed_tex(tex_id); }

static void unload_textures(const ShipWakeFx::Settings &settings)
{
  unload_texture(settings.wakeTexId);
  unload_texture(settings.wakeTrailTexId);
  unload_texture(settings.foamTexId);
  unload_texture(settings.foamHeadTexId);
  unload_texture(settings.foamTurboTexId);
  unload_texture(settings.foamTrailTexId);
  unload_texture(settings.foamDistortedTileTexId);
  unload_texture(settings.foamDistortedParticleTexId);
  unload_texture(settings.foamDistortedGradientTexId);
}

static bool check_textures_loaded(const ShipWakeFx::Settings &settings)
{
  TEXTUREID textures[] = {settings.wakeTexId, settings.wakeTrailTexId, settings.foamTexId, settings.foamHeadTexId,
    settings.foamTurboTexId, settings.foamTrailTexId, settings.foamDistortedTileTexId, settings.foamDistortedParticleTexId,
    settings.foamDistortedGradientTexId};
  return prefetch_and_check_managed_textures_loaded(make_span_const(textures, countof(textures)));
}

struct WwaterProjFxRenderHelper : public IWwaterProjFxRenderHelper
{
  WwaterProjFxRenderHelper(WaterEffects *water_effects) : waterEffects(water_effects) {}

  bool render_geometry()
  {
    if (waterEffects)
    {
      TIME_D3D_PROFILE(water_effects_foam);
      waterEffects->renderFoam();
      waterEffects->renderInternalWaterProjFx();
    }
    return true;
  }

  WaterEffects *waterEffects;
};

WaterEffects::WaterEffects(const Settings &in_settings) : settings(in_settings)
{
  texSizeVarId = ::get_shader_variable_id("water_foam_obstacle_tex_size");
  effects_depth_texVarId = ::get_shader_variable_id("effects_depth_tex");
  wfx_normal_pixel_sizeVarId = ::get_shader_variable_id("wfx_normals_pixel_size");
  wfx_hmapVarId = ::get_shader_variable_id("wfx_hmap");
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    ShaderGlobal::set_sampler(get_shader_variable_id("wfx_hmap_samplerstate"), d3d::request_sampler(smpInfo));
  }

  if (settings.foamFxOn)
    createFoamFx(settings.rtWidth, settings.rtHeight, settings.waterQuality);

  if (settings.unitWakeOn)
  {
    ShipWakeFx::Settings wakeSettings;
    wakeSettings.reduceFoam = true;
    wakeSettings.wakeTexId = loadTexture("wake_hmap");
    wakeSettings.wakeTrailTexId = loadTexture("wake_trail_hmap");
    wakeSettings.foamTexId = loadTexture("foam_main");
    wakeSettings.foamHeadTexId = loadTexture("foam_head");
    wakeSettings.foamTurboTexId = loadTexture("foam_turbo");
    wakeSettings.foamTrailTexId = loadTexture("foam_trail");
    wakeSettings.foamDistortedTileTexId = loadTexture("foam_generator_tile_tex_n");
    wakeSettings.foamDistortedParticleTexId = loadTexture("foam_generator_particle_tex_n");
    wakeSettings.foamDistortedGradientTexId = loadTexture("foam_generator_gradient_tex_n");
    if (wakeSettings.wakeTexId != BAD_TEXTUREID && wakeSettings.wakeTrailTexId != BAD_TEXTUREID &&
        wakeSettings.foamTexId != BAD_TEXTUREID && wakeSettings.foamHeadTexId != BAD_TEXTUREID &&
        wakeSettings.foamTurboTexId != BAD_TEXTUREID && wakeSettings.foamTrailTexId != BAD_TEXTUREID)
    {
      shipWakeFx = eastl::make_unique<ShipWakeFx>(wakeSettings);
      createWaterProjectedFx(settings.rtWidth, settings.rtHeight);
    }
    else
      unload_textures(wakeSettings);
  }

  normalsRender.init("wfx_normals");

  shaders::OverrideState state;
  state.set(shaders::OverrideState::FLIP_CULL);
  flipCullStateId = shaders::overrides::create(state);
}

float WaterEffects::getWaterProjectedFxLodBias() const
{
  bool validParameters = projFxResolution.x > 0 && projFxResolution.y > 0 && settings.rtWidth > 0 && settings.rtHeight > 0;

  G_ASSERTF_ONCE(validParameters, "water_effects: Invalid projFxResolution or settings.rtWidth/rtHeight.");
  if (!validParameters)
    return 0.f;

  return max(log2f((float)projFxResolution.y / settings.rtHeight) + settings.lodBias, 0.f);
}

void WaterEffects::renderInternalWaterProjFx()
{
  TMatrix4 view, proj;
  Point3 pos;
  if (waterProjectedFx && waterProjectedFx->getView(view, proj, pos))
  {
    STATE_GUARD(ShaderGlobal::set_texture(effects_depth_texVarId, VALUE), BAD_TEXTUREID,
      ShaderGlobal::get_tex(effects_depth_texVarId));

    acesfx::renderTransWaterProj(view, proj, pos, getWaterProjectedFxLodBias());
  }
}

WaterEffects::~WaterEffects()
{
  if (shipWakeFx)
    unload_textures(shipWakeFx->getSettings());

  normalsRender.clear();
}

void WaterEffects::update(
  const TMatrix &view_tm, const TMatrix &view_itm, const TMatrix4 &proj_tm, const TMatrix4 &glob_tm, float dt, FFTWater &water)
{
  if (!shipWakeFx)
    return;

  float waterLevel = fft_water::get_height(&water, ::grs_cur_view.pos);
  waterProjectedFx->prepare(view_tm, view_itm, proj_tm, glob_tm, waterLevel, fft_water::get_significant_wave_height(&water),
    dagor_frame_no(), false);

  eastl::vector_map<ecs::EntityId, ShipEffectContext> processedShips;
  processedShips.reserve(knownShips.size());

  GatherEmittersEventCtx evtCtx{water, processedShips, knownShips, *shipWakeFx.get(), minSpeed, maxSpeed, waveScale};
  g_entity_mgr->broadcastEventImmediate(GatherEmittersForWaterEffects(&evtCtx));

  for (auto &iter : knownShips)
    if (processedShips.find(iter.first) == processedShips.end())
      shipWakeFx->removeShip(iter.second.wakeId);

  knownShips.swap(processedShips);

  shipWakeFx->update(dt);
}

bool WaterEffects::shouldUseWfxTextures() const
{
  const bool isShipWakeFxActive = shipWakeFx && shipWakeFx->hasWork() && check_textures_loaded(shipWakeFx->getSettings());
  const bool usingFoamFx = isUsingFoamFx();
  return isShipWakeFxActive || usingFoamFx;
}

bool WaterEffects::isUsingFoamFx() const { return (bool)foamFx; }

void WaterEffects::render(FFTWater &water,
  const TMatrix &view_tm,
  const TMatrix4 &proj_tm,
  const Driver3dPerspective &perps,
  const ManagedTex &heightMaskTex,
  const ManagedTex &normalsTex,
  bool is_under_water)
{
  TIME_D3D_PROFILE(WaterEffects_Render);

  TMatrix4 viewTm = view_tm;
  TMatrix4 projTm = proj_tm;

  if (waterProjectedFx)
  {
    // we use a special projection for the water plane calculated in WaterProjectedFx::prepare
    waterProjectedFx->setView();
    Point3 cameraPos;
    waterProjectedFx->getView(viewTm, projTm, cameraPos);
  }

  if (shouldUseWfxTextures())
  {
    const bool isShipWakeFxActive = shipWakeFx && shipWakeFx->hasWork() && check_textures_loaded(shipWakeFx->getSettings());
    const bool usingFoamFx = isUsingFoamFx();

    int oldBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

    if (usingFoamFx)
    {
      foamFx->beginMaskRender();
      shaders::overrides::reset();
      if (is_under_water)
        shaders::overrides::set(flipCullStateId);
      fft_water::render(&water, inverse(view_tm).getcol(3), BAD_TEXTUREID, Frustum(TMatrix4(view_tm) * TMatrix4(proj_tm)), perps,
        fft_water::GEOM_HIGH, -1, nullptr, fft_water::RenderMode::WATER_DEPTH_SHADER);
      if (is_under_water)
        shaders::overrides::reset();
      if (isShipWakeFxActive)
        shipWakeFx->renderFoamMask();
      foamFx->endMaskRender();

      foamFx->prepare(viewTm, projTm);
    }

    d3d::set_render_target({}, DepthAccess::RW, {{heightMaskTex.getTex2D(), 0}});
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);

    if (usingFoamFx)
    {
      foamFx->renderHeight();
    }
    if (isShipWakeFxActive)
    {
      shipWakeFx->render();
    }
    d3d::resource_barrier({heightMaskTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    ShaderGlobal::set_texture(wfx_hmapVarId, heightMaskTex);

    TextureInfo normalTexInfo;
    normalsTex.getTex2D()->getinfo(normalTexInfo);
    ShaderGlobal::set_color4(wfx_normal_pixel_sizeVarId, 1.0f / normalTexInfo.w, 1.0f / normalTexInfo.h, 0.0f, 0.0f);

    d3d::set_render_target({}, DepthAccess::RW, {{normalsTex.getTex2D(), 0}});
    d3d::clearview(CLEAR_DISCARD, 0, 0, 0);

    d3d::set_render_target(normalsTex.getTex2D(), 0);
    set_viewvecs_to_shader(view_tm, proj_tm);
    normalsRender.render();

    ShaderGlobal::setBlock(oldBlock, ShaderGlobal::LAYER_FRAME);
  }
}

void WaterEffects::renderFoam()
{
  if (isUsingFoamFx())
    foamFx->renderFoam();
  else if (shipWakeFx && shipWakeFx->hasWork() && check_textures_loaded(shipWakeFx->getSettings()))
    shipWakeFx->renderFoam();
}

void WaterEffects::reset()
{
  if (shipWakeFx)
    shipWakeFx->reset();
}

TEXTUREID WaterEffects::loadTexture(const char *name)
{
  TEXTUREID texId = BAD_TEXTUREID;

  if (texId == BAD_TEXTUREID)
  {
    texId = ::get_tex_gameres(name);
    if (texId != BAD_TEXTUREID)
      prefetch_managed_texture(texId);
  }

  return texId;
}

IPoint2 WaterEffects::calcWaterProjectedFxResolution(IPoint2 target_res) const
{
  FxResolutionSetting fxRes = acesfx::getFxResolutionSetting();
  float texDiv = M_SQRT2; // sqrtf(2.f)
  if (fxRes == FX_MEDIUM_RESOLUTION)
    texDiv = 1.f;
  else if (fxRes == FX_HIGH_RESOLUTION)
    texDiv = M_SQRT1_2; // sqrtf(2.f) / 2.f
  return IPoint2(target_res.x / texDiv, target_res.y / texDiv);
}

void WaterEffects::createWaterProjectedFx(int targetWidth, int targetHeight)
{
  if (waterProjectedFx)
    return;

  projFxResolution = calcWaterProjectedFxResolution({targetWidth, targetHeight});

  uint32_t diffuseTexFormat = 0;
  // some Adreno drivers fail to properly load src color
  // at blending while using indirect draw in some conditions
  // when format is ARGB8U, use ARGB16F as workaround
  if (d3d::get_driver_desc().issues.hasRenderPassClearDataRace)
    diffuseTexFormat = TEXFMT_A16B16G16R16F;
  else
    diffuseTexFormat = TEXFMT_A8R8G8B8;

  WaterProjectedFx::TargetDesc targetDesc = {diffuseTexFormat, SimpleString("projected_on_water_effects_tex"), 0xFF000000};
  waterProjectedFx = eastl::make_unique<WaterProjectedFx>(projFxResolution.x, projFxResolution.y,
    dag::Span<WaterProjectedFx::TargetDesc>(&targetDesc, 1), nullptr);
}

void WaterEffects::clearWaterProjectedFx()
{
  waterProjectedFx.reset();
  projFxResolution = IPoint2(0, 0);
}

void WaterEffects::updateWaterProjectedFx()
{
  TIME_D3D_PROFILE(WaterEffects_UpdateWaterProjectedFx);

  if (waterProjectedFx)
  {
    WwaterProjFxRenderHelper render_helper(this);
    waterProjectedFx->render(&render_helper);
  }
}

void WaterEffects::useWaterTextures()
{
  if (waterProjectedFx)
    waterProjectedFx->setTextures();
}

void WaterEffects::setResolution(uint32_t rtWidth, uint32_t rtHeight, float lodBias, int water_quality)
{
  settings.rtWidth = rtWidth;
  settings.rtHeight = rtHeight;
  settings.lodBias = lodBias;

  if (waterProjectedFx)
  {
    clearWaterProjectedFx();
    createWaterProjectedFx(settings.rtWidth, settings.rtHeight);
  }

  if (foamFx)
  {
    clearFoamFx();
    createFoamFx(settings.rtWidth, settings.rtHeight, water_quality);
  }
}

void WaterEffects::setFoamFxParams(const FoamFxParams &params)
{
  if (foamFx)
    foamFx->setParams(params);
}

void WaterEffects::createFoamFx(int target_width, int target_height, int water_quality)
{
  if (foamFx)
    return;

  int targetDiv = 2;
  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
  const DataBlock *divBlk = graphicsBlk->getBlockByNameEx("foamFxTexDividers");
  if (divBlk && water_quality >= 0 && water_quality < divBlk->paramCount())
    targetDiv = divBlk->getInt(water_quality);

  int foamFxWidth = target_width / targetDiv;
  int foamFxHeight = target_height / targetDiv;

  foamFx = eastl::make_unique<FoamFx>(foamFxWidth, foamFxHeight);
}

void WaterEffects::effectsResolutionChanged()
{
  if (waterProjectedFx)
  {
    clearWaterProjectedFx();
    createWaterProjectedFx(settings.rtWidth, settings.rtHeight);
  }
}

void WaterEffects::clearFoamFx() { foamFx.reset(); }

bool test_box_is_in_water(FFTWater &water, const TMatrix &transform, const BBox3 &box)
{
  bbox3f boxV = v_ldu_bbox3(box);
  mat44f transformV;
  v_mat44_make_from_43cu_unsafe(transformV, transform[0]);
  bbox3f worldBoxV;
  v_bbox3_init(worldBoxV, transformV, boxV);
  Point3_vec4 worldCenter;
  v_st(&worldCenter.x, v_bbox3_center(worldBoxV));

  if (v_extract_y(worldBoxV.bmin) > fft_water::get_max_level(&water) + fft_water::get_max_wave(&water)) // check if it is too high
    return false;

  float waterHeight = fft_water::get_height(&water, worldCenter);
  // test if waterHeight is inside worldBox
  return !v_test_vec_x_eq_0(
    v_splat_y(v_and(v_cmp_ge(v_splats(waterHeight), worldBoxV.bmin), v_cmp_ge(worldBoxV.bmax, v_splats(waterHeight)))));
}
