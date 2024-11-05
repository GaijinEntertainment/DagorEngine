// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include "render/renderEvent.h"
#include <render/renderSettings.h>
#include "render/skies.h"
#include <render/daBfg/ecs/frameGraphNode.h>

#define INSIDE_RENDERER 1
#include "render/world/private_worldRenderer.h"
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_matricesAndPerspective.h>

#define GLOBAL_VARS_LIST            \
  VAR(distant_haze_geometry_slices) \
  VAR(distant_haze_minh)            \
  VAR(distant_haze_maxh)            \
  VAR(distant_haze_radius)          \
  VAR(distant_haze_fade_in)         \
  VAR(distant_haze_fade_out)        \
  VAR(distant_haze_height)          \
  VAR(distant_haze_density)         \
  VAR(distant_haze_strength)        \
  VAR(distant_haze_blur)            \
  VAR(distant_haze_speed)           \
  VAR(distant_haze_center)          \
  VAR(haze_scene_depth_tex)         \
  VAR(haze_scene_depth_tex_lod)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

static constexpr int geom_slices = 36;

template <typename Callable>
static void set_shader_params_ecs_query(Callable c);

template <typename Callable>
static void is_active_ecs_query(Callable c);

template <typename Callable>
inline void distant_haze_node_init_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
static void get_heat_haze_lod_ecs_query(Callable c);

struct DistantHazeManager
{
  ~DistantHazeManager() { tear_down(); }
  DistantHazeManager() {}
  DistantHazeManager(DistantHazeManager &&) = default;

  void init_for_level()
  {
    haze_shader.init("distant_haze", nullptr, 0, "distant_haze");
    if (!haze_shader.shader)
    {
      logerr("Failed to load shader for distant haze!");
      tear_down();
      return;
    }
#define VAR(a) a##VarId = get_shader_variable_id(#a);
    GLOBAL_VARS_LIST
#undef VAR
  }

private:
  void tear_down() { haze_shader.close(); }

public:
  void render(
    float min_terrain_height, float max_terrain_height, const Point3 &camera_position, TEXTUREID depth_tex_id, int depth_tex_lod) const
  {

    if (!haze_shader.shader)
      return;

    d3d::settm(TM_WORLD, TMatrix::IDENT);

    set_shader_params_ecs_query([min_terrain_height, max_terrain_height, camera_position](bool distant_haze__is_center_fixed,
                                  Point2 distant_haze__center, float distant_haze__radius, float distant_haze__fade_in_bottom,
                                  float distant_haze__fade_out_top, float distant_haze__total_height, float distant_haze__size,
                                  float distant_haze__strength, float distant_haze__blur, Point3 distant_haze__speed) {
      float hazeMul = get_daskies() ? get_daskies()->getHazeStrength() : 1;

      ShaderGlobal::set_int(distant_haze_geometry_slicesVarId, geom_slices);
      ShaderGlobal::set_real(distant_haze_minhVarId, min_terrain_height);
      ShaderGlobal::set_real(distant_haze_maxhVarId, max_terrain_height);
      ShaderGlobal::set_real(distant_haze_radiusVarId, distant_haze__radius);
      ShaderGlobal::set_real(distant_haze_fade_inVarId, distant_haze__fade_in_bottom);
      ShaderGlobal::set_real(distant_haze_fade_outVarId, distant_haze__fade_out_top);
      ShaderGlobal::set_real(distant_haze_heightVarId, distant_haze__total_height);
      ShaderGlobal::set_real(distant_haze_densityVarId, distant_haze__size);
      ShaderGlobal::set_real(distant_haze_strengthVarId, distant_haze__strength * hazeMul);
      ShaderGlobal::set_real(distant_haze_blurVarId, distant_haze__blur * hazeMul);
      ShaderGlobal::set_color4(distant_haze_speedVarId, distant_haze__speed);
      ShaderGlobal::set_color4(distant_haze_centerVarId,
        distant_haze__is_center_fixed ? distant_haze__center : Point2(camera_position.x, camera_position.z));
    });

    ShaderGlobal::set_texture(haze_scene_depth_texVarId, depth_tex_id);
    ShaderGlobal::set_real(haze_scene_depth_tex_lodVarId, depth_tex_lod);

    haze_shader.shader->setStates();
    d3d_err(d3d::draw(PRIM_TRISTRIP, 0, geom_slices * 2));
  }

