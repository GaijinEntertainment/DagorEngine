// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "terraformES.cpp.inl"
ECS_DEF_PULL_VAR(terraform);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc terraform_init_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("terraform"), ecs::ComponentTypeInfo<TerraformComponent>()}
};
static void terraform_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventLevelLoaded>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    terraform_init_es_event_handler(static_cast<const EventLevelLoaded&>(evt)
        , ECS_RW_COMP(terraform_init_es_event_handler_comps, "terraform", TerraformComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc terraform_init_es_event_handler_es_desc
(
  "terraform_init_es",
  "prog/daNetGameLibs/terraform/./terraformES.cpp.inl",
  ecs::EntitySystemOps(nullptr, terraform_init_es_event_handler_all_events),
  make_span(terraform_init_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded>::build(),
  0
);
