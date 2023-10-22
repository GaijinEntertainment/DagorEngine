#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t slot_attach__inAttachedList_get_type();
static ecs::LTComponentList slot_attach__inAttachedList_component(ECS_HASH("slot_attach__inAttachedList"), slot_attach__inAttachedList_get_type(), "prog/gameLibs/ecs/anim/slotAttachES.cpp.inl", "get_attaches_list_ecs_query", 0);
#include "slotAttachES.cpp.inl"
ECS_DEF_PULL_VAR(slotAttach);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc attach_to_parent_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("slot_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("slot_attach__inAttachedList"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 no components at [3]
  {ECS_HASH("attachedToParent"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void attach_to_parent_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    attach_to_parent_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(attach_to_parent_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(attach_to_parent_es_comps, "slot_attach__attachedTo", ecs::EntityId)
    , ECS_RO_COMP(attach_to_parent_es_comps, "slot_attach__inAttachedList", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc attach_to_parent_es_es_desc
(
  "attach_to_parent_es",
  "prog/gameLibs/ecs/anim/slotAttachES.cpp.inl",
  ecs::EntitySystemOps(attach_to_parent_es_all),
  empty_span(),
  make_span(attach_to_parent_es_comps+0, 3)/*ro*/,
  empty_span(),
  make_span(attach_to_parent_es_comps+3, 1)/*no*/,
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"gameClient",nullptr,"*");
static constexpr ecs::ComponentDesc slot_attach_on_load_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("slot_attach__slotId"), ecs::ComponentTypeInfo<int>()},
//start of 1 ro components at [1]
  {ECS_HASH("slot_attach__slotName"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void slot_attach_on_load_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    slot_attach_on_load_es_event_handler(evt
        , ECS_RO_COMP(slot_attach_on_load_es_event_handler_comps, "slot_attach__slotName", ecs::string)
    , ECS_RW_COMP(slot_attach_on_load_es_event_handler_comps, "slot_attach__slotId", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc slot_attach_on_load_es_event_handler_es_desc
(
  "slot_attach_on_load_es",
  "prog/gameLibs/ecs/anim/slotAttachES.cpp.inl",
  ecs::EntitySystemOps(nullptr, slot_attach_on_load_es_event_handler_all_events),
  make_span(slot_attach_on_load_es_event_handler_comps+0, 1)/*rw*/,
  make_span(slot_attach_on_load_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc slot_attach_init_es_event_handler_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("slot_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("slot_attach__inAttachedList"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void slot_attach_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<anim::CmdInitSlotAttach>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    slot_attach_init_es_event_handler(static_cast<const anim::CmdInitSlotAttach&>(evt)
        , ECS_RO_COMP(slot_attach_init_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(slot_attach_init_es_event_handler_comps, "slot_attach__attachedTo", ecs::EntityId)
    , ECS_RO_COMP(slot_attach_init_es_event_handler_comps, "slot_attach__inAttachedList", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc slot_attach_init_es_event_handler_es_desc
(
  "slot_attach_init_es",
  "prog/gameLibs/ecs/anim/slotAttachES.cpp.inl",
  ecs::EntitySystemOps(nullptr, slot_attach_init_es_event_handler_all_events),
  empty_span(),
  make_span(slot_attach_init_es_event_handler_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<anim::CmdInitSlotAttach>::build(),
  0
,"gameClient");
static constexpr ecs::ComponentDesc attach_to_parent_on_create_es_event_handler_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("slot_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("slot_attach__inAttachedList"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void attach_to_parent_on_create_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    attach_to_parent_on_create_es_event_handler(evt
        , ECS_RO_COMP(attach_to_parent_on_create_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(attach_to_parent_on_create_es_event_handler_comps, "slot_attach__attachedTo", ecs::EntityId)
    , ECS_RO_COMP(attach_to_parent_on_create_es_event_handler_comps, "slot_attach__inAttachedList", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc attach_to_parent_on_create_es_event_handler_es_desc
(
  "attach_to_parent_on_create_es",
  "prog/gameLibs/ecs/anim/slotAttachES.cpp.inl",
  ecs::EntitySystemOps(nullptr, attach_to_parent_on_create_es_event_handler_all_events),
  empty_span(),
  make_span(attach_to_parent_on_create_es_event_handler_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc attach_on_parent_create_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>()}
};
static void attach_on_parent_create_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    attach_on_parent_create_es_event_handler(static_cast<const ecs::EventEntityCreated&>(evt)
        , ECS_RO_COMP(attach_on_parent_create_es_event_handler_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc attach_on_parent_create_es_event_handler_es_desc
(
  "attach_on_parent_create_es",
  "prog/gameLibs/ecs/anim/slotAttachES.cpp.inl",
  ecs::EntitySystemOps(nullptr, attach_on_parent_create_es_event_handler_all_events),
  empty_span(),
  make_span(attach_on_parent_create_es_event_handler_comps+0, 1)/*ro*/,
  make_span(attach_on_parent_create_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated>::build(),
  0
);
static constexpr ecs::ComponentDesc detach_from_parent_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("slot_attach__inAttachedList"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 2 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("slot_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void detach_from_parent_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityDestroyed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    detach_from_parent_es_event_handler(static_cast<const ecs::EventEntityDestroyed&>(evt)
        , ECS_RO_COMP(detach_from_parent_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(detach_from_parent_es_event_handler_comps, "slot_attach__attachedTo", ecs::EntityId)
    , ECS_RW_COMP(detach_from_parent_es_event_handler_comps, "slot_attach__inAttachedList", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc detach_from_parent_es_event_handler_es_desc
(
  "detach_from_parent_es",
  "prog/gameLibs/ecs/anim/slotAttachES.cpp.inl",
  ecs::EntitySystemOps(nullptr, detach_from_parent_es_event_handler_all_events),
  make_span(detach_from_parent_es_event_handler_comps+0, 1)/*rw*/,
  make_span(detach_from_parent_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed>::build(),
  0
);
static constexpr ecs::ComponentDesc slot_attached_to_disappear_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("slot_attach__inAttachedList"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [2]
  {ECS_HASH("slot_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void slot_attached_to_disappear_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventComponentsDisappear>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    slot_attached_to_disappear_es_event_handler(static_cast<const ecs::EventComponentsDisappear&>(evt)
        , ECS_RO_COMP(slot_attached_to_disappear_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(slot_attached_to_disappear_es_event_handler_comps, "slot_attach__inAttachedList", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc slot_attached_to_disappear_es_event_handler_es_desc
(
  "slot_attached_to_disappear_es",
  "prog/gameLibs/ecs/anim/slotAttachES.cpp.inl",
  ecs::EntitySystemOps(nullptr, slot_attached_to_disappear_es_event_handler_all_events),
  empty_span(),
  make_span(slot_attached_to_disappear_es_event_handler_comps+0, 2)/*ro*/,
  make_span(slot_attached_to_disappear_es_event_handler_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc track_attach_es_event_handler_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("slot_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("slot_attach__inAttachedList"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void track_attach_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    track_attach_es_event_handler(evt
        , ECS_RO_COMP(track_attach_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(track_attach_es_event_handler_comps, "slot_attach__attachedTo", ecs::EntityId)
    , ECS_RO_COMP(track_attach_es_event_handler_comps, "slot_attach__inAttachedList", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc track_attach_es_event_handler_es_desc
(
  "track_attach_es",
  "prog/gameLibs/ecs/anim/slotAttachES.cpp.inl",
  ecs::EntitySystemOps(nullptr, track_attach_es_event_handler_all_events),
  empty_span(),
  make_span(track_attach_es_event_handler_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"slot_attach__attachedTo");
static constexpr ecs::ComponentDesc get_attaches_list_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>()}
};
static ecs::CompileTimeQueryDesc get_attaches_list_ecs_query_desc
(
  "get_attaches_list_ecs_query",
  make_span(get_attaches_list_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline bool get_attaches_list_ecs_query(ecs::EntityId eid, Callable function)
{
  return perform_query(g_entity_mgr, eid, get_attaches_list_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(get_attaches_list_ecs_query_comps, "attaches_list", ecs::EidList)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_all_attaches_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("slot_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("slot_attach__inAttachedList"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc get_all_attaches_ecs_query_desc
(
  "get_all_attaches_ecs_query",
  empty_span(),
  make_span(get_all_attaches_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_all_attaches_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_all_attaches_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_all_attaches_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(get_all_attaches_ecs_query_comps, "slot_attach__attachedTo", ecs::EntityId)
            , ECS_RO_COMP(get_all_attaches_ecs_query_comps, "slot_attach__inAttachedList", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t slot_attach__inAttachedList_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
