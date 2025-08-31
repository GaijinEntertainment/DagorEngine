// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "projectorsES.cpp.inl"
ECS_DEF_PULL_VAR(projectors);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc update_projectors_atmosphere_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("projectors_manager"), ecs::ComponentTypeInfo<ProjectorsManager>()},
//start of 2 ro components at [1]
  {ECS_HASH("projectors_manager__atmosphereMoveDir"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("projectors_manager__atmosphereMoveSpeed"), ecs::ComponentTypeInfo<float>()}
};
static void update_projectors_atmosphere_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    update_projectors_atmosphere_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(update_projectors_atmosphere_es_comps, "projectors_manager", ProjectorsManager)
    , ECS_RO_COMP(update_projectors_atmosphere_es_comps, "projectors_manager__atmosphereMoveDir", Point3)
    , ECS_RO_COMP(update_projectors_atmosphere_es_comps, "projectors_manager__atmosphereMoveSpeed", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_projectors_atmosphere_es_es_desc
(
  "update_projectors_atmosphere_es",
  "prog/daNetGameLibs/projectors/render/projectorsES.cpp.inl",
  ecs::EntitySystemOps(update_projectors_atmosphere_es_all),
  make_span(update_projectors_atmosphere_es_comps+0, 1)/*rw*/,
  make_span(update_projectors_atmosphere_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,nullptr,"update_projector_es");
static constexpr ecs::ComponentDesc update_projector_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("projector__phase"), ecs::ComponentTypeInfo<float>()},
//start of 5 ro components at [1]
  {ECS_HASH("projector__id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("projector__period"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("projector__xAngleAmplitude"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("projector__zAngleAmplitude"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void update_projector_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    update_projector_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(update_projector_es_comps, "projector__id", int)
    , ECS_RW_COMP(update_projector_es_comps, "projector__phase", float)
    , ECS_RO_COMP(update_projector_es_comps, "projector__period", float)
    , ECS_RO_COMP(update_projector_es_comps, "projector__xAngleAmplitude", float)
    , ECS_RO_COMP(update_projector_es_comps, "projector__zAngleAmplitude", float)
    , ECS_RO_COMP(update_projector_es_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_projector_es_es_desc
(
  "update_projector_es",
  "prog/daNetGameLibs/projectors/render/projectorsES.cpp.inl",
  ecs::EntitySystemOps(update_projector_es_all),
  make_span(update_projector_es_comps+0, 1)/*rw*/,
  make_span(update_projector_es_comps+1, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc init_manager_es_event_handler_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("projectors_manager"), ecs::ComponentTypeInfo<ProjectorsManager>()},
  {ECS_HASH("projectors_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("projectors_manager__atmosphereMoveDir"), ecs::ComponentTypeInfo<Point3>()},
//start of 3 ro components at [3]
  {ECS_HASH("projectors_manager__atmosphereDensity"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("projectors_manager__noiseScale"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("projectors_manager__noiseStrength"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static void init_manager_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_manager_es_event_handler(evt
        , ECS_RW_COMP(init_manager_es_event_handler_comps, "projectors_manager", ProjectorsManager)
    , ECS_RW_COMP(init_manager_es_event_handler_comps, "projectors_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_manager_es_event_handler_comps, "projectors_manager__atmosphereMoveDir", Point3)
    , ECS_RO_COMP_OR(init_manager_es_event_handler_comps, "projectors_manager__atmosphereDensity", float(1.0f))
    , ECS_RO_COMP_OR(init_manager_es_event_handler_comps, "projectors_manager__noiseScale", float(1.0f))
    , ECS_RO_COMP_OR(init_manager_es_event_handler_comps, "projectors_manager__noiseStrength", float(0.0f))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_manager_es_event_handler_es_desc
(
  "init_manager_es",
  "prog/daNetGameLibs/projectors/render/projectorsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_manager_es_event_handler_all_events),
  make_span(init_manager_es_event_handler_comps+0, 3)/*rw*/,
  make_span(init_manager_es_event_handler_comps+3, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc create_projector_es_event_handler_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("projector__id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("projector__xAngleAmplitude"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("projector__zAngleAmplitude"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 5 ro components at [4]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("projector__color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("projector__angle"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("projector__length"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("projector__sourceWidth"), ecs::ComponentTypeInfo<float>()}
};
static void create_projector_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_projector_es_event_handler(evt
        , ECS_RO_COMP(create_projector_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(create_projector_es_event_handler_comps, "projector__id", int)
    , ECS_RO_COMP(create_projector_es_event_handler_comps, "projector__color", Point3)
    , ECS_RO_COMP(create_projector_es_event_handler_comps, "projector__angle", float)
    , ECS_RO_COMP(create_projector_es_event_handler_comps, "projector__length", float)
    , ECS_RO_COMP(create_projector_es_event_handler_comps, "projector__sourceWidth", float)
    , ECS_RW_COMP(create_projector_es_event_handler_comps, "projector__xAngleAmplitude", float)
    , ECS_RW_COMP(create_projector_es_event_handler_comps, "projector__zAngleAmplitude", float)
    , ECS_RW_COMP(create_projector_es_event_handler_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_projector_es_event_handler_es_desc
(
  "create_projector_es",
  "prog/daNetGameLibs/projectors/render/projectorsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_projector_es_event_handler_all_events),
  make_span(create_projector_es_event_handler_comps+0, 4)/*rw*/,
  make_span(create_projector_es_event_handler_comps+4, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc update_projector_color_changed_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("projector__id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("projector__color"), ecs::ComponentTypeInfo<Point3>()}
};
static void update_projector_color_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_projector_color_changed_es(evt
        , ECS_RO_COMP(update_projector_color_changed_es_comps, "projector__id", int)
    , ECS_RO_COMP(update_projector_color_changed_es_comps, "projector__color", Point3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_projector_color_changed_es_es_desc
(
  "update_projector_color_changed_es",
  "prog/daNetGameLibs/projectors/render/projectorsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_projector_color_changed_es_all_events),
  empty_span(),
  make_span(update_projector_color_changed_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","projector__color");
static constexpr ecs::ComponentDesc add_projector_ecs_query_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("projector__id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("projector__xAngleAmplitude"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("projector__zAngleAmplitude"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 5 ro components at [4]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("projector__color"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("projector__angle"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("projector__length"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("projector__sourceWidth"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc add_projector_ecs_query_desc
(
  "add_projector_ecs_query",
  make_span(add_projector_ecs_query_comps+0, 4)/*rw*/,
  make_span(add_projector_ecs_query_comps+4, 5)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void add_projector_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, add_projector_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(add_projector_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(add_projector_ecs_query_comps, "projector__id", int)
            , ECS_RO_COMP(add_projector_ecs_query_comps, "projector__color", Point3)
            , ECS_RO_COMP(add_projector_ecs_query_comps, "projector__angle", float)
            , ECS_RO_COMP(add_projector_ecs_query_comps, "projector__length", float)
            , ECS_RO_COMP(add_projector_ecs_query_comps, "projector__sourceWidth", float)
            , ECS_RW_COMP(add_projector_ecs_query_comps, "projector__xAngleAmplitude", float)
            , ECS_RW_COMP(add_projector_ecs_query_comps, "projector__zAngleAmplitude", float)
            , ECS_RW_COMP(add_projector_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc projector_update_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("projectors_manager"), ecs::ComponentTypeInfo<ProjectorsManager>()}
};
static ecs::CompileTimeQueryDesc projector_update_ecs_query_desc
(
  "projector_update_ecs_query",
  make_span(projector_update_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void projector_update_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, projector_update_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(projector_update_ecs_query_comps, "projectors_manager", ProjectorsManager)
            );

        }while (++comp != compE);
    }
  );
}
