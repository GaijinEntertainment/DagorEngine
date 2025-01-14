// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_quadIndexBuffer.h>
#include <render/viewVecs.h>
#include <render/world/cameraParams.h>

#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/updateStageRender.h>
#include <ecs/render/shaders.h>
#include <ecs/render/shaderVar.h>
#include <ecs/render/resPtr.h>

#include <render/daBfg/ecs/frameGraphNode.h>
#include <math/dag_hlsl_floatx.h>
#include <static_flares/shaders/staticFlares.hlsli>

#include <render/world/frameGraphHelpers.h>


using StaticFlareInstances = dag::Vector<StaticFlareInstance>;

ECS_DECLARE_RELOCATABLE_TYPE(StaticFlareInstances);
ECS_REGISTER_RELOCATABLE_TYPE(StaticFlareInstances, nullptr);


template <typename C>
ECS_REQUIRE(eastl::true_type static_flares__visible)
static void get_static_flares_ecs_query(C);

template <typename C>
static void add_static_flare_ecs_query(C);


ECS_ON_EVENT(on_appear)
static void init_static_flares_es(const ecs::Event &,
  int static_flares__maxCount,
  UniqueBufHolder &static_flares__instancesBuf,
  StaticFlareInstances &static_flares__instances,
  dabfg::NodeHandle &static_flares__node)
{
  static_flares__instances.resize(static_flares__maxCount);
  static_flares__instancesBuf = dag::create_sbuffer(sizeof(StaticFlareInstance), static_flares__maxCount,
    SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES, 0, "static_flares_buf");

  static_flares__node = dabfg::register_node("static_flares_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.read("downsampled_depth_with_early_after_envi_water")
      .texture()
      .atStage(dabfg::Stage::PS)
      .bindToShaderVar("downsampled_depth");

    render_transparency_published(registry);

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    registry.requestState().setFrameBlock("global_frame");

    return [cameraHndl]() {
      get_static_flares_ecs_query(
        [&](const ShadersECS &static_flares__shader, float static_flares__minSize, float static_flares__maxSize,
          ShaderVar static_flares_ctg_min_max, int static_flares__copiedCount) {
          if (!static_flares__copiedCount)
            return;

          float fov_scale = cameraHndl.ref().jitterPersp.hk;
          float fov = atan(fov_scale);
          Color4 currentCtgMinMax(1.0 / tan(0.5 * fov), // ctg for formula screen_size = real_size / distance * ctg(fov/2)
            static_flares__minSize, static_flares__maxSize, 0);
          static_flares_ctg_min_max.set_color4(currentCtgMinMax);

          index_buffer::use_quads_16bit();
          set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
          static_flares__shader.shElem->setStates();
          d3d_err(d3d::setvsrc(0, 0, 0));
          d3d_err(d3d::drawind(PRIM_TRILIST, 0, static_flares__copiedCount * 2, 0));
        });
    };
  });
}


static uint32_t get_premultiplied_color(E3DCOLOR color)
{
  return e3dcolor_mul(color, E3DCOLOR(color.a, color.a, color.a, 0xFF)); // premultiplied alpha
}

void add_static_flare(const TMatrix &tm,
  E3DCOLOR color,
  float size,
  float fade_distance_from,
  float fade_distance_to,
  float horz_dir_deg,
  float horz_delta_deg,
  float vert_dir_deg,
  float vert_delta_deg)
{
  add_static_flare_ecs_query(
    [&](StaticFlareInstances &static_flares__instances, int static_flares__maxCount, int &static_flares__count) {
      if (static_flares__count >= static_flares__maxCount)
      {
        LOGERR_ONCE("static flares limit is exceeded %d", static_flares__maxCount);
        return;
      }
      int i = static_flares__count;

      const Point3 &pos = tm.getcol(3);
      static_flares__instances[i].xz = float2(pos.x, pos.z);
      static_flares__instances[i].y_size = (float_to_half(pos.y) << 16) | float_to_half(size);
      static_flares__instances[i].color = get_premultiplied_color(color);

      const float horzRotOffset = horz_delta_deg < 180.0f ? atan2f(-tm.getcol(0).z, tm.getcol(0).x) : 0.0f;
      horz_dir_deg = DegToRad(horz_dir_deg) + horzRotOffset;
      vert_dir_deg = DegToRad(vert_dir_deg); // vertical would ignore the orientation

      float4 dirs = float4(sin(horz_dir_deg), cos(horz_dir_deg), cos(vert_dir_deg), sin(vert_dir_deg));
      float2 dots = float2(horz_delta_deg = cos(DegToRad(horz_delta_deg)), vert_delta_deg = cos(DegToRad(vert_delta_deg)));

      static_flares__instances[i].dirs_dots_start_coef = uint4((float_to_half(dirs.x) << 16) | float_to_half(dots.x),
        (float_to_half(dirs.y) << 16) | float_to_half(dots.y), (float_to_half(dirs.z) << 16) | float_to_half(fade_distance_from),
        (float_to_half(dirs.w) << 16) | float_to_half(1.0 / (fade_distance_to - fade_distance_from)));

      ++static_flares__count;
    });
}

ECS_REQUIRE(eastl::true_type static_flares__visible)
static void static_flare_before_render_es(const UpdateStageInfoBeforeRender &,
  const UniqueBufHolder &static_flares__instancesBuf,
  const StaticFlareInstances &static_flares__instances,
  int static_flares__count,
  int &static_flares__copiedCount)
{
  if (static_flares__count > static_flares__copiedCount)
  {
    static_flares__instancesBuf.getBuf()->updateData(static_flares__copiedCount * sizeof(static_flares__instances[0]),
      (static_flares__count - static_flares__copiedCount) * sizeof(static_flares__instances[0]),
      static_flares__instances.data() + static_flares__copiedCount, 0);
    static_flares__copiedCount = static_flares__count;
  }
}