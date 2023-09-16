options no_aot
module verify_common shared

require ecs
require EcsUtils
require DagorSystem
require ECSGlobalTags
require daslib/ast_boost public
require daslib/strings_boost


def join_keys(tab)
  return build_string() <| $(str)
    var first = true
    for it in keys(tab)
      if first
        first = false
      else
        str |> write(", ")
      str |> write(it)

def validate_template(name : string; comps : table<string; uint>; cb : block<(success : bool; tmpl : ecs::Template const?) : void>) : void
  let tmplPtr = getTemplateByName(name)
  if tmplPtr == null
    cb |> invoke(false, tmplPtr)
    return
  assume tmpl = *tmplPtr
  let compTypes & = getComponentTypes()
  let dataComps & = getDataComponents()

  var res = true
  for compName, compType in keys(comps), values(comps)
    let compNameHash = ecs_hash(compName)
    if !templateHasComponent(tmpl, compName, compNameHash)
      logwarn("ecs_verify: comp '{compName}' wasn't found inside '{name}' template")
      res = false
      break
    let typeIdx = compTypes |> component_types_findType(compType)
    if typeIdx == INVALID_COMPONENT_TYPE_INDEX
      logerr("ecs_verify: comp '{compName}' hash<{compNameHash}> with unknown type hash<{compType}>")
      res = false
      break
    let templateComp = tmpl |> getTemplateComponent(compName, compNameHash)
    if templateComp == null || templateComp.isNull
      // data component!?
      let typName = compTypes |> component_types_getTypeNameById(typeIdx)
      let dataCompId = dataComps |> data_components_findComponentId(compNameHash)
      if dataCompId == INVALID_COMPONENT_INDEX
        logerr("ecs_verify: unknown data comp '{compName}' hash<{compNameHash}>. Expected type <{typName}|{typeIdx}>")
        res = false
        break
      let compData = dataComps |> data_components_getComponentById(dataCompId)
      if typeIdx != compData.componentType
        let actualTypName = compTypes |> component_types_getTypeNameById(compData.componentType)
        logerr("ecs_verify: wrong data comp type '{compName}' hash<{compNameHash}>. Expected type <{typName}|{typeIdx}>, actual type <{actualTypName}|{compData.componentType}>")
        res = false
        return

    elif templateComp.typeId != typeIdx
      let typName = compTypes |> component_types_getTypeNameById(typeIdx)
      let actualTypName = compTypes |> component_types_getTypeNameById(templateComp.typeId)
      logerr("ecs_verify: wrong comp type '{compName}' hash<{compType}>. Expected type <{typName}|{typeIdx}>, actual type <{actualTypName}|{templateComp.typeId}>")
      res = false
      break

  cb |> invoke(res, tmplPtr)


