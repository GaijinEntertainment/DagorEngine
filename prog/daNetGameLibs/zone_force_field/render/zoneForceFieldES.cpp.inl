// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "zoneForceField.h"

#include <3d/dag_lockSbuffer.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <ecs/render/shaderVar.h>
#include <ecs/render/updateStageRender.h>
#include "render/renderEvent.h"
#include <render/primitiveObjects.h>
#include "main/level.h"
#include <shaders/dag_shaderBlock.h>
#include <perfMon/dag_statDrv.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/postfxRender.h>
#include <render/world/wrDispatcher.h>
#include <math/dag_mathUtils.h>

#include "render/world/global_vars.h"
#include <util/dag_convar.h>
#include <util/dag_stdint.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>

ECS_REGISTER_BOXED_TYPE(ZoneForceFieldRenderer, nullptr);

CONSOLE_FLOAT_VAL_MINMAX("force_field", radius_offset, 0.02, 0, 1);

static int forcefield_bilateral_upscale_enabledVarId = -1;

static CompiledShaderChannelId forcefield_channels[] = {{SCTYPE_FLOAT3, SCUSAGE_POS, 0, 0}};

template <typename Callable>
inline void zone_force_field_render_ecs_query(ecs::EntityManager &manager, Callable fn);
template <typename Callable>
inline void local_player_immune_to_forcefield_ecs_query(ecs::EntityManager &manager, Callable fn);
template <typename Callable>
inline void update_transparent_partition_ecs_query(ecs::EntityManager &manager, Callable fn);

ECS_TAG(render)
ECS_NO_ORDER
static void gather_spheres_es_event_handler(
  const UpdateStageInfoBeforeRender &evt, ecs::EntityManager &manager, ZoneForceFieldRenderer &zone_force_field)
{
  zone_force_field.gatherForceFields(evt.viewItm, evt.mainCullingFrustum, nullptr);

  PartitionSphere closestSphere = zone_force_field.getClosestForceField(evt.camPos);
  update_transparent_partition_ecs_query(manager, [&](PartitionSphere &partition_sphere) { partition_sphere = closestSphere; });
}


ZoneForceFieldRenderer::~ZoneForceFieldRenderer()
{
  shaders::overrides::destroy(zonesInState);
  shaders::overrides::destroy(zonesOutState);
}

ZoneForceFieldRenderer::ZoneForceFieldRenderer()
{
  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_CLAMP_ENABLED);
  zonesInState = shaders::overrides::create(state);
  state.set(shaders::OverrideState::FLIP_CULL);
  zonesOutState = shaders::overrides::create(state);

  bilateral_upscale = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("forcefield_bilateral_upscale_enabled", true);

  forcefield_bilateral_upscale_enabledVarId = get_shader_variable_id("forcefield_bilateral_upscale_enabled", true);
  if (VariableMap::isGlobVariablePresent(forcefield_bilateral_upscale_enabledVarId))
  {
    ShaderGlobal::set_int(forcefield_bilateral_upscale_enabledVarId, (int)bilateral_upscale);
  }

  calc_sphere_vertex_face_count(SLICES, SLICES, false, v_count, f_count);
  sphereVb = dag::create_vb(v_count * sizeof(Point3), 0, "force_field_vertices");
  sphereIb = dag::create_ib(f_count * 6, 0, "force_field_indices");
  fillBuffers();
}

void ZoneForceFieldRenderer::fillBuffers()
{
  auto indices = lock_sbuffer<uint8_t>(sphereIb.getBuf(), 0, 6 * f_count, VBLOCK_WRITEONLY);
  auto vertices = lock_sbuffer<uint8_t>(sphereVb.getBuf(), 0, v_count * sizeof(Point3), VBLOCK_WRITEONLY);
  if (!indices || !vertices)
    return;

  create_sphere_mesh(dag::Span<uint8_t>(vertices.get(), v_count * sizeof(Point3)), dag::Span<uint8_t>(indices.get(), f_count * 6),
    1.0f, SLICES, SLICES, sizeof(Point3), false, false, false, false);
}

