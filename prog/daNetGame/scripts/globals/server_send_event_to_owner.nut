let ecs = require("ecs")
let {INVALID_CONNECTION_ID} = require("net")
let {find_connected_player_that_possess} = require("common_queries.nut")
let {server_send_net_sqevent} = require("ecs.netevent")

function server_send_event_to_owner(eid, evt) {
  let possessesPlayerEid = find_connected_player_that_possess(eid) ?? ecs.INVALID_ENTITY_ID
  let connectionsToSend = [ecs.obsolete_dbg_get_comp_val(possessesPlayerEid, "connid", INVALID_CONNECTION_ID)]
  server_send_net_sqevent(eid, evt, connectionsToSend)
}

return server_send_event_to_owner
