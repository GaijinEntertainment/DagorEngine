//set of functions to make easier work with ecs. Better to move it all to native code

// register es with easier api - name, table of events like
// {  onUpdate = onUpdateFunc
//    [ecs.EventEntityCreated] = @(evt, eid, comp) {}
//    [ecs.EventComponentChanged] = @(evt, eid, comp) {}
// }
let { logerr } = require("dagor.debug")
let { kwarg } = require("%sqstd/functools.nut")
//let { flatten } = require("%sqstd/underscore.nut")
let { DBGLEVEL } = require("dagor.system")
let ecs = require("ecs")


let unicastSqEvents = {}
//local broadcastNativeEvents = ["onUpdate", ecs.EventComponentChanged, ecs.EventComponentsAppear, ecs.EventComponentsDisappear, ecs.EventEntityCreated, ecs.EventEntityDestroyed]

let sqEvents = {}
const VERBOSE_PRINT = false //getroottable()?.__is_stub__
let verbose_print = VERBOSE_PRINT ? @(val) print(val) : @(_) null

function mkEsFuncNamed(esname, func) {
  assert(["function", "instance", "table"].indexof(type(func)) != null, $"esHandler can be only function or callable, for ES '{esname}', got type: {type(func)}")
  let infos = func?.getfuncinfos?()
  assert(infos!=null, "esHandler can be only function or callable, ES:{0}".subst(esname))
  let len = infos.parameters.len()
  assert (len==1 || (len < 5 && len > 2),
    $"ES function should have no arguemnts, or at least 2 argument - eid, comp, or 3 - evt, eid, comp. function name:{infos?.name}, argnum:{" ".join(infos?.parameters ?? [])}, arguments:{len}, es name:{esname}")
  if (len==1)
    return function(_evt, _eid, _comp) {func()}
  if (len==4)
    return func
  else
    return function(_evt, eid, comp) {func(eid, comp)}
}

function gatherComponentNames(component_list){
  let res = []
  foreach (component in component_list){
    if (type(component) =="string")
      res.append(component)
    else
      res.append(component[0])
  }
  return res
}

const INTERNAL_REGISTER_ECS = "___register_entity_system_internal___"
if (INTERNAL_REGISTER_ECS not in ecs) {
  ecs[INTERNAL_REGISTER_ECS] <- ecs.register_entity_system
  ecs.register_entity_system = @(_name, _events, _comps, _params) assert(false, "register_entity_system considered unsafe. use ecs.register_es instead")
}

let ecs_register_entity_system = ecs[INTERNAL_REGISTER_ECS]

