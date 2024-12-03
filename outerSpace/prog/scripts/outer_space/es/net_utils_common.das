module net_utils_common shared

require ecs
require net
require DngNet

def target_entity_conn(target_entity : EntityId) : array<net::IConnection?>
  var recipients : array<net::IConnection?>
  query(target_entity) <| $ [es] (replication : NetObject)
    recipients |> push(replication |> net_object_getControlledBy() |> get_client_connection())
  return <- recipients
