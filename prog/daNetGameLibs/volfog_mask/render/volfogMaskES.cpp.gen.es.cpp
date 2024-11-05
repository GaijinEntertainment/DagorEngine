#include "volfogMaskES.cpp.inl"
ECS_DEF_PULL_VAR(volfogMask);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc volfog_mask_appear_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("volfog_mask__tex_holder"), ecs::ComponentTypeInfo<SharedTexHolder>()},
  {ECS_HASH("volfog_mask__do_update"), ecs::ComponentTypeInfo<bool>()},
//start of 3 ro components at [2]
  {ECS_HASH("volfog_mask__size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("volfog_mask__bounds"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("volfog_mask__data"), ecs::ComponentTypeInfo<ecs::FloatList>()}
};
static void volfog_mask_appear_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    volfog_mask_appear_es_event_handler(evt
        , ECS_RW_COMP(volfog_mask_appear_es_event_handler_comps, "volfog_mask__tex_holder", SharedTexHolder)
    , ECS_RO_COMP(volfog_mask_appear_es_event_handler_comps, "volfog_mask__size", int)
    , ECS_RW_COMP(volfog_mask_appear_es_event_handler_comps, "volfog_mask__do_update", bool)
    , ECS_RO_COMP(volfog_mask_appear_es_event_handler_comps, "volfog_mask__bounds", Point4)
    , ECS_RO_COMP(volfog_mask_appear_es_event_handler_comps, "volfog_mask__data", ecs::FloatList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc volfog_mask_appear_es_event_handler_es_desc
(
  "volfog_mask_appear_es",
  "prog/daNetGameLibs/volfog_mask/render/volfogMaskES.cpp.inl",
  ecs::EntitySystemOps(nullptr, volfog_mask_appear_es_event_handler_all_events),
  make_span(volfog_mask_appear_es_event_handler_comps+0, 2)/*rw*/,
  make_span(volfog_mask_appear_es_event_handler_comps+2, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc volfog_mask_render_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("volfog_mask__tex_holder"), ecs::ComponentTypeInfo<SharedTexHolder>()},
  {ECS_HASH("volfog_mask__do_update"), ecs::ComponentTypeInfo<bool>()},
//start of 3 ro components at [2]
  {ECS_HASH("volfog_mask__size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("volfog_mask__bounds"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("volfog_mask__data"), ecs::ComponentTypeInfo<ecs::FloatList>()}
};
static void volfog_mask_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RW_COMP(volfog_mask_render_es_comps, "volfog_mask__do_update", bool)) )
      continue;
    volfog_mask_render_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
          , ECS_RW_COMP(volfog_mask_render_es_comps, "volfog_mask__tex_holder", SharedTexHolder)
      , ECS_RO_COMP(volfog_mask_render_es_comps, "volfog_mask__size", int)
      , ECS_RW_COMP(volfog_mask_render_es_comps, "volfog_mask__do_update", bool)
      , ECS_RO_COMP(volfog_mask_render_es_comps, "volfog_mask__bounds", Point4)
      , ECS_RO_COMP(volfog_mask_render_es_comps, "volfog_mask__data", ecs::FloatList)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc volfog_mask_render_es_es_desc
(
  "volfog_mask_render_es",
  "prog/daNetGameLibs/volfog_mask/render/volfogMaskES.cpp.inl",
  ecs::EntitySystemOps(nullptr, volfog_mask_render_es_all_events),
  make_span(volfog_mask_render_es_comps+0, 2)/*rw*/,
  make_span(volfog_mask_render_es_comps+2, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
