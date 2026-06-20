// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "netLifecycleES.cpp.inl"
ECS_DEF_PULL_VAR(netLifecycle);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc net_lifecycle_init_client_es_comps[] ={};
static void net_lifecycle_init_client_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnNetInitClient>());
  net_lifecycle_init_client_es(static_cast<const OnNetInitClient&>(evt)
        );
}
static ecs::EntitySystemDesc net_lifecycle_init_client_es_es_desc
(
  "net_lifecycle_init_client_es",
  "prog/daNetGame/net/netLifecycleES.cpp.inl",
  ecs::EntitySystemOps(nullptr, net_lifecycle_init_client_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnNetInitClient>::build(),
  0
,"gEntityMgr",nullptr,"*");
//static constexpr ecs::ComponentDesc net_lifecycle_init_server_es_comps[] ={};
static void net_lifecycle_init_server_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnNetInitServer>());
  net_lifecycle_init_server_es(static_cast<const OnNetInitServer&>(evt)
        );
}
static ecs::EntitySystemDesc net_lifecycle_init_server_es_es_desc
(
  "net_lifecycle_init_server_es",
  "prog/daNetGame/net/netLifecycleES.cpp.inl",
  ecs::EntitySystemOps(nullptr, net_lifecycle_init_server_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnNetInitServer>::build(),
  0
,"gEntityMgr",nullptr,"*");
//static constexpr ecs::ComponentDesc net_lifecycle_pre_emgr_clear_es_comps[] ={};
static void net_lifecycle_pre_emgr_clear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventOnNetworkDestroyed>());
  net_lifecycle_pre_emgr_clear_es(static_cast<const EventOnNetworkDestroyed&>(evt)
        );
}
static ecs::EntitySystemDesc net_lifecycle_pre_emgr_clear_es_es_desc
(
  "net_lifecycle_pre_emgr_clear_es",
  "prog/daNetGame/net/netLifecycleES.cpp.inl",
  ecs::EntitySystemOps(nullptr, net_lifecycle_pre_emgr_clear_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnNetworkDestroyed>::build(),
  0
,"gEntityMgr",nullptr,"*");
//static constexpr ecs::ComponentDesc net_lifecycle_destroy_es_comps[] ={};
static void net_lifecycle_destroy_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<OnNetDestroy>());
  net_lifecycle_destroy_es(static_cast<const OnNetDestroy&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc net_lifecycle_destroy_es_es_desc
(
  "net_lifecycle_destroy_es",
  "prog/daNetGame/net/netLifecycleES.cpp.inl",
  ecs::EntitySystemOps(nullptr, net_lifecycle_destroy_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnNetDestroy>::build(),
  0
,"gEntityMgr",nullptr,"*");
//static constexpr ecs::ComponentDesc net_lifecycle_torn_down_es_comps[] ={};
static void net_lifecycle_torn_down_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventNetTornDown>());
  net_lifecycle_torn_down_es(static_cast<const EventNetTornDown&>(evt)
        );
}
static ecs::EntitySystemDesc net_lifecycle_torn_down_es_es_desc
(
  "net_lifecycle_torn_down_es",
  "prog/daNetGame/net/netLifecycleES.cpp.inl",
  ecs::EntitySystemOps(nullptr, net_lifecycle_torn_down_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventNetTornDown>::build(),
  0
,"gEntityMgr",nullptr,"*");
