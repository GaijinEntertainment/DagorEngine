// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "keyTrackAnimES.cpp.inl"
ECS_DEF_PULL_VAR(keyTrackAnim);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc key_track_anim_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("anim_key_track"), ecs::ComponentTypeInfo<AnimKeyTrackTransform>()},
  {ECS_HASH("anim_track_on"), ecs::ComponentTypeInfo<bool>()}
};
static void key_track_anim_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(ECS_RW_COMP(key_track_anim_es_comps, "anim_track_on", bool)) )
      continue;
    key_track_anim_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(key_track_anim_es_comps, "transform", TMatrix)
    , ECS_RW_COMP(key_track_anim_es_comps, "anim_key_track", AnimKeyTrackTransform)
    , ECS_RW_COMP(key_track_anim_es_comps, "anim_track_on", bool)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc key_track_anim_es_es_desc
(
  "key_track_anim_es",
  "prog/gameLibs/ecs/game/generic/./keyTrackAnimES.cpp.inl",
  ecs::EntitySystemOps(key_track_anim_es_all),
  make_span(key_track_anim_es_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
);
static constexpr ecs::ComponentDesc attr_float_track_anim_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("anim_float_attr"), ecs::ComponentTypeInfo<AttrAnimKeyTrackFloat>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void attr_float_track_anim_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    attr_float_track_anim_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(attr_float_track_anim_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(attr_float_track_anim_es_comps, "anim_float_attr", AttrAnimKeyTrackFloat)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc attr_float_track_anim_es_es_desc
(
  "attr_float_track_anim_es",
  "prog/gameLibs/ecs/game/generic/./keyTrackAnimES.cpp.inl",
  ecs::EntitySystemOps(attr_float_track_anim_es_all),
  make_span(attr_float_track_anim_es_comps+0, 1)/*rw*/,
  make_span(attr_float_track_anim_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
);
static constexpr ecs::ComponentDesc key_track_change_anim_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("anim_key_track"), ecs::ComponentTypeInfo<AnimKeyTrackTransform>()},
  {ECS_HASH("anim_track_on"), ecs::ComponentTypeInfo<bool>()}
};
static void key_track_change_anim_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
if (evt.is<key_track_anim::CmdStopAnim>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      key_track_change_anim_es_event_handler(static_cast<const key_track_anim::CmdStopAnim&>(evt)
            , ECS_RW_COMP(key_track_change_anim_es_event_handler_comps, "anim_track_on", bool)
      );
    while (++comp != compE);
  } else if (evt.is<key_track_anim::CmdAddRotAnim>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      key_track_change_anim_es_event_handler(static_cast<const key_track_anim::CmdAddRotAnim&>(evt)
            , ECS_RW_COMP(key_track_change_anim_es_event_handler_comps, "anim_key_track", AnimKeyTrackTransform)
      , ECS_RW_COMP(key_track_change_anim_es_event_handler_comps, "anim_track_on", bool)
      );
    while (++comp != compE);
  } else if (evt.is<key_track_anim::CmdAddPosAnim>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      key_track_change_anim_es_event_handler(static_cast<const key_track_anim::CmdAddPosAnim&>(evt)
            , ECS_RW_COMP(key_track_change_anim_es_event_handler_comps, "anim_key_track", AnimKeyTrackTransform)
      , ECS_RW_COMP(key_track_change_anim_es_event_handler_comps, "anim_track_on", bool)
      );
    while (++comp != compE);
  } else if (evt.is<key_track_anim::CmdResetRotAnim>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      key_track_change_anim_es_event_handler(static_cast<const key_track_anim::CmdResetRotAnim&>(evt)
            , ECS_RW_COMP(key_track_change_anim_es_event_handler_comps, "anim_key_track", AnimKeyTrackTransform)
      , ECS_RW_COMP(key_track_change_anim_es_event_handler_comps, "anim_track_on", bool)
      );
    while (++comp != compE);
  } else if (evt.is<key_track_anim::CmdResetPosAnim>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      key_track_change_anim_es_event_handler(static_cast<const key_track_anim::CmdResetPosAnim&>(evt)
            , ECS_RW_COMP(key_track_change_anim_es_event_handler_comps, "anim_key_track", AnimKeyTrackTransform)
      , ECS_RW_COMP(key_track_change_anim_es_event_handler_comps, "anim_track_on", bool)
      );
    while (++comp != compE);
    } else {G_ASSERTF(0, "Unexpected event type <%s> in key_track_change_anim_es_event_handler", evt.getName());}
}
static ecs::EntitySystemDesc key_track_change_anim_es_event_handler_es_desc
(
  "key_track_change_anim_es",
  "prog/gameLibs/ecs/game/generic/./keyTrackAnimES.cpp.inl",
  ecs::EntitySystemOps(nullptr, key_track_change_anim_es_event_handler_all_events),
  make_span(key_track_change_anim_es_event_handler_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<key_track_anim::CmdResetPosAnim,
                       key_track_anim::CmdResetRotAnim,
                       key_track_anim::CmdAddPosAnim,
                       key_track_anim::CmdAddRotAnim,
                       key_track_anim::CmdStopAnim>::build(),
  0
);
static constexpr ecs::ComponentDesc attr_float_key_track_change_anim_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("anim_float_attr"), ecs::ComponentTypeInfo<AttrAnimKeyTrackFloat>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void attr_float_key_track_change_anim_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
if (evt.is<key_track_anim::CmdAddAttrFloatAnim>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      attr_float_key_track_change_anim_es_event_handler(static_cast<const key_track_anim::CmdAddAttrFloatAnim&>(evt)
            , ECS_RO_COMP(attr_float_key_track_change_anim_es_event_handler_comps, "eid", ecs::EntityId)
      , ECS_RW_COMP(attr_float_key_track_change_anim_es_event_handler_comps, "anim_float_attr", AttrAnimKeyTrackFloat)
      );
    while (++comp != compE);
  } else if (evt.is<key_track_anim::CmdResetAttrFloatAnim>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      attr_float_key_track_change_anim_es_event_handler(static_cast<const key_track_anim::CmdResetAttrFloatAnim&>(evt)
            , ECS_RO_COMP(attr_float_key_track_change_anim_es_event_handler_comps, "eid", ecs::EntityId)
      , ECS_RW_COMP(attr_float_key_track_change_anim_es_event_handler_comps, "anim_float_attr", AttrAnimKeyTrackFloat)
      );
    while (++comp != compE);
    } else {G_ASSERTF(0, "Unexpected event type <%s> in attr_float_key_track_change_anim_es_event_handler", evt.getName());}
}
static ecs::EntitySystemDesc attr_float_key_track_change_anim_es_event_handler_es_desc
(
  "attr_float_key_track_change_anim_es",
  "prog/gameLibs/ecs/game/generic/./keyTrackAnimES.cpp.inl",
  ecs::EntitySystemOps(nullptr, attr_float_key_track_change_anim_es_event_handler_all_events),
  make_span(attr_float_key_track_change_anim_es_event_handler_comps+0, 1)/*rw*/,
  make_span(attr_float_key_track_change_anim_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<key_track_anim::CmdResetAttrFloatAnim,
                       key_track_anim::CmdAddAttrFloatAnim>::build(),
  0
);
