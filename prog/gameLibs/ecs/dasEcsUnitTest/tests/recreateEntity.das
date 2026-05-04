require ecs
require DasEcsUnitTest

[es(on_event=EventStart)]
def recreate_within_query_es(evt : Event)
  createEntitySync("testReCreate1")
  ecs::query() <| $ [es] (eid : EntityId; var some_int_var1 : int&; var ecs_array_int_1 : Array)
    some_int_var1++
    set(ecs_array_int_1[0], 2)
    reCreateEntityFrom(eid, "testReCreateMemLeak") <| $(var init : ComponentsInitializer)
      set(init, "string_variable", "1111111111111111111111111111111111111111111111111")
      set(init, "string_variable", "1111111111111111111111111111111111111111111111111")

[es(track=some_int_var1, REQUIRE=some_int_var1)]
def recreate_within_query_track1(evt : Event)
  set_test_value("some_int_var1_tracked", 1)

[es(track=ecs_array_int_1, REQUIRE=ecs_array_int_1)]
def recreate_within_query_track2(evt : Event)
  set_test_value("ecs_array_int_1", 1)

[es(on_event=EventStart)]
def recreate_with_initializer_es(evt : Event)
  let eid = createEntitySync("testReCreateInit1")
  reCreateEntityFrom(eid, "testReCreateInit2") <| $(var init : ComponentsInitializer)
    set(init, "some_int_var2", 1)//int_variable from initializer

[es(track=some_int_var2)]
def recreate_with_initializer_track(evt : Event; some_int_var2 : int)
  set_test_value("some_int_var2", some_int_var2)


[es(on_event=EventEnd)]
def recreate_within_query_track_verifier(evt : Event)
  verify(get_test_value("some_int_var1_tracked") == 1)
  verify(get_test_value("ecs_array_int_1") == 1)
  verify(get_test_value("some_int_var2") == 1)

