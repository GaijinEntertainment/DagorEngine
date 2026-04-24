require ecs
require DasEcsUnitTest

[es(on_appear, REQUIRE=some_component2)]
def on_cpp_component_create(evt : Event)
  set_test_value("some_component2", 1)

[es(on_event=EventEnd)]
def on_cpp_component_end(evt : Event)
  verify(get_test_value("some_component2") == 1)
