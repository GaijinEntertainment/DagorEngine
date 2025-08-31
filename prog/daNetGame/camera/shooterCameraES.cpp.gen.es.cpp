// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "shooterCameraES.cpp.inl"
ECS_DEF_PULL_VAR(shooterCamera);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc shooter_cam_act_es_comps[] =
{
//start of 10 rw components at [0]
  {ECS_HASH("shooter_cam"), ecs::ComponentTypeInfo<shootercam::Camera>()},
  {ECS_HASH("camera__offset"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("camera__pivotPos"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("camera__velTau"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera__velFactor"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera__tau"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera__vertOffset"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera__tauPos"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera__tmToAnimRatio"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera__animTau"), ecs::ComponentTypeInfo<float>()},
//start of 2 ro components at [10]
  {ECS_HASH("zoom"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("shooter_cam__alternative_settings"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static void shooter_cam_act_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    shooter_cam_act_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(shooter_cam_act_es_comps, "shooter_cam", shootercam::Camera)
    , ECS_RW_COMP(shooter_cam_act_es_comps, "camera__offset", Point3)
    , ECS_RW_COMP(shooter_cam_act_es_comps, "camera__pivotPos", Point3)
    , ECS_RW_COMP(shooter_cam_act_es_comps, "camera__velTau", float)
    , ECS_RW_COMP(shooter_cam_act_es_comps, "camera__velFactor", float)
    , ECS_RW_COMP(shooter_cam_act_es_comps, "camera__tau", float)
    , ECS_RW_COMP(shooter_cam_act_es_comps, "camera__vertOffset", float)
    , ECS_RW_COMP(shooter_cam_act_es_comps, "camera__tauPos", float)
    , ECS_RW_COMP(shooter_cam_act_es_comps, "camera__tmToAnimRatio", float)
    , ECS_RW_COMP(shooter_cam_act_es_comps, "camera__animTau", float)
    , ECS_RO_COMP_OR(shooter_cam_act_es_comps, "zoom", float(0.f))
    , ECS_RO_COMP_OR(shooter_cam_act_es_comps, "shooter_cam__alternative_settings", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc shooter_cam_act_es_es_desc
(
  "shooter_cam_act_es",
  "prog/daNetGame/camera/shooterCameraES.cpp.inl",
  ecs::EntitySystemOps(shooter_cam_act_es_all),
  make_span(shooter_cam_act_es_comps+0, 10)/*rw*/,
  make_span(shooter_cam_act_es_comps+10, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"after_camera_sync","before_camera_sync");
static constexpr ecs::ComponentDesc shooter_cam_init_es_event_handler_comps[] =
{
//start of 10 rw components at [0]
  {ECS_HASH("shooter_cam"), ecs::ComponentTypeInfo<shootercam::Camera>()},
  {ECS_HASH("shooter_cam__lastDir"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("shooter_cam__wasAlternative"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("shooter_cam__punchTau"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shooter_cam__punchFadeoutTau"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shooter_cam__punchXRange"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("shooter_cam__punchYRange"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("shooter_cam__punchStrength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shooter_cam__tauOnChange"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("shooter_cam__rayMatId"), ecs::ComponentTypeInfo<int>()},
//start of 11 ro components at [10]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("shooter_cam__blk"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("shooter_cam__alt_blk"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("shooter_cam__alternative_settings"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("shooter_cam__rayMat"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("shooter_cam__punch_tau"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("shooter_cam__punch_fadeout_tau"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("shooter_cam__punch_x_range"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("shooter_cam__punch_y_range"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("shooter_cam__punch_strength"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("shooter_cam__tau_on_change"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static void shooter_cam_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    shooter_cam_init_es_event_handler(evt
        , ECS_RO_COMP(shooter_cam_init_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(shooter_cam_init_es_event_handler_comps, "shooter_cam__blk", ecs::string)
    , ECS_RO_COMP_PTR(shooter_cam_init_es_event_handler_comps, "shooter_cam__alt_blk", ecs::string)
    , ECS_RO_COMP(shooter_cam_init_es_event_handler_comps, "shooter_cam__alternative_settings", bool)
    , ECS_RW_COMP(shooter_cam_init_es_event_handler_comps, "shooter_cam", shootercam::Camera)
    , ECS_RW_COMP(shooter_cam_init_es_event_handler_comps, "shooter_cam__lastDir", Point3)
    , ECS_RW_COMP(shooter_cam_init_es_event_handler_comps, "shooter_cam__wasAlternative", bool)
    , ECS_RW_COMP(shooter_cam_init_es_event_handler_comps, "shooter_cam__punchTau", float)
    , ECS_RW_COMP(shooter_cam_init_es_event_handler_comps, "shooter_cam__punchFadeoutTau", float)
    , ECS_RW_COMP(shooter_cam_init_es_event_handler_comps, "shooter_cam__punchXRange", Point2)
    , ECS_RW_COMP(shooter_cam_init_es_event_handler_comps, "shooter_cam__punchYRange", Point2)
    , ECS_RW_COMP(shooter_cam_init_es_event_handler_comps, "shooter_cam__punchStrength", float)
    , ECS_RW_COMP(shooter_cam_init_es_event_handler_comps, "shooter_cam__tauOnChange", float)
    , ECS_RW_COMP(shooter_cam_init_es_event_handler_comps, "shooter_cam__rayMatId", int)
    , ECS_RO_COMP_PTR(shooter_cam_init_es_event_handler_comps, "shooter_cam__rayMat", ecs::string)
    , ECS_RO_COMP_OR(shooter_cam_init_es_event_handler_comps, "shooter_cam__punch_tau", float(0.03f))
    , ECS_RO_COMP_OR(shooter_cam_init_es_event_handler_comps, "shooter_cam__punch_fadeout_tau", float(0.1f))
    , ECS_RO_COMP_OR(shooter_cam_init_es_event_handler_comps, "shooter_cam__punch_x_range", Point2(Point2(-0.25f, 0.25f)))
    , ECS_RO_COMP_OR(shooter_cam_init_es_event_handler_comps, "shooter_cam__punch_y_range", Point2(Point2(-0.125f, 0.25f)))
    , ECS_RO_COMP_OR(shooter_cam_init_es_event_handler_comps, "shooter_cam__punch_strength", float(3.f))
    , ECS_RO_COMP_OR(shooter_cam_init_es_event_handler_comps, "shooter_cam__tau_on_change", float(0.4f))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc shooter_cam_init_es_event_handler_es_desc
(
  "shooter_cam_init_es",
  "prog/daNetGame/camera/shooterCameraES.cpp.inl",
  ecs::EntitySystemOps(nullptr, shooter_cam_init_es_event_handler_all_events),
  make_span(shooter_cam_init_es_event_handler_comps+0, 10)/*rw*/,
  make_span(shooter_cam_init_es_event_handler_comps+10, 11)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
