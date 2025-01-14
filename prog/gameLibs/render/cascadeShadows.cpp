// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/cascadeShadows.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_render.h>
#include <shaders/dag_shaderBlock.h>
#include <stdio.h>
#include <math/dag_TMatrix4more.h>
#include <math/dag_vecMathCompatibility.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_span.h>
#include <util/dag_simpleString.h>
#include <generic/dag_tab.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IBBox2.h>
#include <generic/dag_carray.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_TMatrix4.h>
#include <render/dynmodelRenderer.h>
#include <shaders/dag_overrideStates.h>
#include "EASTL/unique_ptr.h"
#include <3d/dag_resPtr.h>
#include <util/dag_convar.h>


CONSOLE_FLOAT_VAL_MINMAX("shadow", csm_cascade_z_expansion_znear_offset, 0.0f, 0.0f, 1000.0f);


float shadow_render_expand_mul = 1.0f; // up to 1.f for full shadow length expansion if any occur.
float shadow_render_expand_to_sun_mul = 0.0f;
float shadow_render_expand_from_sun_mul = 0.0f;
#define SHADOW_CULLING_POS_EXPAND_MUL 0.0 // No artifacts without expansions, multipliers can be increased
// if we support different culling from rendering matrix - shadow_render_expand_mul should be 0
#define SHADOW_FAR_CASCADE_DEPTH_MUL \
  2.f // Trade more shadow distance for less depth quality. Is roughly a projection of the last cascade width to virtual ground in
      // light space.

#define SHADOW_SAMPLING_MAX_OFFSET \
  2.5f // 2 texels from FXAA, 0.5 texels from PCF. Should be multiplied by sqrt(2) for the worst case, but looks good even multiplied
       // by shadowDepthSlopeBias.

#define USE_SHADOW_DEPTH_CLAMP 1

static int csmShadowFadeOutVarId = -1, shadowCascadeDepthRangeVarId = -1, deferredShadowPassVarId = -1;

#define GLOBAL_VARS_OPTIONAL_LIST   \
  VAR(csm_world_view_pos)           \
  VAR(csm_distance)                 \
  VAR(num_of_cascades)              \
  VAR(csm_range_cascade_0)          \
  VAR(csm_range_cascade_1)          \
  VAR(csm_range_cascade_2)          \
  VAR(csm_meter_to_uv_cascade_0)    \
  VAR(csm_meter_to_uv_cascade_1)    \
  VAR(csm_meter_to_uv_cascade_2)    \
  VAR(csm_uv_minmax_cascade_0)      \
  VAR(csm_uv_minmax_cascade_1)      \
  VAR(csm_uv_minmax_cascade_2)      \
  VAR(shadow_cascade_data)          \
  VAR(shadow_cascade_tm_transp)     \
  VAR(shadow_cascade_tc_mul_offset) \
  VAR(csm_culling_near_planes)

#define GLOBAL_CONST_LIST

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_OPTIONAL_LIST
#undef VAR

#define VAR(a) static int a##_const_no = -1;
GLOBAL_CONST_LIST
#undef VAR

class CascadeShadowsPrivate
{
public:
  String resPostfix;
  CascadeShadowsPrivate(ICascadeShadowsClient *in_client, const CascadeShadows::Settings &in_settings, String &&res_postfix);

  ~CascadeShadowsPrivate();

  void prepareShadowCascades(const CascadeShadows::ModeSettings &mode_settings, const Point3 &dir_to_sun, const TMatrix &view_matrix,
    const Point3 &camera_pos, const TMatrix4 &proj_tm, const Frustum &view_frustum, const Point2 &scene_z_near_far,
    float z_near_for_cascade_distribution);

  const CascadeShadows::Settings &getSettings() const { return settings; }
  void setDepthBiasSettings(const CascadeShadows::Settings &set)
  {
    settings.shadowDepthBias = set.shadowDepthBias;
    settings.shadowConstDepthBias = set.shadowConstDepthBias;
    settings.shadowDepthSlopeBias = set.shadowDepthSlopeBias;
    settings.zRangeToDepthBiasScale = set.zRangeToDepthBiasScale;
  }
  void setCascadeWidth(int width)
  {
    if (settings.cascadeWidth != width)
    {
      settings.cascadeWidth = width;
      createDepthShadow(settings.splitsW, settings.splitsH, settings.cascadeWidth, settings.cascadeWidth,
        settings.cascadeDepthHighPrecision);
    }
  }
  void renderShadowsCascades();

  void renderShadowsCascadesCb(const csm_render_cascades_cb_t &render_cascades_cb, ManagedTexView external_cascades = {});
  void renderShadowCascadeDepth(int cascadeNo, bool clearPerView, ManagedTexView external_cascades = {});
  void calcTMs();
  carray<Color4, CascadeShadows::MAX_CASCADES * 4> getTransposedCSMTm();
  void setSamplingBiasToShader(float value);
  void setCascadesToShader();
  void setNearCullingNearPlaneToShader();
  void disable() { numCascadesToRender = 0; }
  bool isEnabled() const { return numCascadesToRender != 0; }
  void uploadCSMBuffer(const Color4 *tm, const Color4 *off);
  void invalidate()
  {
    for (unsigned int cascadeNo = 0; cascadeNo < shadowSplits.size(); cascadeNo++)
      shadowSplits[cascadeNo].frames = 0xFFFF;
  }

  int getNumCascadesToRender() const { return numCascadesToRender; }
  const Point2 &getZnZf(int cascade_no) const { return shadowSplits[cascade_no].znzf; }
  const Frustum &getFrustum(int cascade_no) const { return shadowSplits[cascade_no].frustum; }
  const Point3 &getRenderCameraWorldViewPos(int cascade_no) const { return shadowSplits[cascade_no].viewPos; }
  const TMatrix &getShadowViewItm(int cascade_no) const { return shadowSplits[cascade_no].shadowViewItm; }
  const TMatrix4_vec4 &getCameraRenderMatrix(int cascade_no) const { return shadowSplits[cascade_no].cameraRenderMatrix; }
  const TMatrix4_vec4 &getWorldCullingMatrix(int cascade_no) const { return shadowSplits[cascade_no].worldCullingMatrix; }
  const TMatrix4_vec4 &getWorldRenderMatrix(int cascade_no) const { return shadowSplits[cascade_no].worldRenderMatrix; }

  const TMatrix4_vec4 &getRenderViewMatrix(int cascade_no) const { return shadowSplits[cascade_no].renderViewMatrix; }
  const TMatrix4_vec4 &getRenderProjMatrix(int cascade_no) const { return shadowSplits[cascade_no].renderProjMatrix; }
  const Point3 &shadowWidth(int cascade_no) const { return shadowSplits[cascade_no].shadowWidth; }

  const BBox3 &getWorldBox(int cascade_no) const { return shadowSplits[cascade_no].worldBox; }
  bool shouldUpdateCascade(int cascade_no) const { return shadowSplits[cascade_no].shouldUpdate; }
  bool isCascadeValid(int cascade_no) const { return shadowSplits[cascade_no].to > shadowSplits[cascade_no].from; }

  void copyFromSparsed(int cascade_no)
  {
    shadowSplits[cascade_no] = sparsedShadowSplits[cascade_no];
    shadowSplits[cascade_no].frustum.construct(shadowSplits[cascade_no].worldCullingMatrix);
  }

  float getMaxDistance() const { return modeSettings.maxDist; }

  float getMaxShadowDistance() const { return csmDistance; }

  float getCascadeDistance(int cascade_no) const { return shadowSplits[cascade_no].to; }

  const Frustum &getWholeCoveredFrustum() const { return wholeCoveredSpaceFrustum; }

  const String &setShadowCascadeDistanceDbg(const Point2 &scene_z_near_far, int tex_size, int splits_w, int splits_h,
    float shadow_distance, float pow_weight);

  void debugSetParams(float shadow_depth_bias, float shadow_const_depth_bias, float shadow_depth_slope_bias);

