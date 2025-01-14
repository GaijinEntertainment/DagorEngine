// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/bvh.h>
#include <bvh/bvh.h>
#include <render/denoiser.h>
#include <render/rtsm.h>
#include <render/rtr.h>
#include <render/rtao.h>
#include <bvh/bvh_connection.h>

#include <render/daBfg/bfg.h>
#include "frameGraphHelpers.h"
#include <landMesh/lmeshManager.h>
#include <render/viewVecs.h>
#include <render/world/cameraParams.h>
#include <render/world/wrDispatcher.h>
#include <ecs/render/updateStageRender.h>
#include <render/daBfg/ecs/frameGraphNode.h>
#include <rendInst/visibility.h>
#include <render/skies.h>
#include <render/fx/fxRenderTags.h>
#include <render/cables.h>
#include <shaders/dag_shaderBlock.h>
#include "frameGraphNodes/frameGraphNodes.h"

#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <render/renderEvent.h>
#include <util/dag_convar.h>

CONSOLE_BOOL_VAL("raytracing", rtr_shadow, false);
CONSOLE_BOOL_VAL("raytracing", rtr_use_csm, true);
static constexpr float threshold_64 = 0.038, threshold_16 = 0.017, threshold_4 = 0;

// Context ID with the same lifespan as WorldRenderer.
// Can't be stored in ECS, because then it'd be reset on level change.
static bvh::ContextId bvhRenderingContextId;
static BVHConnection *fxBvhConnection = nullptr;
static bool cablesChanged = false;

struct BvhHeightProvider final : public bvh::HeightProvider
{
  LandMeshManager *lmeshMgr = nullptr;
  bool embedNormals() const override final { return false; }
  void getHeight(void *data, const Point2 &origin, int cell_size, int cell_count) const override final
  {
    using TerrainVertex = Point3;

    Point3 *scratch = (Point3 *)data;

    for (int z = 0; z <= cell_count; ++z)
      for (int x = 0; x <= cell_count; ++x)
      {
        Point2 loc(x * cell_size, z * cell_size);

        TerrainVertex &v = scratch[z * (cell_count + 1) + x];
        v.x = loc.x;
        v.z = loc.y;

        if (!lmeshMgr->getHeight(loc + origin, v.y, nullptr))
          v.y = 0;
      }
  }
};
ECS_DECLARE_BOXED_TYPE(BvhHeightProvider);
ECS_REGISTER_BOXED_TYPE(BvhHeightProvider, nullptr);

struct RiGenVisibilityECS
{
  RiGenVisibility *visibility;
};
ECS_DECLARE_BOXED_TYPE(RiGenVisibilityECS);
ECS_REGISTER_BOXED_TYPE(RiGenVisibilityECS, nullptr);

namespace bvh
{
static void process_elem(const ShaderMesh::RElem &elem,
  MeshInfo &mesh_info,
  const ChannelParser &parser,
  const RenderableInstanceLodsResource::ImpostorParams *impostor_params,
  const RenderableInstanceLodsResource::ImpostorTextures *impostor_textures);
}

void initBVH()
{
  if (is_bvh_enabled())
  {
    bvh::init(bvh::process_elem);
    bvhRenderingContextId = bvh::create_context("Rendering",
      static_cast<bvh::Features>(bvh::Features::Terrain | bvh::Features::RIFull | bvh::Features::Fx | bvh::Features::Cable));
    bvh::connect_fx(bvhRenderingContextId, [](BVHConnection *connection) { fxBvhConnection = connection; });
    // TODO proper resolution setting support
    // TODO dynamic resolution support
    int w, h;
    d3d::get_screen_size(w, h);
    if (is_denoiser_enabled())
      denoiser::initialize(w, h);
    if (is_rtsm_enabled())
      rtsm::initialize(w, h, rtsm::RenderMode::Denoised, false);
    if (is_rtr_enabled())
    {
      rtr::set_classify_threshold(threshold_64, threshold_16, threshold_4);
      rtr::set_reflection_method(denoiser::ReflectionMethod::Reblur);
      rtr::initialize(true, true, 1.f);
    }
    if (is_rtao_enabled())
      rtao::initialize(true);
  }
}

