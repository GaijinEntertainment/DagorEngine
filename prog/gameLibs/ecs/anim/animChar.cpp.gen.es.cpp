#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t animchar_get_type();
static ecs::LTComponentList animchar_component(ECS_HASH("animchar"), animchar_get_type(), "prog/gameLibs/ecs/anim/animChar.cpp.inl", "animchar_pre_update_ecs_query", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "animChar.cpp.inl"
ECS_DEF_PULL_VAR(animChar);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc animchar__updater_es_comps[] ={};
static void animchar__updater_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateAnimcharEvent>());
  animchar__updater_es(static_cast<const UpdateAnimcharEvent&>(evt)
        );
}
static ecs::EntitySystemDesc animchar__updater_es_es_desc
(
  "animchar__updater_es",
  "prog/gameLibs/ecs/anim/animChar.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar__updater_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateAnimcharEvent>::build(),
  0
,nullptr,nullptr,"after_animchar_update_sync","before_animchar_update_sync");
static constexpr ecs::ComponentDesc update_animchar_on_create_es_event_handler_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar__accumDt"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__root_pos"), ecs::ComponentTypeInfo<vec3f>(), ecs::CDF_OPTIONAL},
//start of 3 ro components at [4]
  {ECS_HASH("animchar__dtThreshold"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar__turnDir"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [7]
  {ECS_HASH("animchar__actOnCreate"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void update_animchar_on_create_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_animchar_on_create_es_event_handler(evt
        , ECS_RW_COMP(update_animchar_on_create_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RW_COMP_PTR(update_animchar_on_create_es_event_handler_comps, "animchar__accumDt", float)
    , ECS_RO_COMP_PTR(update_animchar_on_create_es_event_handler_comps, "animchar__dtThreshold", float)
    , ECS_RW_COMP_PTR(update_animchar_on_create_es_event_handler_comps, "animchar_node_wtm", AnimcharNodesMat44)
    , ECS_RW_COMP_PTR(update_animchar_on_create_es_event_handler_comps, "animchar_render__root_pos", vec3f)
    , ECS_RO_COMP(update_animchar_on_create_es_event_handler_comps, "transform", TMatrix)
    , ECS_RO_COMP_OR(update_animchar_on_create_es_event_handler_comps, "animchar__turnDir", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_animchar_on_create_es_event_handler_es_desc
(
  "update_animchar_on_create_es",
  "prog/gameLibs/ecs/anim/animChar.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_animchar_on_create_es_event_handler_all_events),
  make_span(update_animchar_on_create_es_event_handler_comps+0, 4)/*rw*/,
  make_span(update_animchar_on_create_es_event_handler_comps+4, 3)/*ro*/,
  make_span(update_animchar_on_create_es_event_handler_comps+7, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,nullptr,nullptr,"anim_phys_es");
static constexpr ecs::ComponentDesc animchar_update_animstate_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar__animState"), ecs::ComponentTypeInfo<ecs::Object>()},
//start of 3 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar__animStateNames"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void animchar_update_animstate_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_update_animstate_es_event_handler(evt
        , ECS_RO_COMP(animchar_update_animstate_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(animchar_update_animstate_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(animchar_update_animstate_es_event_handler_comps, "animchar__animStateNames", ecs::Object)
    , ECS_RW_COMP(animchar_update_animstate_es_event_handler_comps, "animchar__animState", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_update_animstate_es_event_handler_es_desc
(
  "animchar_update_animstate_es",
  "prog/gameLibs/ecs/anim/animChar.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_update_animstate_es_event_handler_all_events),
  make_span(animchar_update_animstate_es_event_handler_comps+0, 1)/*rw*/,
  make_span(animchar_update_animstate_es_event_handler_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"animchar__animStateNames");
static constexpr ecs::ComponentDesc animchar_act_on_demand_es_event_handler_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__root_pos"), ecs::ComponentTypeInfo<vec3f>(), ecs::CDF_OPTIONAL},
//start of 2 ro components at [3]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar__turnDir"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [5]
  {ECS_HASH("animchar__actOnDemand"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void animchar_act_on_demand_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_act_on_demand_es_event_handler(evt
        , ECS_RW_COMP(animchar_act_on_demand_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RW_COMP_PTR(animchar_act_on_demand_es_event_handler_comps, "animchar_node_wtm", AnimcharNodesMat44)
    , ECS_RW_COMP_PTR(animchar_act_on_demand_es_event_handler_comps, "animchar_render__root_pos", vec3f)
    , ECS_RO_COMP(animchar_act_on_demand_es_event_handler_comps, "transform", TMatrix)
    , ECS_RO_COMP_OR(animchar_act_on_demand_es_event_handler_comps, "animchar__turnDir", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_act_on_demand_es_event_handler_es_desc
(
  "animchar_act_on_demand_es",
  "prog/gameLibs/ecs/anim/animChar.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_act_on_demand_es_event_handler_all_events),
  make_span(animchar_act_on_demand_es_event_handler_comps+0, 3)/*rw*/,
  make_span(animchar_act_on_demand_es_event_handler_comps+3, 2)/*ro*/,
  make_span(animchar_act_on_demand_es_event_handler_comps+5, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"animchar__turnDir,transform");
static constexpr ecs::ComponentDesc animchar_act_on_demand_detach_es_event_handler_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__root_pos"), ecs::ComponentTypeInfo<vec3f>(), ecs::CDF_OPTIONAL},
//start of 2 ro components at [3]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar__turnDir"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 2 rq components at [5]
  {ECS_HASH("animchar__actOnDemand"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("attachedToParent"), ecs::ComponentTypeInfo<ecs::auto_type>()}
};
static void animchar_act_on_demand_detach_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_act_on_demand_detach_es_event_handler(evt
        , ECS_RW_COMP(animchar_act_on_demand_detach_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RW_COMP_PTR(animchar_act_on_demand_detach_es_event_handler_comps, "animchar_node_wtm", AnimcharNodesMat44)
    , ECS_RW_COMP_PTR(animchar_act_on_demand_detach_es_event_handler_comps, "animchar_render__root_pos", vec3f)
    , ECS_RO_COMP(animchar_act_on_demand_detach_es_event_handler_comps, "transform", TMatrix)
    , ECS_RO_COMP_OR(animchar_act_on_demand_detach_es_event_handler_comps, "animchar__turnDir", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_act_on_demand_detach_es_event_handler_es_desc
(
  "animchar_act_on_demand_detach_es",
  "prog/gameLibs/ecs/anim/animChar.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_act_on_demand_detach_es_event_handler_all_events),
  make_span(animchar_act_on_demand_detach_es_event_handler_comps+0, 3)/*rw*/,
  make_span(animchar_act_on_demand_detach_es_event_handler_comps+3, 2)/*ro*/,
  make_span(animchar_act_on_demand_detach_es_event_handler_comps+5, 2)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc animchar_non_updatable_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__root_pos"), ecs::ComponentTypeInfo<vec3f>(), ecs::CDF_OPTIONAL},
//start of 2 ro components at [3]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar__turnDir"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [5]
  {ECS_HASH("disableUpdate"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void animchar_non_updatable_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_non_updatable_es(evt
        , ECS_RW_COMP(animchar_non_updatable_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RW_COMP_PTR(animchar_non_updatable_es_comps, "animchar_node_wtm", AnimcharNodesMat44)
    , ECS_RW_COMP_PTR(animchar_non_updatable_es_comps, "animchar_render__root_pos", vec3f)
    , ECS_RO_COMP(animchar_non_updatable_es_comps, "transform", TMatrix)
    , ECS_RO_COMP_OR(animchar_non_updatable_es_comps, "animchar__turnDir", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_non_updatable_es_es_desc
(
  "animchar_non_updatable_es",
  "prog/gameLibs/ecs/anim/animChar.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_non_updatable_es_all_events),
  make_span(animchar_non_updatable_es_comps+0, 3)/*rw*/,
  make_span(animchar_non_updatable_es_comps+3, 2)/*ro*/,
  make_span(animchar_non_updatable_es_comps+5, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"animchar__turnDir,transform");
static constexpr ecs::ComponentDesc animchar_non_updatable_detach_es_event_handler_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__root_pos"), ecs::ComponentTypeInfo<vec3f>(), ecs::CDF_OPTIONAL},
//start of 2 ro components at [3]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar__turnDir"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 2 rq components at [5]
  {ECS_HASH("disableUpdate"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("attachedToParent"), ecs::ComponentTypeInfo<ecs::auto_type>()}
};
static void animchar_non_updatable_detach_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_non_updatable_detach_es_event_handler(evt
        , ECS_RW_COMP(animchar_non_updatable_detach_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RW_COMP_PTR(animchar_non_updatable_detach_es_event_handler_comps, "animchar_node_wtm", AnimcharNodesMat44)
    , ECS_RW_COMP_PTR(animchar_non_updatable_detach_es_event_handler_comps, "animchar_render__root_pos", vec3f)
    , ECS_RO_COMP(animchar_non_updatable_detach_es_event_handler_comps, "transform", TMatrix)
    , ECS_RO_COMP_OR(animchar_non_updatable_detach_es_event_handler_comps, "animchar__turnDir", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_non_updatable_detach_es_event_handler_es_desc
(
  "animchar_non_updatable_detach_es",
  "prog/gameLibs/ecs/anim/animChar.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_non_updatable_detach_es_event_handler_all_events),
  make_span(animchar_non_updatable_detach_es_event_handler_comps+0, 3)/*rw*/,
  make_span(animchar_non_updatable_detach_es_event_handler_comps+3, 2)/*ro*/,
  make_span(animchar_non_updatable_detach_es_event_handler_comps+5, 2)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc animchar_skeleton_attach_destroy_attach_es_event_handler_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__root_pos"), ecs::ComponentTypeInfo<vec3f>(), ecs::CDF_OPTIONAL},
//start of 3 ro components at [3]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar__turnDir"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("skeleton_attach__attached"), ecs::ComponentTypeInfo<bool>()}
};
static void animchar_skeleton_attach_destroy_attach_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(!ECS_RO_COMP(animchar_skeleton_attach_destroy_attach_es_event_handler_comps, "skeleton_attach__attached", bool)) )
      continue;
    animchar_skeleton_attach_destroy_attach_es_event_handler(evt
          , ECS_RW_COMP(animchar_skeleton_attach_destroy_attach_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
      , ECS_RW_COMP_PTR(animchar_skeleton_attach_destroy_attach_es_event_handler_comps, "animchar_node_wtm", AnimcharNodesMat44)
      , ECS_RW_COMP_PTR(animchar_skeleton_attach_destroy_attach_es_event_handler_comps, "animchar_render__root_pos", vec3f)
      , ECS_RO_COMP(animchar_skeleton_attach_destroy_attach_es_event_handler_comps, "transform", TMatrix)
      , ECS_RO_COMP_OR(animchar_skeleton_attach_destroy_attach_es_event_handler_comps, "animchar__turnDir", bool(false))
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_skeleton_attach_destroy_attach_es_event_handler_es_desc
(
  "animchar_skeleton_attach_destroy_attach_es",
  "prog/gameLibs/ecs/anim/animChar.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_skeleton_attach_destroy_attach_es_event_handler_all_events),
  make_span(animchar_skeleton_attach_destroy_attach_es_event_handler_comps+0, 3)/*rw*/,
  make_span(animchar_skeleton_attach_destroy_attach_es_event_handler_comps+3, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"gameClient","skeleton_attach__attached");
static constexpr ecs::ComponentDesc animchar_act_on_phys_teleport_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__root_pos"), ecs::ComponentTypeInfo<vec3f>(), ecs::CDF_OPTIONAL},
//start of 2 ro components at [3]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar__turnDir"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static void animchar_act_on_phys_teleport_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventOnEntityTeleported>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_act_on_phys_teleport_es(static_cast<const EventOnEntityTeleported&>(evt)
        , ECS_RW_COMP(animchar_act_on_phys_teleport_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RW_COMP_PTR(animchar_act_on_phys_teleport_es_comps, "animchar_node_wtm", AnimcharNodesMat44)
    , ECS_RW_COMP_PTR(animchar_act_on_phys_teleport_es_comps, "animchar_render__root_pos", vec3f)
    , ECS_RO_COMP(animchar_act_on_phys_teleport_es_comps, "transform", TMatrix)
    , ECS_RO_COMP_OR(animchar_act_on_phys_teleport_es_comps, "animchar__turnDir", bool(false))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_act_on_phys_teleport_es_es_desc
(
  "animchar_act_on_phys_teleport_es",
  "prog/gameLibs/ecs/anim/animChar.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_act_on_phys_teleport_es_all_events),
  make_span(animchar_act_on_phys_teleport_es_comps+0, 3)/*rw*/,
  make_span(animchar_act_on_phys_teleport_es_comps+3, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnEntityTeleported>::build(),
  0
);
static constexpr ecs::ComponentDesc animchar_replace_texture_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 3 ro components at [1]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar__res"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("animchar__objTexReplace"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void animchar_replace_texture_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_replace_texture_es_event_handler(evt
        , ECS_RW_COMP(animchar_replace_texture_es_event_handler_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RO_COMP(animchar_replace_texture_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(animchar_replace_texture_es_event_handler_comps, "animchar__res", ecs::string)
    , ECS_RO_COMP(animchar_replace_texture_es_event_handler_comps, "animchar__objTexReplace", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_replace_texture_es_event_handler_es_desc
(
  "animchar_replace_texture_es",
  "prog/gameLibs/ecs/anim/animChar.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_replace_texture_es_event_handler_all_events),
  make_span(animchar_replace_texture_es_event_handler_comps+0, 1)/*rw*/,
  make_span(animchar_replace_texture_es_event_handler_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"animchar__objTexReplace");
static constexpr ecs::ComponentDesc bindpose_init_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()}
};
static void bindpose_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    bindpose_init_es_event_handler(evt
        , ECS_RW_COMP(bindpose_init_es_event_handler_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bindpose_init_es_event_handler_es_desc
(
  "bindpose_init_es",
  "prog/gameLibs/ecs/anim/animChar.cpp.inl",
  ecs::EntitySystemOps(nullptr, bindpose_init_es_event_handler_all_events),
  make_span(bindpose_init_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc animchar_pre_update_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 3 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("slot_attach__slotId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("animchar_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [4]
  {ECS_HASH("attachmentUpdate"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc animchar_pre_update_ecs_query_desc
(
  "animchar_pre_update_ecs_query",
  make_span(animchar_pre_update_ecs_query_comps+0, 1)/*rw*/,
  make_span(animchar_pre_update_ecs_query_comps+1, 3)/*ro*/,
  make_span(animchar_pre_update_ecs_query_comps+4, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void animchar_pre_update_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, animchar_pre_update_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(animchar_pre_update_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(animchar_pre_update_ecs_query_comps, "slot_attach__slotId", int)
            , ECS_RW_COMP(animchar_pre_update_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            , ECS_RO_COMP(animchar_pre_update_ecs_query_comps, "animchar_attach__attachedTo", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc animchar_update_ecs_query_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar__accumDt"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__root_pos"), ecs::ComponentTypeInfo<vec3f>(), ecs::CDF_OPTIONAL},
//start of 4 ro components at [4]
  {ECS_HASH("animchar__dtThreshold"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar__updatable"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar__turnDir"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 2 no components at [8]
  {ECS_HASH("animchar__actOnDemand"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("animchar__physSymDependence"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc animchar_update_ecs_query_desc
(
  "animchar_update_ecs_query",
  make_span(animchar_update_ecs_query_comps+0, 4)/*rw*/,
  make_span(animchar_update_ecs_query_comps+4, 4)/*ro*/,
  empty_span(),
  make_span(animchar_update_ecs_query_comps+8, 2)/*no*/
  , 1);
template<typename Callable>
inline void animchar_update_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, animchar_update_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(animchar_update_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            , ECS_RW_COMP_PTR(animchar_update_ecs_query_comps, "animchar__accumDt", float)
            , ECS_RO_COMP_PTR(animchar_update_ecs_query_comps, "animchar__dtThreshold", float)
            , ECS_RW_COMP_PTR(animchar_update_ecs_query_comps, "animchar_node_wtm", AnimcharNodesMat44)
            , ECS_RW_COMP_PTR(animchar_update_ecs_query_comps, "animchar_render__root_pos", vec3f)
            , ECS_RO_COMP(animchar_update_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP_OR(animchar_update_ecs_query_comps, "animchar__updatable", bool(true))
            , ECS_RO_COMP_OR(animchar_update_ecs_query_comps, "animchar__turnDir", bool(false))
            );

        }while (++comp != compE);
    }
    , nullptr, animchar_update_ecs_query_desc.getQuant());
}
static constexpr ecs::component_t animchar_get_type(){return ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>::type; }