  void debugGetParams(float &shadow_depth_bias, float &shadow_const_depth_bias, float &shadow_depth_slope_bias);


  BaseTexture *getShadowsCascade() const { return internalCascades.getTex2D(); }
  d3d::SamplerHandle getShadowsCascadeSampler() const { return internalCascadesSampler; }
  const TextureInfo &getShadowCascadeTexInfo() const { return shadowCascadesTexInfo; }

  void setNeedSsss(bool need_ssss) { needSsss = need_ssss; }

private:
  struct ShadowSplit
  {
    ShadowSplit() : from(0), to(1), frames(0xFFFF), shouldUpdate(1) {}
    float from, to;
    Point2 znzf;
    Point3 shadowWidth;
    Point3 viewPos; // for shadowFromCamldViewProjMatrix
    TMatrix shadowViewItm;
    TMatrix4_vec4 cameraCullingMatrix; // made with camViewTm
    TMatrix4_vec4 cameraRenderMatrix;
    TMatrix4_vec4 worldCullingMatrix;
    TMatrix4_vec4 worldRenderMatrix;
    TMatrix4_vec4 renderViewMatrix;
    TMatrix4_vec4 renderProjMatrix;
    Frustum frustum;
    BBox3 worldBox;
    IBBox2 viewport;
    uint16_t frames; // how many frames it was not updated
    uint16_t shouldUpdate;
  };

  ICascadeShadowsClient *client;
  CascadeShadows::Settings settings;
  CascadeShadows::ModeSettings modeSettings;
  bool dbgModeSettings;
  Frustum wholeCoveredSpaceFrustum;


  UniqueTexHolder internalCascades;
  TextureInfo shadowCascadesTexInfo;
  d3d::SamplerHandle internalCascadesSampler = d3d::INVALID_SAMPLER_HANDLE;

  UniqueTexHolder shadowCascadesFakeRT;
  d3d::RenderPass *mobileAreaUpdateRP;

  int numCascadesToRender = 0;
  carray<ShadowSplit, CascadeShadows::MAX_CASCADES> shadowSplits;
  carray<ShadowSplit, CascadeShadows::MAX_CASCADES> sparsedShadowSplits;
  carray<Color4, CascadeShadows::MAX_CASCADES * 3> shadowCascadeTm;
  carray<Color4, CascadeShadows::MAX_CASCADES> shadowCascadeTcMulOffset;

  float csmDistance = 0.0f;
  carray<shaders::UniqueOverrideStateId, CascadeShadows::MAX_CASCADES> cascadeOverride;
  void destroyOverrides();
  void createOverrides();

  void createMobileRP(uint32_t depth_fmt, uint32_t rt_fmt);
  void createDepthShadow(int splits_w, int splits_h, int width, int height, bool high_precision_depth);
  void closeDepthShadow();
  IBBox2 getViewPort(int cascade, const IPoint2 &tex_width) const;
  TMatrix4 getShadowViewMatrix(const Point3 &dir_to_sun, const Point3 &camera_pos, bool world_space);
  void setFadeOutToShaders(float max_dist);

  void buildShadowProjectionMatrix(const Point3 &dir_to_sun, const TMatrix &view_matrix, const Point3 &camera_pos,
    const TMatrix4 &projtm, float z_near, float z_far, float next_z_far, const Point3 &anchor, float anchor_radius,
    ShadowSplit &split);

  UniqueBuf csmBuffer;
  bool needSsss = false;
  int maxCascadesPossible = CascadeShadows::MAX_CASCADES;
};

void CascadeShadowsPrivate::destroyOverrides()
{
  for (auto &s : cascadeOverride)
    shaders::overrides::destroy(s);
}

void CascadeShadowsPrivate::createOverrides()
{
  for (int ss = 0; ss < settings.splitsW * settings.splitsH; ++ss)
  {
    IBBox2 viewport = getViewPort(ss, {shadowCascadesTexInfo.w, shadowCascadesTexInfo.h});
    shaders::OverrideState state;
    state.set(shaders::OverrideState::Z_BIAS);
    state.set(shaders::OverrideState::Z_CLAMP_ENABLED);
    state.set(shaders::OverrideState::Z_FUNC);
    state.zFunc = CMPF_LESSEQUAL;
    state.slopeZBias = settings.shadowDepthSlopeBias * SHADOW_SAMPLING_MAX_OFFSET;
    state.zBias = settings.shadowConstDepthBias + settings.shadowDepthBias / viewport.width().x;
    shaders::OverrideState oldState = shaders::overrides::get(cascadeOverride[ss]);
    if (oldState.zBias == state.zBias && oldState.slopeZBias == state.slopeZBias && oldState.bits == state.bits) // optimized of if
                                                                                                                 // (oldState == state)
      continue;
    cascadeOverride[ss].reset(shaders::overrides::create(state)); // will just increase reference if same
  }
}


