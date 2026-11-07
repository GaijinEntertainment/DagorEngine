// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "iblCubeRegistry.h"
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <math/dag_TMatrix.h>
#include <math/dag_Point2.h>
#include <math/dag_mathBase.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_texMgr.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_barrier.h>
#include <ecs/render/resPtr.h>
#include <ecs/render/updateStageRender.h>
#include <ecs/render/renderEvent.h>
#include <perfMon/dag_statDrv.h>

#define IBL_CUBE_VARS                    \
  VAR(ibl_cube__bake_roughness)          \
  VAR(ibl_cube_source)                   \
  VAR(ibl_cube_atlas)                    \
  VAR(ibl_cube_is_inside)                \
  VAR(ibl_cube_mul_specular)             \
  VAR(ibl_cube_mul_diffuse)              \
  VAR(ibl_cube_mul_specular_falloff_end) \
  VAR(ibl_cube_mul_diffuse_falloff_end)  \
  VAR(ibl_cube_falloff_start)            \
  VAR(ibl_cube_falloff_end)              \
  VAR(ibl_cube_center)                   \
  VAR(ibl_cube_rotation_row0)            \
  VAR(ibl_cube_rotation_row1)            \
  VAR(ibl_cube_rotation_row2)            \
  VAR(ibl_cube_world_to_local_row0)      \
  VAR(ibl_cube_world_to_local_row1)      \
  VAR(ibl_cube_world_to_local_row2)

#define VAR(a) static int a##VarId = -1;
IBL_CUBE_VARS
#undef VAR

static void init_ibl_cube_vars()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  IBL_CUBE_VARS
#undef VAR
}

static bool camera_inside_ibl_cube(const TMatrix &transform, const Point3 &cam_pos)
{
  const Point3 local = inverse(transform) * cam_pos;
  return fabsf(local.x) <= 0.5f && fabsf(local.y) <= 0.5f && fabsf(local.z) <= 0.5f;
}

static UniqueTex create_ibl_cube_atlas()
{
  static int atlas_counter = 0;
  const int R = IBLCubeRegistry::SLICE_SIZE;
  return dag::create_tex(nullptr, 4 * R, R, TEXFMT_A16B16G16R16F | TEXCF_RTARGET, 1,
    String(32, "ibl_cube_atlas_tex_%d", atlas_counter++));
}

static void bake_ibl_cube_atlas(const UniqueTex &atlas, const PostFxRenderer &prefilter)
{
  if (!atlas)
    return;
  TIME_D3D_PROFILE(ibl_cube_bake);

  const int R = IBLCubeRegistry::SLICE_SIZE;
  const int qw = R;
  const int qh = R / 2;
  struct SliceRect
  {
    int x, y, w, h;
    float roughness;
  };
  const SliceRect slices[5] = {
    {0, 0, 2 * R, R, 0.0f},
    {2 * R + 0 * qw, 1 * qh, qw, qh, 0.25f},
    {2 * R + 1 * qw, 1 * qh, qw, qh, 0.50f},
    {2 * R + 0 * qw, 0 * qh, qw, qh, 0.75f},
    {2 * R + 1 * qw, 0 * qh, qw, qh, 1.00f},
  };

  SCOPE_RENDER_TARGET;
  SCOPE_VIEWPORT;
  d3d::set_render_target({nullptr, 0u, 0u}, DepthAccess::RW, {{atlas.getTex2D(), 0u, 0u}});
  for (const SliceRect &s : slices)
  {
    ShaderGlobal::set_float(ibl_cube__bake_roughnessVarId, s.roughness);
    d3d::setview(s.x, s.y, s.w, s.h, 0.0f, 1.0f);
    prefilter.render();
  }
  d3d::resource_barrier({atlas.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});
}

static PostFxRenderer ibl_cube_prefilter;

IBLCubeRegistry::IBLCubeRegistry() { init_ibl_cube_vars(); }

void IBLCubeRegistry::acquire(const char *source)
{
  if (!source || !*source)
    return;
  AtlasEntry &entry = atlases[source];
  if (!entry.atlas)
    entry.atlas = create_ibl_cube_atlas();
  ++entry.refCount;
}

