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
namespace var
{
static ShaderVariableInfo water_ripples_drop_force("water_ripples_drop_force");
static ShaderVariableInfo water_ripples_back_force("water_ripples_back_force");
static ShaderVariableInfo water_ripples_damping("water_ripples_damping");
static ShaderVariableInfo water_ripples_distortion_params_3d("water_ripples_distortion_params_3d");
static ShaderVariableInfo water_ripples_distortion_params_puddles("water_ripples_distortion_params_puddles");
} // namespace var

template <typename Callable>
inline bool get_water_ripples_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);
template <typename Callable>
inline bool get_water_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);
template <typename Callable>
static void effect_ripple_ecs_query(ecs::EntityManager &manager, Callable c);
template <typename Callable>
static void get_fft_water_quality_ecs_query(ecs::EntityManager &manager, Callable c);


// Below this minimum radius ripples become visually square due to low resolution.
// Check impulseVolumeRadius in WT for the same purpose.
static constexpr float MINIMUM_RIPPLE_RADIUS = 0.3f;

static ecs::EntityId water_ripples_eid = ecs::INVALID_ENTITY_ID;

enum class RipplesQuality
{
  LOW = 0,
  MEDIUM = 1,
  HIGH = 2,
  COUNT = 3,
};

RipplesQuality strToRipplesQuality(const char *quality_str)
{
  eastl::string_view quality = quality_str;
  if (quality == "low")
    return RipplesQuality::LOW;
  else if (quality == "medium")
    return RipplesQuality::MEDIUM;
  else if (quality == "high")
    return RipplesQuality::MEDIUM;
  else if (quality == "ultra")
    return RipplesQuality::HIGH;
  logerr("Ripples: can't convert str(`%s`) to quality", quality_str);

  return RipplesQuality::HIGH;
}

RipplesQuality getRipplesQuality()
{
  bool found = false;
  RipplesQuality quality = RipplesQuality::HIGH;
  get_fft_water_quality_ecs_query(*g_entity_mgr, [&quality, &found](const ecs::string &render_settings__fftWaterQuality) {
    found = true;
    quality = strToRipplesQuality(render_settings__fftWaterQuality.c_str());
  });

  if (!found)
    logerr("Ripples: can't get fftWaterQuality from ecs");

  return quality;
}

class WaterRipplesCTM final : public ecs::ComponentTypeManager
{
public:
  using component_type = WaterRipples;

  void create(void *d, ecs::EntityManager &, ecs::EntityId, const ecs::ComponentsMap &, ecs::component_index_t) override
  {
    struct RipplesSetting
    {
      float worldSize;
      int texSize;
      bool useFlowmap;
      bool useDistortion;
    };

    auto quality = getRipplesQuality();
    eastl::array<RipplesSetting, eastl::to_underlying(RipplesQuality::COUNT)> ripplesSettings{{
      {64.0f, 256, false, false}, // Low
      {64.0f, 256, true, true},   // Medium
      {64.0f, 512, true, true}    // High
    }};

    auto &settings = ripplesSettings[eastl::to_underlying(quality)];

    const DataBlock &blk = *::dgs_get_game_params()->getBlockByNameEx("waterRipples");
    *(ecs::PtrComponentType<WaterRipples>::ptr_type(d)) = new WaterRipples(settings.worldSize, settings.texSize,
      blk.getReal("waterDisplacementMax", 0.125f), settings.useFlowmap, settings.useDistortion);
  }

  void destroy(void *d) override { delete *(ecs::PtrComponentType<WaterRipples>::ptr_type(d)); }
};

ECS_REGISTER_MANAGED_TYPE(WaterRipples, nullptr, WaterRipplesCTM);
ECS_AUTO_REGISTER_COMPONENT(WaterRipples, "water_ripples", nullptr, 0);

static void destroy_water_ripples_entity(ecs::EntityManager &manager)
{
  manager.destroyEntity(water_ripples_eid);
  water_ripples_eid = ecs::INVALID_ENTITY_ID;
}

