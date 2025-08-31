#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t gunmod__collimator_moa_img_eid_get_type();
static ecs::LTComponentList gunmod__collimator_moa_img_eid_component(ECS_HASH("gunmod__collimator_moa_img_eid"), gunmod__collimator_moa_img_eid_get_type(), "prog/gameLibs/render/collimatorMoaCore/render/collimatorMoaUpdateShapesES.cpp.inl", "", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "collimatorMoaUpdateShapesES.cpp.inl"
ECS_DEF_PULL_VAR(collimatorMoaUpdateShapes);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc collimator_moa_track_selected_image_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("collimator_moa_render__active_image_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("collimator_moa_render__active_shapes_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("collimator_moa_render__shapes_buf_reg_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("collimator_moa_render__current_shapes_buf"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
//start of 1 ro components at [4]
  {ECS_HASH("collimator_moa_render__gun_mod_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void collimator_moa_track_selected_image_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    collimator_moa_track_selected_image_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RO_COMP(collimator_moa_track_selected_image_es_comps, "collimator_moa_render__gun_mod_eid", ecs::EntityId)
    , ECS_RW_COMP(collimator_moa_track_selected_image_es_comps, "collimator_moa_render__active_image_eid", ecs::EntityId)
    , ECS_RW_COMP(collimator_moa_track_selected_image_es_comps, "collimator_moa_render__active_shapes_count", int)
    , ECS_RW_COMP(collimator_moa_track_selected_image_es_comps, "collimator_moa_render__shapes_buf_reg_count", int)
    , ECS_RW_COMP(collimator_moa_track_selected_image_es_comps, "collimator_moa_render__current_shapes_buf", UniqueBufHolder)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc collimator_moa_track_selected_image_es_es_desc
(
  "collimator_moa_track_selected_image_es",
  "prog/gameLibs/render/collimatorMoaCore/render/collimatorMoaUpdateShapesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, collimator_moa_track_selected_image_es_all_events),
  make_span(collimator_moa_track_selected_image_es_comps+0, 4)/*rw*/,
  make_span(collimator_moa_track_selected_image_es_comps+4, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc collimator_moa_image_validation_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("collimator_moa_image__valid"), ecs::ComponentTypeInfo<bool>()},
//start of 2 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("collimator_moa_image__shapes"), ecs::ComponentTypeInfo<ecs::Array>()}
};
static void collimator_moa_image_validation_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    collimator_moa_image_validation_es_event_handler(evt
        , ECS_RO_COMP(collimator_moa_image_validation_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(collimator_moa_image_validation_es_event_handler_comps, "collimator_moa_image__shapes", ecs::Array)
    , ECS_RW_COMP(collimator_moa_image_validation_es_event_handler_comps, "collimator_moa_image__valid", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc collimator_moa_image_validation_es_event_handler_es_desc
(
  "collimator_moa_image_validation_es",
  "prog/gameLibs/render/collimatorMoaCore/render/collimatorMoaUpdateShapesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, collimator_moa_image_validation_es_event_handler_all_events),
  make_span(collimator_moa_image_validation_es_event_handler_comps+0, 1)/*rw*/,
  make_span(collimator_moa_image_validation_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","collimator_moa_image__shapes");
static constexpr ecs::ComponentDesc gunmod_track_collimator_moa_img_eid_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("gunmod__collimator_moa_img_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 ro components at [1]
  {ECS_HASH("gunmod__collimator_moa_img_template"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void gunmod_track_collimator_moa_img_eid_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    gunmod_track_collimator_moa_img_eid_es_event_handler(evt
        , ECS_RO_COMP(gunmod_track_collimator_moa_img_eid_es_event_handler_comps, "gunmod__collimator_moa_img_template", ecs::string)
    , ECS_RW_COMP(gunmod_track_collimator_moa_img_eid_es_event_handler_comps, "gunmod__collimator_moa_img_eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc gunmod_track_collimator_moa_img_eid_es_event_handler_es_desc
(
  "gunmod_track_collimator_moa_img_eid_es",
  "prog/gameLibs/render/collimatorMoaCore/render/collimatorMoaUpdateShapesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gunmod_track_collimator_moa_img_eid_es_event_handler_all_events),
  make_span(gunmod_track_collimator_moa_img_eid_es_event_handler_comps+0, 1)/*rw*/,
  make_span(gunmod_track_collimator_moa_img_eid_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","gunmod__collimator_moa_img_template");
static constexpr ecs::ComponentDesc gather_gun_mode_collimator_moa_shapes_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("collimator_moa_image__shapes"), ecs::ComponentTypeInfo<ecs::Array>()},
  {ECS_HASH("collimator_moa_image__valid"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc gather_gun_mode_collimator_moa_shapes_ecs_query_desc
(
  "gather_gun_mode_collimator_moa_shapes_ecs_query",
  empty_span(),
  make_span(gather_gun_mode_collimator_moa_shapes_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gather_gun_mode_collimator_moa_shapes_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, gather_gun_mode_collimator_moa_shapes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(gather_gun_mode_collimator_moa_shapes_ecs_query_comps, "collimator_moa_image__shapes", ecs::Array)
            , ECS_RO_COMP(gather_gun_mode_collimator_moa_shapes_ecs_query_comps, "collimator_moa_image__valid", bool)
            );

        }
    }
  );
}
static constexpr ecs::component_t gunmod__collimator_moa_img_eid_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
