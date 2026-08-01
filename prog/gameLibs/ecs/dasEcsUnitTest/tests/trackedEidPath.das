require ecs
require DasEcsUnitTest

// regression tests for the tracked-changes eid path (entityManagerTrack.cpp):
// - a change made only via set() by eid (no rw query runs) must fire exactly once
// - a change on a component that is both set() by eid and covered by a scheduled
//   archetype-wide scan in the same tick must fire exactly once, not twice or zero

[es(on_appear, REQUIRE=eidtrack_val)]
def tracked_eidpath_appear(evt : Event)
  set_test_value("EidPathChanged", 0)
  set_test_value("EidMixedChanged", 0)

[es(track=eidtrack_val, REQUIRE=eidtrack_val)]
def eidtrack_val_changed(evt : Event)
  set_test_value("EidPathChanged", get_test_value("EidPathChanged") + 1)

[es(track=eidtrack_mixed, REQUIRE=eidtrack_mixed)]
def eidtrack_mixed_changed(evt : Event)
  set_test_value("EidMixedChanged", get_test_value("EidMixedChanged") + 1)

[es(on_event=EventStart)]
def eidtrack_write(evt : Event; eid : EntityId; eidtrack_val : int; var eidtrack_mixed : int&)
  // pure eid path: nothing has rw access to eidtrack_val, only set() by eid
  set(eid, "eidtrack_val", eidtrack_val + 1)
  // mixed path: rw access here schedules the archetype-wide scan for eidtrack_mixed,
  // and set() additionally enqueues the same (entity, component) into the eid queue
  eidtrack_mixed = eidtrack_mixed + 1
  set(eid, "eidtrack_mixed", eidtrack_mixed)

[es(on_event=EventStart2)]
def eidtrack_touch(evt : Event; eid : EntityId; eidtrack_val : int; var eidtrack_mixed : int&)
  // rewrite the same values: neither the eid compare nor the rescheduled
  // archetype scan may fire additional change events
  eidtrack_mixed = eidtrack_mixed
  set(eid, "eidtrack_val", eidtrack_val)
  set(eid, "eidtrack_mixed", eidtrack_mixed)

[es(on_event=EventEnd)]
def verify_eidtrack(evt : Event; eidtrack_val : int; eidtrack_mixed : int)
  let eidChanged = get_test_value("EidPathChanged")
  let mixedChanged = get_test_value("EidMixedChanged")
  print("tracked eid path: eid={eidChanged} mixed={mixedChanged} (all must be 1)")
  verify(eidChanged == 1)
  verify(mixedChanged == 1)
  verify(eidtrack_val == 1)
  verify(eidtrack_mixed == 1)