void closeBVH()
{
  if (bvhRenderingContextId)
  {
    rtao::teardown();
    rtr::teardown();
    rtsm::turn_off();
    rtsm::teardown();
    denoiser::teardown();
    bvh::teardown(bvhRenderingContextId);
    bvh::teardown();
  }
}

ECS_TAG(render)
ECS_NO_ORDER
ECS_REQUIRE(eastl::false_type bvh__initialized)
static void setup_bvh_scene_es(const UpdateStageInfoBeforeRender &, BvhHeightProvider &bvh__heightProvider, bool &bvh__initialized)
{
  bvh__initialized = true;
  if (bvhRenderingContextId)
  {
    auto lmeshMgr = WRDispatcher::getLandMeshManager();
    if (lmeshMgr)
    {
      bvh__heightProvider.lmeshMgr = lmeshMgr;
      bvh::add_terrain(bvhRenderingContextId, &bvh__heightProvider);
    }
  }
}

ECS_TAG(render)
ECS_ON_EVENT(UnloadLevel)
ECS_REQUIRE(BvhHeightProvider bvh__heightProvider)
static void close_bvh_scene_es(const ecs::Event &)
{
  if (bvhRenderingContextId)
  {
    bvh::remove_terrain(bvhRenderingContextId);
    bvh::on_before_unload_scene(bvhRenderingContextId);
    bvh::on_unload_scene(bvhRenderingContextId);
  }
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
static void bvh_free_visibility_es(const ecs::Event &, RiGenVisibilityECS &bvh__rendinst_visibility)
{
  rendinst::destroyRIGenVisibility(bvh__rendinst_visibility.visibility);
  bvh__rendinst_visibility.visibility = nullptr;
}

template <typename Callable>
static void get_bvh_rigen_visibility_ecs_query(Callable c);
static RiGenVisibility *get_bvh_rigen_visibility()
{
  RiGenVisibility *visibility;
  get_bvh_rigen_visibility_ecs_query(
    [&](RiGenVisibilityECS bvh__rendinst_visibility) { visibility = bvh__rendinst_visibility.visibility; });
  return visibility;
}

static TMatrix4 get_bvh_culling_matrix(const Point3 &cameraPos)
{
  TMatrix viewItm;
  viewItm.setcol(0, {1, 0, 0});
  viewItm.setcol(1, {0, 0, 1});
  viewItm.setcol(2, {0, 1, 0});
  viewItm.setcol(3, Point3::x0z(cameraPos));
  TMatrix4 cullView = inverse(viewItm);
  float minHt, maxHt;
  WRDispatcher::getMinMaxZ(minHt, maxHt);
  TMatrix4 cullProj = matrix_ortho_lh_forward(8000, 8000, minHt, maxHt);
  TMatrix4 cullingViewProj = cullView * cullProj;
  return cullingViewProj;
}

void prepareFXForBVH(const Point3 &cameraPos) { acesfx::prepare_bvh_culling(get_bvh_culling_matrix(cameraPos)); }

bool is_bvh_enabled()
{
  // Allow for debugging in the ImGUI window, even with no render features
  constexpr bool isDebug = DAGOR_DBGLEVEL > 0;
  static bool isEnabled =
    isDebug && bvh::is_available() && dgs_get_settings()->getBlockByNameEx("graphics")->getBool("enableBVH", false);
  return isEnabled || is_rtsm_enabled() || is_rtr_enabled() || is_rtao_enabled() || is_rt_water_enabled();
}

bool is_rtsm_enabled()
{
  static bool isEnabled = [] {
    const char *rtsmSetting = dgs_get_settings()->getBlockByNameEx("graphics")->getStr("enableRTSM", "off");
    return bvh::is_available() && (strcmp(rtsmSetting, "sun_and_dynamic") == 0 || strcmp(rtsmSetting, "sun") == 0);
  }();
  return isEnabled;
}

bool is_rtr_enabled()
{
  static bool isEnabled = bvh::is_available() && dgs_get_settings()->getBlockByNameEx("graphics")->getBool("enableRTR", false);
  return isEnabled;
}

bool is_rtao_enabled()
{
  static bool isEnabled = bvh::is_available() && dgs_get_settings()->getBlockByNameEx("graphics")->getBool("enableRTAO", false);
  return isEnabled;
}

bool is_rt_water_enabled()
{
  static bool isEnabled = bvh::is_available() && dgs_get_settings()->getBlockByNameEx("graphics")->getBool("RTRWater", false);
  return isEnabled;
}

bool is_denoiser_enabled() { return is_rtsm_enabled() || is_rtr_enabled() || is_rtao_enabled(); }

void draw_rtr_validation() { rtr::render_validation_layer(); }

void bvh_cables_changed() { cablesChanged = true; }

static void update_fx_for_bvh()
{
  TIME_D3D_PROFILE(bvh_fx)
  d3d::set_render_target(Driver3dRenderTarget{});
  fxBvhConnection->prepare();
  dafx::render(acesfx::get_dafx_context(), acesfx::get_cull_bvh_id(), render_tags[ERT_TAG_BVH], 0,
    [](TEXTUREID id) { fxBvhConnection->textureUsed(id); });
  fxBvhConnection->done();
}

static dabfg::NodeHandle makeBVHUpdateNode()
{
  if (!is_bvh_enabled())
    return {};
  return dabfg::register_node("bvh_update", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.executionHas(dabfg::SideEffects::External);
    registry.createBlob<OrderingToken>("bvh_ready_token", dabfg::History::No);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.readBlob<OrderingToken>("acesfx_update_token");
    auto rtWaterHndl = registry.createBlob<int>("water_rt_enabled", dabfg::History::No).handle();
    return [cameraHndl, rtWaterHndl]() {
      rtWaterHndl.ref() = is_rt_water_enabled();
      RiGenVisibility *visibility = get_bvh_rigen_visibility();
      Point3 cameraPos = Point3::xyz(cameraHndl.ref().cameraWorldPos);
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      // TODO Make ALL of this multi-threaded
      bvh::start_frame();
      bvh::process_meshes(bvhRenderingContextId);
      bvh::update_terrain(bvhRenderingContextId, Point2::xz(cameraPos));
      bvh::prepare_ri_extra_instances();
      Frustum frustum = get_bvh_culling_matrix(cameraPos);
      rendinst::prepareRIGenVisibility(frustum, cameraPos, visibility, false, nullptr);
      bvh::update_instances(bvhRenderingContextId, cameraPos, cameraHndl.ref().jitterFrustum, nullptr, nullptr, visibility);
      update_fx_for_bvh();
      if (auto cables = get_cables_mgr())
      {
        if (cablesChanged)
        {
          static int globalFrameBlockId = ShaderGlobal::getBlockId("global_frame");
          FRAME_LAYER_GUARD(globalFrameBlockId);
          bvh::on_cables_changed(cables, bvhRenderingContextId);
          cablesChanged = false;
        }
      }
      bvh::build(bvhRenderingContextId, cameraHndl.ref().viewItm, cameraHndl.ref().jitterProjTm, cameraPos, Point3::ZERO);
      // TODO: data race, because there is no order with daskies::prepare
      bvh::finalize_async_atmosphere_update(bvhRenderingContextId);
      auto callback = [](const Point3 &view_pos, const Point3 &view_dir, float d, Color3 &insc, Color3 &loss) {
        get_daskies()->getCpuFogSingle(view_pos, view_dir, d, insc, loss);
      };
      bvh::start_async_atmosphere_update(bvhRenderingContextId, cameraPos, callback);
    };
  });
}

