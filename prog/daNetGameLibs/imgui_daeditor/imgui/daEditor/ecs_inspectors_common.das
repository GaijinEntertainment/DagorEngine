options no_global_variables = false
options persistent_heap
module ecs_inspectors_common shared

require ecs
require rtti
require debugapi
require strings
require daslib/apply_in_context

class ContextInspectorFn
  ctx : Context?
  fn : InspectorFn

  def ContextInspectorFn(var f : InspectorFn; var context : Context)
    unsafe
      ctx = addr(context)
    fn <- f

  def call(eid : array < EntityId > const implicit; flags : uint; name : string; refs : array < ComponentRenderInfo# >#; level : int; var changed : bool?; sub_menu : block < (var changed : bool?) : bool >) : void
    unsafe
      lock_debug_agent() <| $()
        ctx |> invoke_in_context(fn, eid, flags, name, refs, level, changed, sub_menu)

class ContextEditorFn
  ctx : Context?
  fn : EditorFn

  def ContextEditorFn(var f : EditorFn; var context : Context)
    unsafe
      ctx = addr(context)
    fn <- f

  def call(eid : array < EntityId > const#) : void
    unsafe
      lock_debug_agent() <| $()
        ctx |> invoke_in_context(fn, eid)


class ContextEntityNameDescFn
  ctx : Context?
  fn : EntityNameDescFn

  def ContextEntityNameDescFn(var f : EntityNameDescFn; var context : Context)
    unsafe
      ctx = addr(context)
    fn <- f

  def call(eid : EntityId) : string
    var out = ""
    unsafe
      lock_debug_agent() <| $()
        ctx |> invoke_in_context(fn, eid, addr(out))
        out = clone_string(out)
    return out


class ContextUnselectedInspectorFn
  ctx : Context?
  fn : UnselectedInspectorFn

  def ContextUnselectedInspectorFn(var f : UnselectedInspectorFn; var context : Context)
    unsafe
      ctx = addr(context)
    fn <- f

  def call() : void
    unsafe
      lock_debug_agent() <| $()
        ctx |> invoke_in_context(fn)


struct InspectorDesc
  field_name : string
  tag : uint
  fn : ContextInspectorFn?


struct EditorDesc
  prefix : bool
  tags : array<uint>
  fn : ContextEditorFn?

struct ComponentRenderInfo
  rawData : void? // Pointer to raw data to handle randering with custom renderes
  typeName : string
  // When no ecs type can be inferred contains das generated read only display string
  // If not empty idx and componentId are invalid.
  roVal : string
  // In case described above stores das type string to display as additional info in inspector, can be useful.
  // typeName will contain a placeholder value so inspectors know to display it a specific way
  dasTypeName : string
  idx : uint
  componentId : uint16
  // Are idx and componentId are of a valid component
  isValidComponent : bool

typedef
  InspectorFn = function<(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void>
  EditorFn = function<(eid : array<EntityId>#) : void>
  EntityNameDescFn = function<(eid : EntityId; var out_string : string?) : void>
  UnselectedInspectorFn = function<() : void>


let AGENT_NAME = "ecs_inspectors"
// TODO: rewrite using invoke_debug_agent_function
var private @ignore_heap_usage inspectors : table<string; array<InspectorDesc>>
var private @ignore_heap_usage editors : array<EditorDesc>
var private @ignore_heap_usage unselectedInspectors : array<ContextUnselectedInspectorFn?>
var private @ignore_heap_usage nameDescriptor : ContextEntityNameDescFn?


def not_in_ecs_inspectors_context()
  unsafe
    if has_debug_agent_context(AGENT_NAME) && addr(get_debug_agent_context(AGENT_NAME))  != addr(this_context())
      return true
  return false


class EcsInspectorAgent : DapiDebugAgent
  def override onDestroyContext(ctx : Context)
    var toRemove : array<string>
    for typeName, list in keys(inspectors), values(inspectors)
      var idx = length(list) - 1
      while idx >= 0
        unsafe
          let linkedCtx & = *(list[idx].fn.ctx)
          if addr(linkedCtx) == addr(ctx)
            list |> erase(idx)
        idx -= 1
      if length(list) == 0
        toRemove |> push(typeName)
    for typeName in toRemove
      inspectors |> erase(typeName)

    let eLen = length(editors)
    for ridx in iter_range(editors)
      let idx = eLen - ridx - 1
      unsafe
        let linkedCtx & = *(editors[idx].fn.ctx)
        if addr(linkedCtx) == addr(ctx)
          editors |> erase(idx)

    let uiLen = length(unselectedInspectors)
    for ridx in iter_range(unselectedInspectors)
      let idx = uiLen - ridx - 1
      unsafe
        let linkedCtx & = *(unselectedInspectors[idx].ctx)
        if addr(linkedCtx) == addr(ctx)
          unselectedInspectors |> erase(idx)

    unsafe
      if nameDescriptor != null
        let linkedCtx & = *(nameDescriptor.ctx)
        if addr(linkedCtx) == addr(ctx)
          delete nameDescriptor
          nameDescriptor = null


[export, unused_argument(ctx)]
def private start_debug_agent(ctx : Context)
  install_new_debug_agent(new EcsInspectorAgent(), AGENT_NAME)


[export]
def start_ecs_inspectors_agent()
  unsafe
    lock_debug_agent() <| $()
      if !has_debug_agent_context(AGENT_NAME) && !is_in_aot() && !is_in_debug_agent_creation()
        fork_debug_agent_context(@@start_debug_agent)


[apply_in_context(ecs_inspectors)]
def add_inspector(typ_name, field_name, tag : string implicit; var fn : InspectorFn; var ctx : Context implicit) : bool
  let type_name = clone_string(typ_name)
  let tag_hash = !empty(tag) ? ecs_hash(tag) : 0u
  if !key_exists(inspectors, type_name)
    inspectors[type_name] <- array<InspectorDesc>()
  inspectors |> get(type_name) <| $(list)
    for it in list
      if it.field_name == field_name && it.tag == tag_hash
        delete * it.fn
        it.fn = new ContextInspectorFn(fn, ctx)
        return

    list |> emplace <| InspectorDesc(field_name = clone_string(field_name), tag = tag_hash, fn = new ContextInspectorFn(fn, ctx))
  return true


[apply_in_context(ecs_inspectors)]
def has_inspector(type_name : string implicit) : bool
  return inspectors |> key_exists(type_name)


[apply_in_context(ecs_inspectors)]
def call_inspector(type_name : string implicit; eid : array<EntityId> implicit; flags : uint; field_name : string implicit; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool? implicit; sub_menu : block<(var changed : bool?) : bool> implicit) : bool
  var res = false
  inspectors |> get(type_name) <| $(list)
    res = true
    if length(list) == 1
      (list)[0].fn->call(eid, flags, field_name, refs, level, changed, sub_menu)
      return

    // field_name and tag
    for it in list
      if it.field_name == field_name && it.tag != 0u && has(eid[0], "", it.tag)
        it.fn->call(eid, flags, field_name, refs, level, changed, sub_menu)
        return

    // tag
    for it in list
      if it.tag != 0u && has(eid[0], "", it.tag)
        it.fn->call(eid, flags, field_name, refs, level, changed, sub_menu)
        return

    // field_name
    for it in list
      if it.field_name == field_name
        it.fn->call(eid, flags, field_name, refs, level, changed, sub_menu)
        return

    // default renderer
    for it in list
      if empty(it.field_name) && it.tag == 0u
        it.fn->call(eid, flags, field_name, refs, level, changed, sub_menu)
        return

    // no valid renderers
    res = false
  return res


[apply_in_context(ecs_inspectors)]
def add_editor(prefix : bool; tags : array<string> implicit; var fn : EditorFn; var ctx : Context implicit) : bool
  var tagHashes <- [ for tag in tags; !empty(tag) ? ecs_hash(tag) : 0u ]
  editors |> emplace <| EditorDesc(prefix = prefix, tags <- tagHashes, fn = new ContextEditorFn(fn, ctx))
  return true


[apply_in_context(ecs_inspectors)]
def call_editor(prefix : bool; eid : array<EntityId>#) : bool
  var res = false
  for it in editors
    if it.prefix != prefix
      continue
    var valid = true
    for tag in it.tags
      if !eid[0] |> has("", tag)
        valid = false
        break
    if valid
      (*it.fn)->call(eid)
      res = true

  return res

[apply_in_context(ecs_inspectors)]
def set_entity_name_getter(var fn : EntityNameDescFn; var ctx : Context implicit) : void
  nameDescriptor = new ContextEntityNameDescFn(fn, ctx)

[apply_in_context(ecs_inspectors)]
def call_entity_extra_name_getter(eid : EntityId) : string
  var extra_name = ""
  if nameDescriptor != null
    extra_name := nameDescriptor->call(eid)
  return extra_name


[apply_in_context(ecs_inspectors)]
def add_unselected_inspector(var fn : UnselectedInspectorFn; var ctx : Context implicit) : bool
  unselectedInspectors |> push <| new ContextUnselectedInspectorFn(fn, ctx)
  return true


[apply_in_context(ecs_inspectors)]
def call_unselected_inspector() : bool
  for it in unselectedInspectors
    (*it)->call()
  return length(unselectedInspectors) > 0