def find_template(comps : table<string; uint>; cb : block<(name : string#; tmpl : ecs::Template#) : bool>) : bool
  var res = false
  getTemplateDB() |> find_templateDB() <| $ [unused_argument(name)] (tmplName; tmpl)
    let compTypes & = getComponentTypes()
    let dataComps & = getDataComponents()

    for compName, compType in keys(comps), values(comps)
      let compNameHash = ecs_hash(compName)
      if !templateHasComponent(tmpl, compName, compNameHash)
        return false
      let typeIdx = compTypes |> component_types_findType(compType)
      if typeIdx == INVALID_COMPONENT_TYPE_INDEX
        logerr("ecs_verify: comp '{compName}' hash<{compNameHash}> with unknown type hash<{compType}>")
        return false
      let templateComp = tmpl |> getTemplateComponent(compName, compNameHash)
      if templateComp == null || templateComp.isNull
        // data component!?
        let typName = compTypes |> component_types_getTypeNameById(typeIdx)
        let dataCompId = dataComps |> data_components_findComponentId(compNameHash)
        if dataCompId == INVALID_COMPONENT_INDEX
          logerr("ecs_verify: unknown data comp '{compName}' hash<{compNameHash}>. Expected type <{typName}|{typeIdx}>")
          return false
        let compData = dataComps |> data_components_getComponentById(dataCompId)
        if typeIdx != compData.componentType
          let actualTypName = compTypes |> component_types_getTypeNameById(compData.componentType)
          logerr("ecs_verify: wrong data comp type '{compName}' hash<{compNameHash}>. Expected type <{typName}|{typeIdx}>, actual type <{actualTypName}|{compData.componentType}>")
          return false

      elif templateComp.typeId != typeIdx
        let typName = compTypes |> component_types_getTypeNameById(typeIdx)
        let actualTypName = compTypes |> component_types_getTypeNameById(templateComp.typeId)
        logerr("ecs_verify: wrong comp type '{compName}' hash<{compNameHash}>. Expected type <{typName}|{typeIdx}>, actual type <{actualTypName}|{templateComp.typeId}>")
        return false

    if cb |> invoke(tmplName, tmpl)
      res = true
    return res
  return res


def req_template(name : string; comps : table<string; uint>) : bool
  var res = false
  validate_template(name, comps) <| $(success; tmpl)
    if success
      res = true
      print("ecs_verify: required template '{name}' contains all components \"{join_keys(comps)}\"")
    elif tmpl != null
      logerr("ecs_verify: required template '{name}' was found, but doesn't contain all components \"{join_keys(comps)}\"")
    else
      logerr("ecs_verify: unable to find required template '{name}' with components \"{join_keys(comps)}\"")
  return res


def req_template(comps : table<string; uint>) : bool
  let res = find_template(comps) <| $ [unused_argument(tmpl)] (tmplName, tmpl)
    print("ecs_verify: template '{tmplName}' contains all required components \"{join_keys(comps)}\"")
    return true
  if !res
    logerr("ecs_verify: unable to find template with required components \"{join_keys(comps)}\"")
  return res


def dump_system(name : string#; system : EntitySystemDesc#)
  let events = build_string() <| $(str)
    system |> getEvSet() <| $(evt)
      str |> write(evt)
      str |> write(" ")
  print("system: {name} empty: {system.isEmpty} dynamic: {system.isDynamic} tags: {system.tags} before: {system.before} after: {system.after} compSet: {system.compSet} module: {system.moduleName}\nevents: {events}")


struct ReqSystemFilter
  allEvents : table<uint>
  anyEvents : table<uint>


def find_event_handler(filter : ReqSystemFilter; cb : block<(name : string#; system : EntitySystemDesc#) : bool>)
  var res = false
  find_systemDB() <| $ [unused_argument(system)] (name; system)

    var match = true
    if length(filter.allEvents) > 0
      system |> getEvSet() <| $(systemEvents)
        if length(filter.allEvents) != length(systemEvents)
          match = false
          return
        for evt in systemEvents
          if !key_exists(filter.allEvents, evt)
            match = false
            break
      if !match
        return false

    if length(filter.anyEvents) > 0
      match = false
      system |> getEvSet() <| $(systemEvents)
        for evt in systemEvents
          if key_exists(filter.anyEvents, evt)
            match = true
            break
      if !match
        return false

    if cb |> invoke(name, system)
      res = true
      return true

    return false

  return res


def req_unique_event_handler(event_name : string) : bool
  var filter : ReqSystemFilter
  filter.allEvents |> insert(ecs_hash(event_name))
  let res = find_event_handler(filter) <| $(sys_name, system)
    print("ecs_verify: system '{sys_name}' listen event '{event_name}'")
    dump_system(sys_name, system)
    return true
  if !res
    logerr("ecs_verify: unable to find unique event handler '{event_name}'")
  return res


def req_event_handler(event_name : string) : bool
  var filter : ReqSystemFilter
  filter.anyEvents |> insert(ecs_hash(event_name))
  let res = find_event_handler(filter) <| $(sys_name, system)
    print("ecs_verify: system '{sys_name}' listen event '{event_name}'")
    dump_system(sys_name, system)
    return true
  if !res
    logerr("ecs_verify: unable to find event handler '{event_name}'")
  return res
