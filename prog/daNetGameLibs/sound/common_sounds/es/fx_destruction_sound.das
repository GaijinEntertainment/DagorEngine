require ecs
require soundEvent


[es(tag=sound, on_appear)]
def fx_destruction_sound(evt : Event;
                         fx_destruction_sound__path : string;
                         transform : float3x4)

  find_query() <| $ [es(REQUIRE=watchedByPlr)] ()
    oneshot(fx_destruction_sound__path, transform[3])
    return true