dafg::NodeHandle ZoneForceFieldRenderer::createRenderingNode(const char *rendering_shader_name, uint32_t render_target_fmt)
{
  auto nodeNs = dafg::root() / "transparent" / "middle";
  return nodeNs.registerNode("zone_renderer", DAFG_PP_NODE_SRC,
    [this, rendering_shader_name, render_target_fmt](dafg::Registry registry) {
      if (
        !forceFieldShader.init(rendering_shader_name, forcefield_channels, countof(forcefield_channels), rendering_shader_name, true))
        logerr("Forcefield rendering shader initialization failed");

      registry.allowAsyncPipelines();

      auto lowResFfHndl =
        registry.create("low_res_forcefield")
          .texture(dafg::Texture2dCreateInfo{render_target_fmt | TEXCF_RTARGET, registry.getResolution<2>("main_view", .5f), 1})
          .atStage(dafg::Stage::POST_RASTER)
          .useAs(dafg::Usage::COLOR_ATTACHMENT)
          .handle();

      // To do this through FG render passes, we need to support clears
      eastl::optional<dafg::VirtualResourceHandle<const Texture, true, false>> maybeDownsampledDepthHndl;

      maybeDownsampledDepthHndl = registry.read("downsampled_depth_with_early_after_envi_water")
                                    .texture()
                                    .atStage(dafg::Stage::PS)
                                    .useAs(dafg::Usage::DEPTH_ATTACHMENT)
                                    .handle();


      // shader does not use half_res_depth_tex on mobile

      if (bilateral_upscale && renderer_has_feature(FeatureRenderFlags::FULL_DEFERRED))
      {
        registry.read("checkerboard_depth").texture().atStage(dafg::Stage::PS).bindToShaderVar("downsampled_checkerboard_depth_tex");
        registry.read("checkerboard_depth_sampler")
          .blob<d3d::SamplerHandle>()
          .bindToShaderVar("downsampled_checkerboard_depth_tex_samplerstate");
      }
      else
      {
        registry.read("far_downsampled_depth").texture().atStage(dafg::Stage::PS).bindToShaderVar("downsampled_far_depth_tex");
        registry.read("far_downsampled_depth_sampler")
          .blob<d3d::SamplerHandle>()
          .bindToShaderVar("downsampled_far_depth_tex_samplerstate");
      }

      registry.requestState().setFrameBlock("global_frame");

      auto camera = use_camera_in_camera(registry);
      auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

      return [this, cameraHndl, lowResFfHndl, maybeDownsampledDepthHndl](const dafg::multiplexing::Index &multiplexing_index) {
        camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref(), camera_in_camera::USE_STENCIL};

        TextureInfo info;
        lowResFfHndl.ref().getinfo(info);
        texHt = info.h;

        // Gather them again, in order to apply occlusion.
        Occlusion *occlusion = cameraHndl.ref().jobsMgr->getOcclusion();
        gatherForceFields(cameraHndl.ref().viewItm, cameraHndl.ref().jitterFrustum, occlusion);

        if (frameZones.empty())
          return;

        TIME_D3D_PROFILE(zone_force_field_rendering);

        ScopeRenderTarget scopeRT;

        d3d::set_render_target(lowResFfHndl.get(), 0);
        if (maybeDownsampledDepthHndl)
          d3d::set_depth(maybeDownsampledDepthHndl->view().getTex2D(), DepthAccess::SampledRO);
        d3d::clearview(CLEAR_TARGET, E3DCOLOR(0, 0, 0, 0), 1, 0);

        startZoneIn();
        render(frameZones.begin(), frameZones.size());
        if (!frameZonesOut.empty())
        {
          startZoneOut();
          render(frameZonesOut.begin(), frameZonesOut.size());
        }
        // as we are not applying force field right away, we must reset overrides by our own
        shaders::overrides::reset();
      };
    });
}

