options no_global_variables = false
require ecs
require DasEcsUnitTest
require DagorMath
require %daslibEcs/ecs_template

//[ecs_template (extends=(a, b))]
[ecs_template]
struct some_template
  [[undef]] some_component3 : SomeComponent
  [[undef]] some_component2
  [[t]] some_template_int : int = 1
  [[t, r, tag=(render, net, a)]] some_template_float : float = 2.
  [[t]] some_template_eid : EntityId = INVALID_ENTITY_ID
  [[t]] some_template_str : string = "1212"
  [[t]] some_template_f2 : float2 = float2(1., 2.)
  [[undef]] some_tag : ecs::Tag
  some_template_col : E3DCOLOR = E3DCOLOR(uint4(25u))
  //some_template_i2:int2 = int2(1,1)

[es(on_event=EventStart)]
def template_create_es(evt : Event)
  destroyEntity(createEntity("some_template"))

//count creation and destroyed amount
var
  total_created, total_destroyed : int = 0

[es(on_appear, REQUIRE=some_template_int)]
def template_creation_verifier(evt : Event; eid : EntityId; some_template_int : int)
  set_test_value("create_some_template_int_{eid}", some_template_int)
  total_created++

[es(on_disappear)]
def template_destroyer_verifier(evt : Event; eid : EntityId; some_template_int : int)
  verify(get_test_value("create_some_template_int_{eid}") == some_template_int)
  verify(get_test_value("create_some_template_int_{eid}") == 1)
  total_destroyed++

[es(on_event=EventEnd)]
def template_count_creation_verifier(evt : Event)
  verify(total_created == 1)
  verify(total_destroyed == 1)