void IBLCubeRegistry::release(const char *source)
{
  if (!source || !*source)
    return;
  auto it = atlases.find_as(source, eastl::less<>());
  if (it != atlases.end() && --it->second.refCount <= 0)
    atlases.erase(it);
}

eastl::pair<const UniqueTex *, bool> IBLCubeRegistry::getOrCreateAtlas(const eastl::string &source)
{
  auto it = atlases.find(source);
  if (it == atlases.end() || !it->second.atlas)
    return {nullptr, false};
  return {&it->second.atlas, it->second.baked};
}

bool IBLCubeRegistry::isBaked(const char *source) const
{
  if (!source || !*source)
    return false;
  auto it = atlases.find_as(source, eastl::less<>());
  return it != atlases.end() && it->second.baked;
}

void IBLCubeRegistry::markBaked(const char *source)
{
  if (!source || !*source)
    return;
  auto it = atlases.find_as(source, eastl::less<>());
  if (it != atlases.end())
    it->second.baked = true;
}

void IBLCubeRegistry::resetBaked()
{
  for (auto &e : atlases)
    e.second.baked = false;
}

ECS_REGISTER_BOXED_TYPE(IBLCubeRegistry, nullptr);

static IBLCubeRegistry &get_ibl_registry()
{
  const ecs::EntityId eid = g_entity_mgr->getOrCreateSingletonEntity(ECS_HASH("ibl_cube_registry"));
  return g_entity_mgr->getRW<IBLCubeRegistry>(eid, ECS_HASH("ibl_cube__registry"));
}

static IBLCubeRegistry *find_ibl_registry()
{
  const ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH("ibl_cube_registry"));
  return g_entity_mgr->getNullableRW<IBLCubeRegistry>(eid, ECS_HASH("ibl_cube__registry"));
}

static void load_ibl_cube_panorama(SharedTexWithShaderVar &panorama, const ecs::string &res, const ecs::string &var)
{
  panorama.close();
  panorama = dag::get_tex_gameres(res.c_str(), var.c_str());
  if (!res.empty() && panorama.getTexId() == BAD_TEXTUREID)
    logerr("ibl_cube: panorama asset '%s' not found", res.c_str());
  prefetch_and_check_managed_texture_loaded(panorama.getTexId(), true);
  get_ibl_registry().acquire(res.c_str());
}

ECS_TAG(render)
static void ibl_cube_created_es(const ecs::EventEntityCreated &, SharedTexWithShaderVar &ibl_cube__panorama,
  const ecs::string &ibl_cube__panorama_res, const ecs::string &ibl_cube__panorama_var)
{
  load_ibl_cube_panorama(ibl_cube__panorama, ibl_cube__panorama_res, ibl_cube__panorama_var);
}

ECS_TAG(render)
ECS_TRACK(ibl_cube__panorama_res)
static void ibl_cube_panorama_changed_es(const ecs::Event &, SharedTexWithShaderVar &ibl_cube__panorama,
  const ecs::string &ibl_cube__panorama_res, const ecs::string &ibl_cube__panorama_var)
{
  load_ibl_cube_panorama(ibl_cube__panorama, ibl_cube__panorama_res, ibl_cube__panorama_var);
}

ECS_TAG(render)
static void ibl_cube_destroyed_es(const ecs::EventEntityDestroyed &, const ecs::string &ibl_cube__panorama_res)
{
  if (IBLCubeRegistry *reg = find_ibl_registry())
    reg->release(ibl_cube__panorama_res.c_str());
}

ECS_TAG(render)
static void ibl_cube_after_reset_es(const EventAfterDeviceReset &)
{
  if (IBLCubeRegistry *reg = find_ibl_registry())
    reg->resetBaked();
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es)
ECS_BEFORE(ibl_cube_before_render_es)
static void ibl_cube_reset_es(const UpdateStageInfoBeforeRender &)
{
  if (ibl_cube_is_insideVarId >= 0)
    ShaderGlobal::set_int(ibl_cube_is_insideVarId, 0);
}

