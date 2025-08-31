options indenting = 2
options gen2 = false
require ecs
require math public
let
  INVALID_ENTITY_ID : EntityId = EntityId(INVALID_ENTITY_ID_VAL)


[generic]
def getOrBuildTemplateByName(template_name : string) : Template const? const
  let res = getTemplateByName(template_name)
  if res != null
    return res
  return buildTemplateByName(template_name)


[generic]
def addSubTemplate(eid : EntityId const; addname : string) : EntityId
  return addSubTemplate(eid, addname) <| $ [unused_argument(init)] (var init : ComponentsInitializer) {}
[generic]
def removeSubTemplate(eid : EntityId const; removename : string) : EntityId
  return removeSubTemplate(eid, removename) <| $ [unused_argument(init)] (var init : ComponentsInitializer) {}

[generic]
def sendEvent(eid : EntityId const; var evt)
  concept_assert(typeinfo struct_has_annotation < cpp_event > (evt) || typeinfo struct_has_annotation < event > (evt), "Event type should be struct with event or cpp_event annotations, and it is not <{typeinfo typename(evt)}>")
  concept_assert(!typeinfo struct_safe_has_annotation_argument < event; routing > (evt),
    "Trying to send a network event using non-networked sendEvent. Use send_net_event or non networked event. <{typeinfo typename(evt)}>")
  let evt_name = typeinfo undecorated_typename(evt)
  evt.eventType = ecs_hash(evt_name)
  evt.eventSize = uint16(typeinfo sizeof(evt))
  static_if typeinfo struct_safe_has_annotation_argument < event; unicast > (evt) || typeinfo struct_safe_has_annotation_argument < cpp_event; unicast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_UNICAST)
  static_if typeinfo struct_safe_has_annotation_argument < event; broadcast > (evt) || typeinfo struct_safe_has_annotation_argument < cpp_event; broadcast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_BROADCAST)
  if !typeinfo is_raw(evt)
    _builtin_event_dup(evt)
    evt.eventFlags = uint16(uint(evt.eventFlags) | uint(ecs::EVFLG_DESTROY))
  _builtin_send_blobevent(eid, evt, evt_name)

[generic]
def send_schemeless_event(eid : EntityId const; name : string)
  send_schemeless_event(eid, name) <| $ [unused_argument(init)] (var init : Object) {}

[generic]
def broadcast_schemeless_event(name : string)
  broadcast_schemeless_event(name) <| $ [unused_argument(init)] (var init : Object) {}

[generic]
def sendEventImmediate(eid : EntityId const; var evt)
  concept_assert(typeinfo struct_has_annotation < cpp_event > (evt) || typeinfo struct_has_annotation < event > (evt), "Event type should be struct with event or cpp_event annotations, and it is not <{typeinfo typename(evt)}>")
  concept_assert(!typeinfo struct_safe_has_annotation_argument < event; routing > (evt),
    "Trying to send a network event using non-networked sendEventImmediate. Use send_net_event or non networked event. <{typeinfo typename(evt)}>")
  let evt_name = typeinfo undecorated_typename(evt)
  evt.eventType = ecs_hash(evt_name)
  evt.eventSize = uint16(typeinfo sizeof(evt))
  static_if typeinfo struct_safe_has_annotation_argument < event; unicast > (evt) || typeinfo struct_safe_has_annotation_argument < cpp_event; unicast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_UNICAST)
  static_if typeinfo struct_safe_has_annotation_argument < event; broadcast > (evt) || typeinfo struct_safe_has_annotation_argument < cpp_event; broadcast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_BROADCAST)
  if !typeinfo is_raw(evt)
    _builtin_event_dup(evt)
    evt.eventFlags = uint16(uint(evt.eventFlags) | uint(ecs::EVFLG_DESTROY))
  _builtin_send_blobevent_immediate(eid, evt, evt_name)

[generic]
def broadcastEvent(var evt)
  concept_assert(typeinfo struct_has_annotation < cpp_event > (evt) || typeinfo struct_has_annotation < event > (evt), "Event type should be struct with event or cpp_event annotations, and it is not <{typeinfo typename(evt)}>")
  concept_assert(!typeinfo struct_safe_has_annotation_argument < event; routing > (evt),
    "Trying to send a network event using non-networked broadcastEvent. Use broadcast_net_event or non networked event. <{typeinfo typename(evt)}>")
  let evt_name = typeinfo undecorated_typename(evt)
  evt.eventType = ecs_hash(evt_name)
  evt.eventSize = uint16(typeinfo sizeof(evt))
  static_if typeinfo struct_safe_has_annotation_argument < event; unicast > (evt) || typeinfo struct_safe_has_annotation_argument < cpp_event; unicast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_UNICAST)
  static_if typeinfo struct_safe_has_annotation_argument < event; broadcast > (evt) || typeinfo struct_safe_has_annotation_argument < cpp_event; broadcast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_BROADCAST)
  if !typeinfo is_raw(evt)
    _builtin_event_dup(evt)
    evt.eventFlags = uint16(uint(evt.eventFlags) | uint(ecs::EVFLG_DESTROY))
  _builtin_broadcast_blobevent(evt, evt_name)

[generic]
def broadcastEventImmediate(var evt)
  concept_assert(typeinfo struct_has_annotation < cpp_event > (evt) || typeinfo struct_has_annotation < event > (evt), "Event type should be struct with event or cpp_event annotations, and it is not <{typeinfo typename(evt)}>")
  concept_assert(!typeinfo struct_safe_has_annotation_argument < event; routing > (evt),
    "Trying to send a network event using non-networked broadcastEvent. Use broadcast_net_event or non networked event. <{typeinfo typename(evt)}>")
  let evt_name = typeinfo undecorated_typename(evt)
  evt.eventType = ecs_hash(evt_name)
  evt.eventSize = uint16(typeinfo sizeof(evt))
  static_if typeinfo struct_safe_has_annotation_argument < event; unicast > (evt) || typeinfo struct_safe_has_annotation_argument < cpp_event; unicast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_UNICAST)
  static_if typeinfo struct_safe_has_annotation_argument < event; broadcast > (evt) || typeinfo struct_safe_has_annotation_argument < cpp_event; broadcast > (evt)
    evt.eventFlags = uint16(ecs::EVCAST_BROADCAST)
  if !typeinfo is_raw(evt)
    _builtin_event_dup(evt)
    evt.eventFlags = uint16(uint(evt.eventFlags) | uint(ecs::EVFLG_DESTROY))
  _builtin_broadcast_blobevent_immediate(evt, evt_name)
