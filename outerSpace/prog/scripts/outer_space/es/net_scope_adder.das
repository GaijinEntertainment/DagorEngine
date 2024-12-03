require ecs
require net
require DngNet
require outer_space.events_outer_space

[es(tag=(server, net))]
def add_initial_entities_in_net_scope(evt : CmdAddInitialEntitiesInNetScope)
  var conn = evt.connid |> get_client_connection()
  query() <| $ [es] (var replication : NetObject)
    *conn |> setObjectInScopeAlways(replication)

[es(tag=(server, net), on_appear)]
def net_scope_adder(evt : Event; var replication : NetObject&)
  query() <| $ [es] (connid : int)
    var conn : IConnection? = connid |> get_client_connection()
    if conn != null
      *conn |> setObjectInScopeAlways(replication)
