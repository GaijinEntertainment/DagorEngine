require ecs

[event(broadcast, version=1)]
struct SchemeHashVersion1Event
  i : int

[event(broadcast, version=2)]
struct SchemeHashVersion2Event
  i : int

def scheme_hash(event_name : string)
  let eventId = get_events_db() |> events_db_findEvent(ecs_hash(event_name))
  return get_events_db() |> events_db_getEventSchemeHash(eventId)

[es(on_event=EventStart)]
def event_version_changes_scheme_hash(evt : Event)
  verify(scheme_hash("SchemeHashVersion1Event") != scheme_hash("SchemeHashVersion2Event"))
