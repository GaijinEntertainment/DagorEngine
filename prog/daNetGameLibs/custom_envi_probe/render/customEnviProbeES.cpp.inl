// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaders.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/updateStage.h>
#include <math/dag_cube_matrix.h>
#include <render/viewVecs.h>
#include <shaders/dag_shaderBlock.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <perfMon/dag_statDrv.h>
#include <render/renderer.h>
#include <render/renderEvent.h>
#include <render/sphHarmCalc.h>
#include <render/indoorProbeManager.h>
#include <render/world/wrDispatcher.h>
#include <ecs/render/resPtr.h>
#include <ecs/render/postfx_renderer.h>
#include <ecs/render/updateStageRender.h>
#include <3d/dag_resPtr.h>


#define CUSTOM_ENVI_PROBE_VARS \
  VAR(envi_probe_rotation)     \
  VAR(envi_probe_mul)

#define VAR(a) static int a##VarId = -1;
CUSTOM_ENVI_PROBE_VARS
#undef VAR

static void custom_envi_probe_set_shadervars(float x_rotation, float y_rotation, float mul)
{
  float sinX, cosX;
  sincos(DegToRad(x_rotation), sinX, cosX);
  float sinY, cosY;
  sincos(DegToRad(y_rotation), sinY, cosY);
  ShaderGlobal::set_color4(envi_probe_rotationVarId, cosX, sinX, cosY, sinY);
  ShaderGlobal::set_real(envi_probe_mulVarId, mul);
}

ECS_TAG(render)
static void custom_envi_probe_created_es_event_handler(
  const ecs::EventEntityCreated &, const SharedTexHolder &custom_envi_probe__cubemap, bool &custom_envi_probe__needs_render)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a);
  CUSTOM_ENVI_PROBE_VARS
#undef VAR
  prefetch_and_check_managed_texture_loaded(custom_envi_probe__cubemap.getTexId(), true);
  custom_envi_probe__needs_render = true;
}

ECS_TAG(render, dev)
ECS_TRACK(custom_envi_probe__cubemap_res)
static void custom_cube_texture_name_changed_es_event_handler(ecs::EventComponentChanged,
  SharedTexHolder &custom_envi_probe__cubemap,
  const ecs::string &custom_envi_probe__cubemap_res,
  const ecs::string &custom_envi_probe__cubemap_var,
  bool &custom_envi_probe__needs_render,
  ecs::Point4List &custom_envi_probe__spherical_harmonics_outside,
  ecs::Point4List &custom_envi_probe__spherical_harmonics_inside)
{
  custom_envi_probe__cubemap.close();
  custom_envi_probe__cubemap = dag::get_tex_gameres(custom_envi_probe__cubemap_res.c_str(), custom_envi_probe__cubemap_var.c_str());
  prefetch_and_check_managed_texture_loaded(custom_envi_probe__cubemap.getTexId(), true);
  custom_envi_probe__needs_render = true;
  custom_envi_probe__spherical_harmonics_outside = {};
  custom_envi_probe__spherical_harmonics_inside = {};
  debug("custom_envi_probe__name changed, recalculating spherical harmonics...");
}

ECS_TAG(render, dev)
ECS_TRACK(custom_envi_probe__x_rotation, custom_envi_probe__y_rotation, custom_envi_probe__outside_mul, custom_envi_probe__inside_mul)
ECS_REQUIRE(float custom_envi_probe__x_rotation,
  float custom_envi_probe__y_rotation,
  float custom_envi_probe__outside_mul,
  float custom_envi_probe__inside_mul)
