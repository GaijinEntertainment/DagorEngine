//set of functions to make life possible with very poor network messages and events native API
// better to implement it native code and even not at daNetGame but in some gameLib

let ecs = require("%sqstd/ecs.nut")
let { has_network, is_server } = require("net")
let console = require("console")

let {
  server_send_net_sqevent, server_broadcast_net_sqevent,
  client_request_unicast_net_sqevent, client_request_broadcast_net_sqevent
} = require("ecs.netevent")


function update_component(eid, component_name) {
  console.command($"ecs.update_component {eid} {component_name}")
}

let _get_msgSink = ecs.SqQuery("_get_msgSink", {comps_rq = ["msg_sink"]})
function _get_msg_sink_eid(){
  return _get_msgSink.perform(@(eid, _comp) eid) ?? ecs.INVALID_ENTITY_ID
}

function client_send_event(eid, evt){
  if (!is_server())
    return client_request_unicast_net_sqevent(eid, evt)
  else
    return ecs.g_entity_mgr.sendEvent(eid, evt)
}

function client_broadcast_event(evt){
  if (!is_server())
    return client_request_broadcast_net_sqevent(evt)
  else
    return ecs.g_entity_mgr.broadcastEvent(evt)
}

function client_msg_sink(evt) {
  return client_send_event(_get_msg_sink_eid(), evt)
}

// Note: empty (zero len) connids would send to no-one, null connids would send to everyone (broadcast)
function server_broadcast_event(evt, connids=null){
  if (has_network())
    server_broadcast_net_sqevent(evt, connids)
  else
    ecs.g_entity_mgr.broadcastEvent(evt)
}

// Note: empty (zero len) connids would send to no-one, null connids would send to everyone (broadcast)
function server_send_event(eid, evt, connids=null){
  if (has_network())
    server_send_net_sqevent(eid, evt, connids)
  else
    ecs.g_entity_mgr.sendEvent(eid, evt)
}

function server_msg_sink(evt, connids=null) {
  server_send_event(_get_msg_sink_eid(), evt, connids)
}

return ecs.__update({
  client_msg_sink
  client_send_event
  client_broadcast_event

  server_msg_sink
  server_send_event
  server_broadcast_event

  update_component
})
