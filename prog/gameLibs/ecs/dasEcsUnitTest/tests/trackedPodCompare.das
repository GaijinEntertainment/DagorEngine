require ecs
require DasEcsUnitTest
require math
require DagorMath

// regression tests for tracked POD compare on tick (entityManagerTrack.cpp):
// change detection must be bitwise for any POD size.
// - 24-byte components (DPoint3) used to be compared as 3 doubles: a NaN payload
//   refired the change event on every rescan, and a 0.0 -> -0.0 flip was invisible
// - PODs of size%16==0 above 48 bytes (TMatrix4, 64) used to compare only the
//   first 16 bytes of each entity, so tail-only changes were silently lost

[es(on_appear, REQUIRE=nan_dp3)]
def tracked_pod_compare_appear(evt : Event)
  set_test_value("NanDp3Changed", 0)
  set_test_value("NegZeroDp3Changed", 0)
  set_test_value("BigPodChanged", 0)

[es(track=nan_dp3, REQUIRE=nan_dp3)]
def nan_dp3_changed(evt : Event)
  set_test_value("NanDp3Changed", get_test_value("NanDp3Changed") + 1)

[es(track=negzero_dp3, REQUIRE=negzero_dp3)]
def negzero_dp3_changed(evt : Event)
  set_test_value("NegZeroDp3Changed", get_test_value("NegZeroDp3Changed") + 1)

[es(track=big_pod, REQUIRE=big_pod)]
def big_pod_changed(evt : Event)
  set_test_value("BigPodChanged", get_test_value("BigPodChanged") + 1)

[es(on_event=EventStart)]
def write_tracked_pods(evt : Event; var nan_dp3 : DPoint3&; var negzero_dp3 : DPoint3&; var big_pod : float4x4&)
  nan_dp3.x = make_nan_double() // real change, must fire exactly once
  negzero_dp3.y = make_neg_zero_double() // 0.0 -> -0.0 is a bitwise change, must fire
  big_pod[3] = float4(1, 2, 3, 4) // changes only bytes 48..63 of the 64-byte pod

[es(on_event=EventStart2)]
def touch_tracked_pods(evt : Event; var nan_dp3 : DPoint3&; var negzero_dp3 : DPoint3&; var big_pod : float4x4&)
  // rewrite the same bytes: rw access reschedules the archetype rescan while values
  // stay bitwise-identical, so no additional change events may fire (NaN included)
  nan_dp3.x = nan_dp3.x
  negzero_dp3.y = negzero_dp3.y
  big_pod[3] = big_pod[3]

[es(on_event=EventEnd)]
def verify_tracked_pods(evt : Event)
  let nanChanged = get_test_value("NanDp3Changed")
  let negZeroChanged = get_test_value("NegZeroDp3Changed")
  let bigPodChanged = get_test_value("BigPodChanged")
  print("tracked pod compare: nan={nanChanged} negzero={negZeroChanged} bigpod={bigPodChanged} (all must be 1)")
  verify(nanChanged == 1)
  verify(negZeroChanged == 1)
  verify(bigPodChanged == 1)