CascadeShadowsPrivate::CascadeShadowsPrivate(ICascadeShadowsClient *in_client, const CascadeShadows::Settings &in_settings,
  String &&res_postfix) :
  client(in_client), settings(in_settings), dbgModeSettings(false), mobileAreaUpdateRP(nullptr), resPostfix(std::move(res_postfix))
{
  G_ASSERT(client);

#define VAR(a)                                              \
  {                                                         \
    int tmp = get_shader_variable_id(#a "_const_no", true); \
    if (VariableMap::isVariablePresent(tmp))                \
      a##_const_no = ShaderGlobal::get_int_fast(tmp);       \
  }
  GLOBAL_CONST_LIST
#undef VAR

  shadowCascadeDepthRangeVarId = get_shader_variable_id("shadow_cascade_depth_range", true);
  csmShadowFadeOutVarId = get_shader_variable_id("csm_shadow_fade_out", true);
  deferredShadowPassVarId = get_shader_variable_id("deferred_shadow_pass", true);
  int maxVarId = get_shader_variable_id("csm_max_cascades_possible", true);
  if (VariableMap::isVariablePresent(maxVarId))
  {
    maxCascadesPossible = clamp<int>(ShaderGlobal::get_int_fast(maxVarId), 2, CascadeShadows::MAX_CASCADES);
    debug("CSM maxCascadesPossible = %d varId %d", maxCascadesPossible, maxVarId);
  }

  G_ASSERT(settings.splitsW * settings.splitsH <= CascadeShadows::MAX_CASCADES);
  createDepthShadow(settings.splitsW, settings.splitsH, settings.cascadeWidth, settings.cascadeWidth,
    settings.cascadeDepthHighPrecision);
}

void CascadeShadowsPrivate::createMobileRP(uint32_t depth_fmt, uint32_t rt_fmt)
{
  RenderPassTargetDesc targets[] = {{nullptr, depth_fmt, false}, {nullptr, rt_fmt, false}};
  RenderPassBind binds[] = {{0, 0, RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL, RP_TA_LOAD_CLEAR | RP_TA_SUBPASS_WRITE,
                              RB_STAGE_PIXEL | RB_RW_DEPTH_STENCIL_TARGET},
    {1, 0, 0, RP_TA_LOAD_NO_CARE | RP_TA_SUBPASS_WRITE, RB_STAGE_PIXEL | RB_RW_RENDER_TARGET},
    {0, RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END, 0, RP_TA_STORE_WRITE, RB_STAGE_PIXEL | RB_RO_SRV},
    {1, RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END, 0, RP_TA_STORE_NO_CARE, RB_STAGE_PIXEL | RB_RO_SRV}};

  mobileAreaUpdateRP = d3d::create_render_pass(
    {"shadowCascadeAreaUpdateRP", sizeof(targets) / sizeof(targets[0]), sizeof(binds) / sizeof(binds[0]), targets, binds, 0});
}

void CascadeShadowsPrivate::createDepthShadow(int splits_w, int splits_h, int width, int height, bool high_precision_depth)
{
  closeDepthShadow();

  const uint32_t format = high_precision_depth ? TEXFMT_DEPTH32 : TEXFMT_DEPTH16;
  shadowCascadesTexInfo.cflg = format | TEXCF_RTARGET | TEXCF_TC_COMPATIBLE;
  shadowCascadesTexInfo.mipLevels = 1;
  shadowCascadesTexInfo.resType = RES3D_TEX;
  shadowCascadesTexInfo.w = splits_w * width;
  shadowCascadesTexInfo.h = splits_h * height;

  if (settings.resourceAccessStrategy == CascadeShadows::Settings::ResourceAccessStrategy::Internal)
  {
    internalCascades = UniqueTexHolder(dag::create_tex(NULL, shadowCascadesTexInfo.w, shadowCascadesTexInfo.h,
                                         shadowCascadesTexInfo.cflg, shadowCascadesTexInfo.mipLevels, "shadowCascadeDepthTex2D"),
      "shadow_cascade_depth_tex");
    d3d_err(internalCascades.getTex2D());
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.filter_mode = d3d::FilterMode::Compare;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      internalCascadesSampler = d3d::request_sampler(smpInfo);
      ShaderGlobal::set_sampler(get_shader_variable_id("shadow_cascade_depth_tex_samplerstate", true), internalCascadesSampler);
    }
    internalCascades->disableSampler();

    // sometimes we use this target as SRV while not writing something to it
    // causing it be in initial clear RT/DS state
    d3d::resource_barrier({internalCascades.getBaseTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

    debug("2d texture for shadows created");
  }

  if (d3d::get_driver_desc().issues.hasRenderPassClearDataRace)
  {
    const uint32_t rtFmt = TEXFMT_A8R8G8B8 | TEXCF_RTARGET | TEXCF_TRANSIENT;
    shadowCascadesFakeRT =
      UniqueTexHolder(dag::create_tex(NULL, splits_w * width, splits_h * height, rtFmt, 1, "shadowCascadeDepthTex2D_fakeRT"),
        "shadow_cascade_depth_tex");
    createMobileRP(format, rtFmt);
  }

  if (shadow_cascade_dataVarId != -1)
    csmBuffer = dag::buffers::create_one_frame_cb(CascadeShadows::MAX_CASCADES * 4 + CascadeShadows::MAX_CASCADES,
      String(0, "shadow_cascade_data%s", resPostfix.c_str()).c_str());
}


CascadeShadowsPrivate::~CascadeShadowsPrivate()
{
  closeDepthShadow();
  destroyOverrides();
}


void CascadeShadowsPrivate::closeDepthShadow()
{
  internalCascades.close();
  shadowCascadesFakeRT.close();
  csmBuffer.close();

  if (mobileAreaUpdateRP)
  {
    d3d::delete_render_pass(mobileAreaUpdateRP);
    mobileAreaUpdateRP = nullptr;
  }
}


void CascadeShadowsPrivate::renderShadowsCascadesCb(const csm_render_cascades_cb_t &render_cascades_cb,
  ManagedTexView external_cascades)
{
  if (numCascadesToRender == 0)
    return;

  TIME_D3D_PROFILE(renderShadows);

  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;

  if (settings.resourceAccessStrategy == CascadeShadows::Settings::ResourceAccessStrategy::Internal)
    ShaderGlobal::set_texture(internalCascades.getVarId(), BAD_TEXTUREID);

  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);

  d3d::set_render_target();
  d3d::set_render_target(nullptr, 0);
  bool clearPerView = false;
  for (int cascadeNo = numCascadesToRender - 1; cascadeNo >= 0; cascadeNo--)
  {
    if (!shadowSplits[cascadeNo].shouldUpdate)
    {
      clearPerView = true;
      break;
    }
  }

  G_ASSERT(settings.resourceAccessStrategy != CascadeShadows::Settings::ResourceAccessStrategy::External || external_cascades);
  ManagedTexView cascades = settings.resourceAccessStrategy == CascadeShadows::Settings::ResourceAccessStrategy::Internal
                              ? internalCascades
                              : external_cascades;
  d3d::set_depth(cascades.getTex2D(), DepthAccess::RW);

  if (!clearPerView)
  {
    TIME_D3D_PROFILE(clearShadows);
    d3d::clearview(CLEAR_ZBUFFER, 0, 1.f, 0);
  }

  shaders::OverrideStateId curStateId = shaders::overrides::get_current();
  if (curStateId)
    shaders::overrides::reset();

  client->prepareRenderShadowCascades();

  render_cascades_cb(numCascadesToRender, clearPerView);

  if (curStateId)
    shaders::overrides::set(curStateId);

  if (settings.resourceAccessStrategy == CascadeShadows::Settings::ResourceAccessStrategy::Internal)
  {
    d3d::resource_barrier({internalCascades.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    internalCascades.setVar();
  }

  static int start_csm_shadow_blendVarId = get_shader_variable_id("start_csm_shadow_blend", true);

  ShaderGlobal::set_real(start_csm_shadow_blendVarId, shadowSplits[numCascadesToRender - 1].to * 0.98);

  static int pcf_lerpVarId = get_shader_variable_id("pcf_lerp", true);
  ShaderGlobal::set_color4(pcf_lerpVarId, 0.5f / shadowCascadesTexInfo.w, 0.5f / shadowCascadesTexInfo.h, shadowCascadesTexInfo.w,
    shadowCascadesTexInfo.h);
}

void CascadeShadowsPrivate::renderShadowsCascades()
{
  renderShadowsCascadesCb([&](int num_cascades_to_render, bool clear_per_view) {
    for (int i = 0; i < num_cascades_to_render; ++i)
      renderShadowCascadeDepth(i, clear_per_view);
  });
}

static void calculate_cascades(float dist, float weight, int cascades, float *distances, float zn)
{
  for (int sliceIt = 0; sliceIt < cascades; sliceIt++)
  {
    float f = float(sliceIt + 1) / cascades;
    float logDistance = zn * pow(dist / zn, f);
    float uniformDistance = zn + (dist - zn) * f;
    distances[sliceIt] = lerp(uniformDistance, logDistance, weight);
  }
}


IBBox2 CascadeShadowsPrivate::getViewPort(int cascade, const IPoint2 &tex_width) const
{
  int cascadeWidth = tex_width.x, cascadeHeight = tex_width.y;
  cascadeWidth /= settings.splitsW;
  cascadeHeight /= settings.splitsH;
  IBBox2 view;
  view[0] = IPoint2((cascade % settings.splitsW) * cascadeWidth, (cascade / settings.splitsW) * cascadeHeight);
  view[1] = view[0] + IPoint2(cascadeWidth, cascadeHeight);
  return view;
}


bool force_no_update_shadows = false;
bool force_update_shadows = false;

void CascadeShadowsPrivate::prepareShadowCascades(const CascadeShadows::ModeSettings &mode_settings, const Point3 &dir_to_sun,
  const TMatrix &view_matrix, const Point3 &camera_pos, const TMatrix4 &proj_tm, const Frustum &view_frustum,
  const Point2 &scene_z_near_far, float z_near_for_cascade_distribution)
{
  G_ASSERT(client);
  if (!dbgModeSettings)
    modeSettings = mode_settings;

  if (modeSettings.numCascades <= 0)
  {
    numCascadesToRender = 0;
    csmDistance = 0.f;
    return;
  }

  if (modeSettings.overrideZNearForCascadeDistribution >= 0)
    z_near_for_cascade_distribution = modeSettings.overrideZNearForCascadeDistribution;

  G_ASSERT(modeSettings.numCascades <= settings.splitsW * settings.splitsH);

  carray<float, CascadeShadows::MAX_CASCADES> distances = {};
  int cascades = modeSettings.numCascades;
  G_ASSERT(cascades <= CascadeShadows::MAX_CASCADES);

  float znear = scene_z_near_far.x;
  float shadowStart = max(znear, modeSettings.shadowStart);
  float zNearForCascadeDistribution = max(z_near_for_cascade_distribution, shadowStart);

  // First N cascades can be set manually - drawn around provided spheres.
  // We shouldn't take them into account when calculating distances for regular cascades.
  int firstRegularCascade = 0;
  carray<Point4, CascadeShadows::MAX_CASCADES> anchors;
  for (int cascadeNo = 0; cascadeNo < cascades; cascadeNo++)
  {
    anchors[cascadeNo] = client->getCascadeShadowAnchor(cascadeNo);
    float anchorRadius = anchors[cascadeNo].w;
    G_ASSERTF(anchorRadius <= 0.0f || firstRegularCascade == cascadeNo,
      "Manual shadow cascades must go sequentially starting from 0th cascade (anchorRadius = %f for cascade %d)", anchorRadius,
      cascadeNo);
    if (anchorRadius > 0.0f)
      firstRegularCascade++;
  }

  G_ASSERT(scene_z_near_far.x > 0.f);

  G_ASSERT(cascades <= distances.size());
  int nRegularCascades = cascades - firstRegularCascade;
  calculate_cascades(modeSettings.maxDist, modeSettings.powWeight, nRegularCascades, distances.data() + firstRegularCascade,
    zNearForCascadeDistribution);
  numCascadesToRender = cascades;
  if (modeSettings.cascade0Dist > 0.f && nRegularCascades > 1)
    distances[firstRegularCascade] = min(modeSettings.cascade0Dist + modeSettings.shadowStart, distances[firstRegularCascade]);

  const int csm_range_cascade_vars[] = {
    int(csm_range_cascade_0VarId),
    int(csm_range_cascade_1VarId),
    int(csm_range_cascade_2VarId),
  };

  const int csm_meter_to_uv_cascade_vars[] = {
    int(csm_meter_to_uv_cascade_0VarId),
    int(csm_meter_to_uv_cascade_1VarId),
    int(csm_meter_to_uv_cascade_2VarId),
  };

  const int csm_uv_minmax_cascade_vars[] = {
    int(csm_uv_minmax_cascade_0VarId),
    int(csm_uv_minmax_cascade_1VarId),
    int(csm_uv_minmax_cascade_2VarId),
  };

  for (unsigned int cascadeNo = 0; cascadeNo < cascades; cascadeNo++)
  {
    float prevCascadeDist = (cascadeNo > firstRegularCascade) ? distances[cascadeNo - 1] : shadowStart;
    float curCascadeDist = distances[cascadeNo];
    float nextCascadeDist =
      (cascadeNo + 1 < cascades) ? distances[cascadeNo + 1] : SHADOW_FAR_CASCADE_DEPTH_MUL * modeSettings.maxDist;
    // For manual cascades extend dist to the nearest regular cascade (affects only shadows depth).
    if (cascadeNo < firstRegularCascade)
      curCascadeDist = nextCascadeDist =
        firstRegularCascade < cascades ? distances[firstRegularCascade] : SHADOW_FAR_CASCADE_DEPTH_MUL * modeSettings.maxDist;

    ShadowSplit ss;
    ss.from = max(prevCascadeDist, znear);
    ss.to = min(curCascadeDist, scene_z_near_far.y);
    nextCascadeDist = min(nextCascadeDist, scene_z_near_far.y);

    G_ASSERT(ss.to > ss.from);

    ss.viewport = getViewPort(cascadeNo, {shadowCascadesTexInfo.w, shadowCascadesTexInfo.h});

    buildShadowProjectionMatrix(dir_to_sun, view_matrix, camera_pos, proj_tm, ss.from, ss.to, nextCascadeDist,
      Point3::xyz(anchors[cascadeNo]), anchors[cascadeNo].w, ss);

    if (needSsss && cascadeNo < CascadeShadows::SSSS_CASCADES)
    {
      ShaderGlobal::set_real(csm_range_cascade_vars[cascadeNo], ss.znzf.y - ss.znzf.x);

      Point3 cascadeWidth = ss.shadowWidth;
      float shadowTexW = shadowCascadesTexInfo.w, shadowTexH = shadowCascadesTexInfo.h;
      Point2 cascadeUv = Point2(float(ss.viewport.width().x) / shadowTexW, float(ss.viewport.width().y) / shadowTexH);
      Point2 meterToUv = div(cascadeUv, Point2::xy(cascadeWidth));
      ShaderGlobal::set_real(csm_meter_to_uv_cascade_vars[cascadeNo], max(meterToUv.x, meterToUv.y));
      Point4 uvMinMax = Point4(ss.viewport.getMin().x / shadowTexW, ss.viewport.getMin().y / shadowTexH,
        ss.viewport.getMax().x / shadowTexW, ss.viewport.getMax().y / shadowTexH);
      ShaderGlobal::set_color4(csm_uv_minmax_cascade_vars[cascadeNo], uvMinMax);
    }

    if (force_update_shadows)
      shadowSplits[cascadeNo].frames = 0xFFFF;

    float minSparseDist;
    int minSparseFrame;
    ss.frustum.construct(ss.worldCullingMatrix);
    client->getCascadeShadowSparseUpdateParams(cascadeNo, ss.frustum, minSparseDist, minSparseFrame);

    if ((ss.from < minSparseDist || shadowSplits[cascadeNo].frames >= minSparseFrame + cascadeNo) && !force_no_update_shadows)
    {
      shadowSplits[cascadeNo] = ss;
      sparsedShadowSplits[cascadeNo] = ss;
    }
    else
    {
      bool shouldUpdate = false;
      if (minSparseDist >= 0.f) // Negative value indicates the camera direction may be ignored.
      {
        Frustum shadowFrustum = view_frustum;
        vec4f curViewPos = v_ldu(&ss.viewPos.x);
        shadowFrustum.camPlanes[Frustum::NEARPLANE] =
          expand_znear_plane(shadowFrustum.camPlanes[Frustum::NEARPLANE], curViewPos, v_splats(ss.from));
        shadowFrustum.camPlanes[Frustum::FARPLANE] =
          shrink_zfar_plane(shadowFrustum.camPlanes[Frustum::FARPLANE], curViewPos, v_splats(ss.to));

        vec3f frustumPoints[8];
        shadowFrustum.generateAllPointFrustm(frustumPoints);

        for (int pt = 0; pt < 8; ++pt)
        {
          vec3f v_pt = frustumPoints[pt];
          vec4f invisible = v_zero();
          for (int plane = 0; plane < 6; ++plane)
            invisible = v_or(invisible, v_plane_dist_x(shadowSplits[cascadeNo].frustum.camPlanes[plane], v_pt));
          invisible = v_and(invisible, v_msbit());
          if (!v_test_vec_x_eqi_0(invisible))
          {
            shouldUpdate = true;
            break;
          }
        }
      }

      sparsedShadowSplits[cascadeNo] = ss;

      if (shouldUpdate && !force_no_update_shadows)
      {
        shadowSplits[cascadeNo] = ss;
      }
      else
      {
        shadowSplits[cascadeNo].frames++;
        shadowSplits[cascadeNo].shouldUpdate = 0;
        TMatrix tm = TMatrix::IDENT;
        tm.setcol(3, ss.viewPos - shadowSplits[cascadeNo].viewPos);
        shadowSplits[cascadeNo].cameraRenderMatrix = TMatrix4(tm) * shadowSplits[cascadeNo].cameraRenderMatrix;
        shadowSplits[cascadeNo].viewPos = ss.viewPos;
      }
    }
  }

  {
    ShadowSplit ss;
    ss.frames = 0;
    ss.from = max(shadowStart, znear);
    ss.to = min(modeSettings.maxDist, scene_z_near_far.y);
    Point4 anchor = client->getCascadeShadowAnchor(-1);
    buildShadowProjectionMatrix(dir_to_sun, view_matrix, camera_pos, proj_tm, ss.from, ss.to,
      min(SHADOW_FAR_CASCADE_DEPTH_MUL * ss.to, scene_z_near_far.y), Point3::xyz(anchor), anchor.w, ss);
    wholeCoveredSpaceFrustum.construct(ss.worldCullingMatrix);
  }

  for (int cascadeNo = 0; cascadeNo < cascades; cascadeNo++)
    shadowSplits[cascadeNo].frustum.construct(shadowSplits[cascadeNo].worldCullingMatrix);
  createOverrides();

  vec3f frustumPoints[8];
  shadowSplits[numCascadesToRender - 1].frustum.generateAllPointFrustm(frustumPoints);
  mat44f globtm44;
  TMatrix4_vec4 globtm;
  d3d::calcglobtm(view_matrix, proj_tm, globtm);
  memcpy(&globtm44, globtm.m, sizeof(mat44f));
  vec4f farPt = v_zero();
  for (int i = 0; i < 8; ++i)
  {
    vec4f point = v_mat44_mul_vec3p(globtm44, frustumPoints[i]);
    farPt = v_sel(farPt, point, v_splat_w(v_cmp_gt(point, farPt)));
  }
  csmDistance = v_extract_w(farPt);
}


void CascadeShadowsPrivate::calcTMs()
{
  if (numCascadesToRender <= 0)
  {
    setFadeOutToShaders(0.f);
    ShaderGlobal::set_real(csm_distanceVarId, 0.f);
    ShaderGlobal::set_int(num_of_cascadesVarId, 0);
    return;
  }
  G_ASSERT(numCascadesToRender <= CascadeShadows::MAX_CASCADES);

  if (ShaderGlobal::is_var_assumed(num_of_cascadesVarId) &&
      ShaderGlobal::get_interval_assumed_value(num_of_cascadesVarId) < numCascadesToRender)
  {
    logerr("num_of_cascades was assumed to %d, but trying to render with %d cascades. "
           "Assumed interval value must be greater or equal than the real number of cascades",
      ShaderGlobal::get_interval_assumed_value(num_of_cascadesVarId), numCascadesToRender);
  }

  setFadeOutToShaders(modeSettings.maxDist * settings.fadeOutMul);
  ShaderGlobal::set_real(csm_distanceVarId, csmDistance);
  ShaderGlobal::set_int(num_of_cascadesVarId, numCascadesToRender);

  int shadowTexW = shadowCascadesTexInfo.w, shadowTexH = shadowCascadesTexInfo.h;
  for (unsigned int cascadeNo = 0; cascadeNo < numCascadesToRender; cascadeNo++)
  {
    const ShadowSplit &ss = shadowSplits[cascadeNo];
    TMatrix4_vec4 texTm = ss.cameraRenderMatrix * screen_to_tex_scale_tm();
    shadowCascadeTm[cascadeNo * 3 + 0] = Color4(texTm.m[0][0], texTm.m[1][0], texTm.m[2][0], texTm.m[3][0] - 0.5);
    shadowCascadeTm[cascadeNo * 3 + 1] = Color4(texTm.m[0][1], texTm.m[1][1], texTm.m[2][1], texTm.m[3][1] - 0.5);
    shadowCascadeTm[cascadeNo * 3 + 2] = Color4(texTm.m[0][2], texTm.m[1][2], texTm.m[2][2], texTm.m[3][2] - 0.5);
    shadowCascadeTcMulOffset[cascadeNo] = Color4(float(ss.viewport.width().x) / shadowTexW, float(ss.viewport.width().y) / shadowTexH,
      float(ss.viewport[0].x) / shadowTexW + float(0.5 * ss.viewport.width().x) / shadowTexW,
      float(ss.viewport[0].y) / shadowTexH + float(0.5 * ss.viewport.width().y) / shadowTexH);
  }
  for (unsigned int cascadeNo = numCascadesToRender; cascadeNo < CascadeShadows::MAX_CASCADES; cascadeNo++)
  {
    shadowCascadeTm[cascadeNo * 3 + 0] = Color4(0, 0, 0, 1);
    shadowCascadeTm[cascadeNo * 3 + 1] = Color4(0, 0, 0, 1);
    shadowCascadeTm[cascadeNo * 3 + 2] = Color4(0, 0, 0, 1);
    shadowCascadeTcMulOffset[cascadeNo] = Color4(0, 0, 0, 0);
  }
}

const String &CascadeShadowsPrivate::setShadowCascadeDistanceDbg(const Point2 &scene_z_near_far, int tex_size, int splits_w,
  int splits_h, float shadow_distance, float pow_weight)
{
  if (tex_size > 0 && splits_w > 0 && splits_h > 0 && splits_w * splits_h <= CascadeShadows::MAX_CASCADES && pow_weight >= 0 &&
      pow_weight <= 1 && shadow_distance > 0)
  {
    dbgModeSettings = true;

    settings.splitsW = splits_w;
    settings.splitsH = splits_h;
    settings.cascadeWidth = tex_size;
    createDepthShadow(settings.splitsW, settings.splitsH, settings.cascadeWidth, settings.cascadeWidth,
      settings.cascadeDepthHighPrecision);

    modeSettings.maxDist = shadow_distance;
    modeSettings.powWeight = pow_weight;
    modeSettings.numCascades = settings.splitsW * settings.splitsH;
  }

  carray<float, CascadeShadows::MAX_CASCADES> distances;
  int cascades = settings.splitsW * settings.splitsH;
  G_ASSERT(cascades <= distances.size());
  calculate_cascades(modeSettings.maxDist, modeSettings.powWeight, cascades, distances.data(), scene_z_near_far.x);

  static String dbg;
  dbg = String(100, "%dx%d, %g weight): cascades = ", shadowCascadesTexInfo.w, shadowCascadesTexInfo.h, modeSettings.powWeight);
  for (unsigned int cascadeNo = 0; cascadeNo < cascades; cascadeNo++)
    dbg += String(100, "%g%s", distances[cascadeNo], (cascadeNo == cascades - 1) ? "\n" : ", ");

  return dbg;
}


TMatrix4 CascadeShadowsPrivate::getShadowViewMatrix(const Point3 &dir_to_sun, const Point3 &camera_pos, bool world_space)
{
  Point3 dirToSun = dir_to_sun;

  TMatrix4 shadowViewMatrix = ::matrix_look_at_lh(ZERO<Point3>(), -dirToSun, Point3(0.f, 1.f, 0.f));


  if (world_space)
  {
    TMatrix4 worldToCamldMatrix(TMatrix4::IDENT);
    worldToCamldMatrix.setrow(3, -camera_pos.x, -camera_pos.y, -camera_pos.z, 1.f);

    shadowViewMatrix = worldToCamldMatrix * shadowViewMatrix;
  }


  return shadowViewMatrix;
}

void CascadeShadowsPrivate::buildShadowProjectionMatrix(const Point3 &dir_to_sun, const TMatrix &view_matrix, const Point3 &camera_pos,
  const TMatrix4 &projTM, float z_near, float z_far, float next_z_far, const Point3 &anchor, float anchor_radius, ShadowSplit &split)
{
  if (!internalCascades && settings.resourceAccessStrategy != CascadeShadows::Settings::ResourceAccessStrategy::External)
    return;

  float shadowCascadeZExpansion_znearOffset =
    csm_cascade_z_expansion_znear_offset > 0 ? csm_cascade_z_expansion_znear_offset : modeSettings.shadowCascadeZExpansion_znearOffset;

  float expandZfar = min(2.f * modeSettings.shadowCascadeZExpansion, safediv(modeSettings.shadowCascadeZExpansion, dir_to_sun.y));
  float expandZnear = expandZfar + max(shadowCascadeZExpansion_znearOffset, 0.0f);

  TMatrix4 shadowViewMatrix = getShadowViewMatrix(dir_to_sun, camera_pos, false); // always the same for all splits! Depends on light
                                                                                  // direction only
  TMatrix shadowViewMatrix3 = tmatrix(shadowViewMatrix); // always the same for all splits! Depends on light direction only
  TMatrix4 shadowWorldViewMatrix = getShadowViewMatrix(dir_to_sun, camera_pos, true);

  const float det = view_matrix.det();
  split.viewPos = fabs(det) > 1e-5 ? inverse(view_matrix, det).getcol(3) : Point3(0, 0, 0);

  TMatrix camViewTm = view_matrix;
  camViewTm.setcol(3, 0.f, 0.f, 0.f);
  TMatrix4 camViewProjTm = TMatrix4(camViewTm) * projTM;

  Frustum frustum;
  frustum.construct(camViewProjTm);
  // debug("zn = %g zf=%g <- %g %g", v_extract_w(frustum.camPlanes[Frustum::NEARPLANE]),
  // v_extract_w(frustum.camPlanes[Frustum::FARPLANE]), z_near, z_far);
  frustum.camPlanes[Frustum::FARPLANE] = v_perm_xyzd(frustum.camPlanes[Frustum::FARPLANE], v_splats(z_far));
  frustum.camPlanes[Frustum::NEARPLANE] = v_perm_xyzd(frustum.camPlanes[Frustum::NEARPLANE], v_splats(-z_near));
  vec3f frustumPoints[8];
  frustum.generateAllPointFrustm(frustumPoints);
  vec3f frustumPointsInLS[8];
  mat33f v_shadowView;
  v_shadowView.col0 = v_ldu(shadowViewMatrix3[0]);
  v_shadowView.col1 = v_ldu(shadowViewMatrix3[1]);
  v_shadowView.col2 = v_ldu(shadowViewMatrix3[2]);
  for (int i = 0; i < 8; ++i)
    frustumPointsInLS[i] = v_mat33_mul_vec3(v_shadowView, frustumPoints[i]);

  bbox3f v_frustumInLSBox;
  v_bbox3_init(v_frustumInLSBox, frustumPointsInLS[0]);
  for (int i = 1; i < 8; ++i)
    v_bbox3_add_pt(v_frustumInLSBox, frustumPointsInLS[i]);

  if (next_z_far > z_far) // Extend box along the z-axis to include next cascade frustum.
  {                       // Helps to avoid early cascade switch due to an insufficient depth range.
    frustum.camPlanes[Frustum::FARPLANE] = v_perm_xyzd(frustum.camPlanes[Frustum::FARPLANE], v_splats(next_z_far));
    frustum.generateAllPointFrustm(frustumPoints);
    for (int i = 0; i < 8; ++i)
    {
      vec3f frustumPointInLS = v_mat33_mul_vec3(v_shadowView, frustumPoints[i]);
      v_frustumInLSBox.bmin = v_perm_xycw(v_frustumInLSBox.bmin, v_min(v_frustumInLSBox.bmin, frustumPointInLS));
      v_frustumInLSBox.bmax = v_perm_xycw(v_frustumInLSBox.bmax, v_max(v_frustumInLSBox.bmax, frustumPointInLS));
    }
  }

  BBox3 shadowProjectionBox;
  v_stu(&shadowProjectionBox[0].x, v_frustumInLSBox.bmin);
  v_stu_p3(&shadowProjectionBox[1].x, v_frustumInLSBox.bmax);

  if (anchor_radius > 0.0f)
  {
    Point3 anchorPoint = shadowViewMatrix3 * anchor;
    shadowProjectionBox.lim[0].x = anchorPoint.x - anchor_radius;
    shadowProjectionBox.lim[0].y = anchorPoint.y - anchor_radius;
    shadowProjectionBox.lim[1].x = anchorPoint.x + anchor_radius;
    shadowProjectionBox.lim[1].y = anchorPoint.y + anchor_radius;
  }
  else
  {
    if (!split.viewport.isEmpty())
    {
      // Align box with shadow texels.
      const int borderPixels = 4;
      float texelWidth = shadowProjectionBox.width().x / split.viewport.width().x; // Add border pixels before adding the reserve for
                                                                                   // camera rotation to measure this reserve in
                                                                                   // constant units.
      float texelHeight = shadowProjectionBox.width().y / split.viewport.width().y;
      shadowProjectionBox.lim[0].x -= borderPixels * texelWidth;
      shadowProjectionBox.lim[0].y -= borderPixels * texelHeight;
      shadowProjectionBox.lim[1].x += borderPixels * texelWidth;
      shadowProjectionBox.lim[1].y += borderPixels * texelHeight;

      float step = modeSettings.shadowCascadeRotationMargin * z_far;
      shadowProjectionBox.lim[0].x = step * floorf(shadowProjectionBox.lim[0].x / step);
      shadowProjectionBox.lim[0].y = step * floorf(shadowProjectionBox.lim[0].y / step);
      shadowProjectionBox.lim[1].x = step * ceilf(shadowProjectionBox.lim[1].x / step);
      shadowProjectionBox.lim[1].y = step * ceilf(shadowProjectionBox.lim[1].y / step);

      Point3 anchorPoint = shadowViewMatrix3 * anchor;
      texelWidth = shadowProjectionBox.width().x / split.viewport.width().x; // Box size was changed, recalculate the exact texel size
                                                                             // to snap to pixel.
      texelHeight = shadowProjectionBox.width().y / split.viewport.width().y;
      shadowProjectionBox.lim[0].x = anchorPoint.x + floorf((shadowProjectionBox.lim[0].x - anchorPoint.x) / texelWidth) * texelWidth;
      shadowProjectionBox.lim[0].y =
        anchorPoint.y + floorf((shadowProjectionBox.lim[0].y - anchorPoint.y) / texelHeight) * texelHeight;
      shadowProjectionBox.lim[1].x = anchorPoint.x + ceilf((shadowProjectionBox.lim[1].x - anchorPoint.x) / texelWidth) * texelWidth;
      shadowProjectionBox.lim[1].y = anchorPoint.y + ceilf((shadowProjectionBox.lim[1].y - anchorPoint.y) / texelHeight) * texelHeight;
    }
  }

  // Shadow projection matrix.
  split.znzf =
    Point2(shadowProjectionBox.lim[0].z - expandZnear, shadowProjectionBox.lim[1].z + SHADOW_CULLING_POS_EXPAND_MUL * expandZfar);

  TMatrix4 shadowProjectionCullingMatrix = ::matrix_ortho_off_center_lh(shadowProjectionBox.lim[0].x, shadowProjectionBox.lim[1].x,
    shadowProjectionBox.lim[0].y, shadowProjectionBox.lim[1].y, split.znzf.x, split.znzf.y);

  TMatrix4 shadowProjectionRenderMatrix;
#if USE_SHADOW_DEPTH_CLAMP
  shadowProjectionBox.lim[0].z -= shadow_render_expand_mul * expandZnear + shadow_render_expand_to_sun_mul * (split.to - split.from);
  shadowProjectionBox.lim[1].z += shadow_render_expand_mul * expandZfar + shadow_render_expand_from_sun_mul * (split.to - split.from);

  split.znzf = Point2(shadowProjectionBox.lim[0].z, shadowProjectionBox.lim[1].z);

  shadowProjectionRenderMatrix = ::matrix_ortho_off_center_lh(shadowProjectionBox.lim[0].x, shadowProjectionBox.lim[1].x,
    shadowProjectionBox.lim[0].y, shadowProjectionBox.lim[1].y, split.znzf.x, split.znzf.y);
#else
  shadowProjectionRenderMatrix = shadowProjectionCullingMatrix;
#endif

  split.shadowWidth = shadowProjectionBox.width();
  split.cameraCullingMatrix = shadowViewMatrix * shadowProjectionCullingMatrix;
  split.cameraRenderMatrix = shadowViewMatrix * shadowProjectionRenderMatrix;
  split.worldCullingMatrix = shadowWorldViewMatrix * shadowProjectionCullingMatrix;
  split.worldRenderMatrix = shadowWorldViewMatrix * shadowProjectionRenderMatrix;

  split.renderViewMatrix = shadowWorldViewMatrix;
  split.renderProjMatrix = shadowProjectionRenderMatrix;

  TMatrix invShadowViewMatrix = orthonormalized_inverse(shadowViewMatrix3); // always the same for all splits! Depends on light
                                                                            // direction only
  split.shadowViewItm = invShadowViewMatrix; // it is actually split independent Depends on light direction only

  Frustum shadowFrustum;
  shadowFrustum.construct(split.cameraCullingMatrix);

  bbox3f frustumWorldBox;
  shadowFrustum.calcFrustumBBox(frustumWorldBox);
  v_stu(&split.worldBox[0].x, v_add(frustumWorldBox.bmin, v_ldu(&camera_pos.x)));
  v_stu_p3(&split.worldBox[1].x, v_add(frustumWorldBox.bmax, v_ldu(&camera_pos.x)));
}


void CascadeShadowsPrivate::renderShadowCascadeDepth(int cascadeNo, bool clearPerView, ManagedTexView external_cascades)
{
  const ShadowSplit &ss = shadowSplits[cascadeNo];
  G_ASSERT(ss.to > ss.from);

  G_ASSERT(settings.resourceAccessStrategy != CascadeShadows::Settings::ResourceAccessStrategy::External || external_cascades);
  ManagedTexView cascades = settings.resourceAccessStrategy == CascadeShadows::Settings::ResourceAccessStrategy::Internal
                              ? internalCascades
                              : external_cascades;

  if (ss.shouldUpdate)
  {
    d3d::setview(ss.viewport[0].x, ss.viewport[0].y, ss.viewport.width().x, ss.viewport.width().y, 0, 1);
    if (clearPerView && !mobileAreaUpdateRP)
    {
#if DAGOR_DBGLEVEL > 0 // don't waste perf in release on searching perf marker names
      char name[64];
      SNPRINTF(name, sizeof(name), "clear_cascade_%d", cascadeNo);
      TIME_D3D_PROFILE_NAME(renderShadowCascade, name);
#else
      TIME_D3D_PROFILE(clear_cascade);
#endif
      d3d::clearview(CLEAR_ZBUFFER, 0, 1.f, 0);
    }
    if (cascades)
    {
#if DAGOR_DBGLEVEL > 0 // don't waste perf in release on searching perf marker names
      char name[64] = "shadow_cascade_0_render";
      name[sizeof("shadow_cascade_") - 1] += cascadeNo;
      TIME_D3D_PROFILE_NAME(renderShadowCascade, name);
#else
      TIME_D3D_PROFILE(shadow_cascade_render);
#endif
      auto clientRenderDepth = [&]() {
        shaders::overrides::set(cascadeOverride[cascadeNo]);
        client->renderCascadeShadowDepth(cascadeNo, ss.znzf);
        shaders::overrides::reset();
      };

      if (mobileAreaUpdateRP)
      {
        SCOPE_RENDER_TARGET;

        RenderPassArea area = {(uint32_t)ss.viewport[0].x, (uint32_t)ss.viewport[0].y, (uint32_t)ss.viewport.width().x,
          (uint32_t)ss.viewport.width().y, 0, 1};
        RenderPassTarget targets[] = {{{cascades.getTex2D(), 0, 0}, make_clear_value(1.0f, 0)},
          {{shadowCascadesFakeRT.getTex2D(), 0, 0}, make_clear_value(0, 0, 0, 0)}};

        d3d::begin_render_pass(mobileAreaUpdateRP, area, targets);
        clientRenderDepth();
        d3d::end_render_pass();
      }
      else
        clientRenderDepth();
    }
  }
}


void CascadeShadowsPrivate::setFadeOutToShaders(float max_dist)
{
  G_ASSERT(settings.shadowFadeOut > 0.f);

  // x,y - fade for csm
  // z,w - fade for next shadow layer (clipmap shadows for example)
  ShaderGlobal::set_color4(csmShadowFadeOutVarId, -1.f / settings.shadowFadeOut, max_dist / settings.shadowFadeOut,
    -1.f / settings.shadowFadeOut, max(max_dist - settings.shadowFadeOut * 1.5f, 0.0f) / settings.shadowFadeOut);
}

carray<Color4, CascadeShadows::MAX_CASCADES * 4> CascadeShadowsPrivate::getTransposedCSMTm()
{
  carray<Color4, CascadeShadows::MAX_CASCADES * 4> transposed{};
  for (int i = 0; i < numCascadesToRender; ++i)
  {
    transposed[i * 4 + 0] = Color4(shadowCascadeTm[i * 3 + 0].r, shadowCascadeTm[i * 3 + 1].r, shadowCascadeTm[i * 3 + 2].r, 0);
    transposed[i * 4 + 1] = Color4(shadowCascadeTm[i * 3 + 0].g, shadowCascadeTm[i * 3 + 1].g, shadowCascadeTm[i * 3 + 2].g, 0);
    transposed[i * 4 + 2] = Color4(shadowCascadeTm[i * 3 + 0].b, shadowCascadeTm[i * 3 + 1].b, shadowCascadeTm[i * 3 + 2].b, 0);
    transposed[i * 4 + 3] = Color4(shadowCascadeTm[i * 3 + 0].a, shadowCascadeTm[i * 3 + 1].a, shadowCascadeTm[i * 3 + 2].a, 0);
  }
  for (int i = numCascadesToRender; i < CascadeShadows::MAX_CASCADES; ++i)
  {
    transposed[i * 4 + 0] = Color4(0, 0, 0, 0);
    transposed[i * 4 + 1] = Color4(0, 0, 0, 0);
    transposed[i * 4 + 2] = Color4(0, 0, 0, 0);
    transposed[i * 4 + 3] = Color4(1, 1, 1, 0);
  }
  return transposed;
}

void CascadeShadowsPrivate::uploadCSMBuffer(const Color4 *tm, const Color4 *off)
{
  if (csmBuffer)
  {
    Color4 uploadData[CascadeShadows::MAX_CASCADES * 4 + CascadeShadows::MAX_CASCADES];
    memcpy(uploadData, tm, sizeof(Color4) * CascadeShadows::MAX_CASCADES * 4);
    memcpy(uploadData + CascadeShadows::MAX_CASCADES * 4, off, sizeof(Color4) * CascadeShadows::MAX_CASCADES);

    G_ASSERT_RETURN(csmBuffer, );
    csmBuffer->updateData(0, sizeof(uploadData), uploadData, VBLOCK_DISCARD);
    ShaderGlobal::set_buffer(shadow_cascade_dataVarId, csmBuffer.getBufId());
  }

  ShaderGlobal::set_color4_array(shadow_cascade_tm_transpVarId, tm, maxCascadesPossible * 4);
  ShaderGlobal::set_color4_array(shadow_cascade_tc_mul_offsetVarId, off, maxCascadesPossible);
}

void CascadeShadowsPrivate::setSamplingBiasToShader(float value)
{
  carray<Color4, CascadeShadows::MAX_CASCADES * 4> transposed = getTransposedCSMTm();
  for (int i = 0; i < numCascadesToRender; ++i)
  {
    // z component of 4-th column of every matrix is resposible for cmp value
    transposed[i * 4 + 3].b += value;
  }
  uploadCSMBuffer(transposed.data(), shadowCascadeTcMulOffset.data());
}

void CascadeShadowsPrivate::setCascadesToShader()
{
  calcTMs();

  const carray<Color4, CascadeShadows::MAX_CASCADES * 4> transposed = getTransposedCSMTm();

  uploadCSMBuffer(transposed.data(), shadowCascadeTcMulOffset.data());
  ShaderGlobal::set_color4(csm_world_view_posVarId, P3D(shadowSplits[0].viewPos), 0);
}

void CascadeShadowsPrivate::setNearCullingNearPlaneToShader()
{
  Point4 nearPlanes;
  for (int i = 0; i < min(4, numCascadesToRender); i++)
  {
    Frustum frustum = getWorldCullingMatrix(i);
    nearPlanes[i] = v_extract_w(frustum.camPlanes[Frustum::FARPLANE]); // Naming is based on reverse Z
  }
  ShaderGlobal::set_color4(csm_culling_near_planesVarId, -nearPlanes);
}

void CascadeShadowsPrivate::debugSetParams(float shadow_depth_bias, float shadow_const_depth_bias, float shadow_depth_slope_bias)
{
  settings.shadowDepthBias = shadow_depth_bias;
  settings.shadowConstDepthBias = shadow_const_depth_bias;
  settings.shadowDepthSlopeBias = shadow_depth_slope_bias;
}


void CascadeShadowsPrivate::debugGetParams(float &shadow_depth_bias, float &shadow_const_depth_bias, float &shadow_depth_slope_bias)
{
  shadow_depth_bias = settings.shadowDepthBias;
  shadow_const_depth_bias = settings.shadowConstDepthBias;
  shadow_depth_slope_bias = settings.shadowDepthSlopeBias;
}


//
// CascadeShadows
//

CascadeShadows::ModeSettings::ModeSettings()
{
  memset(this, 0, sizeof(ModeSettings));
  powWeight = 0.99f;
  maxDist = 1000.f;
  shadowStart = 0.f;
  numCascades = 4;
  shadowCascadeZExpansion = 100.f;
  shadowCascadeZExpansion_znearOffset = 0.f;
  shadowCascadeRotationMargin = 0.1f;
  cascade0Dist = -1;
  overrideZNearForCascadeDistribution = -1;
}

/*static*/
CascadeShadows *CascadeShadows::make(ICascadeShadowsClient *in_client, const Settings &in_settings, String &&res_postfix)
{
  int cspalign = max((int)alignof(CascadeShadowsPrivate) - (int)sizeof(CascadeShadows), 0);
  char *m = (char *)memalloc(sizeof(CascadeShadows) + cspalign + sizeof(CascadeShadowsPrivate));
  auto pSelf = (CascadeShadows *)m;
  pSelf->d =
    new ((char *)(pSelf + 1) + cspalign, _NEW_INPLACE) CascadeShadowsPrivate(in_client, in_settings, eastl::move(res_postfix));
  return pSelf;
}

CascadeShadows::~CascadeShadows() { d->~CascadeShadowsPrivate(); }

void CascadeShadows::prepareShadowCascades(const CascadeShadows::ModeSettings &mode_settings, const Point3 &dir_to_sun,
  const TMatrix &view_matrix, const Point3 &camera_pos, const TMatrix4 &proj_tm, const Frustum &view_frustum,
  const Point2 &scene_z_near_far, float z_near_for_cascade_distribution)
{
  d->prepareShadowCascades(mode_settings, dir_to_sun, view_matrix, camera_pos, proj_tm, view_frustum, scene_z_near_far,
    z_near_for_cascade_distribution);
}

void CascadeShadows::renderShadowsCascadesCb(const csm_render_cascades_cb_t &render_cascades_cb, ManagedTexView external_cascades)
{
  d->renderShadowsCascadesCb(render_cascades_cb, external_cascades);
}

void CascadeShadows::renderShadowsCascades() { d->renderShadowsCascades(); }

void CascadeShadows::renderShadowCascadeDepth(int cascadeNo, bool clearPerView, ManagedTexView external_cascades)
{
  d->renderShadowCascadeDepth(cascadeNo, clearPerView, external_cascades);
}

void CascadeShadows::setSamplingBiasToShader(float value) { d->setSamplingBiasToShader(value); }


void CascadeShadows::setCascadesToShader() { d->setCascadesToShader(); }

void CascadeShadows::setNearCullingNearPlaneToShader() { d->setNearCullingNearPlaneToShader(); }

void CascadeShadows::disable() { d->disable(); }

bool CascadeShadows::isEnabled() const { return d->isEnabled(); }

void CascadeShadows::invalidate() { d->invalidate(); }

int CascadeShadows::getNumCascadesToRender() const { return d->getNumCascadesToRender(); }

const Frustum &CascadeShadows::getFrustum(int cascade_no) const { return d->getFrustum(cascade_no); }
const Point3 &CascadeShadows::getRenderCameraWorldViewPos(int cascade_no) const { return d->getRenderCameraWorldViewPos(cascade_no); }

const TMatrix &CascadeShadows::getShadowViewItm(int cascade_no) const { return d->getShadowViewItm(cascade_no); }

const TMatrix4_vec4 &CascadeShadows::getCameraRenderMatrix(int cascade_no) const { return d->getCameraRenderMatrix(cascade_no); }

const TMatrix4_vec4 &CascadeShadows::getWorldCullingMatrix(int cascade_no) const { return d->getWorldCullingMatrix(cascade_no); }

const TMatrix4_vec4 &CascadeShadows::getWorldRenderMatrix(int cascade_no) const { return d->getWorldRenderMatrix(cascade_no); }

const TMatrix4_vec4 &CascadeShadows::getRenderViewMatrix(int cascade_no) const { return d->getRenderViewMatrix(cascade_no); }

const Point3 &CascadeShadows::shadowWidth(int cascade_no) const { return d->shadowWidth(cascade_no); }

const TMatrix4_vec4 &CascadeShadows::getRenderProjMatrix(int cascade_no) const { return d->getRenderProjMatrix(cascade_no); }

const BBox3 &CascadeShadows::getWorldBox(int cascade_no) const { return d->getWorldBox(cascade_no); }

bool CascadeShadows::shouldUpdateCascade(int cascade_no) const { return d->shouldUpdateCascade(cascade_no); }

bool CascadeShadows::isCascadeValid(int cascade_no) const { return d->isCascadeValid(cascade_no); }

void CascadeShadows::copyFromSparsed(int cascade_no) { d->copyFromSparsed(cascade_no); }

float CascadeShadows::getMaxDistance() const { return d->getMaxDistance(); }

float CascadeShadows::getMaxShadowDistance() const { return d->getMaxShadowDistance(); }

float CascadeShadows::getCascadeDistance(int cascade_no) const { return d->getCascadeDistance(cascade_no); }

const Frustum &CascadeShadows::getWholeCoveredFrustum() const { return d->getWholeCoveredFrustum(); }

const String &CascadeShadows::setShadowCascadeDistanceDbg(const Point2 &scene_z_near_far, int tex_size, int splits_w, int splits_h,
  float shadow_distance, float pow_weight)
{
  return d->setShadowCascadeDistanceDbg(scene_z_near_far, tex_size, splits_w, splits_h, shadow_distance, pow_weight);
}

void CascadeShadows::debugSetParams(float shadow_depth_bias, float shadow_const_depth_bias, float shadow_depth_slope_bias)
{
  d->debugSetParams(shadow_depth_bias, shadow_const_depth_bias, shadow_depth_slope_bias);
}

void CascadeShadows::debugGetParams(float &shadow_depth_bias, float &shadow_const_depth_bias, float &shadow_depth_slope_bias)
{
  d->debugGetParams(shadow_depth_bias, shadow_const_depth_bias, shadow_depth_slope_bias);
}

void CascadeShadows::setNeedSsss(bool need_ssss) { d->setNeedSsss(need_ssss); }

const CascadeShadows::Settings &CascadeShadows::getSettings() const { return d->getSettings(); }
void CascadeShadows::setDepthBiasSettings(const CascadeShadows::Settings &set) { return d->setDepthBiasSettings(set); }
void CascadeShadows::setCascadeWidth(int width) { return d->setCascadeWidth(width); }

BaseTexture *CascadeShadows::getShadowsCascade() const { return d->getShadowsCascade(); }
d3d::SamplerHandle CascadeShadows::getShadowsCascadeSampler() const { return d->getShadowsCascadeSampler(); }
const TextureInfo &CascadeShadows::getShadowCascadeTexInfo() const { return d->getShadowCascadeTexInfo(); }

const Point2 &CascadeShadows::getZnZf(int cascade_no) const { return d->getZnZf(cascade_no); }
