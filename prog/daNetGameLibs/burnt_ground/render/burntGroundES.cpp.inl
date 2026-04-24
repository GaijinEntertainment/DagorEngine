// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <ecs/render/updateStageRender.h>
#include <render/clipmapDecals.h>
#include <render/renderEvent.h>
#include <main/level.h>
#include <render/daFrameGraph/nodeHandle.h>
#include <ecs/render/resPtr.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <ecs/render/decalsES.h>
#include <gamePhys/collision/collisionLib.h>
#include <math/dag_hlsl_floatx.h>

#include "burntGroundNodes.h"
#include "burntGround.h"
#include "burntGroundEvents.h"

#include <burnt_ground/shaders/burnt_ground.hlsli>
#include <landMesh/biomeQuery.h>

#define BURNT_GROUND_VARS                      \
  VAR(autodetect_burnt_ground_selfillum_color) \
  VAR(burnt_ground_selfillum_strength)         \
  VAR(burnt_ground_selfillum_worldscale)       \
  VAR(burnt_ground_edge_params)                \
  VAR(burnt_ground_tiling_scale)               \
  VAR(burnt_ground_allowed_biomes_1)           \
  VAR(burnt_ground_allowed_biomes_2)

#define VAR(a) static int a##VarId = -1;
BURNT_GROUND_VARS
#undef VAR

template <typename Callable>
static void init_static_burnt_ground_decals_ecs_query(ecs::EntityManager &manager, Callable c);

ECS_TAG(render)
ECS_ON_EVENT(on_appear, OnLevelLoaded)
static void burnt_ground_renderer_on_appear_es(const ecs::Event &,
  ecs::EntityManager &manager,
  dafg::NodeHandle &burnt_ground_renderer__prepare_decals_node,
  UniqueBuf &burnt_ground_renderer__decals_staging_buf,
  UniqueBuf &burnt_ground_renderer__decals_indices_staging_buf,
  const SharedTexHolder &burnt_ground_renderer__tex_d,
  const SharedTexHolder &burnt_ground_renderer__tex_n,
  const SharedTexHolder &burnt_ground_renderer__tex_m,
  float burnt_ground_renderer__edge_noise_scale,
  float burnt_ground_renderer__edge_width,
  int burnt_ground_renderer__max_clipmap_decal_count,
  int burnt_ground_renderer__max_animating_decal_count,
  int &burnt_ground_renderer__clipmap_decal_type)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a);
  BURNT_GROUND_VARS
#undef VAR

  if (burnt_ground_renderer__max_clipmap_decal_count == 0 || burnt_ground_renderer__clipmap_decal_type != -1)
    return;
  burnt_ground_renderer__clipmap_decal_type =
    clipmap_decals_mgr::createDecalType(burnt_ground_renderer__tex_d.getTexId(), burnt_ground_renderer__tex_n.getTexId(),
      burnt_ground_renderer__tex_m.getTexId(), burnt_ground_renderer__max_clipmap_decal_count, "clipmap_burnt_ground_decal", false);
  if (burnt_ground_renderer__clipmap_decal_type == -1)
  {
    if (is_level_loaded())
      logerr("Could not initialize clipmap decal for burnt ground: %d", burnt_ground_renderer__clipmap_decal_type);
    burnt_ground_renderer__prepare_decals_node = {};
    burnt_ground_renderer__decals_staging_buf.close();
    burnt_ground_renderer__decals_indices_staging_buf.close();
    return;
  }
  clipmap_decals_mgr::createDecalSubType(burnt_ground_renderer__clipmap_decal_type, Point4(0, 0, 1, 1), 1);

  if (burnt_ground_renderer__max_animating_decal_count > 0)
  {
    burnt_ground_renderer__decals_staging_buf = dag::buffers::create_staging(
      sizeof(BurntGroundDecal) * burnt_ground_renderer__max_animating_decal_count, "burnt_ground_decal_staging_buffer");
    burnt_ground_renderer__decals_indices_staging_buf = dag::buffers::create_staging(
      sizeof(uint32_t) * BURNT_GROUND_TILE_COUNT * BURNT_GROUND_TILE_COUNT, "burnt_ground_decal_indices_staging_buffer");
    burnt_ground_renderer__prepare_decals_node =
      create_burnt_ground_prepare_decals_node(burnt_ground_renderer__max_animating_decal_count,
        burnt_ground_renderer__decals_staging_buf.getBuf(), burnt_ground_renderer__decals_indices_staging_buf.getBuf());
  }
  else
  {
    burnt_ground_renderer__prepare_decals_node = {};
    burnt_ground_renderer__decals_staging_buf.close();
    burnt_ground_renderer__decals_indices_staging_buf.close();
  }

  init_static_burnt_ground_decals_ecs_query(manager, [&](const TMatrix &transform, float static_burnt_ground_decal__radius) {
    const Point3 pos = transform.getcol(3);
    if (static_burnt_ground_decal__radius > 0)
    {
      float decalSize =
        static_burnt_ground_decal__radius + burnt_ground_renderer__edge_noise_scale + burnt_ground_renderer__edge_width;
      clipmap_decals_mgr::createDecal(burnt_ground_renderer__clipmap_decal_type, Point2(pos.x, pos.z), 0, Point2(decalSize, decalSize),
        0);
    }
  });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, OnLevelLoaded)