static void custom_cube_texture_vars_changed_es_event_handler(ecs::EventComponentChanged,
  bool &custom_envi_probe__needs_render,
  ecs::Point4List &custom_envi_probe__spherical_harmonics_outside,
  ecs::Point4List &custom_envi_probe__spherical_harmonics_inside)
{
  custom_envi_probe__needs_render = true;
  custom_envi_probe__spherical_harmonics_outside = {};
  custom_envi_probe__spherical_harmonics_inside = {};
  debug("custom_envi_probe__<parameter> changed, recalculating spherical harmonics...");
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static void custom_cube_texture_before_render_es(const UpdateStageInfoBeforeRender &event,
  bool &custom_envi_probe__needs_render,
  bool &custom_envi_probe__is_inside,
  const SharedTexHolder &custom_envi_probe__cubemap)
{
  TIME_D3D_PROFILE(custom_cube_texture_before_render)
  const auto *indoorProbeMgr = WRDispatcher::getIndoorProbeManager();
  const bool isInside = indoorProbeMgr && indoorProbeMgr->isWorldPosInIndoorProbe(event.camPos);
  if (isInside != custom_envi_probe__is_inside)
  {
    custom_envi_probe__needs_render = true;
    custom_envi_probe__is_inside = isInside;
  }

  if (!custom_envi_probe__needs_render)
    return;
  if (prefetch_and_check_managed_texture_loaded(custom_envi_probe__cubemap.getTexId(), true))
  {
    int frameBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    get_world_renderer()->reloadCube(false);
    custom_envi_probe__needs_render = false;
    ShaderGlobal::setBlock(frameBlock, ShaderGlobal::LAYER_FRAME);
  }
}

static void custom_envi_probe_after_reset_es(const AfterDeviceReset &, bool &custom_envi_probe__needs_render)
{
  custom_envi_probe__needs_render = true;
}

ECS_TAG(render)
static void custom_envi_probe_render_es_event_handler(const CustomEnviProbeRender &evt,
  const PostFxRenderer &custom_envi_probe__postfx,
  bool custom_envi_probe__is_inside,
  float custom_envi_probe__x_rotation,
  float custom_envi_probe__y_rotation,
  float custom_envi_probe__outside_mul,
  float custom_envi_probe__inside_mul,
  const SharedTexHolder &custom_envi_probe__cubemap)
{
  const ManagedTex *cubeTarget = evt.get<0>();
  int faceNum = evt.get<1>();

  custom_envi_probe_set_shadervars(custom_envi_probe__x_rotation, custom_envi_probe__y_rotation,
    custom_envi_probe__is_inside ? custom_envi_probe__inside_mul : custom_envi_probe__outside_mul);

  const int faceBegin = faceNum == -1 ? 0 : faceNum;
  const int faceEnd = faceNum == -1 ? 6 : faceNum + 1;

  if (check_managed_texture_loaded(custom_envi_probe__cubemap.getTexId(), true))
  {
    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;
    SCOPE_VIEWPORT;
    for (int i = faceBegin; i < faceEnd; i++)
    {
      d3d::set_render_target(cubeTarget->getCubeTex(), i, 0);
      TMatrix4 projTm;
      d3d::setpersp(Driver3dPerspective(1, 1, 0.25, 400), &projTm); // set a square perspective, zn zf doesn't matter
      TMatrix cameraMatrix = cube_matrix(TMatrix::IDENT, i);
      TMatrix view = orthonormalized_inverse(cameraMatrix);
      d3d::settm(TM_VIEW, view);
      set_viewvecs_to_shader(view, projTm);
      custom_envi_probe__postfx.render();
    }
  }
  else
  {
    for (uint32_t i = faceBegin; i < faceEnd; i++)
    {
      d3d::clear_rt({cubeTarget->getCubeTex(), 0u, i});
    }
  }
}

ECS_TAG(render)
static void custom_envi_probe_get_spherical_harmonics_es_event_handler(CustomEnviProbeGetSphericalHarmonics &evt,
  const ecs::Point4List &custom_envi_probe__spherical_harmonics_outside,
  const ecs::Point4List &custom_envi_probe__spherical_harmonics_inside,
  bool custom_envi_probe__is_inside)
{
  eastl::vector<Color4> &sphHarm = evt.get<0>();
  if (custom_envi_probe__spherical_harmonics_outside.size() != SphHarmCalc::SPH_COUNT ||
      custom_envi_probe__spherical_harmonics_inside.size() != SphHarmCalc::SPH_COUNT)
  {
    debug(custom_envi_probe__is_inside ? "calculating spherical harmonics inside..." : "calculating spherical harmonics outside...");
    sphHarm.resize(0);
  }
  else
  {
    const ecs::Point4List &src =
      custom_envi_probe__is_inside ? custom_envi_probe__spherical_harmonics_inside : custom_envi_probe__spherical_harmonics_outside;
    sphHarm.resize(SphHarmCalc::SPH_COUNT);
    for (int i = 0; i < SphHarmCalc::SPH_COUNT; i++)
      sphHarm[i] = Color4::xyzw(src[i]);
  }
}

ECS_TAG(render, dev)
static void custom_envi_probe_log_spherical_harmonics_es_event_handler(const CustomEnviProbeLogSphericalHarmonics &evt,
  ecs::Point4List &custom_envi_probe__spherical_harmonics_outside,
  ecs::Point4List &custom_envi_probe__spherical_harmonics_inside,
  bool custom_envi_probe__is_inside)
{
  if ((custom_envi_probe__spherical_harmonics_outside.size() != SphHarmCalc::SPH_COUNT ||
        custom_envi_probe__spherical_harmonics_inside.size() != SphHarmCalc::SPH_COUNT))
  {
    const Color4 *sphHarm = evt.get<0>();
    ecs::Point4List &dst =
      custom_envi_probe__is_inside ? custom_envi_probe__spherical_harmonics_inside : custom_envi_probe__spherical_harmonics_outside;
    dst.clear();
    for (int i = 0; i < SphHarmCalc::SPH_COUNT; i++)
      dst.push_back(Point4::rgba(sphHarm[i]));
    const char *name = custom_envi_probe__is_inside ? "custom_envi_probe__spherical_harmonics_inside"
                                                    : "custom_envi_probe__spherical_harmonics_outside";
    debug("Recalculating %s", name);
// Need this macro, because %@ doesn't print it with enough precision
#define EXPAND(value) value.r, value.g, value.b, value.a
    debug("Result to copy paste into entity \n"
          "  \"%s:list<p4>\"{\n"
          "    value:p4 = %f, %f, %f, %f\n"
          "    value:p4 = %f, %f, %f, %f\n"
          "    value:p4 = %f, %f, %f, %f\n"
          "    value:p4 = %f, %f, %f, %f\n"
          "    value:p4 = %f, %f, %f, %f\n"
          "    value:p4 = %f, %f, %f, %f\n"
          "    value:p4 = %f, %f, %f, %f\n"
          "  }",
      name, EXPAND(sphHarm[0]), EXPAND(sphHarm[1]), EXPAND(sphHarm[2]), EXPAND(sphHarm[3]), EXPAND(sphHarm[4]), EXPAND(sphHarm[5]),
      EXPAND(sphHarm[6]));
#undef EXPAND
  }
}