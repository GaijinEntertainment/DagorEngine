#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t effect_get_type();
static ecs::LTComponentList effect_component(ECS_HASH("effect"), effect_get_type(), "prog/daNetGame/render/entityBindedEffectES.cpp.inl", "", 0);
static constexpr ecs::component_t entity_binded_effect__collNodeId_get_type();
static ecs::LTComponentList entity_binded_effect__collNodeId_component(ECS_HASH("entity_binded_effect__collNodeId"), entity_binded_effect__collNodeId_get_type(), "prog/daNetGame/render/entityBindedEffectES.cpp.inl", "", 0);
static constexpr ecs::component_t entity_binded_effect__entity_get_type();
static ecs::LTComponentList entity_binded_effect__entity_component(ECS_HASH("entity_binded_effect__entity"), entity_binded_effect__entity_get_type(), "prog/daNetGame/render/entityBindedEffectES.cpp.inl", "", 0);
static constexpr ecs::component_t entity_binded_effect__localEmitter_get_type();
static ecs::LTComponentList entity_binded_effect__localEmitter_component(ECS_HASH("entity_binded_effect__localEmitter"), entity_binded_effect__localEmitter_get_type(), "prog/daNetGame/render/entityBindedEffectES.cpp.inl", "", 0);
#include "entityBindedEffectES.cpp.inl"
ECS_DEF_PULL_VAR(entityBindedEffect);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc entity_binded_effect_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 3 ro components at [1]
  {ECS_HASH("entity_binded_effect__entity"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("entity_binded_effect__collNodeId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("entity_binded_effect__localEmitter"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void entity_binded_effect_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    entity_binded_effect_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(entity_binded_effect_es_comps, "entity_binded_effect__entity", ecs::EntityId)
    , ECS_RW_COMP(entity_binded_effect_es_comps, "effect", TheEffect)
    , ECS_RO_COMP(entity_binded_effect_es_comps, "entity_binded_effect__collNodeId", int)
    , ECS_RO_COMP(entity_binded_effect_es_comps, "entity_binded_effect__localEmitter", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc entity_binded_effect_es_es_desc
(
  "entity_binded_effect_es",
  "prog/daNetGame/render/entityBindedEffectES.cpp.inl",
  ecs::EntitySystemOps(entity_binded_effect_es_all),
  make_span(entity_binded_effect_es_comps+0, 1)/*rw*/,
  make_span(entity_binded_effect_es_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,nullptr,"auto_delete_entity_with_effect_component_es");
static constexpr ecs::ComponentDesc auto_delete_client_entity_with_effect_component_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 1 rq components at [2]
  {ECS_HASH("autodeleteEffectEntity"), ecs::ComponentTypeInfo<ecs::Tag>()},
//start of 1 no components at [3]
  {ECS_HASH("replication"), ecs::ComponentTypeInfo<ecs::auto_type>()}
};
static void auto_delete_client_entity_with_effect_component_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    auto_delete_client_entity_with_effect_component_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(auto_delete_client_entity_with_effect_component_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(auto_delete_client_entity_with_effect_component_es_comps, "effect", TheEffect)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc auto_delete_client_entity_with_effect_component_es_es_desc
(
  "auto_delete_client_entity_with_effect_component_es",
  "prog/daNetGame/render/entityBindedEffectES.cpp.inl",
  ecs::EntitySystemOps(auto_delete_client_entity_with_effect_component_es_all),
  empty_span(),
  make_span(auto_delete_client_entity_with_effect_component_es_comps+0, 2)/*ro*/,
  make_span(auto_delete_client_entity_with_effect_component_es_comps+2, 1)/*rq*/,
  make_span(auto_delete_client_entity_with_effect_component_es_comps+3, 1)/*no*/,
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc validate_auto_delete_tag_on_client_only_entities_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 3 rq components at [1]
  {ECS_HASH("autodeleteEffectEntity"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<ecs::auto_type>()},
  {ECS_HASH("replication"), ecs::ComponentTypeInfo<ecs::auto_type>()}
};
static void validate_auto_delete_tag_on_client_only_entities_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    validate_auto_delete_tag_on_client_only_entities_es(evt
        , ECS_RO_COMP(validate_auto_delete_tag_on_client_only_entities_es_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc validate_auto_delete_tag_on_client_only_entities_es_es_desc
(
  "validate_auto_delete_tag_on_client_only_entities_es",
  "prog/daNetGame/render/entityBindedEffectES.cpp.inl",
  ecs::EntitySystemOps(nullptr, validate_auto_delete_tag_on_client_only_entities_es_all_events),
  empty_span(),
  make_span(validate_auto_delete_tag_on_client_only_entities_es_comps+0, 1)/*ro*/,
  make_span(validate_auto_delete_tag_on_client_only_entities_es_comps+1, 3)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc get_animchar_collision_transform_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc get_animchar_collision_transform_ecs_query_desc
(
  "get_animchar_collision_transform_ecs_query",
  empty_span(),
  make_span(get_animchar_collision_transform_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline bool get_animchar_collision_transform_ecs_query(ecs::EntityId eid, Callable function)
{
  return perform_query(g_entity_mgr, eid, get_animchar_collision_transform_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_animchar_collision_transform_ecs_query_comps, "collres", CollisionResource)
            , ECS_RO_COMP(get_animchar_collision_transform_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            , ECS_RO_COMP_OR(get_animchar_collision_transform_ecs_query_comps, "transform", TMatrix(TMatrix::IDENT))
            );

        }
    }
  );
}
static constexpr ecs::component_t effect_get_type(){return ecs::ComponentTypeInfo<TheEffect>::type; }
static constexpr ecs::component_t entity_binded_effect__collNodeId_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t entity_binded_effect__entity_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t entity_binded_effect__localEmitter_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
