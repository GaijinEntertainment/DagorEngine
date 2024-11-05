options no_aot
require ecs
require DagorConsole
require vehicle
require DagorSystem
require danetlibs.console_commands.main.das_debug_common
require ecs.ecs_template

[console_cmd(name="das.common_code")]
def das_common_code()
  console_print("before common code")
  print_hero()
  console_print("after common code")


[console_cmd(name="das.logerr")]
def das_logerr(msg = "Hello from dascript")
  logerr(msg)


[console_cmd(name="das.stackwalk")]
def das_stackwalk()
  print("before stackwalk")
  stackwalk()
  print("after stackwalk")


[console_cmd(name="das.throw_exception")]
def das_throw_exception()
  query() <| $ [es(REQUIRE=msg_sink)] (var unavailable_comp__ : ecs::EidList?)
    print("throw exception now")
    *unavailable_comp__ |> clear()


[console_cmd(name="das.throw_exception_no_aot"), no_aot]
def das_throw_exception_no_aot()
  query() <| $ [es(REQUIRE=msg_sink)] (var unavailable_comp__ : ecs::EidList?)
    print("throw exception now")
    *unavailable_comp__ |> clear()


[ecs_template]
struct endless_exception_template
  endless_exception = true

[console_cmd(name="das.throw_endless_exception")]
def das_start_endless_exception()
  let tmpl = "endless_exception_template"
  let found = find_query() <| $ [es(REQUIRE=endless_exception)] (eid : EntityId)
    destroyEntity(eid)
    console_print("stop endless exceptions")
    return true
  if !found
    createEntitySync(tmpl)
    console_print("start endless exceptions")


[es(no_order)]
def endless_exception_update(evt : UpdateStageInfoAct; var unavailable_comp__ : ecs::EidList?; endless_exception : bool)
  if (endless_exception)
    print("throw exception now")
    *unavailable_comp__ |> clear()