function register_es(name, onEvents={}, compsDesc={}, params = {}) {
  const DOTS_ERROR = "dots in ES components"
  try{
    foreach (k, _v in compsDesc)
      assert(["comps_ro","comps_rw","comps_rq","comps_no","comps_track"].indexof(k) != null, $"incorrect comps description, incorrect key: {k}, es name:{name}")
    local comps = compsDesc

    let remappedEvents = {}
    let mkEsFunc = @(func) mkEsFuncNamed(name, func)
    let remap = {
      onInit = [ecs.EventEntityCreated, ecs.EventScriptReloaded, ecs.EventComponentsAppear],
      onChange = [ecs.EventComponentChanged],
      onDestroy = [ecs.EventEntityDestroyed, ecs.EventComponentsDisappear]
    }
//    let keys = flatten(onEvents.keys()).reduce(function(res, k) {res[k] <- k; return res;}, {})
//    if (("onInit" in keys || "onChange" in keys) && "onDestroy" not in keys)
//      println($"ES requires destroy: {name}")
    foreach (k, func in onEvents) {
      if (k in remap) {
        foreach (j in remap[k])
          remappedEvents[j] <- mkEsFunc(func)
      }
      else if (type(k) == "array") {
        foreach (e in k) {
          if (e in remap) {
            foreach (j in remap[e])
              remappedEvents[j] <- mkEsFunc(func)
          }
          else
            remappedEvents[e] <- mkEsFunc(func)
        }
      }
      else
        remappedEvents[k] <- mkEsFunc(func)
    }
    assert(remappedEvents.len()>0, $"can't register ES '{name}' without any events")
    assert(!("OnUpdate" in remappedEvents), $"ES: {name}, OnUpdate is incorrect eventListener, should be onUpdate")
    foreach (k, _v in remappedEvents) {
      assert(k in sqEvents || ["Timer","onUpdate"].indexof(k) != null || (type(k) == "class") || (type(k) == "integer"), $"unknown event {k}. Script events should be registered via ecs.register_event()")
    }
    let isChangedTracked = ecs.EventComponentChanged in remappedEvents
    let comps_track = comps?.comps_track ?? []
    assert((!isChangedTracked && comps_track.len()==0) || (isChangedTracked && comps_track.len()>0) || ((params?.track?.len() ?? 0) >0), "ecs registered for EventComponentChanged should have comps_track in components or CSV 'track' in params!")
    let comps_ro = [].extend(comps?.comps_ro ?? []).extend(comps_track)
    comps = clone comps
    if (comps_ro.len() > 0)
      comps.comps_ro <- comps_ro
    assert(!(comps_track.len()>0 && params?.track!=null), "es cannot be registered if both 'comp_tracks' in components and 'track' in params")
    if (comps_track.len()>0) {
      comps.$rawdelete("comps_track")
      params = (clone params).__merge({track=",".join(gatherComponentNames(comps_track))})
    }
    if ((comps?.comps_ro ?? []).filter(@(v) type(v)!="array").len()!=0)
     logerr($"{name} register error: all read only components should be specified with type")
    if ((comps?.comps_rw ?? []).filter(@(v) type(v)!="array").len()!=0)
      logerr($"{name} register error: all read only components should be specified with type")
    verbose_print($"registering {name};")
    ecs_register_entity_system(name, remappedEvents, comps, params)
    verbose_print($"ecs: {name} registered;")
    local comps_len = 0
    comps_len += comps?.comps_ro?.len != null ? comps.comps_ro.len() : 0
    comps_len += comps?.comps_rw?.len != null ? comps.comps_rw.len() : 0
    comps_len += comps?.comps_rq?.len != null ? comps.comps_rq.len() : 0
    if (comps_len == 0) {
      let unicastEvents = remappedEvents.filter(@(_v, k) k in unicastSqEvents ) //do not enumerate native events
      assert(unicastEvents.len() == 0, $"es {name} registered for unicast events without any components!")
      verbose_print($"ecs: '{name}' is registered for performing queries, as it has zero required components; ")
    }
    if ("onUpdate" in remappedEvents) {
      assert("before" in params || "after" in params, @() $"{name} need syncpoints ('before' and/or 'after' in params)")
    }
    let comps_ro_optional_num = comps_ro.filter(@(v) type(v)=="array" && v.len() > 2).len()
    let comps_rw = comps?.comps_rw ?? []
    assert(
      comps_len == 0 || !(comps_ro_optional_num == comps_ro.len() && (comps_rw.len() + (comps?.comps_rq.len() ?? 0))==0),
      @() $"es {name} registered with all optional components"
    )
    assert(
      comps_rw.filter(@(v) type(v) != "array" || v.len()==1).len()==0
      $"es {name} registered with rw components without type specified. should be [<name>:string, <type>:type]"
    )
    assert(
      comps_rw.filter(@(v) v.len()>2).len()==0
      $"es {name} registered with rw components with specified default value"
    )
    assert(
      comps_rw.indexof("eid")==null, "eid can't be writable component"
    )
    assert(
      comps?.comps_no.indexof("eid")==null, "eid can't be set as 'no' component"
    )
    if (params?.track.contains(".") ) {
      println($"dots in es '{name}': {params?.track}")
      throw(DOTS_ERROR)
    }
    foreach (_, compslist in comps){
      foreach (comp in compslist){
        local compName = comp
        if (type(compName) == "array")
          compName = comp[0]
        if (compName?.contains(".")) {
          println($"dots in component {comp?[0]} es {name}")
          throw(DOTS_ERROR)
        }
      }
    }
  }
  catch (e){
    logerr(": ".concat($"error in '{name}'", e))
  }
}

function makeTemplate(params={}){
  let addTemplates = params?.addTemplates ?? []
  let removeTemplates = [].extend(params?.removeTemplates ?? [], addTemplates)
  let baseTemplates = params?.baseTemplate.split("+") ?? []
  return "+".join(
            baseTemplates
            .filter(@(v) !removeTemplates.contains(v))
            .extend(addTemplates)
          )
}

