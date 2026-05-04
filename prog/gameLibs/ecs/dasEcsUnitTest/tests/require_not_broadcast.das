require ecs
require DasEcsUnitTest

[es(on_event=EventStart, REQUIRE=(some_missing_comp))]
def unresolved_require_not(evt : Event; missing_comp : int const?)
  if missing_comp != null
    set_test_value("ShouldNotHappen", 1)

[es(REQUIRE=no_int_value)]
def unresolved_require_not_stage(info : UpdateStageInfoAct; missing_comp : int const?)
  if missing_comp != null
    set_test_value("ShouldNotHappen", 1)

[es(on_appear, REQUIRE=eid)]
def count_all(evt : Event)
  let a = get_test_value("CountCreationOfAll")
  set_test_value("CountCreationOfAll", (a >= 0 ? a : 0) + 1)

[es(on_appear, REQUIRE=(no_int_value))]
def count_no_int(evt : Event)
  let a = get_test_value("CountCreationOfno_int_value")
  set_test_value("CountCreationOfno_int_value", (a >= 0 ? a : 0) + 1)


[es(on_event=EventEnd, REQUIRE_NOT=(no_int_value))]
def resolved_require_not(evt : Event)
  set_test_value("RequireNot_no_int_value", 1)

[es(on_event=EventEnd)]
def verify_require_not(evt : Event)
  verify(get_test_value("ShouldNotHappen") != 1)
  verify(get_test_value("CountCreationOfno_int_value") > 0)
  verify(get_test_value("RequireNot_no_int_value") > 0)