static dabfg::NodeHandle makeDenoiserPrepareNode()
{
  if (!is_denoiser_enabled())
    return {};
  return dabfg::register_node("denoiser_prepare", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.executionHas(dabfg::SideEffects::External);
    registry.createBlob<OrderingToken>("denoiser_ready_token", dabfg::History::No);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto prevCameraHndl = registry.readBlobHistory<CameraParams>("current_camera").handle();
    read_gbuffer(registry, dabfg::Stage::PS_OR_CS);
    auto motionVecsHndl = read_gbuffer_motion(registry, dabfg::Stage::PS_OR_CS).handle();
    read_gbuffer_depth(registry, dabfg::Stage::PS_OR_CS);
    auto downsampledMotionVectorsHndl = registry.read("downsampled_motion_vectors_tex")
                                          .texture()
                                          .atStage(dabfg::Stage::PS_OR_CS)
                                          .useAs(dabfg::Usage::SHADER_RESOURCE)
                                          .handle();
    auto downsampledNormalsHndl =
      registry.readTexture("downsampled_normals").atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
    registry.read("close_depth").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("downsampled_close_depth_tex");

    return [cameraHndl, prevCameraHndl, motionVecsHndl, downsampledMotionVectorsHndl, downsampledNormalsHndl]() {
      denoiser::FrameParams params;
      params.viewPos = cameraHndl.ref().viewItm.getcol(3);
      params.prevViewPos = prevCameraHndl.ref().viewItm.getcol(3);
      params.viewDir = cameraHndl.ref().viewItm.getcol(2);
      params.prevViewDir = prevCameraHndl.ref().viewItm.getcol(2);
      params.viewItm = cameraHndl.ref().viewItm;
      params.prevViewItm = prevCameraHndl.ref().viewItm;
      params.projTm = cameraHndl.ref().noJitterProjTm;
      params.prevProjTm = prevCameraHndl.ref().noJitterProjTm;
      params.jitter = Point2(cameraHndl.ref().jitterPersp.ox, cameraHndl.ref().jitterPersp.oy);
      params.prevJitter = Point2(prevCameraHndl.ref().jitterPersp.ox, prevCameraHndl.ref().jitterPersp.oy);
      params.motionMultiplier = Point3::ONE;
      params.motionVectors = motionVecsHndl.view().getTex2D();
      params.halfMotionVectors = downsampledMotionVectorsHndl.view().getTex2D();
      params.halfNormals = {downsampledNormalsHndl.view().getTex2D(), downsampledNormalsHndl.view().getTexId()};
      denoiser::prepare(params);
    };
  });
}

