// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "waterRipplesGenerator.h"
#include "waterRipplesGeneratorEvent.h"

#include <daECS/core/updateStage.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/updateStageRender.h>
#include <render/renderer.h>
#include <render/renderEvent.h>
#include <render/rendererFeatures.h>
#include <main/water.h>
#include <main/level.h>

#include <render/waterRipples.h>
#include <ioSys/dag_dataBlock.h>

class WaterRipplesCTM final : public ecs::ComponentTypeManager
{
public:
  using component_type = WaterRipples;

  void create(void *d, ecs::EntityManager &, ecs::EntityId, const ecs::ComponentsMap &, ecs::component_index_t) override
  {
    const DataBlock &blk = *::dgs_get_game_params()->getBlockByNameEx("waterRipples");
    *(ecs::PtrComponentType<WaterRipples>::ptr_type(d)) =
      new WaterRipples(blk.getReal("worldSize", 64.0f), blk.getInt("texSize", 256));
  }

  void destroy(void *d) override { delete *(ecs::PtrComponentType<WaterRipples>::ptr_type(d)); }
};

ECS_REGISTER_MANAGED_TYPE(WaterRipples, nullptr, WaterRipplesCTM);
ECS_AUTO_REGISTER_COMPONENT(WaterRipples, "water_ripples", nullptr, 0);

template <typename Callable>
inline bool get_water_ripples_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
inline bool get_water_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
static void effect_ripple_ecs_query(Callable c);

static void destroy_water_ripples_entity()
{
  g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("water_ripples")));
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, EventLevelLoaded, ChangeRenderFeatures)
static void attempt_to_enable_water_ripples_es(const ecs::Event &, FFTWater &water)
{
  if (!renderer_has_feature(FeatureRenderFlags::RIPPLES))
  {
    destroy_water_ripples_entity();
    return;
  }
  if (g_entity_mgr->getSingletonEntity(ECS_HASH("water_ripples")))
    return;
  ecs::ComponentsInitializer init;
  init[ECS_HASH("water_ripples__water_level")] = fft_water::get_level(&water);

  g_entity_mgr->createEntityAsync("water_ripples", eastl::move(init));
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(const FFTWater &water)
static void disable_water_ripples_es(const ecs::Event &) { destroy_water_ripples_entity(); }

ECS_TAG(render)
ECS_ON_EVENT(AfterDeviceReset)
static void reset_water_ripples_es(const ecs::Event &, WaterRipples &water_ripples) { water_ripples.reset(); }

static BBox3 get_origin_box(const Point3 &camera_position, const WaterRipples &water_ripples)
{
  return BBox3(camera_position, water_ripples.getWorldSize());
}

void GatherEmittersEventCtx::addEmitter(const gamephys::Loc &location,
  Point4 &emittance_history,
  float mass,
  float speed,
  float maxSpeed,
  float minEmitStrength,
  float maxEmitStrength,
  float emitDistance,
  float emitPeriod,
  const BBox3 &box,
  bool emitPeriodicRipples)
{
  float radius = (box.lim[1].x - box.lim[0].x + box.lim[1].z - box.lim[0].z) * 0.25;
  if (radius <= 1e-5) // bounding box can be empty, because resource hasn't been loaded yet
    return;

  float &period = emittance_history.w;

  // Vehicles only emit waves if they are moving, or periodically when turned on
  bool movedEnough = (Point3::xyz(emittance_history) - Point3::xyz(location.P)).length() >= emitDistance;
  if (!movedEnough)
  {
    if (emitPeriodicRipples)
      period -= dt;
    if (period > 0)
      return;
  }

  float speedRate = speed / maxSpeed;
  float strength = lerp(minEmitStrength, maxEmitStrength, speedRate);
  water_ripples::add_impulse(Point3(location.P), mass * strength, radius);

  emittance_history = Point4::xyzV(location.P, emitPeriod);
};

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // required to increase parallelity (start `animchar_before_render_es` as early as possible)
static void update_water_ripples_es(
  const UpdateStageInfoBeforeRender &evt, WaterRipples &water_ripples, BBox3 &water_ripples__origin_bbox)
{
  if (evt.dt <= 0.0)
    return;

  TIME_D3D_PROFILE(water_ripples);
  static int render_with_normalmapVarId = get_shader_variable_id("render_with_normalmap", true);
  ShaderGlobal::set_int(render_with_normalmapVarId, 0);

  water_ripples__origin_bbox = get_origin_box(evt.camPos, water_ripples);
  Point3 origin = water_ripples__origin_bbox.center();

  water_ripples.updateSteps(evt.dt);

  get_water_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("water")), [dt = evt.dt](FFTWater &water) {
    GatherEmittersEventCtx evtCtx{water, dt};
    g_entity_mgr->broadcastEventImmediate(GatherEmittersForWaterRipples(&evtCtx));
  });

  effect_ripple_ecs_query(
    [&](const TMatrix &transform, bool &water_ripple__requireStart, float &water_ripple__mass, float &water_ripple__radius) {
      if (water_ripple__requireStart)
      {
        water_ripples::add_impulse(transform.getcol(3), water_ripple__mass, water_ripple__radius);
        water_ripple__requireStart = false;
      }
    });

  water_ripples.advance(evt.dt, Point2::xz(origin));
}

namespace water_ripples
{
void add_impulse(const Point3 &pos, float mass, float volume_radius)
{
  get_water_ripples_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("water_ripples")),
    [pos, strength = min(mass, 100.0f), radius = volume_radius](const BBox3 &water_ripples__origin_bbox, WaterRipples &water_ripples) {
      if (water_ripples__origin_bbox & pos)
        water_ripples.placeDropCorrected(Point2::xz(pos), strength, radius);
    });
}
} // namespace water_ripples
