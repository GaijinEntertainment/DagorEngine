// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/render/resPtr.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/updateStageRender.h>
#include <ecs/anim/anim.h>
#include "shaders/dag_overrideStateId.h"
#include "shaders/dag_shaderBlock.h"
#include "shaders/metatex_const.hlsli"
#include <EASTL/optional.h>
#include <EASTL/unordered_map.h>
#include <perfMon/dag_statDrv.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_lockTexture.h>
#include <daECS/core/componentTypes.h>
#include <nodeBasedShaderManager/nodeBasedShaderManager.h>


#define VARS VAR(world_to_volfog_mask)

#define VAR(T) static int T##VarId = -1;
VARS
#undef VAR


  inline bool
  update_volfog_mask_texture(SharedTexHolder &tex_holder, int tex_size, Point4 bounds, const ecs::FloatList &data)
{
  if (!data.size())
    return false;

  G_ASSERT(data.size() == tex_size * tex_size);

  bool recreate = !tex_holder.getTex2D();
  if (!recreate)
  {
    TextureInfo ti;
    tex_holder->getinfo(ti);
    recreate = (ti.w != tex_size || ti.h != tex_size);
  }

  if (recreate)
  {
    tex_holder.close();
    tex_holder = dag::create_tex(nullptr, tex_size, tex_size, TEXCF_DYNAMIC | TEXFMT_L8, 1, "volfog_mask_tex");
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Linear;
    ShaderGlobal::set_sampler(get_shader_variable_id("volfog_mask_tex_samplerstate", true), d3d::request_sampler(smpInfo));
    tex_holder->disableSampler();
    NodeBasedShaderManager::clearAllCachedResources();
  }

  Point2 min_bound = Point2(bounds.x, bounds.y);
  Point2 max_bound = Point2(bounds.z, bounds.w);
  float invX = safeinv(max_bound.x - min_bound.x);
  float invY = safeinv(max_bound.y - min_bound.y);
  ShaderGlobal::set_color4(world_to_volfog_maskVarId, Color4(invX, invY, -invX * min_bound.x, -invY * min_bound.y));

  int pos = 0;
  if (tex_holder.getTex2D())
    if (auto to = lock_texture<uint8_t>(tex_holder.getTex2D(), 0, TEXLOCK_WRITE))
      for (int y = 0; y < tex_size; y++)
      {
        for (int x = 0; x < tex_size; x++, pos++)
          to.at(x, y) = clamp(int(255.0 * data[pos]), 0, 255);
      }

  d3d::resource_barrier({tex_holder.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  return true;
}


ECS_ON_EVENT(on_appear)
inline void volfog_mask_appear_es_event_handler(const ecs::Event &,
  SharedTexHolder &volfog_mask__tex_holder,
  int volfog_mask__size,
  bool &volfog_mask__do_update,
  Point4 volfog_mask__bounds,             // minx, minz, maxx, maxz
  const ecs::FloatList &volfog_mask__data // array [size * size] of float (0..1)
)
{
#define VAR(T) T##VarId = get_shader_variable_id(#T);
  VARS
#undef VAR

    G_ASSERT(volfog_mask__size);
  volfog_mask__do_update = false;

  update_volfog_mask_texture(volfog_mask__tex_holder, volfog_mask__size, volfog_mask__bounds, volfog_mask__data);
}


ECS_TAG(render)
ECS_REQUIRE(eastl::true_type volfog_mask__do_update)
inline void volfog_mask_render_es(const UpdateStageInfoBeforeRender &stg,
  SharedTexHolder &volfog_mask__tex_holder,
  int volfog_mask__size,
  bool &volfog_mask__do_update,
  Point4 volfog_mask__bounds,             // minx, minz, maxx, maxz
  const ecs::FloatList &volfog_mask__data // array [size * size] of float (0..1)
)
{
  G_UNUSED(stg);
  if (update_volfog_mask_texture(volfog_mask__tex_holder, volfog_mask__size, volfog_mask__bounds, volfog_mask__data))
    volfog_mask__do_update = false;
}