let recreateEntityWithTemplates = kwarg(function(eid=ecs.INVALID_ENTITY_ID, removeTemplates=null, addTemplates=null, comps={}, callback=null, checkComps = DBGLEVEL > 0){
  let resRemoveTemplates = []
  if (removeTemplates != null && removeTemplates.len() > 0){
    foreach (templN in removeTemplates){
      let templName = type(templN)=="string" ? templN : templN?.template
      if (checkComps) {
        let templComps = type(templN)=="string" ? [templN] : templN?.comps
        if (templComps == null || templName == null) {
          logerr($"addTemplates should have specified components that should be added. {templName}") //todo: check that components exists in removed templates
        }
        else {
          let templ = templName != null ? ecs.g_entity_mgr.getTemplateDB().getTemplateByName(templName) : null
          if (templ == null || (templComps?.len() ?? 0 )==0)
            continue
          foreach (compName in templComps) {
            if (!templ?.hasComponent(compName)) {
              logerr($"template '{templName}' has no '{compName}'!")
              break
            }
          }
        }
      }
      resRemoveTemplates.append(templName)
    }
  }

  let resAddTemplates = []
  if (addTemplates != null && addTemplates.len() > 0) {
    foreach (templN in addTemplates){
      let templName = type(templN)=="string" ? templN : templN?.template
      if (checkComps) {
        let templComps = type(templN)=="string" ? [templN] : templN?.comps
        if (templComps == null || templName == null) {
          logerr($"addTemplates should have specified components that should be added. {templName}") //todo: check that components exists in removed templates
        }
        else {
          let templ = templName != null ? ecs.g_entity_mgr.getTemplateDB().getTemplateByName(templName) : null
          if (templ == null) {
            logerr($"no template '{templName}' found on recreateTemplates!")
            continue
          }
          foreach(compName in templComps){
            if (!templ.hasComponent(compName))
              logerr($"template '{templName}' has no '{compName}'!")
          }
        }
      }
      resAddTemplates.append(templName)
    }
  }
  resRemoveTemplates.extend(resAddTemplates)
  if (eid == ecs.INVALID_ENTITY_ID || !ecs.g_entity_mgr.doesEntityExist(eid))
    return
  let curTemplate = ecs.g_entity_mgr.getEntityFutureTemplateName(eid)
  // curTemplate is null when destroyEntity() is called right before recreateEntityWithTemplates
  // In such case doesEntityExist() still true
  if (curTemplate == null)
    return
  let newTemplatesName = makeTemplate({baseTemplate=curTemplate, addTemplates=resAddTemplates, removeTemplates=resRemoveTemplates})
  if (newTemplatesName != curTemplate)
    ecs.g_entity_mgr.reCreateEntityFrom(eid, newTemplatesName, comps, callback)
})

function query_map(query, func, filter_str = null){
  assert(query instanceof ecs.SqQuery, "need SqQuery instance as first argument")
  assert(filter_str == null  || type(filter_str) == "string", "filter string should be string or null")
  let res = []
  if (filter_str != null)
    query.perform(function(eid, comp) {res.append(func(eid, comp))}, filter_str)
  else
    query.perform(function(eid, comp) {res.append(func(eid, comp))})
  return res
}

function list2array(list){
  let res = []
  foreach (v in list){
    res.append(v)
  }
  return res
}

function set_array2list(array_, list){
  list.clear()
  foreach (v in array_)
    list.append(v)
  return list
}

function register_event(name, eventType, structure=null){

  assert(ecs.EVCAST_UNICAST == eventType || ecs.EVCAST_BROADCAST == eventType,
            "eventType should be ecs.EVCAST_UNICAST or ecs.EVCAST_BROADCAST")
  verbose_print($"registering sq event {name}; ")
  assert (type(name) =="string", "event name should be string")
  assert(!(name in sqEvents), @() $"event: '{name}' already registered!")
  sqEvents[name] <- name
  let eventRegisteredName = ecs.register_sq_event(name, eventType)
  function mkEvent(payload=null){
//  todo - add type checking
    if (structure == null) {
      assert (payload == null)
      return ecs.SQEvent(eventRegisteredName)
    }
    assert (payload != null)
    return ecs.SQEvent(eventRegisteredName, payload)
  }
  let ctorName = $"mk{name}"
  sqEvents[ctorName] <- mkEvent
  return {[ctorName] = mkEvent, [name]=name}
}


let mkRegisterEventByType = @(eventType) function(payload, eventName){
  if (payload == null)
    return register_event(eventName, eventType)
  return register_event(eventName, eventType, payload)
}
let _registerUnicastEvent = mkRegisterEventByType(ecs.EVCAST_UNICAST)
unicastSqEvents.clear()
function registerUnicastEvent(payload, eventName){
  unicastSqEvents[eventName] <- payload
  return _registerUnicastEvent(payload, eventName)
}
let registerBroadcastEvent = mkRegisterEventByType(ecs.EVCAST_BROADCAST)
//this is done here only to have all events in all VMs
//broadcastSqEvents.__update(events.broadcastEvents)//for type check in register es

return ecs.__merge({
  //this APIs needed cause we have different VMs and need to have all events be accessible
  registerUnicastEvent
  registerBroadcastEvent
  register_event

  //this is needed for pure native APIs replacement
  register_es
  recreateEntityWithTemplates
  makeTemplate

  //just some godies. Query map is useful when you need to map entities components to array
  query_map

  map_list2array = @(list,func) list2array(list).map(func)
  array2list = set_array2list
  list2array
})