static ShaderVariableInfo sun_dir_for_shadowsVarId = ShaderVariableInfo("sun_dir_for_shadows", true);
static ShaderVariableInfo from_sun_directionVarId = ShaderVariableInfo("from_sun_direction");

static dabfg::NodeHandle makeRTSMNode()
{
  if (!is_rtsm_enabled())
    return {};
  return dabfg::register_node("rtsm", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.executionHas(dabfg::SideEffects::External);
    registry.readBlob<OrderingToken>("bvh_ready_token");
    registry.readBlob<OrderingToken>("denoiser_ready_token");
    registry.createBlob<OrderingToken>("rtsm_token", dabfg::History::No);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto viewPosHndl = registry.readBlob<Point4>("world_view_pos").handle();
    read_gbuffer(registry, dabfg::Stage::PS_OR_CS);
    read_gbuffer_motion(registry, dabfg::Stage::PS_OR_CS);
    read_gbuffer_depth(registry, dabfg::Stage::PS_OR_CS);
    return [cameraHndl, viewPosHndl]() {
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      ShaderGlobal::set_color4(sun_dir_for_shadowsVarId, ShaderGlobal::get_color4(from_sun_directionVarId)); // For WT compatibility
      rtsm::render(bvhRenderingContextId, Point3::xyz(viewPosHndl.ref()), cameraHndl.ref().jitterProjTm);
    };
  });
}

static dabfg::NodeHandle makeRTRNode()
{
  if (!is_rtr_enabled())
    return {};
  return dabfg::register_node("rtr", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.executionHas(dabfg::SideEffects::External);
    registry.readBlob<OrderingToken>("bvh_ready_token");
    registry.readBlob<OrderingToken>("denoiser_ready_token");
    registry.createBlob<OrderingToken>("rtr_token", dabfg::History::No);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    read_gbuffer(registry, dabfg::Stage::PS_OR_CS);
    read_gbuffer_motion(registry, dabfg::Stage::PS_OR_CS);
    read_gbuffer_depth(registry, dabfg::Stage::PS_OR_CS);
    auto closeDepthHndl =
      registry.read("close_depth").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("downsampled_close_depth_tex").handle();
    return [cameraHndl, closeDepthHndl]() {
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      rtr::render(bvhRenderingContextId, cameraHndl.ref().jitterProjTm, rtr_shadow, rtr_use_csm, false,
        closeDepthHndl.view().getTexId());
    };
  });
}

