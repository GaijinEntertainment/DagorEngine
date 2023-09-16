require ecs
require strings
require DasEcsUnitTest
require events


let
  GLOBAL_INT = 42
  GLOBAL_YES_BOOL = true
  GLOBAL_NO_BOOL = false

[es(on_event=EventStart, REQUIRE=eid)]
def default_global_args_es(evt : Event)
  var triggered = false
  query() <| $ [es, unused_argument(pos)] (pos : float3; global_int_int = int(GLOBAL_INT); global_int = GLOBAL_INT;
                                           global_yes = GLOBAL_YES_BOOL; global_no = GLOBAL_NO_BOOL; global_eid = INVALID_ENTITY_ID)
    triggered = true
    verify(global_int_int == 42)
    verify(global_int == 42)
    verify(global_yes)
    verify(!global_no)
    verify(uint(global_eid) == 0u)
  verify(triggered)

[es(on_event=EventStart, REQUIRE=eid)]
def default_args_es(evt : Event; no_int : int const?; no_string : das_string const?; def_int : int = 13; def_str : string = "11"; def_eid = INVALID_ENTITY_ID)
  verify(def_int == 13)
  verify(def_str == "11")
  verify(uint(def_eid) == 0u)
  if no_int != null
    set_test_value("no_int_val", 1)
  if no_string != null
    set_test_value("no_string_val", 1)
  set_test_value("def_int_val", def_int)
  set_test_value("def_string_val", int(def_str))

[es(on_event=EventEnd)]
def default_args_verifier_es(evt : Event)
  verify(get_test_value("no_int_val") == -1)
  verify(get_test_value("no_string_val") == -1)
  verify(get_test_value("def_int_val") == 13)
  verify(get_test_value("def_string_val") == 11)

[es(on_event=EventStart)]
def test_null_value(evt : Event; pos : float3; def_int : int = 13; var no_int_value : int?; some_eid = INVALID_ENTITY_ID)
  debug(some_eid)
  debug(pos)
  debug(no_int_value ?? -1)
  verify(def_int == 13)
  if no_int_value != null
    *no_int_value = def_int
