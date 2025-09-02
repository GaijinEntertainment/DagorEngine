// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "blurredUIES.cpp.inl"
ECS_DEF_PULL_VAR(blurredUI);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc set_resolution_blurred_ui_manager_es_comps[] =
{
//start of 5 rw components at [0]
  {ECS_HASH("blurred_ui__manager"), ecs::ComponentTypeInfo<BlurredUI>()},
  {ECS_HASH("blurred_ui__texid"), ecs::ComponentTypeInfo<TEXTUREID>()},
  {ECS_HASH("blurred_ui__smp"), ecs::ComponentTypeInfo<d3d::SamplerHandle>()},
  {ECS_HASH("blurred_ui_sdr__texid"), ecs::ComponentTypeInfo<TEXTUREID>()},
  {ECS_HASH("blurred_ui_sdr__smp"), ecs::ComponentTypeInfo<d3d::SamplerHandle>()},
//start of 1 ro components at [5]
  {ECS_HASH("blurred_ui__levels"), ecs::ComponentTypeInfo<uint32_t>()}
};
static void set_resolution_blurred_ui_manager_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<SetResolutionEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    set_resolution_blurred_ui_manager_es(static_cast<const SetResolutionEvent&>(evt)
        , ECS_RW_COMP(set_resolution_blurred_ui_manager_es_comps, "blurred_ui__manager", BlurredUI)
    , ECS_RO_COMP(set_resolution_blurred_ui_manager_es_comps, "blurred_ui__levels", uint32_t)
    , ECS_RW_COMP(set_resolution_blurred_ui_manager_es_comps, "blurred_ui__texid", TEXTUREID)
    , ECS_RW_COMP(set_resolution_blurred_ui_manager_es_comps, "blurred_ui__smp", d3d::SamplerHandle)
    , ECS_RW_COMP(set_resolution_blurred_ui_manager_es_comps, "blurred_ui_sdr__texid", TEXTUREID)
    , ECS_RW_COMP(set_resolution_blurred_ui_manager_es_comps, "blurred_ui_sdr__smp", d3d::SamplerHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc set_resolution_blurred_ui_manager_es_es_desc
(
  "set_resolution_blurred_ui_manager_es",
  "prog/daNetGameLibs/blurred_ui/render/blurredUIES.cpp.inl",
  ecs::EntitySystemOps(nullptr, set_resolution_blurred_ui_manager_es_all_events),
  make_span(set_resolution_blurred_ui_manager_es_comps+0, 5)/*rw*/,
  make_span(set_resolution_blurred_ui_manager_es_comps+5, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<SetResolutionEvent>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc init_blurred_ui_manager_es_comps[] =
{
//start of 5 rw components at [0]
  {ECS_HASH("blurred_ui__manager"), ecs::ComponentTypeInfo<BlurredUI>()},
  {ECS_HASH("blurred_ui__texid"), ecs::ComponentTypeInfo<TEXTUREID>()},
  {ECS_HASH("blurred_ui__smp"), ecs::ComponentTypeInfo<d3d::SamplerHandle>()},
  {ECS_HASH("blurred_ui_sdr__texid"), ecs::ComponentTypeInfo<TEXTUREID>()},
  {ECS_HASH("blurred_ui_sdr__smp"), ecs::ComponentTypeInfo<d3d::SamplerHandle>()},
//start of 1 ro components at [5]
  {ECS_HASH("blurred_ui__levels"), ecs::ComponentTypeInfo<uint32_t>()}
};
static void init_blurred_ui_manager_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_blurred_ui_manager_es(evt
        , ECS_RW_COMP(init_blurred_ui_manager_es_comps, "blurred_ui__manager", BlurredUI)
    , ECS_RO_COMP(init_blurred_ui_manager_es_comps, "blurred_ui__levels", uint32_t)
    , ECS_RW_COMP(init_blurred_ui_manager_es_comps, "blurred_ui__texid", TEXTUREID)
    , ECS_RW_COMP(init_blurred_ui_manager_es_comps, "blurred_ui__smp", d3d::SamplerHandle)
    , ECS_RW_COMP(init_blurred_ui_manager_es_comps, "blurred_ui_sdr__texid", TEXTUREID)
    , ECS_RW_COMP(init_blurred_ui_manager_es_comps, "blurred_ui_sdr__smp", d3d::SamplerHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_blurred_ui_manager_es_es_desc
(
  "init_blurred_ui_manager_es",
  "prog/daNetGameLibs/blurred_ui/render/blurredUIES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_blurred_ui_manager_es_all_events),
  make_span(init_blurred_ui_manager_es_comps+0, 5)/*rw*/,
  make_span(init_blurred_ui_manager_es_comps+5, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc update_blurred_ui_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("blurred_ui__manager"), ecs::ComponentTypeInfo<BlurredUI>()}
};
static void update_blurred_ui_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateBlurredUI>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_blurred_ui_es(static_cast<const UpdateBlurredUI&>(evt)
        , ECS_RW_COMP(update_blurred_ui_es_comps, "blurred_ui__manager", BlurredUI)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_blurred_ui_es_es_desc
(
  "update_blurred_ui_es",
  "prog/daNetGameLibs/blurred_ui/render/blurredUIES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_blurred_ui_es_all_events),
  make_span(update_blurred_ui_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateBlurredUI>::build(),
  0
,"render");
