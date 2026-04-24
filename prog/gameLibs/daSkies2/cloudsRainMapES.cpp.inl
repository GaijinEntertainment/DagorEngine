// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <ecs/render/updateStageRender.h>

#include <EASTL/unique_ptr.h>

#include <shaders/dag_computeShaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/daFrameGraph/daFG.h>
#include <3d/dag_resPtr.h>

#include <util/dag_console.h>


namespace var
{
static ShaderVariableInfo clouds_rain_map_render_offset_xz__scale__resolution("clouds_rain_map_render_offset_xz__scale__resolution");
static ShaderVariableInfo clouds_rain_map_offset_scale("clouds_rain_map_offset_scale");
static ShaderVariableInfo clouds_rain_map_valid("clouds_rain_map_valid");
static ShaderVariableInfo skies_world_view_pos("skies_world_view_pos");
} // namespace var

struct RainMapData
{
  dafg::NodeHandle rainMapNode;
  eastl::unique_ptr<ComputeShaderElement> clouds_rain_map_cs;
  float range = 1000;
  float texelSize = 100;
  float resolution = 20;
  float gridOffset = -950;
  float scale = 0.0005;
  bool isInit = false;

  void refreshData()
  {
    resolution = range * 2 / texelSize;
    gridOffset = 0.5 * texelSize - range;
    scale = 1 / (texelSize * resolution);
  }

  void createNode()
  {
    refreshData();

    ShaderGlobal::set_int(var::clouds_rain_map_valid, 1);

    rainMapNode = dafg::register_node("render_rain_map_node", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
      registry.createTexture2d("clouds_rain_map_tex", {TEXFMT_G16R16F | TEXCF_UNORDERED, IPoint2(resolution, resolution), 1})
        .atStage(dafg::Stage::CS)
        .bindToShaderVar("clouds_rain_map_tex");


      return [this]() {
        const Color4 &camPos = ShaderGlobal::get_float4(var::skies_world_view_pos);
        const Point2 gridAlignedCamPosXZ = Point2(roundf(camPos.r / texelSize) * texelSize, roundf(camPos.b / texelSize) * texelSize);

        ShaderGlobal::set_float4(var::clouds_rain_map_render_offset_xz__scale__resolution, gridAlignedCamPosXZ.x + gridOffset,
          gridAlignedCamPosXZ.y + gridOffset, texelSize, resolution);
        ShaderGlobal::set_float4(var::clouds_rain_map_offset_scale, 0.5 - gridAlignedCamPosXZ.x * scale,
          0.5 - gridAlignedCamPosXZ.y * scale, scale);

        clouds_rain_map_cs->dispatchThreads(resolution, resolution, 1);
      };
    });
  }

  void init(float map_range, float texel_size)
  {
    range = map_range;
    texelSize = texel_size;
    clouds_rain_map_cs.reset(new_compute_shader("clouds_rain_map_cs"));
    isInit = true;
    createNode();
  }

  void setRange(float map_range)
  {
    range = map_range;
    if (isInit)
      createNode();
  }

  void close()
  {
    rainMapNode = {};
    ShaderGlobal::set_int(var::clouds_rain_map_valid, 0);
  }
};

static RainMapData rainMapData;

template <typename Callable>
inline void volfog_quality_ecs_query(ecs::EntityManager &manager, Callable c);

// Todo: Refactor this to be the same function as volfog to avoid errors in the future
static float getRangeFromFogQuality(const ecs::string &render_settings__volumeFogQuality)
{
  if (render_settings__volumeFogQuality == "far" || render_settings__volumeFogQuality == "movie")
    return 5000.f;

  return 1000.f;
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void init_cloud_rain_map_es(const ecs::Event &, ecs::EntityManager &manager, bool clouds_rain_map,
  float clouds_rain_map_texel_size)
{
  if (clouds_rain_map)
  {
    float range = 1000.f;
    volfog_quality_ecs_query(manager, [&range](const ecs::string &render_settings__volumeFogQuality) {
      range = getRangeFromFogQuality(render_settings__volumeFogQuality);
    });

    rainMapData.init(range, clouds_rain_map_texel_size);
  }
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(bool clouds_rain_map)
static void destroy_cloud_rain_map_es(const ecs::Event &) { rainMapData.rainMapNode = dafg::NodeHandle(); }


ECS_TAG(render)
ECS_TRACK(render_settings__volumeFogQuality)
ECS_ON_EVENT(on_appear)
static void on_volfog_setting_change_es(const ecs::Event &, const ecs::string &render_settings__volumeFogQuality)
{
  rainMapData.setRange(getRangeFromFogQuality(render_settings__volumeFogQuality));
}


#if DAGOR_DBGLEVEL > 0

static dafg::NodeHandle dbgNode;
static PostFxRenderer dbgRainMapRenderer;

namespace var
{
static ShaderVariableInfo rain_map_dbg_strength("rain_map_dbg_strength");
static ShaderVariableInfo rain_map_dbg_density__height_mul("rain_map_dbg_density__height_mul");
static ShaderVariableInfo rain_map_dbg_mode("rain_map_dbg_mode");
} // namespace var

static bool rain_map_debug_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME_EX("render", "debug_rain_clouds", 1, 4, "Toggles rain cloud overlay on the ground!",
    "<mode: 0/1 strength/height> <overlay_strength: [0..1]> <overlay_density / height_mul : [0..1] (density threshold / height mul)>")
  {
    float overlayStrength = 0.5f;
    float overlayDensity = 1.f;
    int overlayMode = 0;

    if (argc == 1 && dbgNode.valid())
    {
      dbgNode = {};
      console::print_d("Rain Map debug is turned off!");
      return true;
    }

    if (argc > 1)
      overlayMode = console::to_int(argv[1]);

    if (argc > 2)
      overlayStrength = console::to_real(argv[2]);

    if (argc > 3)
      overlayDensity = console::to_real(argv[3]);

    if (!rainMapData.rainMapNode.valid())
    {
      console::print_d("Cannot turn on debug_rain_clouds as entity 'rain_map_render' is not present!");
      return true;
    }
    else
    {
      ShaderGlobal::set_float(var::rain_map_dbg_strength, overlayStrength);
      ShaderGlobal::set_float(var::rain_map_dbg_density__height_mul, overlayDensity);
      ShaderGlobal::set_int(var::rain_map_dbg_mode, overlayMode);

      dbgNode = dafg::register_node("rain_map_debug_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
        auto debugNs = registry.root() / "debug";
        auto colorTarget = debugNs.modifyTexture("target_for_debug");
        registry.readTexture("clouds_rain_map_tex").atStage(dafg::Stage::PS).bindToShaderVar("clouds_rain_map_tex");
        registry.readTexture("depth_for_postfx").atStage(dafg::Stage::PS).bindToShaderVar("depth_gbuf");
        registry.requestRenderPass().color({colorTarget});

        dbgRainMapRenderer = PostFxRenderer("rain_map_dbg");

        return []() { dbgRainMapRenderer.render(); };
      });
      console::print_d("RainMap debug is turned on with mode: %d overlay strength: %f, %s : %f", overlayMode, overlayStrength,
        overlayMode ? "height mul" : "density threshold", overlayDensity);
    }

    return true;
  }
  return found;
}


REGISTER_CONSOLE_HANDLER(rain_map_debug_console_handler);
#endif