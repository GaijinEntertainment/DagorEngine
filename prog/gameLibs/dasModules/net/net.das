require ecs
require net
options indenting = 2

let INVALID_CONNECTION_ID = -1

[generic]
def server_send_schemeless_event(eid : EntityId const; evt_name : string const)
  server_send_schemeless_event(eid, evt_name) <| $ [unused_argument(init)] (var init : Object) {}

[generic]
def server_send_schemeless_event(eid : EntityId const; evt_name : string const; to_whom : EntityId const)
  server_send_schemeless_event(eid, evt_name, to_whom) <| $ [unused_argument(init)] (var init : Object) {}

[generic]
def server_broadcast_schemeless_event(evt_name : string const)
  server_broadcast_schemeless_event(evt_name) <| $ [unused_argument(init)] (var init : Object) {}

[generic]
def server_broadcast_schemeless_event(evt_name : string const; to_whom : EntityId const)
  server_broadcast_schemeless_event(evt_name, to_whom) <| $ [unused_argument(init)] (var init : Object) {}

[generic]
def client_send_schemeless_event(eid : EntityId const; evt_name : string const)
  client_send_schemeless_event(eid, evt_name) <| $ [unused_argument(init)] (var init : Object) {}

[generic]
def client_send_schemeless_event(evt_name : string const)
  client_send_schemeless_event(evt_name) <| $ [unused_argument(init)] (var init : Object) {}

[generic]
def send_net_event(eid : EntityId const; var evt)
  concept_assert(typeinfo struct_has_annotation < event > (evt) && typeinfo struct_safe_has_annotation_argument < event; routing > (evt),
    "Event type should be struct with event annotation with routing argument.   <{typeinfo typename(evt)}>")
  let evt_name = typeinfo undecorated_typename(evt)
  evt.eventType = ecs_hash(evt_name)
  evt.eventSize = uint16(typeinfo sizeof(evt))
  static_if typeinfo struct_safe_has_annotation_argument < event; unicast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_UNICAST)
  static_if typeinfo struct_safe_has_annotation_argument < event; broadcast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_BROADCAST)
  if !typeinfo is_raw(evt)
    _builtin_event_dup(evt)
    evt.eventFlags = uint16(uint(evt.eventFlags) | uint(ecs::EVFLG_DESTROY))
  _builtin_send_net_blobevent(eid, evt, evt_name)

[generic]
def send_net_event(eid : EntityId const; var evt; explicit_recipients : array<net::IConnection?>)
  concept_assert(typeinfo struct_has_annotation < event > (evt) && typeinfo struct_safe_has_annotation_argument < event; routing > (evt),
    "Event type should be struct with event annotation with routing argument.   <{typeinfo typename(evt)}>")
  let evt_name = typeinfo undecorated_typename(evt)
  evt.eventType = ecs_hash(evt_name)
  evt.eventSize = uint16(typeinfo sizeof(evt))
  static_if typeinfo struct_safe_has_annotation_argument < event; unicast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_UNICAST)
  static_if typeinfo struct_safe_has_annotation_argument < event; broadcast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_BROADCAST)
  if !typeinfo is_raw(evt)
    _builtin_event_dup(evt)
    evt.eventFlags = uint16(uint(evt.eventFlags) | uint(ecs::EVFLG_DESTROY))
  _builtin_send_net_blobevent_with_recipients(eid, evt, evt_name, explicit_recipients)

[generic]
def broadcast_net_event(var evt)
  concept_assert(typeinfo struct_has_annotation < event > (evt) && typeinfo struct_safe_has_annotation_argument < event; routing > (evt),
    "Event type should be struct with event annotation with routing argument.   <{typeinfo typename(evt)}>")
  let evt_name = typeinfo undecorated_typename(evt)
  evt.eventType = ecs_hash(evt_name)
  evt.eventSize = uint16(typeinfo sizeof(evt))
  static_if typeinfo struct_safe_has_annotation_argument < event; unicast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_UNICAST)
  static_if typeinfo struct_safe_has_annotation_argument < event; broadcast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_BROADCAST)
  if !typeinfo is_raw(evt)
    _builtin_event_dup(evt)
    evt.eventFlags = uint16(uint(evt.eventFlags) | uint(ecs::EVFLG_DESTROY))
  _builtin_broadcast_net_blobevent(evt, evt_name)

[generic]
def broadcast_net_event(var evt; explicit_recipients : array<net::IConnection?>)
  concept_assert(typeinfo struct_has_annotation < event > (evt) && typeinfo struct_safe_has_annotation_argument < event; routing > (evt),
    "Event type should be struct with event annotation with routing argument.   <{typeinfo typename(evt)}>")
  let evt_name = typeinfo undecorated_typename(evt)
  evt.eventType = ecs_hash(evt_name)
  evt.eventSize = uint16(typeinfo sizeof(evt))
  evt.eventFlags = uint16(0)//should not be needed
  static_if typeinfo struct_safe_has_annotation_argument < event; unicast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_UNICAST)
  static_if typeinfo struct_safe_has_annotation_argument < event; broadcast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_BROADCAST)
  if !typeinfo is_raw(evt)
    _builtin_event_dup(evt)
    evt.eventFlags = uint16(uint(evt.eventFlags) | uint(ecs::EVFLG_DESTROY))
  _builtin_broadcast_net_blobevent_with_recipients(evt, evt_name, explicit_recipients)