ECS_TRACK(*)
static void burnt_ground_renderer_on_change_es(const ecs::Event &,
  const Point4 &burnt_ground_renderer__selfillum_color,
  float burnt_ground_renderer__selfillum_strength,
  float burnt_ground_renderer__selfillum_worldscale,
  float burnt_ground_renderer__edge_noise_frequency,
  float burnt_ground_renderer__edge_noise_scale,
  float burnt_ground_renderer__edge_width,
  const Point2 &burnt_ground_renderer__tile_size,
  const ecs::StringList &burnt_ground_renderer__biomeNames,
  ecs::List<int> &burnt_ground_renderer__allowed_biome_groups)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a);
  BURNT_GROUND_VARS
#undef VAR

  ShaderGlobal::set_float4(autodetect_burnt_ground_selfillum_colorVarId, burnt_ground_renderer__selfillum_color);
  ShaderGlobal::set_float(burnt_ground_selfillum_strengthVarId, burnt_ground_renderer__selfillum_strength);
  ShaderGlobal::set_float(burnt_ground_selfillum_worldscaleVarId, burnt_ground_renderer__selfillum_worldscale);
  ShaderGlobal::set_float4(burnt_ground_edge_paramsVarId, burnt_ground_renderer__edge_noise_frequency,
    burnt_ground_renderer__edge_noise_scale, burnt_ground_renderer__edge_width, 0);
  ShaderGlobal::set_float4(burnt_ground_tiling_scaleVarId, burnt_ground_renderer__tile_size.x, burnt_ground_renderer__tile_size.y, 0,
    0);
  burnt_ground_renderer__allowed_biome_groups.clear();
  for (const auto &biome : burnt_ground_renderer__biomeNames)
  {
    int groupId = biome_query::get_biome_group_id(biome.c_str());
    if (groupId < 0)
      continue;
    burnt_ground_renderer__allowed_biome_groups.push_back(groupId);
  }
  eastl::array<uint32_t, 8> biomes = {0};
  for (int biomeId = 0; biomeId < 256; ++biomeId)
  {
    int biomeGroupId = biome_query::get_biome_group(biomeId);
    if (biomeGroupId == -1)
      continue;
    if (eastl::find(burnt_ground_renderer__allowed_biome_groups.begin(), burnt_ground_renderer__allowed_biome_groups.end(),
          biomeGroupId) == burnt_ground_renderer__allowed_biome_groups.end())
      continue;
    biomes[biomeId / 32] |= 1u << (biomeId % 32);
  }
  ShaderGlobal::set_int4(burnt_ground_allowed_biomes_1VarId, biomes[0], biomes[1], biomes[2], biomes[3]);
  ShaderGlobal::set_int4(burnt_ground_allowed_biomes_2VarId, biomes[4], biomes[5], biomes[6], biomes[7]);
}


template <typename Callable>
static void get_burnt_ground_renderer_ecs_query(ecs::EntityManager &manager, Callable c);

