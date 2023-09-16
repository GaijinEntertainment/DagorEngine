#include <daECS/net/recipientFilters.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/object.h>
#include <memory/dag_framemem.h>
#include <daECS/net/netbase.h>

namespace rcptf
{

net::IConnection *get_entity_ctrl_conn(ecs::EntityId eid)
{
  const net::Object *robj = g_entity_mgr->getNullable<net::Object>(eid, ECS_HASH("replication"));
  return robj ? get_client_connection(robj->getControlledBy()) : nullptr;
}

} // namespace rcptf
