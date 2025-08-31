// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "reCreateEntityES.cpp.inl"
ECS_DEF_PULL_VAR(reCreateEntity);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc remote_recreate_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void remote_recreate_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<CmdRemoteRecreate>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    remote_recreate_es(static_cast<const CmdRemoteRecreate&>(evt)
        , ECS_RO_COMP(remote_recreate_es_comps, "eid", ecs::EntityId)
    , components.manager()
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc remote_recreate_es_es_desc
(
  "remote_recreate_es",
  "prog/daNetGame/net/reCreateEntityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, remote_recreate_es_all_events),
  empty_span(),
  make_span(remote_recreate_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CmdRemoteRecreate>::build(),
  0
,"netClient");
