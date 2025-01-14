options stack = 102400

require remote.jsonrpc_boost
require daslib/json_boost

require DagorSystem
require ecs
require BehNodes
require DngWalkerai
require walkerai
require ecs.ecs_template


[ecs_template]
struct track_beh_tree
  beh_tree__track_mind = true

def track_beh_tree(eid : EntityId; track_mind : bool)
  let tmpl = "track_beh_tree"
  var found = false
  query(eid) <| $ [es] (eid : EntityId; var beh_tree__track_mind : bool&)
    beh_tree__track_mind = track_mind
    if !track_mind
      removeSubTemplate(eid, tmpl)
    found = true

  if !found && track_mind
    reCreateEntityFrom(eid, "{getEntityFutureTemplateName(eid)}+{tmpl}")


[rpc_call(name="ai_log.track_mind")]
def track_mind(eid : uint; track_mind : bool; id : uint64)
  track_beh_tree(EntityId(eid), track_mind)
  send_response(JV(eid, EntityId(eid) |> get_bool("beh_tree__debug") ?? false, track_mind), id)


[rpc_call(name="ai_log.beh_tree_debug")]
def toggle_beh_tree_debug(eid : uint; id : uint64)
  query(EntityId(eid)) <| $ [es] (beh_tree__track_mind = false; var beh_tree__debug : bool&)
    beh_tree__debug = !beh_tree__debug
    send_response(JV(eid, beh_tree__debug, beh_tree__track_mind), id)


[rpc_call(name="ai_log.get_bots")]
def get_bots_list(id : uint64)
  var res : array<JsonValue?>
  query() <| $ [es] (eid : EntityId;
                     beh_tree__enabled : bool;
                     beh_tree__debug = false;
                     beh_tree__track_mind = false;
                     daeditor__selected : Tag const?;
                     beh_tree__debugInWebUI : Tag const?)
    if beh_tree__enabled || daeditor__selected != null || beh_tree__debugInWebUI != null
      res |> push(JV("{eid}", getEntityTemplateName(eid), beh_tree__debug, beh_tree__track_mind))
  send_response(JV(res), id)


[es(tag=dasDebug, after=walker_agent_es, REQUIRE=beh_tree__track_mind)]
def ai_log_mind_update(act : UpdateStageInfoAct; eid : EntityId; beh_tree : BehaviourTree)
  if beh_tree.changeNodeReason != null || beh_tree.resetReason != null
    let name = (beh_tree.changeNodeReason != null
                ? (*beh_tree.changeNodeReason) |> beh_node_hierarchy_name(true)
                : (*beh_tree.resetReason) |> beh_node_hierarchy_name(true))
    send_notification("ai_log.mind", JV(uint(eid), name))

[rpc_call(name="ai_log.get_beh_tree")]
def get_beh_tree(eid : uint; id : uint64)
  query(EntityId(eid)) <| $ [es] (var beh_tree : BehaviourTree)
    var nodes : array<tuple<node : BehNode?; jv : JsonValue?>>
    find_beh_node(*(beh_tree.root)) <| $(node)
      var jv = JV({{ "name" => JV(node.name);  "typeName" => JV(node.typeName); "isReaction" => JV(node.getType == BN_REACTION)}})
      if node.getType == BN_REACTION
        let reaction = ReactionNode(*node)
        insert((jv as _object), "reactionName", JV(*reaction |> reaction_getEventId() |> get_reaction_name()))
      var describe : array<JsonValue?>
      *node |> beh_node_describeDebug() <| $(name; value)
        describe |> push(JV([{JsonValue? JV(name); JV(value)}]))
      insert((jv as _object), "describe", JV(describe))
      let wrapper = DebugWrapperNode(*node)
      if wrapper != null
        insert((jv as _object), "id", JV(wrapper.id))
        insert((jv as _object), "forceResult", JV(wrapper.forceResult))
      nodes |> push([[auto node, jv]])

      if node.parent == null
        return false
      var found = false
      var parentJv : JsonValue?
      for it in nodes
        if it.node == node.parent
          parentJv = it.jv
          found = true
          break
      verify(found)
      if found
        found = false
        (parentJv as _object) |> get("children") <| $(val)
          found = true
          val as _array |> push(jv)
        if !found
          insert((parentJv as _object), "children", JV([{auto[] jv}]))
      return false

    send_response(nodes[0].jv, id)


