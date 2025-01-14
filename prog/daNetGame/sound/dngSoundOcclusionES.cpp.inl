// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/fixed_vector.h>

#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>

#include <atomic>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathUtils.h>
#include <gamePhys/collision/collisionLib.h>
#include <soundSystem/occlusion.h>
#include "main/gameObjects.h"
#include "main/level.h"
#include "net/time.h"

template <typename Callable>
static void game_objects_ecs_query(Callable c);

static constexpr float g_cached_indoors_move_theshold = 5.f;
static constexpr float g_cached_indoors_time_threshold = 1.f;
static float g_cached_indoors_next_time = 0.f;
static Point3 g_cached_indoors_last_listener_pos;

static std::atomic_bool g_cached_indoors_inited = ATOMIC_VAR_INIT(false);
static eastl::fixed_vector<TMatrix, 8, true> g_cached_indoors;
static bool g_is_listener_indoor = false;
static float g_listener_height_above_ground = 0.f;

static bool is_pos_indoor_impl(const Point3 &pos)
{
  for (const TMatrix &tm : g_cached_indoors)
  {
    if (BBox3::IDENT & (tm * pos))
      return true;
  }
  return false;
}

static float height_above_ground_impl(const Point3 &pos)
{
  const float lmeshHt = dacoll::traceht_lmesh(Point2::xz(pos));
  return pos.y - lmeshHt; // if no grnd or there is a hole then result should be far
}

static void cache_indoors_impl(const Point3 &listener, float radius)
{
  g_cached_indoors.clear();
  game_objects_ecs_query([&](const GameObjects &game_objects) {
    if (game_objects.indoors == nullptr)
      return;
    const BBox3 bbox3(listener, radius);
    Point3_vec4 minPoint(bbox3.lim[0]);
    Point3_vec4 maxPoint(bbox3.lim[1]);
    bbox3f aabb;
    aabb.bmin = v_ldu(&minPoint.x);
    aabb.bmax = v_ldu(&maxPoint.x);
    game_objects.indoors->boxCull<false, true>(aabb, 0, 0, [&](scene::node_index /*ni*/, const mat44f &m) {
      TMatrix tm;
      v_mat_43cu_from_mat44(tm.m[0], m);
      g_cached_indoors.push_back(inverse(tm));
    });
  });
}

static PhysMat::MatID g_ray_mat_id = PHYSMAT_INVALID;
static constexpr const char *g_ray_mat_name = "sndOcclusionRay";

static float g_indoor_mul = 1.f;        // INSIDE (listener AND source)
static float g_outdoor_mul = 1.f;       // OUTSIDE (listener AND source)
static float g_sourceIndoor_mul = 1.f;  // source is INSIDE, listener is OUTSIDE
static float g_sourceOutdoor_mul = 1.f; // source is OUTSIDE, listener is INSIDE
static float g_undergroundDepthDistInv = 0.f;
static constexpr float g_suppressed_value = 2.f;

static float indoor_factor(const Point3 &source, float value)
{
  if (is_pos_indoor_impl(source))
  {
    if (g_is_listener_indoor)
      return g_indoor_mul * value;
    return g_sourceIndoor_mul * value;
  }
  if (g_is_listener_indoor)
    return g_sourceOutdoor_mul * value;
  return g_outdoor_mul * value;
}

static float ground_factor(const Point3 &source, float value)
{
  const float sourceHeightAboveGround = height_above_ground_impl(source);

  if (g_listener_height_above_ground > 0.f && sourceHeightAboveGround < 0.f)
  {
    // listener is above, source is underground
    const float undergroundDepth = -sourceHeightAboveGround;
    const float t = saturate(undergroundDepth * g_undergroundDepthDistInv);
    return lerp(value, g_suppressed_value, t);
  }

  if (g_listener_height_above_ground < 0.f && sourceHeightAboveGround > 0.f)
  {
    // listener is underground, source is above
    const float undergroundDepth = -g_listener_height_above_ground;
    const float t = saturate(undergroundDepth * g_undergroundDepthDistInv);
    return lerp(value, g_suppressed_value, t);
  }

  return value;
}

static float trace_occlusion_impl(const Point3 &from, const Point3 &to, float near, float far)
{
  float value = 0.f;

  const Point3 dir = to - from;

  const float distance = length(dir);

  if (distance > 0.01)
  {
    float accumulated = 0.f;
    float maximum = 0.f;
    dacoll::rayhit_normalized_sound_occlusion(from, dir / distance, distance, g_ray_mat_id, accumulated, maximum);

    value = maximum;

    value = indoor_factor(to, value);

    value = ground_factor(to, value);
  }

  if (distance > near)
    value = cvt(distance, near, far, value, 0.f);

  return value;
}

static void before_trace_proc(const Point3 &listener, float far)
{
  if (!g_cached_indoors_inited || get_sync_time() >= g_cached_indoors_next_time ||
      lengthSq(listener - g_cached_indoors_last_listener_pos) > sqr(g_cached_indoors_move_theshold))
  {
    g_cached_indoors_inited = false;
    g_cached_indoors.clear();

    if (!lengthSq(listener) || !is_level_loaded())
      return;

    g_cached_indoors_next_time = get_sync_time() + g_cached_indoors_time_threshold;
    g_cached_indoors_last_listener_pos = listener;
    g_cached_indoors_inited = true;

    cache_indoors_impl(listener, g_cached_indoors_move_theshold + far);
  }
  g_is_listener_indoor = is_pos_indoor_impl(listener);
  g_listener_height_above_ground = height_above_ground_impl(listener);
}

ECS_TAG(sound)
ECS_ON_EVENT(EventGameObjectsCreated)
ECS_REQUIRE(ecs::Tag msg_sink)
static void dngsound_occlusion_gameobjects_created_es(const ecs::Event &) { g_cached_indoors_inited = false; }

ECS_TAG(sound)
ECS_ON_EVENT(on_appear, EventLevelLoaded)
ECS_REQUIRE(ecs::Tag soundOcclusion)
static void dngsound_occlusion_enable_es(const ecs::Event &,
  float sound_occlusion__indoorMul,
  float sound_occlusion__outdoorMul,
  float sound_occlusion__sourceIndoorMul,
  float sound_occlusion__sourceOutdoorMul,
  float sound_occlusion__undergroundDepthDist)
{
  const bool enabled = sndsys::occlusion::is_inited() && is_level_loaded();
  if (enabled)
  {
    g_ray_mat_id = PhysMat::getMaterialId(g_ray_mat_name);
    sndsys::occlusion::set_trace_proc(&trace_occlusion_impl);
    sndsys::occlusion::set_before_trace_proc(&before_trace_proc);

    g_indoor_mul = sound_occlusion__indoorMul;
    g_outdoor_mul = sound_occlusion__outdoorMul;
    g_sourceIndoor_mul = sound_occlusion__sourceIndoorMul;
    g_sourceOutdoor_mul = sound_occlusion__sourceOutdoorMul;
    g_undergroundDepthDistInv = safeinv(sound_occlusion__undergroundDepthDist);
  }
  sndsys::occlusion::enable(enabled);
  g_cached_indoors_inited = false;
}

ECS_TAG(sound)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::Tag soundOcclusion)
static void dngsound_occlusion_disappear_es(const ecs::Event &) { sndsys::occlusion::enable(false); }