  bool is_active() const
  {
    bool active = false;

    is_active_ecs_query(
      [&active](ECS_REQUIRE(bool distant_haze__is_center_fixed, Point2 distant_haze__center, float distant_haze__radius,
        float distant_haze__fade_in_bottom, float distant_haze__fade_out_top, float distant_haze__total_height,
        float distant_haze__size, float distant_haze__strength, float distant_haze__blur,
        Point3 distant_haze__speed) float distant_haze__strength) { active = distant_haze__strength > 0; });
    return active;
  }

private:
  DynamicShaderHelper haze_shader;
};

ECS_DECLARE_BOXED_TYPE(DistantHazeManager);
ECS_REGISTER_BOXED_TYPE(DistantHazeManager, nullptr);
ECS_AUTO_REGISTER_COMPONENT(DistantHazeManager, "distant_haze__manager", nullptr, 0);

ECS_TAG(render)
ECS_ON_EVENT(OnLevelLoaded)
static void init_distant_haze_manager_es_event_handler(const ecs::Event &, DistantHazeManager &distant_haze__manager)
{
  distant_haze__manager.init_for_level();
}

enum class DistantHazeNodeType
{
  Normal,
  Color
};

template <DistantHazeNodeType type>
dabfg::NodeHandle createDistantHazeNode(DistantHazeManager &distant_haze__manager, int depthlod)
{
  const char *nodeName =
    type == DistantHazeNodeType::Normal ? "heat_haze_render_distant_haze_node" : "heat_haze_render_distant_haze_node_color";

  return dabfg::register_node(nodeName, DABFG_PP_NODE_SRC, [&distant_haze__manager, depthlod](dabfg::Registry registry) {
    registry.orderMeAfter("heat_haze_render_particles_node");

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto hazeRenderedHndl = registry.modifyBlob<bool>("haze_rendered").handle();

    registry.requestRenderPass().depthRw("haze_depth").color({(type == DistantHazeNodeType::Normal) ? "haze_offset" : "haze_color"});

    auto depthForTransparencyHndl =
      registry.readTexture("depth_for_transparency").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    auto farDownsampledDepthHndl =
      registry.readTexture("far_downsampled_depth").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    return [&distant_haze__manager, cameraHndl, hazeRenderedHndl, depthForTransparencyHndl, farDownsampledDepthHndl, depthlod] {
      if (!distant_haze__manager.is_active())
        return;

      // TODO: rid off the WorldRender dependecy here.
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      Point3 cameraPosition = cameraHndl.ref().cameraWorldPos;
      float heightUnderCamera;

      // TODO: definitly bad practice to have WorldRender dependecy here.
      if (!wr.getLandHeight(Point2::xz(cameraPosition), heightUnderCamera, nullptr))
        return;

      if (cameraPosition.y - heightUnderCamera > 30)
        return;

      hazeRenderedHndl.ref() = true;

      float minH, maxH;
      wr.getMinMaxZ(minH, maxH); // TODO: get rid of wr dependecy here.

      G_ASSERT(depthlod >= 0);
      const auto depthTex = depthlod == 0 ? depthForTransparencyHndl.view() : farDownsampledDepthHndl.view();

      // TODO: Don't store this logic in manager
      distant_haze__manager.render(minH, maxH, cameraPosition, depthTex.getTexId(), eastl::max(depthlod - 1, 0));
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__haze)
ECS_REQUIRE(bool render_settings__haze)
static void distant_haze_settings_tracking_es(const ecs::Event &, bool render_settings__haze)
{
  int depthlod = -1;
  get_heat_haze_lod_ecs_query([&depthlod](int heat_haze__lod) { depthlod = heat_haze__lod; });

  distant_haze_node_init_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("distant_haze_manager")),
    [&](DistantHazeManager &distant_haze__manager, dabfg::NodeHandle &distant_haze__node,
      dabfg::NodeHandle &distant_haze__color_node) {
      distant_haze__node = {};
      distant_haze__color_node = {};

      if (!render_settings__haze)
        return;

      const bool isColoredHazeSuppored = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("coloredHaze", false);

      distant_haze__node = createDistantHazeNode<DistantHazeNodeType::Normal>(distant_haze__manager, depthlod);
      if (isColoredHazeSuppored)
        distant_haze__color_node = createDistantHazeNode<DistantHazeNodeType::Color>(distant_haze__manager, depthlod);
    });
}
