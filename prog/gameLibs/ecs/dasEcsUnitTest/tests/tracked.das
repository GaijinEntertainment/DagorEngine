require ecs
require DasEcsUnitTest

[es(track=trackable, REQUIRE=trackable)]
def das_track_changed(evt : Event; eid : EntityId)
  set_test_value("TrackChanged", 1)
  print("{eid}")

[es(on_event=EventStart)]
def das_modify_trackable(evt : Event; var trackable : bool&)
  trackable = !trackable

[es(on_event=EventEnd)]
def das_verify_trackable(evt : Event)
  verify(get_test_value("TrackChanged") == 1)

[es(track=trackable2, REQUIRE=trackable2)]
def das_track_changed2(evt : Event)
  set_test_value("TrackChanged2", 1)

[es(on_event=EventStart)]
def das_modify_trackable2(evt : Event; var trackable2 : bool&)
  trackable2 = !trackable2

[es(on_event=EventEnd)]
def das_verify_trackable2(evt : Event)
  verify(get_test_value("TrackChanged2") == 1)
