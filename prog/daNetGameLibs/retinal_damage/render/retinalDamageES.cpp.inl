// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <ecs/camera/getActiveCameraSetup.h>
#include <ecs/render/resPtr.h>
#include <render/daBfg/ecs/frameGraphNode.h>
#include <render/resolution.h>
#include <shaders/dag_postFxRenderer.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_renderTarget.h>


template <typename Callable>
inline void start_retinal_damage_ecs_query(Callable c);

template <typename Callable>
inline void stop_retinal_damage_ecs_query(Callable c);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void retinal_damage_es_event_handler(
  const ecs::Event &, const Point3 &retinal_damage__explosion_world_position, float retinal_damage__scale)
{
  start_retinal_damage_ecs_query(
    [retinal_damage__explosion_world_position, retinal_damage__scale](float retinal_damage__total_lifetime,
      float retinal_damage__base_scale, float retinal_damage__scale_increase_rate, float &retinal_damage__remaining_time,
      dabfg::NodeHandle &retinal_damage__node_handle, UniqueTexHolder &retinal_damage__tex) {
      TMatrix4 globTm = calc_active_camera_globtm();
      Point4 pos = Point4::xyz1(retinal_damage__explosion_world_position) * globTm;
      if (!is_equal_float(pos.w, 0.f))
        pos *= safeinv(pos.w);
      Point2 screen_pos = {0.5f + pos.x * 0.5f, 0.5f - pos.y * 0.5f};
      if (screen_pos.x < 0.f || screen_pos.x > 1.f || screen_pos.y < 0.f || screen_pos.y > 1.f)
        return;

      retinal_damage__remaining_time = retinal_damage__total_lifetime;
      IPoint2 render_resolution = get_rendering_resolution();
      ShaderGlobal::set_color4(::get_shader_variable_id("retinal_damage_screen_position", true), screen_pos.x, screen_pos.y);
      ShaderGlobal::set_real(::get_shader_variable_id("screen_aspect_ratio"), (float)render_resolution.x / (float)render_resolution.y);
      ShaderGlobal::set_real(::get_shader_variable_id("retinal_damage_effect_scale"),
        retinal_damage__scale * retinal_damage__base_scale);
      ShaderGlobal::set_real(::get_shader_variable_id("retinal_damage_scale_increase_rate"), retinal_damage__scale_increase_rate);

      float seed = sin((pos.x + pos.y) * 1000.); // for better control over randomness of an appearing spot
      ShaderGlobal::set_color4(::get_shader_variable_id("retinal_damage_seeds"), seed, cos(seed * 2.), sin(seed * 4.));

      retinal_damage__tex.close();
      retinal_damage__tex = dag::create_tex(NULL, render_resolution.x / 2, render_resolution.y / 2, TEXFMT_R11G11B10F | TEXCF_RTARGET,
        1, "retinal_damage_tex");
      PostFxRenderer renderer = PostFxRenderer("retinal_damage_init");
      {
        TIME_D3D_PROFILE(render_retinal_damage_tex);
        d3d::GpuAutoLock gpuLock;
        SCOPE_RENDER_TARGET;
        d3d::set_render_target(retinal_damage__tex.getTex2D(), 0);
        renderer.render();
      }
      retinal_damage__node_handle = dabfg::register_node("retinal_damage_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
        registry.requestRenderPass().color({"final_target_with_motion_blur"});
        return [renderer = PostFxRenderer("retinal_damage_update")] { renderer.render(); };
      });
    });
}

ECS_TAG(render)
ECS_REQUIRE(Point3 retinal_damage__explosion_world_position, float retinal_damage__scale)
ECS_ON_EVENT(on_disappear)
static void retinal_damage_stop_es_event_handler(const ecs::Event &)
{
  stop_retinal_damage_ecs_query(
    [](dabfg::NodeHandle &retinal_damage__node_handle, UniqueTexHolder &retinal_damage__tex, float &retinal_damage__remaining_time) {
      retinal_damage__tex.close();
      retinal_damage__remaining_time = 0.0;
      ShaderGlobal::set_real(::get_shader_variable_id("retinal_damage_remaining_time_fraction", true), 0.0);
      ShaderGlobal::set_real(::get_shader_variable_id("retinal_damage_opacity", true), 0.0);
      retinal_damage__node_handle = dabfg::NodeHandle();
    });
}