static dabfg::NodeHandle makeRTAONode()
{
  if (!is_rtao_enabled())
    return {};
  return dabfg::register_node("rtao", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.executionHas(dabfg::SideEffects::External);
    registry.readBlob<OrderingToken>("bvh_ready_token");
    registry.readBlob<OrderingToken>("denoiser_ready_token");
    registry.createBlob<OrderingToken>("rtao_token", dabfg::History::No);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    read_gbuffer(registry, dabfg::Stage::PS_OR_CS);
    read_gbuffer_motion(registry, dabfg::Stage::PS_OR_CS);
    read_gbuffer_depth(registry, dabfg::Stage::PS_OR_CS);
    auto closeDepthHndl =
      registry.read("close_depth").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("downsampled_close_depth_tex").handle();
    return [cameraHndl, closeDepthHndl]() {
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      rtao::render(bvhRenderingContextId, cameraHndl.ref().jitterProjTm, true, closeDepthHndl.view().getTexId());
    };
  });
}

static ShaderVariableInfo water_rt_frame_indexVarId = ShaderVariableInfo("water_rt_frame_index", true);

const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_RT_NODE_NAMES = {
  "water_rt_early_before_envi_node", "water_rt_early_after_envi_node", "water_rt_late_node"};

dabfg::NodeHandle makeWaterRTNode(WaterRenderMode mode)
{
  if (!is_rt_water_enabled())
    return {};
  const uint32_t modeIdx = eastl::to_underlying(mode);
  return dabfg::register_node(WATER_RT_NODE_NAMES[modeIdx], DABFG_PP_NODE_SRC, [mode, modeIdx](dabfg::Registry registry) {
    registry.readBlob<OrderingToken>("bvh_ready_token");
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");

    auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();

    registry.read(WATER_REFLECT_DIR_TEX[modeIdx]).texture().atStage(dabfg::Stage::CS).bindToShaderVar("water_reflect_dir");
    registry.modify(WATER_SSR_COLOR_TEX[modeIdx + 1]).texture().atStage(dabfg::Stage::CS).bindToShaderVar("water_reflection_tex_uav");
    registry.modify(WATER_SSR_STRENGTH_TEX[modeIdx + 1])
      .texture()
      .atStage(dabfg::Stage::CS)
      .bindToShaderVar("water_reflection_strength_tex_uav");

    registry.historyFor(WATER_SSR_COLOR_TEX[eastl::to_underlying(WaterRenderMode::COUNT)])
      .texture()
      .atStage(dabfg::Stage::CS)
      .bindToShaderVar("water_reflection_tex");
    registry.historyFor(WATER_SSR_STRENGTH_TEX[eastl::to_underlying(WaterRenderMode::COUNT)])
      .texture()
      .atStage(dabfg::Stage::CS)
      .bindToShaderVar("water_reflection_strength_tex");
    registry.read("water_ssr_linear_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("water_reflection_tex_samplerstate");

    registry.read(WATER_SSR_DEPTH_TEX[modeIdx + 1]).texture().atStage(dabfg::Stage::CS).bindToShaderVar("downsampled_depth");

    auto resHndl = registry.getResolution<2>("main_view", 0.5f);

    return [mode, waterModeHndl, resHndl, cameraHndl, water_rt = Ptr(new_compute_shader("raytraced_water_reflections")),
             rtr_shadowVarId = get_shader_variable_id("rtr_shadow"), rtr_use_csmVarId = get_shader_variable_id("rtr_use_csm")]() {
      if (waterModeHndl.ref() != mode)
        return;
      static int frameIdx = 0;
      ShaderGlobal::set_int(water_rt_frame_indexVarId, (frameIdx++) % 32);
      ShaderGlobal::set_color4(sun_dir_for_shadowsVarId, ShaderGlobal::get_color4(from_sun_directionVarId)); // For WT compatibility
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      ShaderGlobal::set_int(rtr_shadowVarId, rtr_shadow ? 1 : 0);
      ShaderGlobal::set_int(rtr_use_csmVarId, rtr_use_csm ? 1 : 0);
      IPoint2 res = resHndl.get();
      bvh::bind_resources(bvhRenderingContextId, res.x);
      rtr::set_water_params();
      water_rt->dispatchThreads(res.x, res.y, 1);
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void init_bvh_scene_es(const ecs::Event &,
  dabfg::NodeHandle &bvh__update_node,
  dabfg::NodeHandle &denoiser_prepare_node,
  dabfg::NodeHandle &rtsm_node,
  dabfg::NodeHandle &rtr_node,
  dabfg::NodeHandle &rtao_node,
  dabfg::NodeHandle &water_rt_early_before_envi_node,
  dabfg::NodeHandle &water_rt_early_after_envi_node,
  dabfg::NodeHandle &water_rt_late_node,
  RiGenVisibilityECS &bvh__rendinst_visibility)
{
  bvh__update_node = makeBVHUpdateNode();
  denoiser_prepare_node = makeDenoiserPrepareNode();
  rtsm_node = makeRTSMNode();
  rtr_node = makeRTRNode();
  rtao_node = makeRTAONode();
  water_rt_early_before_envi_node = makeWaterRTNode(WaterRenderMode::EARLY_BEFORE_ENVI);
  water_rt_early_after_envi_node = makeWaterRTNode(WaterRenderMode::EARLY_AFTER_ENVI);
  water_rt_late_node = makeWaterRTNode(WaterRenderMode::LATE);
  bvh__rendinst_visibility.visibility = rendinst::createRIGenVisibility(midmem);
}

namespace bvh
{
static void process_elem(const ShaderMesh::RElem &elem,
  MeshInfo &mesh_info,
  const ChannelParser &parser,
  const RenderableInstanceLodsResource::ImpostorParams *impostor_params,
  const RenderableInstanceLodsResource::ImpostorTextures *impostor_textures)
{
  G_UNUSED(parser);
  static int atestVarId = get_shader_variable_id("atest");
  static int paint_detailsVarId = get_shader_variable_id("paint_details");
  static int palette_indexVarId = get_shader_variable_id("palette_index");
  static int use_paintingVarId = get_shader_variable_id("use_painting");
  static int paint_palette_rowVarId = get_shader_variable_id("paint_palette_row", true);
  static int detailsTileVarId = get_shader_variable_id("details_tile");
  static int atlas_tile_uVarId = get_shader_variable_id("atlas_tile_u");
  static int atlas_tile_vVarId = get_shader_variable_id("atlas_tile_v");
  static int atlas_first_tileVarId = get_shader_variable_id("atlas_first_tile");
  static int atlas_last_tileVarId = get_shader_variable_id("atlas_last_tile");
  static int detail0_const_colorVarId = get_shader_variable_id("detail0_const_color");
  static int paint_const_colorVarId = get_shader_variable_id("paint_const_color");
  static int is_rendinst_clipmapVarId = get_shader_variable_id("is_rendinst_clipmap");

  int isClipmap = 0;
  elem.mat->getIntVariable(is_rendinst_clipmapVarId, isClipmap);
  bool isTree = strncmp(elem.mat->getShaderClassName(), "rendinst_tree", 13) == 0;
  bool isLayered = strncmp(elem.mat->getShaderClassName(), "rendinst_layered", 16) == 0 ||
                   strncmp(elem.mat->getShaderClassName(), "dynamic_layered", 15) == 0;

  bool hasAlphaTest = false;
  TEXTUREID alphaTextureId = BAD_TEXTUREID;

  int alphaTestValue = 0;
  if (elem.mat->getIntVariable(atestVarId, alphaTestValue) && alphaTestValue)
  {
    hasAlphaTest = true;
    if (isTree)
      alphaTextureId = elem.mat->get_texture(1);
  }

  if (impostor_textures)
    hasAlphaTest = true;

  int use_paintingValue = 0;
  elem.mat->getIntVariable(use_paintingVarId, use_paintingValue);
  int palette_indexValue = 0;
  elem.mat->getIntVariable(palette_indexVarId, palette_indexValue);

  int paint_palette_rowValue = 1;
  float paint_details_strength = 0;
  if (elem.mat->getIntVariable(paint_palette_rowVarId, paint_palette_rowValue)) // INIT_SIMPLE_PAINTED
  {
    float paint_detailsValue = 0;
    elem.mat->getRealVariable(paint_detailsVarId, paint_detailsValue);
    paint_details_strength = paint_detailsValue;
    // TODO handle paint_scale_bias_mult_details parameters too
  }
  else
  {
    Color4 paint_detailsValue = Color4(1, 1, 1);
    elem.mat->getColor4Variable(paint_detailsVarId, paint_detailsValue);
    paint_palette_rowValue = paint_detailsValue.a;
    paint_details_strength = paint_detailsValue.r;
  }

  // Only one of these is used at the same time, the other will just fail
  Color4 colorOverride = Color4(0.5, 0.5, 0.5, 0);
  elem.mat->getColor4Variable(detail0_const_colorVarId, colorOverride);
  elem.mat->getColor4Variable(paint_const_colorVarId, colorOverride);

  float atlas_tile_uValue = 1;
  float atlas_tile_vValue = 1;
  int atlas_first_tileValue = 0;
  int atlas_last_tileValue = 0;
  if (isLayered)
  {
    elem.mat->getRealVariable(atlas_tile_uVarId, atlas_tile_uValue);
    elem.mat->getRealVariable(atlas_tile_vVarId, atlas_tile_vValue);
    elem.mat->getIntVariable(atlas_first_tileVarId, atlas_first_tileValue);
    elem.mat->getIntVariable(atlas_last_tileVarId, atlas_last_tileValue);

    G_ASSERT(atlas_first_tileValue >= 0);
    G_ASSERT(atlas_last_tileValue >= atlas_first_tileValue);
  }

  bool isTwoSided = impostor_textures || (elem.mat->get_flags() & (SHFLG_2SIDED | SHFLG_REAL2SIDED));


  if (isClipmap)
    mesh_info.isClipmap = true;

  if (use_paintingValue > 0)
  {
    mesh_info.painted = true;
    mesh_info.paintData = Point4(paint_palette_rowValue, palette_indexValue, 0, paint_details_strength);
  }
  mesh_info.colorOverride = Point4::rgba(colorOverride);

  if (isLayered)
  {
    mesh_info.useAtlas = true;
    mesh_info.atlasTileU = atlas_tile_uValue;
    mesh_info.atlasTileV = atlas_tile_vValue;
    mesh_info.atlasFirstTile = atlas_first_tileValue;
    mesh_info.atlasLastTile = atlas_last_tileValue;

    mesh_info.texcoordOffset = mesh_info.secTexcoordOffset;
  }
  Color4 detailsTile(1, 1, 1, 0);
  if (elem.mat->getColor4Variable(detailsTileVarId, detailsTile))
    mesh_info.texcoordScale = detailsTile.r;

  if (impostor_params)
  {
    mesh_info.isImpostor = true;
    mesh_info.impostorHeightOffset = impostor_params->boundingSphere.y;
    mesh_info.impostorSliceTm1 = impostor_params->perSliceParams[2].sliceTm;
    mesh_info.impostorSliceTm2 = impostor_params->perSliceParams[0].sliceTm;
    mesh_info.impostorSliceClippingLines1 = impostor_params->perSliceParams[2].clippingLines;
    mesh_info.impostorSliceClippingLines2 = impostor_params->perSliceParams[0].clippingLines;
    mesh_info.impostorScale = impostor_params->scale;
    memcpy(mesh_info.impostorOffsets, impostor_params->vertexOffsets.begin(), sizeof(mesh_info.impostorOffsets));
  }

  mesh_info.albedoTextureId = impostor_textures ? impostor_textures->albedo_alpha : elem.mat->get_texture(0);
  mesh_info.normalTextureId = elem.mat->get_texture(2);
  mesh_info.alphaTextureId = alphaTextureId;
  mesh_info.alphaTest = hasAlphaTest;
  mesh_info.enableCulling = !isTwoSided;
}
} // namespace bvh
