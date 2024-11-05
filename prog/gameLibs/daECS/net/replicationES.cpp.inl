// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/net/object.h>
#include <daECS/net/compBlacklist.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>

G_STATIC_ASSERT(!ecs::ComponentTypeInfo<net::Object>::can_be_tracked);

namespace net
{

void server_replication_cb(ecs::EntityId eid, ecs::component_index_t cidx)
{
  auto repl = ECS_GET_NULLABLE_RW(net::Object, eid, replication);
  G_ASSERT_RETURN(repl, );
  repl->onCompChanged(cidx);
}

void client_validate_replication_cb(ecs::EntityId eid, ecs::component_index_t cidx)
{
  auto repl = ECS_GET_NULLABLE(net::Object, eid, replication);
  G_ASSERT_RETURN(repl, );
  net::replicated_component_on_client_change(eid, cidx);
}

ECS_TAG(server, net)
ECS_ON_EVENT(ecs::EventEntityManagerBeforeClear, ecs::EventEntityManagerEsOrderSet)
inline void reset_replication_es_event_handler(const ecs::Event &, ecs::EntityManager &manager) { manager.setReplicationCb(NULL); }


ECS_TAG(netClient, dev)
ECS_AFTER(reset_replication_es)
inline void client_validation_es_event_handler(const ecs::EventEntityManagerEsOrderSet &, ecs::EntityManager &manager)
{
  G_UNUSED(manager);
#if ECS_NET_COMP_BLACKLIST_ENABLED
  manager.setReplicationCb(client_validate_replication_cb);
#endif
}

ECS_TAG(server, net)
ECS_AFTER(reset_replication_es)
inline void server_replication_es_event_handler(const ecs::EventEntityManagerEsOrderSet &, ecs::EntityManager &manager)
{
  manager.setReplicationCb(server_replication_cb);
}

ECS_TAG(server, net)
inline void replication_es_event_handler(const ecs::EventEntityRecreated &, net::Object &replication)
{
  replication.initCompVers();
  if (replication.filteredComponentsBits != 0) // todo: probably remove it, as soon as we implement filters types
    replication.addToDirty();
}

ECS_TAG(netClient, dev)
ECS_ON_EVENT(on_appear)
inline void replication_validation_es_event_handler(const ecs::Event &, net::Object &replication)
{
#if ECS_NET_COMP_BLACKLIST_ENABLED
  replication.initCompVers();
#else
  G_UNUSED(replication);
#endif
}

ECS_TAG(netClient, dev)
ECS_REQUIRE(net::Object replication)
inline void replication_validation_es_event_handler(const ecs::EventEntityDestroyed &, ecs::EntityId eid)
{
  net::replicated_component_on_client_destroy(eid);
}

ECS_TAG(netClient)
inline void replication_destruction_es_event_handler(const ecs::EventEntityManagerBeforeClear &)
{
  net::Object::doNotVerifyDestruction(true);
}

ECS_TAG(netClient)
inline void replication_destruction_es_event_handler(const ecs::EventEntityManagerAfterClear &)
{
  net::Object::doNotVerifyDestruction(false);
  net::Object::clear_pending_destroys();
}

ECS_TAG(netClient)
ECS_REQUIRE_NOT(ecs::Tag client__canDestroyServerEntity)
inline void replication_destruction_logerr_es_event_handler(const ecs::EventEntityDestroyed &, ecs::EntityId eid,
  const net::Object &replication)
{
  if (DAGOR_UNLIKELY(!replication.meantToBeDestroyed()))
  {
    if (!net::Object::remove_from_pending_destroys(eid))
      logerr("Only server has the authority to destroy network entities, destroying %d(%s).", ecs::entity_id_t(eid),
        g_entity_mgr->getEntityTemplateName(eid));
  }
}

/*static*/
net::Object *Object::getByEid(ecs::EntityId eid_) { return ECS_GET_NULLABLE_RW(net::Object, eid_, replication); }

}; // namespace net