dafg::NodeHandle ZoneForceFieldRenderer::createApplyingNode(const char *applying_shader_name,
  const char *fullscreen_applying_shader_name)
{
  auto nodeNs = dafg::root() / "transparent" / "middle";
  return nodeNs.registerNode("zone_applier", DAFG_PP_NODE_SRC,
    [this, applying_shader_name, fullscreen_applying_shader_name](dafg::Registry registry) {
      // TODO: you are NOT supposed to init state within declaration callbacks.
      // It is better to stop storing this object inside the class and simply
      // store it inside the execution lambda's capture.
      if (!forceFieldApplierShader.init(applying_shader_name, forcefield_channels, countof(forcefield_channels), applying_shader_name,
            true))
        logerr("Forcefield applying shader initialization failed");

      forceFieldManyApplier.init(fullscreen_applying_shader_name);

      request_common_transparent_state(registry);

      registry.read("low_res_forcefield").texture().atStage(dafg::Stage::PS).bindToShaderVar("low_res_forcefield");
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.filter_mode = bilateral_upscale && renderer_has_feature(FeatureRenderFlags::FULL_DEFERRED) ? d3d::FilterMode::Point
                                                                                                           : d3d::FilterMode::Linear;
        registry.create("low_res_forcefield_sampler")
          .blob(d3d::request_sampler(smpInfo))
          .bindToShaderVar("low_res_forcefield_samplerstate");
      }

      registry.read("upscale_sampling_tex").texture().atStage(dafg::Stage::PS).bindToShaderVar("upscale_sampling_tex").optional();

      // Distortion is only used in some games
      (registry.root() / "transparent" / "far")
        .read("forcefield_distortion")
        .texture()
        .atStage(dafg::Stage::PS)
        .bindToShaderVar("forcefield_distortion_res_tex")
        .optional();
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
        registry.create("forcefield_distortion_sampler")
          .blob(d3d::request_sampler(smpInfo))
          .bindToShaderVar("forcefield_distortion_res_tex_samplerstate")
          .optional();
      }

      registry.requestState().setFrameBlock("global_frame");

      return [this](const dafg::multiplexing::Index &multiplexing_index) {
        if (frameZones.empty())
          return;

        TIME_D3D_PROFILE(zone_force_field_apply);

        const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

        if (frameZones.size() == 1)
          applyOne(frameZones[0], frameZonesOut.size() == 1);
        else
          applyMany();

        ShaderElement::invalidate_cached_state_block();
      };
    });
}

void ZoneForceFieldRenderer::start() const
{
  forceFieldShader.shader->setStates();
  d3d::setind(sphereIb.getBuf());
  d3d::setvsrc(0, sphereVb.getBuf(), sizeof(Point3));
}

void ZoneForceFieldRenderer::startZoneIn() const
{
  shaders::overrides::reset();
  shaders::overrides::set(zonesInState);
  start();
}

void ZoneForceFieldRenderer::startZoneOut() const
{
  shaders::overrides::reset();
  shaders::overrides::set(zonesOutState);
  start();
}

void ZoneForceFieldRenderer::render(const Point4 *sph, int count) const
{
  enum
  {
    MAX_INSTANCING = 16
  }; // limit amount of max spheres to 16
  static int forcefield_vs_spheres_const_no = ShaderGlobal::get_slot_by_name("forcefield_vs_spheres_const_no");
  for (int i = 0; i < count; i += MAX_INSTANCING, sph += MAX_INSTANCING)
  {
    int renderCount = min(count, (int)MAX_INSTANCING);
    d3d::set_vs_const(forcefield_vs_spheres_const_no, &sph->x, renderCount);
    d3d::drawind_instanced(PRIM_TRILIST, 0, f_count, 0, renderCount);
  }
}

void ZoneForceFieldRenderer::applyOne(const Point4 &sph, bool cull_ccw) const
{
  shaders::overrides::set(cull_ccw ? zonesOutState : zonesInState);
  static int forcefield_vs_sphere_const_no = ShaderGlobal::get_slot_by_name("forcefield_vs_sphere_const_no");
  d3d::set_vs_const(forcefield_vs_sphere_const_no, &sph.x, 1);
  forceFieldApplierShader.shader->setStates();
  d3d::setind(sphereIb.getBuf());
  d3d::setvsrc(0, sphereVb.getBuf(), sizeof(Point3));
  d3d::drawind(PRIM_TRILIST, 0, f_count, 0);

  shaders::overrides::reset();
}

void ZoneForceFieldRenderer::applyMany() const
{
  // todo: depth bounds
  forceFieldManyApplier.render();
}


