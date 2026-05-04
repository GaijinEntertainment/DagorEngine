require ecs
require app
require soundEvent
require DagorSystem
require strings


[es(tag=sound, on_appear)]
def fx_destruction_sound(evt : Event;
                         eid : EntityId;
                         fx_destruction_sound__path : string;
                         transform : float3x4)
  if empty(fx_destruction_sound__path)
    logerr("empty fx_destruction_sound__path in entity {eid} '{getEntityTemplateName(eid)}'")
    return
  find_query() <| $ [es(REQUIRE=watchedByPlr)] ()
    oneshot(fx_destruction_sound__path, transform[3])
    return true
