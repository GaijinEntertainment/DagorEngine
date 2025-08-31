// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "volfogSettingsManagerES.cpp.inl"
ECS_DEF_PULL_VAR(volfogSettingsManager);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc volfog_convar_init_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("render_settings__HQVolfog"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__volumeFogQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void volfog_convar_init_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    volfog_convar_init_es(evt
        , ECS_RO_COMP(volfog_convar_init_es_comps, "render_settings__HQVolfog", bool)
    , ECS_RO_COMP(volfog_convar_init_es_comps, "render_settings__volumeFogQuality", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc volfog_convar_init_es_es_desc
(
  "volfog_convar_init_es",
  "prog/daNetGame/render/world/volfogSettingsManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, volfog_convar_init_es_all_events),
  empty_span(),
  make_span(volfog_convar_init_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"render_settings__HQVolfog,render_settings__volumeFogQuality");
static constexpr ecs::ComponentDesc volfog_convar_change_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("render_settings__HQVolfog"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__volumeFogQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void volfog_convar_change_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    volfog_convar_change_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(volfog_convar_change_es_comps, "render_settings__HQVolfog", bool)
    , ECS_RW_COMP(volfog_convar_change_es_comps, "render_settings__volumeFogQuality", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc volfog_convar_change_es_es_desc
(
  "volfog_convar_change_es",
  "prog/daNetGame/render/world/volfogSettingsManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, volfog_convar_change_es_all_events),
  make_span(volfog_convar_change_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc volfog_change_settings_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("render_settings__HQVolfog"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__volumeFogQuality"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("render_settings__shadowsQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void volfog_change_settings_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    volfog_change_settings_es(evt
        , ECS_RO_COMP(volfog_change_settings_es_comps, "render_settings__HQVolfog", bool)
    , ECS_RO_COMP(volfog_change_settings_es_comps, "render_settings__volumeFogQuality", ecs::string)
    , ECS_RO_COMP(volfog_change_settings_es_comps, "render_settings__shadowsQuality", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc volfog_change_settings_es_es_desc
(
  "volfog_change_settings_es",
  "prog/daNetGame/render/world/volfogSettingsManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, volfog_change_settings_es_all_events),
  empty_span(),
  make_span(volfog_change_settings_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnExplicitVolfogSettingsChange,
                       OnRenderSettingsReady>::build(),
  0
,nullptr,"render_settings__HQVolfog,render_settings__shadowsQuality,render_settings__volumeFogQuality");