[rpc_notification(name="ai_log.set_beh_tree_debug")]
def debug_beh_tree(target_eid : uint; value : bool)
  let eid = EntityId(target_eid)
  if value != has(eid, "behTreeDebugWrapped")
    if value
      addSubTemplate(eid, "beh_tree_debug_wrapped")
    else
      removeSubTemplate(eid, "beh_tree_debug_wrapped")

[rpc_notification(name="ai_log.reset_beh_tree_debug_for_all")]
def reset_tracking_for_all()
  query() <| $ [es(REQUIRE=behTreeDebugWrapped)] (eid : EntityId)
    removeSubTemplate(eid, "beh_tree_debug_wrapped")

struct DebugWrapperNodeState
  updateResult : int8
  reactResult : int8
  forceResult : int8
  describe : array<array<string>>

[es(after=(beh_tree_update_es), REQUIRE=behTreeDebugWrapped)]
def compile_update_result(act : UpdateStageInfoAct; eid : EntityId; var beh_tree : BehaviourTree&)
  var res : table<string; DebugWrapperNodeState>

  find_beh_node(*(beh_tree.root)) <| $(node)
    let wrapper = DebugWrapperNode(*node)
    if wrapper != null && wrapper.wasUpdated
      var state <- [[DebugWrapperNodeState
        updateResult = wrapper.lastUpdateResult,
        reactResult = wrapper.lastReactResult,
        forceResult = wrapper.forceResult
      ]]
      *node |> beh_node_describeDebug() <| $(name; value)
        state.describe |> emplace([{auto[] name; value}])
      res[string(wrapper.id)] <- state
    return false

  send_notification("ai_log.beh_tree_update_result") <| JV("{eid}", act.curTime, res)

[rpc_notification(name="ai_log.set_force_result")]
def force_set_result(eid : uint; id : uint; value : int)
  var found = false
  query(EntityId(eid)) <| $ [es] (var beh_tree : BehaviourTree&)
    found = find_beh_node(*(beh_tree.root)) <| $(node)
      var wrapper = DebugWrapperNode(*node)
      if wrapper != null && wrapper.id == id
        wrapper.forceResult = int8(value)
        return true
      return false

  if !found
    logerr("unable to set node force result {int(eid)}: node id: {int(id)} = {value}")


[es(tag=dasDebug, before=(walker_agent_es), REQUIRE=behTreeDebugWrapped)]
def beh_tree_before_debug_update(act : UpdateStageInfoAct; var beh_tree : BehaviourTree&)
  find_beh_node(*(beh_tree.root)) <| $(node)
    var wrapper = DebugWrapperNode(*node)
    if wrapper != null
      wrapper.wasUpdated = false
    return false

[rpc_notification(name="ai_log.force_recalculate_tree")]
def force_recalculate_tree(eid : uint)
  var found = false
  query(EntityId(eid)) <| $ [es] (var beh_tree : BehaviourTree&)
    beh_tree_reset(beh_tree)
    found = true
  if !found
    logerr("unable to force_recalculate_tree of eid {int(eid)}")

[rpc_notification(name="ai_log.toggle_beh_tree_enabled")]
def toggle_beh_tree_enabled(eid : uint)
  var found = false
  query(EntityId(eid)) <| $ [es] (var beh_tree__enabled : bool&)
    beh_tree__enabled = !beh_tree__enabled
    found = true
  if !found
    logerr("unable to force_recalculate_tree of eid {int(eid)}")

[rpc_notification(name="ai_log.destroy_entity")]
def destroy_agent_entity(eid : uint)
  destroyEntity(EntityId(eid))