static void add_clipmap_decal(const Point3 &pos, float radius)
{
  get_burnt_ground_renderer_ecs_query(*g_entity_mgr,
    [&](int burnt_ground_renderer__clipmap_decal_type, float burnt_ground_renderer__edge_noise_scale,
      float burnt_ground_renderer__edge_width) {
      if (burnt_ground_renderer__clipmap_decal_type != -1)
      {
        float decalSize = radius + burnt_ground_renderer__edge_noise_scale + burnt_ground_renderer__edge_width;
        clipmap_decals_mgr::createDecal(burnt_ground_renderer__clipmap_decal_type, Point2(pos.x, pos.z), 0,
          Point2(decalSize, decalSize), 0);
      }
    });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void static_burnt_ground_decal_on_appear_es(
  const ecs::Event &, const TMatrix &transform, float static_burnt_ground_decal__radius)
{
  if (static_burnt_ground_decal__radius > 0)
  {
    const Point3 pos = transform.getcol(3);
    add_clipmap_decal(pos, static_burnt_ground_decal__radius);
  }
}


ECS_TAG(render)
ECS_ON_EVENT(FireOnGroundEvent)
static void burnt_ground__on_fire_event_es(const FireOnGroundEvent &evt,
  ecs::EntityManager &manager,
  float burnt_ground_renderer__edge_noise_scale,
  float burnt_ground_renderer__edge_width,
  float burnt_ground_renderer__fadein_duration_sec,
  float burnt_ground_renderer__fadeout_duration_sec,
  const ecs::List<int> &burnt_ground_renderer__allowed_biome_groups)
{
  if (evt.adjustedRadius <= 0)
    return;
  bool hadAllowedBiome = false;
  for (int i = 0; i < evt.biomeGroupsIds.size(); ++i)
    if (evt.biomeGroupWeights[i] > 0 &&
        eastl::find(burnt_ground_renderer__allowed_biome_groups.begin(), burnt_ground_renderer__allowed_biome_groups.end(),
          evt.biomeGroupsIds[i]) != burnt_ground_renderer__allowed_biome_groups.end())
      hadAllowedBiome = true;
  if (!hadAllowedBiome)
    return;

  if (evt.animated)
  {
    ecs::ComponentsInitializer attrs;
    TMatrix tm;
    tm.identity();
    tm.setcol(3, evt.position);
    ECS_INIT(attrs, transform, tm);
    ECS_INIT(attrs, burnt_ground_decal_placer__decal_size,
      evt.adjustedRadius + burnt_ground_renderer__edge_noise_scale + burnt_ground_renderer__edge_width);
    ECS_INIT(attrs, burnt_ground_decal_placer__radius, evt.adjustedRadius);
    ECS_INIT(attrs, burnt_ground_decal_placer__fade_in_time, burnt_ground_renderer__fadein_duration_sec);
    ECS_INIT(attrs, burnt_ground_decal_placer__fade_out_time, burnt_ground_renderer__fadeout_duration_sec);
    manager.createEntityAsync("burnt_ground_decal_placer", eastl::move(attrs));
  }
  else
    add_clipmap_decal(evt.position, evt.adjustedRadius);
}

ECS_TAG(render)
static void burnt_ground_decal_placer_update_es(const UpdateStageInfoBeforeRender &evt,
  ecs::EntityManager &manager,
  ecs::EntityId eid,
  const TMatrix &transform,
  float burnt_ground_decal_placer__radius,
  float &burnt_ground_decal_placer__time_passed,
  float burnt_ground_decal_placer__fade_in_time,
  float burnt_ground_decal_placer__fade_out_time)
{
  const auto prevTimePassed = burnt_ground_decal_placer__time_passed;
  burnt_ground_decal_placer__time_passed += evt.dt;
  if (prevTimePassed < burnt_ground_decal_placer__fade_in_time &&
      burnt_ground_decal_placer__time_passed >= burnt_ground_decal_placer__fade_in_time)
    add_clipmap_decal(transform.getcol(3), burnt_ground_decal_placer__radius);

  if (burnt_ground_decal_placer__time_passed >
      burnt_ground_decal_placer__fade_in_time + burnt_ground_decal_placer__fade_out_time + BURNT_GROUND_EXTRA_FADE_OUT_TIME_S)
    manager.destroyEntity(eid);
}

template <typename Callable>
static void gather_burnt_ground_decals_ecs_query(ecs::EntityManager &manager, Callable c);

void gather_burnt_ground_decals(const Point3 &eye_pos,
  float visibility_range,
  const eastl::fixed_function<32, void(const BurntGroundDecalInfo &decal)> &decal_processor)
{
  TIME_D3D_PROFILE(gather_burnt_ground_decals);
  gather_burnt_ground_decals_ecs_query(*g_entity_mgr,
    [&](const TMatrix &transform, float burnt_ground_decal_placer__decal_size, float &burnt_ground_decal_placer__time_passed,
      float burnt_ground_decal_placer__fade_in_time, float burnt_ground_decal_placer__fade_out_time) {
      const float decalDistSq = lengthSq(eye_pos - transform.getcol(3));
      if (decalDistSq > visibility_range * visibility_range)
        return;

      float progress = min(burnt_ground_decal_placer__time_passed / burnt_ground_decal_placer__fade_in_time, 1.f);
      const float fadeoutStart = burnt_ground_decal_placer__fade_in_time + BURNT_GROUND_EXTRA_FADE_OUT_TIME_S;
      if (burnt_ground_decal_placer__time_passed > fadeoutStart)
        progress = min((burnt_ground_decal_placer__time_passed - fadeoutStart) / burnt_ground_decal_placer__fade_out_time, 1.f) + 1.f;
      decal_processor({.worldPos = transform.getcol(3), .decalSize = burnt_ground_decal_placer__decal_size, .progress = progress});
    });
}
