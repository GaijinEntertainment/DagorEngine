// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "laserSightCoreES.cpp.inl"
ECS_DEF_PULL_VAR(laserSightCore);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc create_laser_screen_spot_buffer_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("laser_screen_spot_buffer"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
//start of 1 ro components at [1]
  {ECS_HASH("laser_decal_manager__is_compatible"), ecs::ComponentTypeInfo<bool>()}
};
static void create_laser_screen_spot_buffer_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_laser_screen_spot_buffer_es(evt
        , ECS_RW_COMP(create_laser_screen_spot_buffer_es_comps, "laser_screen_spot_buffer", UniqueBufHolder)
    , ECS_RO_COMP(create_laser_screen_spot_buffer_es_comps, "laser_decal_manager__is_compatible", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_laser_screen_spot_buffer_es_es_desc
(
  "create_laser_screen_spot_buffer_es",
  "prog/gameLibs/render/laserSightCore/laserSightCoreES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_laser_screen_spot_buffer_es_all_events),
  make_span(create_laser_screen_spot_buffer_es_comps+0, 1)/*rw*/,
  make_span(create_laser_screen_spot_buffer_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc update_laser_spots_buffer_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("laser_screen_spot_buffer"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
  {ECS_HASH("laser_screen_spot_count"), ecs::ComponentTypeInfo<int>()},
//start of 1 ro components at [2]
  {ECS_HASH("laser_decal_manager__is_compatible"), ecs::ComponentTypeInfo<bool>()}
};
static void update_laser_spots_buffer_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_laser_spots_buffer_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , components.manager()
    , ECS_RW_COMP(update_laser_spots_buffer_es_comps, "laser_screen_spot_buffer", UniqueBufHolder)
    , ECS_RW_COMP(update_laser_spots_buffer_es_comps, "laser_screen_spot_count", int)
    , ECS_RO_COMP(update_laser_spots_buffer_es_comps, "laser_decal_manager__is_compatible", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_laser_spots_buffer_es_es_desc
(
  "update_laser_spots_buffer_es",
  "prog/gameLibs/render/laserSightCore/laserSightCoreES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_laser_spots_buffer_es_all_events),
  make_span(update_laser_spots_buffer_es_comps+0, 2)/*rw*/,
  make_span(update_laser_spots_buffer_es_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc laser_decals_created_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("laser_decals_manager"), ecs::ComponentTypeInfo<LaserDecalManager>()},
//start of 6 ro components at [1]
  {ECS_HASH("laser_decals_initial_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("laser_decals_generator_shader"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("laser_decals_render_shader"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("laser_point_decal_culler_shader"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("laser_decal_init_shader"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("laser_decal_manager__is_compatible"), ecs::ComponentTypeInfo<bool>()}
};
static void laser_decals_created_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    laser_decals_created_es_event_handler(evt
        , ECS_RW_COMP(laser_decals_created_es_event_handler_comps, "laser_decals_manager", LaserDecalManager)
    , ECS_RO_COMP(laser_decals_created_es_event_handler_comps, "laser_decals_initial_count", int)
    , ECS_RO_COMP(laser_decals_created_es_event_handler_comps, "laser_decals_generator_shader", ecs::string)
    , ECS_RO_COMP(laser_decals_created_es_event_handler_comps, "laser_decals_render_shader", ecs::string)
    , ECS_RO_COMP(laser_decals_created_es_event_handler_comps, "laser_point_decal_culler_shader", ecs::string)
    , ECS_RO_COMP(laser_decals_created_es_event_handler_comps, "laser_decal_init_shader", ecs::string)
    , ECS_RO_COMP(laser_decals_created_es_event_handler_comps, "laser_decal_manager__is_compatible", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc laser_decals_created_es_event_handler_es_desc
(
  "laser_decals_created_es",
  "prog/gameLibs/render/laserSightCore/laserSightCoreES.cpp.inl",
  ecs::EntitySystemOps(nullptr, laser_decals_created_es_event_handler_all_events),
  make_span(laser_decals_created_es_event_handler_comps+0, 1)/*rw*/,
  make_span(laser_decals_created_es_event_handler_comps+1, 6)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc laser_decals_prepare_render_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("laser_decals_manager"), ecs::ComponentTypeInfo<LaserDecalManager>()},
//start of 1 ro components at [1]
  {ECS_HASH("laser_decal_manager__is_compatible"), ecs::ComponentTypeInfo<bool>()}
};
static void laser_decals_prepare_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    laser_decals_prepare_render_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(laser_decals_prepare_render_es_comps, "laser_decals_manager", LaserDecalManager)
    , ECS_RO_COMP(laser_decals_prepare_render_es_comps, "laser_decal_manager__is_compatible", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc laser_decals_prepare_render_es_es_desc
(
  "laser_decals_prepare_render_es",
  "prog/gameLibs/render/laserSightCore/laserSightCoreES.cpp.inl",
  ecs::EntitySystemOps(nullptr, laser_decals_prepare_render_es_all_events),
  make_span(laser_decals_prepare_render_es_comps+0, 1)/*rw*/,
  make_span(laser_decals_prepare_render_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc disable_laser_on_death_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("laserBeamTracerId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("laserDecalId"), ecs::ComponentTypeInfo<int>()}
};
static void disable_laser_on_death_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    disable_laser_on_death_es(evt
        , components.manager()
    , ECS_RW_COMP(disable_laser_on_death_es_comps, "laserBeamTracerId", int)
    , ECS_RW_COMP(disable_laser_on_death_es_comps, "laserDecalId", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc disable_laser_on_death_es_es_desc
(
  "disable_laser_on_death_es",
  "prog/gameLibs/render/laserSightCore/laserSightCoreES.cpp.inl",
  ecs::EntitySystemOps(nullptr, disable_laser_on_death_es_all_events),
  make_span(disable_laser_on_death_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc disable_laser_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("laserBeamTracerId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("laserDecalId"), ecs::ComponentTypeInfo<int>()},
//start of 3 ro components at [2]
  {ECS_HASH("laserActive"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("laserAvailable"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("laserVisible"), ecs::ComponentTypeInfo<bool>()}
};
static void disable_laser_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    disable_laser_es(evt
        , components.manager()
    , ECS_RO_COMP(disable_laser_es_comps, "laserActive", bool)
    , ECS_RO_COMP(disable_laser_es_comps, "laserAvailable", bool)
    , ECS_RO_COMP(disable_laser_es_comps, "laserVisible", bool)
    , ECS_RW_COMP(disable_laser_es_comps, "laserBeamTracerId", int)
    , ECS_RW_COMP(disable_laser_es_comps, "laserDecalId", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc disable_laser_es_es_desc
(
  "disable_laser_es",
  "prog/gameLibs/render/laserSightCore/laserSightCoreES.cpp.inl",
  ecs::EntitySystemOps(nullptr, disable_laser_es_all_events),
  make_span(disable_laser_es_comps+0, 2)/*rw*/,
  make_span(disable_laser_es_comps+2, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","laserActive,laserAvailable,laserVisible");
static constexpr ecs::ComponentDesc update_lasers_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("laserBeamTracerId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("laserDecalId"), ecs::ComponentTypeInfo<int>()},
//start of 18 ro components at [2]
  {ECS_HASH("laserBeamColor"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("laserBeamMaxLength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("laserStartSize"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("laserMaxSize"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("laserMaxIntensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("laserScrollingSpeed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("laserActive"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("laserAvailable"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("laserVisible"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("laser_data__rayHit"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("laser_data__fxPos"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("laser_data__fxDir"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("laser_data__laserLen"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("laser_data__gunOwner"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("laser_data__playerId"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("laser_sight__is_compatible"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("laser_data__dotIntensity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("laserBeamDotColor"), ecs::ComponentTypeInfo<Point3>()}
};
static void update_lasers_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_lasers_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        , components.manager()
    , ECS_RW_COMP(update_lasers_es_comps, "laserBeamTracerId", int)
    , ECS_RW_COMP(update_lasers_es_comps, "laserDecalId", int)
    , ECS_RO_COMP(update_lasers_es_comps, "laserBeamColor", Point3)
    , ECS_RO_COMP(update_lasers_es_comps, "laserBeamMaxLength", float)
    , ECS_RO_COMP(update_lasers_es_comps, "laserStartSize", float)
    , ECS_RO_COMP(update_lasers_es_comps, "laserMaxSize", float)
    , ECS_RO_COMP(update_lasers_es_comps, "laserMaxIntensity", float)
    , ECS_RO_COMP(update_lasers_es_comps, "laserScrollingSpeed", float)
    , ECS_RO_COMP(update_lasers_es_comps, "laserActive", bool)
    , ECS_RO_COMP(update_lasers_es_comps, "laserAvailable", bool)
    , ECS_RO_COMP(update_lasers_es_comps, "laserVisible", bool)
    , ECS_RO_COMP(update_lasers_es_comps, "laser_data__rayHit", Point3)
    , ECS_RO_COMP(update_lasers_es_comps, "laser_data__fxPos", Point3)
    , ECS_RO_COMP(update_lasers_es_comps, "laser_data__fxDir", Point3)
    , ECS_RO_COMP(update_lasers_es_comps, "laser_data__laserLen", float)
    , ECS_RO_COMP(update_lasers_es_comps, "laser_data__gunOwner", ecs::EntityId)
    , ECS_RO_COMP(update_lasers_es_comps, "laser_data__playerId", ecs::EntityId)
    , ECS_RO_COMP(update_lasers_es_comps, "laser_sight__is_compatible", bool)
    , ECS_RO_COMP(update_lasers_es_comps, "laser_data__dotIntensity", float)
    , ECS_RO_COMP(update_lasers_es_comps, "laserBeamDotColor", Point3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_lasers_es_es_desc
(
  "update_lasers_es",
  "prog/gameLibs/render/laserSightCore/laserSightCoreES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_lasers_es_all_events),
  make_span(update_lasers_es_comps+0, 2)/*rw*/,
  make_span(update_lasers_es_comps+2, 18)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc get_laser_manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("laser_decals_manager"), ecs::ComponentTypeInfo<LaserDecalManager>()}
};
static ecs::CompileTimeQueryDesc get_laser_manager_ecs_query_desc
(
  "get_laser_manager_ecs_query",
  make_span(get_laser_manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_laser_manager_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_laser_manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_laser_manager_ecs_query_comps, "laser_decals_manager", LaserDecalManager)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_laser_spots_ecs_query_comps[] =
{
//start of 8 ro components at [0]
  {ECS_HASH("laserActive"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("laserAvailable"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("laserVisible"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("laser_data__fxPos"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("laser_data__fxDir"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("laser_data__intesity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("laser_data__crownSize"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("laserBeamColor"), ecs::ComponentTypeInfo<Point3>()}
};
static ecs::CompileTimeQueryDesc gather_laser_spots_ecs_query_desc
(
  "gather_laser_spots_ecs_query",
  empty_span(),
  make_span(gather_laser_spots_ecs_query_comps+0, 8)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gather_laser_spots_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, gather_laser_spots_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_laser_spots_ecs_query_comps, "laserActive", bool)
            , ECS_RO_COMP(gather_laser_spots_ecs_query_comps, "laserAvailable", bool)
            , ECS_RO_COMP(gather_laser_spots_ecs_query_comps, "laserVisible", bool)
            , ECS_RO_COMP(gather_laser_spots_ecs_query_comps, "laser_data__fxPos", Point3)
            , ECS_RO_COMP(gather_laser_spots_ecs_query_comps, "laser_data__fxDir", Point3)
            , ECS_RO_COMP(gather_laser_spots_ecs_query_comps, "laser_data__intesity", float)
            , ECS_RO_COMP(gather_laser_spots_ecs_query_comps, "laser_data__crownSize", float)
            , ECS_RO_COMP(gather_laser_spots_ecs_query_comps, "laserBeamColor", Point3)
            );

        }while (++comp != compE);
    }
  );
}
