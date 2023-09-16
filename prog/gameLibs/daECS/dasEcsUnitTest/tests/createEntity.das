options no_global_variables = false
require ecs
require DasEcsUnitTest

[es(on_event=EventStart)]
def simple_create_es(evt : Event)
  let eid = createEntity("a")//everything from template
  let eid2 = createEntitySync("a")//everything from template
  set_test_value("create_{eid}", 10)
  set_test_value("create_{eid2}", 10)


[es(on_event=EventStart)]
def initializer_create_es(evt : Event)
  let eid = createEntity("a") <| $(var init : ComponentsInitializer)
    set(init, "int_variable", 1)//int_variable from initializer
  let eid2 = createEntitySync("a") <| $(var init : ComponentsInitializer)
    set(init, "int_variable", 2)//int_variable from initializer
  set_test_value("create_{eid}", 1)
  set_test_value("create_{eid2}", 2)

var
  finished : bool = false
var
  created : bool = false

[es(on_event=EventStart)]
def callback_create_es(evt : Event)
  let callback <- @ <| (id : EntityId)
    assert(finished)
  let eid = createEntity("a", callback) <| $(var init : ComponentsInitializer)
    created = true
    set(init, "int_variable", 11)
  set_test_value("create_{eid}", 11)
  finished = true

[es]
def creation_int_variable_verifier(info : UpdateStageInfoAct; eid : EntityId; int_variable : int)
  let val = get_test_value("create_{eid}")
  assert(val == int_variable)


[es(on_event=EventEnd)]
def callback_creation_verifier(evt : Event)
  assert(created)

//count creation and destroyed amount
var
  total_created, total_destroyed : int = 0

[es(on_appear, REQUIRE=int_variable)]
def creation_verifier(evt : Event)
  total_created++;

[es(on_disappear)]
def destroyer_verifier(evt : Event; eid : EntityId; int_variable : int)
  verify(get_test_value("create_{eid}") == int_variable)
  total_destroyed++;

[es(on_event=EventEnd)]
def count_creation_verifier(evt : Event)
  assert(created)
  assert(total_created == 5)

