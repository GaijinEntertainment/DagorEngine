require ecs
require DasEcsUnitTest

[es(on_event=EventStart)]
def track_after_recreate(evt : Event)
  let eid = createEntitySync("entityWith_tracked_int_var1")
  ecs::query() <| $ [es] (var tracked_int_var1 : int&)
    tracked_int_var1++
  reCreateEntityFrom(eid, "entityWith_tracked_int_var1_and_2")

[es(track=tracked_int_var1, REQUIRE=tracked_int_var1)]
def tracking_tracked_int_var1(evt : Event)
  set_test_value("tracked_int_var1", 1)

[es(on_event=EventEnd)]
def tracking_tracked_int_var1_verifier(evt : Event)
  verify(get_test_value("tracked_int_var1") == 1)

