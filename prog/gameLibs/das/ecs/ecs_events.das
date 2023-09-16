options indenting = 2
module ecs_events shared public
require ecs
require %daslib/templates
require %daslib/type_traits public

//example
//def my_es(evt : Event)
//  if evt.eventType == get_ecs_event_type(type<MyEvent>)
[template(t), unused_argument(t)]
def get_ecs_event_type(t : auto(T))
  let evt_name = typeinfo(undecorated_typename type<T- const>)
  return ecs_hash(evt_name)


[generic]
def getEventName(evt : auto(no_const))
  if typeinfo(safe_has_field < eventName > evt) || typeinfo(safe_has_property < eventName > evt)
    // return invoke(".`eventName", evt)
    return evt.eventName
  elif typeinfo(safe_has_field < eventType > evt) || typeinfo(safe_has_property < eventType > evt)
    // return get_events_db() |> events_db_findEventName(invoke(".`eventType", evt))
    return get_events_db() |> events_db_findEventName(evt.eventType)
  else
    return ""
