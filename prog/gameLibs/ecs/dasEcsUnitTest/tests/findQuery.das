require ecs

// ES with find_query example
[es(on_event=EventStart)]
def find_query_es(evt : Event)
  var eid1 : EntityId
  for i in range(0, 4)
    let eid = createEntitySync("justInt") <| $(var init : ComponentsInitializer)
      set(init, "just_int_variable", i)//int_variable from initializer
    if i == 1
      eid1 = eid
  var lookFor1 : EntityId
  var iterations = 0
  ecs::find_query() <| $ [es] (eid : EntityId; just_int_variable : int)
    iterations++
    if just_int_variable == 1
      lookFor1 = eid
      return true//fixme should be QueryCbResult Stop, currently not aoting
    return false//fixme should be QueryCbResult Continue, currently not aoting
  assert(lookFor1 == eid1)
  assert(iterations == 2)
