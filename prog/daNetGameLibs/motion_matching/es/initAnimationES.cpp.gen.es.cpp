#include "initAnimationES.cpp.inl"
ECS_DEF_PULL_VAR(initAnimation);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_mm_limit_per_frame_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("main_database__perFrameLimit"), ecs::ComponentTypeInfo<int>()}
};
static void init_mm_limit_per_frame_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_mm_limit_per_frame_es(evt
        , ECS_RW_COMP(init_mm_limit_per_frame_es_comps, "main_database__perFrameLimit", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_mm_limit_per_frame_es_es_desc
(
  "init_mm_limit_per_frame_es",
  "prog/daNetGameLibs/motion_matching/es/initAnimationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_mm_limit_per_frame_es_all_events),
  make_span(init_mm_limit_per_frame_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc disable_motion_matching_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 1 rq components at [1]
  {ECS_HASH("deadEntity"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void disable_motion_matching_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    disable_motion_matching_es(evt
        , ECS_RW_COMP(disable_motion_matching_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc disable_motion_matching_es_es_desc
(
  "disable_motion_matching_es",
  "prog/daNetGameLibs/motion_matching/es/initAnimationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, disable_motion_matching_es_all_events),
  make_span(disable_motion_matching_es_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(disable_motion_matching_es_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc on_motion_matching_controller_removed_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 1 rq components at [1]
  {ECS_HASH("motion_matching__controller"), ecs::ComponentTypeInfo<MotionMatchingController>()}
};
static void on_motion_matching_controller_removed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    on_motion_matching_controller_removed_es(evt
        , ECS_RW_COMP(on_motion_matching_controller_removed_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc on_motion_matching_controller_removed_es_es_desc
(
  "on_motion_matching_controller_removed_es",
  "prog/daNetGameLibs/motion_matching/es/initAnimationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, on_motion_matching_controller_removed_es_all_events),
  make_span(on_motion_matching_controller_removed_es_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(on_motion_matching_controller_removed_es_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc init_animations_es_comps[] =
{
//start of 7 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("motion_matching__controller"), ecs::ComponentTypeInfo<MotionMatchingController>()},
  {ECS_HASH("motion_matching__enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("motion_matching__presetIdx"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("motion_matching__dataBaseEid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("motion_matching__goalNodesIdx"), ecs::ComponentTypeInfo<ecs::IntList>()},
  {ECS_HASH("motion_matching__goalFeature"), ecs::ComponentTypeInfo<FrameFeatures>()},
//start of 2 ro components at [7]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("motion_matching__dataBaseTemplateName"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void init_animations_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_animations_es(evt
        , ECS_RO_COMP(init_animations_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(init_animations_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RW_COMP(init_animations_es_comps, "motion_matching__controller", MotionMatchingController)
    , ECS_RW_COMP(init_animations_es_comps, "motion_matching__enabled", bool)
    , ECS_RW_COMP(init_animations_es_comps, "motion_matching__presetIdx", int)
    , ECS_RW_COMP(init_animations_es_comps, "motion_matching__dataBaseEid", ecs::EntityId)
    , ECS_RO_COMP(init_animations_es_comps, "motion_matching__dataBaseTemplateName", ecs::string)
    , ECS_RW_COMP(init_animations_es_comps, "motion_matching__goalNodesIdx", ecs::IntList)
    , ECS_RW_COMP(init_animations_es_comps, "motion_matching__goalFeature", FrameFeatures)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_animations_es_es_desc
(
  "init_animations_es",
  "prog/daNetGameLibs/motion_matching/es/initAnimationES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_animations_es_all_events),
  make_span(init_animations_es_comps+0, 7)/*rw*/,
  make_span(init_animations_es_comps+7, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<InvalidateAnimationDataBase,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc get_data_base_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("main_database__loaded"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dataBase"), ecs::ComponentTypeInfo<AnimationDataBase>()},
//start of 14 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("data_bases_paths"), ecs::ComponentTypeInfo<ecs::StringList>()},
  {ECS_HASH("main_database__availableTags"), ecs::ComponentTypeInfo<ecs::StringList>()},
  {ECS_HASH("main_database__presetsTagsName"), ecs::ComponentTypeInfo<ecs::StringList>()},
  {ECS_HASH("main_database__root_node"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("main_database__root_motion_a2d_node"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("main_database__direction_nodes"), ecs::ComponentTypeInfo<ecs::StringList>()},
  {ECS_HASH("main_database__direction_weights"), ecs::ComponentTypeInfo<ecs::FloatList>()},
  {ECS_HASH("main_database__center_of_mass_nodes"), ecs::ComponentTypeInfo<ecs::StringList>()},
  {ECS_HASH("main_database__center_of_mass_params"), ecs::ComponentTypeInfo<ecs::Point4List>()},
  {ECS_HASH("main_database__nodeMasksPath"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("main_database__pbcWeightOverrides"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("main_database__footLockerCtrlName"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("main_database__footLockerNodes"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static ecs::CompileTimeQueryDesc get_data_base_ecs_query_desc
(
  "get_data_base_ecs_query",
  make_span(get_data_base_ecs_query_comps+0, 2)/*rw*/,
  make_span(get_data_base_ecs_query_comps+2, 14)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_data_base_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_data_base_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_data_base_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(get_data_base_ecs_query_comps, "main_database__loaded", bool)
            , ECS_RO_COMP(get_data_base_ecs_query_comps, "data_bases_paths", ecs::StringList)
            , ECS_RO_COMP(get_data_base_ecs_query_comps, "main_database__availableTags", ecs::StringList)
            , ECS_RO_COMP(get_data_base_ecs_query_comps, "main_database__presetsTagsName", ecs::StringList)
            , ECS_RO_COMP(get_data_base_ecs_query_comps, "main_database__root_node", ecs::string)
            , ECS_RO_COMP(get_data_base_ecs_query_comps, "main_database__root_motion_a2d_node", ecs::string)
            , ECS_RO_COMP(get_data_base_ecs_query_comps, "main_database__direction_nodes", ecs::StringList)
            , ECS_RO_COMP(get_data_base_ecs_query_comps, "main_database__direction_weights", ecs::FloatList)
            , ECS_RO_COMP(get_data_base_ecs_query_comps, "main_database__center_of_mass_nodes", ecs::StringList)
            , ECS_RO_COMP(get_data_base_ecs_query_comps, "main_database__center_of_mass_params", ecs::Point4List)
            , ECS_RO_COMP(get_data_base_ecs_query_comps, "main_database__nodeMasksPath", ecs::string)
            , ECS_RO_COMP(get_data_base_ecs_query_comps, "main_database__pbcWeightOverrides", ecs::Object)
            , ECS_RO_COMP(get_data_base_ecs_query_comps, "main_database__footLockerCtrlName", ecs::string)
            , ECS_RO_COMP(get_data_base_ecs_query_comps, "main_database__footLockerNodes", ecs::StringList)
            , ECS_RW_COMP(get_data_base_ecs_query_comps, "dataBase", AnimationDataBase)
            );

        }while (++comp != compE);
    }
  );
}