ECS_TAG(render)
ECS_AFTER(ibl_cube_before_render_es)
static void ibl_cube_update_is_baked_es(const UpdateStageInfoBeforeRender &, const ecs::string &ibl_cube__panorama_res,
  bool &ibl_cube__is_baked)
{
  IBLCubeRegistry *registry = find_ibl_registry();
  ibl_cube__is_baked = registry && registry->isBaked(ibl_cube__panorama_res.c_str());
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es)
static void ibl_cube_before_render_es(const UpdateStageInfoBeforeRender &evt, const TMatrix &transform, float ibl_cube__mul_specular,
  float ibl_cube__mul_diffuse, float ibl_cube__mul_specular__falloff_end, float ibl_cube__mul_diffuse__falloff_end,
  float ibl_cube__falloff_start, float ibl_cube__falloff_end, const ecs::string &ibl_cube__panorama_res,
  const SharedTexWithShaderVar &ibl_cube__panorama)
{
  TIME_D3D_PROFILE(ibl_cube_before_render);

  IBLCubeRegistry &registry = get_ibl_registry();

  const UniqueTex *atlas = nullptr;
  if (ibl_cube__panorama.getTexId() != BAD_TEXTUREID && prefetch_and_check_managed_texture_loaded(ibl_cube__panorama.getTexId(), true))
  {
    auto [tex, isBaked] = registry.getOrCreateAtlas(ibl_cube__panorama_res.c_str());
    if (tex && !isBaked)
    {
      if (!ibl_cube_prefilter.getElem())
        ibl_cube_prefilter.init("ibl_cube_prefilter");
      if (ibl_cube_prefilter.getElem())
      {
        ShaderGlobal::set_texture(ibl_cube_sourceVarId, ibl_cube__panorama.getTexId());
        bake_ibl_cube_atlas(*tex, ibl_cube_prefilter);
        registry.markBaked(ibl_cube__panorama_res.c_str());
        isBaked = true;
      }
    }
    if (isBaked)
      atlas = tex;
  }

  const bool inside = camera_inside_ibl_cube(transform, evt.camPos);

  if (inside && atlas && *atlas)
  {
    ShaderGlobal::set_texture(ibl_cube_atlasVarId, atlas->getTexId());
    ShaderGlobal::set_int(ibl_cube_is_insideVarId, 1);
    ShaderGlobal::set_float(ibl_cube_mul_specularVarId, ibl_cube__mul_specular);
    ShaderGlobal::set_float(ibl_cube_mul_diffuseVarId, ibl_cube__mul_diffuse);
    ShaderGlobal::set_float(ibl_cube_mul_specular_falloff_endVarId, ibl_cube__mul_specular__falloff_end);
    ShaderGlobal::set_float(ibl_cube_mul_diffuse_falloff_endVarId, ibl_cube__mul_diffuse__falloff_end);
    ShaderGlobal::set_float(ibl_cube_falloff_startVarId, ibl_cube__falloff_start);
    ShaderGlobal::set_float(ibl_cube_falloff_endVarId, ibl_cube__falloff_end);

    const Point3 center = transform.getcol(3);
    ShaderGlobal::set_float4(ibl_cube_centerVarId, center.x, center.y, center.z, 0.0f);

    const Point3 ax = normalize(transform.getcol(0));
    const Point3 ay = normalize(transform.getcol(1));
    const Point3 az = normalize(transform.getcol(2));
    ShaderGlobal::set_float4(ibl_cube_rotation_row0VarId, ax.x, ax.y, ax.z, 0.0f);
    ShaderGlobal::set_float4(ibl_cube_rotation_row1VarId, ay.x, ay.y, ay.z, 0.0f);
    ShaderGlobal::set_float4(ibl_cube_rotation_row2VarId, az.x, az.y, az.z, 0.0f);

    const TMatrix inv = inverse(transform);
    ShaderGlobal::set_float4(ibl_cube_world_to_local_row0VarId, inv.getcol(0).x, inv.getcol(1).x, inv.getcol(2).x, inv.getcol(3).x);
    ShaderGlobal::set_float4(ibl_cube_world_to_local_row1VarId, inv.getcol(0).y, inv.getcol(1).y, inv.getcol(2).y, inv.getcol(3).y);
    ShaderGlobal::set_float4(ibl_cube_world_to_local_row2VarId, inv.getcol(0).z, inv.getcol(1).z, inv.getcol(2).z, inv.getcol(3).z);
  }
}
