import "ecs" as ecs
from "net" import INVALID_CONNECTION_ID
from "common_queries.nut" import find_connected_player_that_possess
from "ecs.netevent" import server_send_net_sqevent

function server_send_event_to_owner(eid, evt) {
  let possessesPlayerEid = find_connected_player_that_possess(eid) ?? ecs.INVALID_ENTITY_ID
  let connectionsToSend = [ecs.obsolete_dbg_get_comp_val(possessesPlayerEid, "connid", INVALID_CONNECTION_ID)]
  server_send_net_sqevent(eid, evt, connectionsToSend)
}

return server_send_event_to_owner
