require UnitTest

typedef
    EntityIdDummy = int

[event]
struct NullEvent {}

[sideeffects]
def _builtin_event_strdup(evt)
    pass
[sideeffects]
def _builtin_send_blobevent(eid, evt, evt_name)
    pass

def ecs_hash(name)
    return hash(name)

[generic]
def sendEvent ( eid:EntityIdDummy const; var evt)
    concept_assert(typeinfo(struct_has_annotation<cpp_event> evt) || typeinfo(struct_has_annotation<event> evt), "Event type should be struct with event or cpp_event annotations>")
    concept_assert(typeinfo(struct_has_annotation<cpp_event> evt) || typeinfo(is_raw evt), "We currently don't support non blob das events")
    if (typeinfo(struct_has_annotation<cpp_event> evt) || typeinfo(struct_has_annotation<event> evt))
        let evt_name = typeinfo(struct_has_annotation<cpp_event> evt) ? typeinfo(undecorated_typename evt) : typeinfo(typename evt)
        evt.eventType = ecs_hash(evt_name)
        evt.eventSize = uint16(typeinfo(sizeof evt))
        assert ( int(evt.eventFlags)==0 )
        if !typeinfo(is_raw evt)
            _builtin_event_strdup(evt)
            evt.eventFlags = uint16(ecs::EVFLG_DESTROY)
        _builtin_send_blobevent(eid, evt, evt_name)

[export]
def test
    sendEvent(13, [[NullEvent]])
    return true