void ZoneForceFieldRenderer::gatherForceFields(const TMatrix &view_itm, const Frustum &culling_frustum, const Occlusion *occlusion)
{
  TIME_PROFILE(zone_force_field_gathering);
  frameZones.clear();
  frameZonesOut.clear();

  bool isPlayerImmuneToForcefield = false;
  local_player_immune_to_forcefield_ecs_query(*g_entity_mgr, [&](ecs::EntityId possessed, bool is_local) {
    if (is_local)
      isPlayerImmuneToForcefield = g_entity_mgr->getOr(possessed, ECS_HASH("corruptionInvulnerability"), false);
  });

  zone_force_field_render_ecs_query(*g_entity_mgr,
    [&](const TMatrix &transform, float sphere_zone__radius, ShaderVar zone_pos_radius ECS_REQUIRE(ecs::Tag render_forcefield),
      uint32_t sphere_zone__slices = ZoneForceFieldRenderer::SLICES, uint32_t sphere_zone__stacks = ZoneForceFieldRenderer::SLICES,
      float forcefieldFog__fadeSlowness = 16.0f, float forcefieldFog__fadeLimit = 0.4f, bool forcefieldAppliesFog = false) {
      Point3 sphC = transform.getcol(3);
      if (forcefieldAppliesFog)
      {
        const bool hasVolumeLight = WRDispatcher::getVolumeLight();
        if (hasVolumeLight)
        {
          ShaderGlobal::set_float4(zone_pos_radius, sphC.x, sphC.y, sphC.z, sphere_zone__radius);
        }
        else
        {
          if (isPlayerImmuneToForcefield)
            set_fade_mul(1);
          else
          {
            float zoneDepth = length(sphC - view_itm.getcol(3)) - sphere_zone__radius;
            // start applying darkness effects if near zone or outside
            if (zoneDepth > 0.0f)
              zoneDepth = max(forcefieldFog__fadeLimit, 1.0f - (zoneDepth / forcefieldFog__fadeSlowness));
            else
              zoneDepth = 1.0f;

            set_fade_mul(zoneDepth);
          }
        }
      }
      if (!culling_frustum.testSphereB(sphC, sphere_zone__radius))
        return;
      Point4 zone = Point4(sphC.x, sphC.y, sphC.z, sphere_zone__radius);
      bool isCameraOutsideMesh =
        is_pos_outside_sphere_mesh(view_itm.getcol(3) - sphC, sphere_zone__radius, sphere_zone__slices, sphere_zone__stacks);
      if (isCameraOutsideMesh)
      {
        zone.w -= radius_offset.get();
        if (occlusion && !occlusion->isVisibleSphere(v_ldu(&sphC.x), v_splats(sphere_zone__radius), v_splats(0.25 / texHt)))
          return;
        frameZonesOut.push_back(zone);
      }
      else
        zone.w += radius_offset.get();
      frameZones.push_back(zone);
    });
}

PartitionSphere ZoneForceFieldRenderer::getClosestForceField(const Point3 &camera_pos) const
{
  PartitionSphere partitionSphere = {};

  auto it = eastl::min_element(frameZones.cbegin(), frameZones.cend(), [&camera_pos](const Point4 &lhs, const Point4 &rhs) {
    Point3 lhs_sph_pos = {lhs.x, lhs.y, lhs.z};
    Point3 rhs_sph_pos = {rhs.x, rhs.y, rhs.z};
    return (lhs_sph_pos - camera_pos).lengthSq() < (rhs_sph_pos - camera_pos).lengthSq();
  });
  if (it != frameZones.cend())
  {
    partitionSphere.sphere = *it;
    if (eastl::find(frameZonesOut.begin(), frameZonesOut.end(), *it) != frameZonesOut.end())
      partitionSphere.status = PartitionSphere::Status::CAMERA_OUTSIDE_SPHERE;
    else
      partitionSphere.status = PartitionSphere::Status::CAMERA_INSIDE_SPHERE;
  }

  partitionSphere.maxRadiusError = calc_sphere_max_radius_error(partitionSphere.sphere.w, SLICES, SLICES);
  return partitionSphere;
}

static void resetZoneShadervar()
{
  int zonePosRadVarId = get_shader_variable_id("zone_pos_radius", true);
  if (zonePosRadVarId >= 0)
    ShaderGlobal::set_float4(zonePosRadVarId, 0, 0, 0, 1e+06);
}

ECS_TAG(dngRenderIsActive)
static __forceinline void zone_reset_on_new_level_es_event_handler(const EventLevelLoaded &)
{
  // reset zone on level reload for volfog, so it has a big enough radius to have no effect
  resetZoneShadervar();

  const bool hasVolumeLight = WRDispatcher::getVolumeLight();
  if (!hasVolumeLight)
    set_fade_mul(1);
}
