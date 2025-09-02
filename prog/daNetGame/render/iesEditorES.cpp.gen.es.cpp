// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "iesEditorES.cpp.inl"
ECS_DEF_PULL_VAR(iesEditor);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc generate_ies_texture_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("ies__editor__editing"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void generate_ies_texture_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoRender>());
  generate_ies_texture_es(static_cast<const UpdateStageInfoRender&>(evt)
        );
}
static ecs::EntitySystemDesc generate_ies_texture_es_es_desc
(
  "generate_ies_texture_es",
  "prog/daNetGame/render/iesEditorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, generate_ies_texture_es_all_events),
  empty_span(),
  empty_span(),
  make_span(generate_ies_texture_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoRender>::build(),
  0
,"dev,render");
static constexpr ecs::ComponentDesc ies_edited_light_appear_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("light__texture_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("ies_editor_editing__original_ies_tex_name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void ies_edited_light_appear_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ies_edited_light_appear_es_event_handler(evt
        , ECS_RW_COMP(ies_edited_light_appear_es_event_handler_comps, "light__texture_name", ecs::string)
    , ECS_RW_COMP(ies_edited_light_appear_es_event_handler_comps, "ies_editor_editing__original_ies_tex_name", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ies_edited_light_appear_es_event_handler_es_desc
(
  "ies_edited_light_appear_es",
  "prog/daNetGame/render/iesEditorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ies_edited_light_appear_es_event_handler_all_events),
  make_span(ies_edited_light_appear_es_event_handler_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"dev");
static constexpr ecs::ComponentDesc ies_edited_light_unedit_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("light__texture_name"), ecs::ComponentTypeInfo<ecs::string>()},
//start of 1 ro components at [1]
  {ECS_HASH("ies_editor_editing__original_ies_tex_name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void ies_edited_light_unedit_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ies_edited_light_unedit_es_event_handler(evt
        , ECS_RW_COMP(ies_edited_light_unedit_es_event_handler_comps, "light__texture_name", ecs::string)
    , ECS_RO_COMP(ies_edited_light_unedit_es_event_handler_comps, "ies_editor_editing__original_ies_tex_name", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ies_edited_light_unedit_es_event_handler_es_desc
(
  "ies_edited_light_unedit_es",
  "prog/daNetGame/render/iesEditorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ies_edited_light_unedit_es_event_handler_all_events),
  make_span(ies_edited_light_unedit_es_event_handler_comps+0, 1)/*rw*/,
  make_span(ies_edited_light_unedit_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"dev");
static constexpr ecs::ComponentDesc ies_edited_light_unselect_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 2 rq components at [1]
  {ECS_HASH("daeditor__selected"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("ies__editor__editing"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void ies_edited_light_unselect_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ies_edited_light_unselect_es_event_handler(evt
        , ECS_RO_COMP(ies_edited_light_unselect_es_event_handler_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ies_edited_light_unselect_es_event_handler_es_desc
(
  "ies_edited_light_unselect_es",
  "prog/daNetGame/render/iesEditorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ies_edited_light_unselect_es_event_handler_all_events),
  empty_span(),
  make_span(ies_edited_light_unselect_es_event_handler_comps+0, 1)/*ro*/,
  make_span(ies_edited_light_unselect_es_event_handler_comps+1, 2)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"dev");
static constexpr ecs::ComponentDesc selected_light_entity_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 2 rq components at [1]
  {ECS_HASH("light__texture_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("daeditor__selected"), ecs::ComponentTypeInfo<ecs::Tag>()},
//start of 1 no components at [3]
  {ECS_HASH("ies__editor__editing"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc selected_light_entity_ecs_query_desc
(
  "selected_light_entity_ecs_query",
  empty_span(),
  make_span(selected_light_entity_ecs_query_comps+0, 1)/*ro*/,
  make_span(selected_light_entity_ecs_query_comps+1, 2)/*rq*/,
  make_span(selected_light_entity_ecs_query_comps+3, 1)/*no*/);
template<typename Callable>
inline void selected_light_entity_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, selected_light_entity_ecs_query_desc.getHandle(),
    ecs::stoppable_query_cb_t([&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(selected_light_entity_ecs_query_comps, "eid", ecs::EntityId)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    })
  );
}