ECS_TAG(render)
ECS_TRACK(render_settings__fftWaterQuality)
ECS_REQUIRE(const ecs::string &render_settings__fftWaterQuality)
ECS_ON_EVENT(on_appear, ChangeRenderFeatures, EventLevelLoaded)
static void attempt_to_enable_water_ripples_es(const ecs::Event &, ecs::EntityManager &manager)
{
  destroy_water_ripples_entity(manager);

  const DataBlock *graphics = ::dgs_get_settings()->getBlockByNameEx("graphics");
  const bool ripples = graphics->getBool("shouldRenderWaterRipples", true) && renderer_has_feature(FeatureRenderFlags::RIPPLES);

  if (!ripples)
    return;

  ecs::ComponentsInitializer init;
  water_ripples_eid = manager.createEntityAsync("water_ripples", eastl::move(init));
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(water_ripples_drop_force,
  water_ripples_back_force,
  water_ripples_damping,
  water_ripples_distortion_params_3d,
  water_ripples_distortion_params_puddles)
static void set_ripples_settings_es(const ecs::Event &,
  Point4 &water_ripples_drop_force,
  Point4 &water_ripples_back_force,
  Point4 &water_ripples_damping,
  Point4 &water_ripples_distortion_params_3d,
  Point4 &water_ripples_distortion_params_puddles)
{
  ShaderGlobal::set_float4(var::water_ripples_drop_force, water_ripples_drop_force);
  ShaderGlobal::set_float4(var::water_ripples_back_force, water_ripples_back_force);
  ShaderGlobal::set_float4(var::water_ripples_damping, water_ripples_damping);
  ShaderGlobal::set_float4(var::water_ripples_distortion_params_3d, water_ripples_distortion_params_3d);
  ShaderGlobal::set_float4(var::water_ripples_distortion_params_puddles, water_ripples_distortion_params_puddles);
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(const FFTWater &water)
static void disable_water_ripples_es(const ecs::Event &, ecs::EntityManager &manager) { destroy_water_ripples_entity(manager); }

ECS_TAG(render)
ECS_ON_EVENT(AfterDeviceReset)
static void reset_water_ripples_es(const ecs::Event &, WaterRipples &water_ripples) { water_ripples.reset(); }

static BBox3 get_origin_box(const Point3 &camera_position, const WaterRipples &water_ripples)
{
  return BBox3(camera_position, water_ripples.getWorldSize());
}

void GatherEmittersEventCtx::emitPeriodicRipple(
  const Point3 &position, Point4 &emittance_history, float mass, float minEmitStrength, float emitPeriod, float radius)
{
  float &period = emittance_history.w;

  if (emitPeriod < 0.0)
    return;

  period -= dt;
  if (period > 0)
    return;

  water_ripples::add_impulse(position, mass * minEmitStrength, 0, radius);
  period = emitPeriod;
}

void GatherEmittersEventCtx::addEmitter(EmitterType type,
  const gamephys::Loc &location,
  Point4 &emittance_history,
  float mass,
  const Point3 &velocity,
  float maxSpeed,
  float minEmitStrength,
  float maxEmitStrength,
  float emitDistance,
  float emitPeriod,
  const BBox3 &box,
  float radius,
  bool emitPeriodicRipples)
{
  if (box.width().lengthSq() <= 1e-5) // bounding box can be empty, because resource hasn't been loaded yet
    return;

  radius = max(radius, MINIMUM_RIPPLE_RADIUS);

  // Vehicles only emit waves if they are moving, or periodically when turned on
  bool movedEnough = (Point3::xyz(emittance_history) - Point3::xyz(location.P)).length() >= emitDistance;
  if (!movedEnough)
  {
    if (type == EmitterType::Box)
    {
      // Even though the emitter is box-shaped, we still use spherical ripples for periodic emission.
      // For box emitters, the radius has a different meaning - it acts as an offset from the box size.
      // Therefore, we need to recalculate the radius to based on box size to generate spherical ripples.
      radius = (box.lim[1].x - box.lim[0].x + box.lim[1].z - box.lim[0].z) * 0.25;
    }

    if (emitPeriodicRipples)
      emitPeriodicRipple(Point3(location.P), emittance_history, mass, minEmitStrength, emitPeriod, radius);

    return;
  }

  float speed = velocity.length();
  float speedRate = speed / maxSpeed;
  float strength = lerp(minEmitStrength, maxEmitStrength, speedRate);

  switch (type)
  {
    case EmitterType::Spherical: water_ripples::add_impulse(Point3(location.P), mass * strength, mass * strength, radius); break;
    case EmitterType::Box:
      bool forwMov = (velocity * location.getFwd()) > 0;
      water_ripples::add_bow_wave(location.makeTM(), box, mass * strength, mass * strength, radius, forwMov);
      break;
  }

  emittance_history = Point4::xyzV(location.P, emitPeriod);
};

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // required to increase parallelity (start `animchar_before_render_es` as early as possible)
static void update_water_ripples_es(
  const UpdateStageInfoBeforeRender &evt, ecs::EntityManager &manager, WaterRipples &water_ripples, BBox3 &water_ripples__origin_bbox)
{
  if (evt.dt <= 0.0)
    return;

  TIME_D3D_PROFILE(water_ripples);
  static int render_with_normalmapVarId = get_shader_variable_id("render_with_normalmap", true);
  ShaderGlobal::set_int(render_with_normalmapVarId, 0);

  water_ripples__origin_bbox = get_origin_box(evt.camPos, water_ripples);
  Point3 origin = water_ripples__origin_bbox.center();

  water_ripples.updateSteps(evt.dt);

  get_water_ecs_query(manager, manager.getSingletonEntity(ECS_HASH("water")), [dt = evt.dt, &manager](FFTWater &water) {
    GatherEmittersEventCtx evtCtx{water, dt};
    manager.broadcastEventImmediate(GatherEmittersForWaterRipples(&evtCtx));
  });

  effect_ripple_ecs_query(manager,
    [&](const TMatrix &transform, bool &water_ripple__requireStart, float &water_ripple__mass, float &water_ripple__radius) {
      if (water_ripple__requireStart)
      {
        water_ripples::add_impulse(transform.getcol(3), water_ripple__mass, water_ripple__mass, water_ripple__radius);
        water_ripple__requireStart = false;
      }
    });

  water_ripples.advance(evt.dt, Point2::xz(origin));
}

namespace water_ripples
{
void add_impulse(const Point3 &pos, float strength, float distortion, float volume_radius)
{
  if (volume_radius < MINIMUM_RIPPLE_RADIUS)
    LOGERR_ONCE("WaterRipples: volume_radius is too small (%.3f), min is %.3f", volume_radius, MINIMUM_RIPPLE_RADIUS);

  get_water_ripples_ecs_query(*g_entity_mgr, g_entity_mgr->getSingletonEntity(ECS_HASH("water_ripples")),
    [pos, strength = min(strength, 100.0f), distortion = min(distortion, 100.0f), radius = volume_radius](
      const BBox3 &water_ripples__origin_bbox, WaterRipples &water_ripples) {
      if (water_ripples__origin_bbox & pos)
        water_ripples.placeDropCorrected(Point2::xz(pos), strength, distortion, radius);
    });
}

void add_bow_wave(const TMatrix &transform, BBox3 box, float mass, float distortion, float radius, bool foward_movement)
{
  get_water_ripples_ecs_query(*g_entity_mgr, g_entity_mgr->getSingletonEntity(ECS_HASH("water_ripples")),
    [transform, box, mass, radius, distortion, foward_movement](const BBox3 &water_ripples__origin_bbox, WaterRipples &water_ripples) {
      Point3 wCenter = transform * box.center();
      Point2 size = Point2::xz(box.width());

      Point2 localX = Point2::xz(transform.getcol(0));
      localX.normalize();
      Point2 localY = Point2::xz(transform.getcol(2));
      localY.normalize();

      if (water_ripples__origin_bbox & wCenter)
        // Naming is confusing, since it's produce only line in front of BBOX
        water_ripples.placeSolidBoxCorrected(Point2::xz(wCenter), size, foward_movement ? localX : -localX, localY, mass, distortion,
          radius);
    });
}

} // namespace water_ripples

template <typename Callable>
static void set_fft_water_quality_ecs_query(ecs::EntityManager &manager, Callable c);

#include <util/dag_console.h>
#include "camera/sceneCam.h"
static bool ripples_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("render.waterripples", "quality", 1, 2)
  {
    int quality = argc > 1 ? atoi(argv[1]) : 0;
    set_fft_water_quality_ecs_query(*g_entity_mgr, [&quality](ecs::string &render_settings__fftWaterQuality) {
      switch (quality)
      {
        case 0: render_settings__fftWaterQuality = "low"; break;
        case 1: render_settings__fftWaterQuality = "medium"; break;
        case 2: render_settings__fftWaterQuality = "ultra"; break;
        default: break;
      }
      console::print("fftWaterQuality set to: %s", render_settings__fftWaterQuality.c_str());
    });
  }
  CONSOLE_CHECK_NAME("render.waterripples", "spawnAtCamera", 1, 3)
  {
    float mass = argc > 1 ? atof(argv[1]) : 10;
    float radius = argc > 2 ? atof(argv[2]) : 2;
    TMatrix itm = get_cam_itm();
    water_ripples::add_impulse(itm.getcol(3), mass, mass, radius);
  }
  return found;
}
REGISTER_CONSOLE_HANDLER(ripples_console_handler);