require ecs
require ecs.safe
require danetlibs.node_collapser.render.node_collapser_events
require danetlibs.node_collapser.render.node_collapser_common
require danetlibs.renderer.includes.render_events
require AnimV20
require GeomNodeTree
require DagorSystem
require math.base
require ecs.soa_template

[soa_es, es(on_appear)]
def node_collapser_create(evt : Event;
                          eid : EntityId;
                          var animchar_node_collapser : AnimcharNodeCollapser_SOA&;
                          var animchar_node_collapser__children : IntList&;
                          animchar_node_collapser__nodes : Object const?;
                          animchar : AnimcharBaseComponent;
                          animchar_render : AnimcharRendComponent)
  var isDisabled = false
  animchar_node_collapser |> clear()

  let scene = animchar_render.sceneInstance
  let geomtree = animchar.nodeTree
  if animchar_node_collapser__nodes != null
    isDisabled = *animchar_node_collapser__nodes |> empty()
    for iter in *animchar_node_collapser__nodes
      let rootNodeIdx = *geomtree |> geomtree_findNodeIndex(iter.key)
      let rootRenderNodeIdx = *scene |> scene_instance_getNodeId(iter.key)
      if rootNodeIdx <= 0 || rootRenderNodeIdx <= 0
        continue
      if length(animchar_node_collapser) >= MAX_COLLAPSE_NODES
        logerr("Attempt to collapse > {MAX_COLLAPSE_NODES} nodes in {getEntityTemplateName(eid)}")
        return

      var childNodes : array<int>
      childNodes |> push(rootNodeIdx)
      var i = 0
      var childrenCount = 0
      while i < length(childNodes)
        let nodeName = *geomtree |> geomtree_getNodeName(childNodes[i])
        if i > 0 && has(*animchar_node_collapser__nodes, nodeName)
          i += 1
          continue
        let nodeId = *scene |> scene_instance_getNodeId(nodeName)
        if nodeId > 0
          animchar_node_collapser__children |> push(nodeId)
          childrenCount += 1
        let childCount = *geomtree |> geomtree_getChildCount(childNodes[i])
        for j in range(childCount)
          childNodes |> push(*geomtree |> geomtree_getChildNodeIdx(childNodes[i], uint(j)))
        i += 1
      let target = iter.value ?? ""
      animchar_node_collapser |> push() <| [[AnimcharNodeCollapser
        animchar_node_collapser__rootNode = rootRenderNodeIdx,
        animchar_node_collapser__targetNode = target == "" ? -1 : *scene |> scene_instance_getNodeId(target),
        animchar_node_collapser__childrenCount = childrenCount
      ]]

  if length(animchar_node_collapser) == 0 && !isDisabled
    logwarn("animchar_node_collapser: not found any nodes of entity {eid} <{getEntityTemplateName(eid)}>")

[es(tag=render, before=animchar_before_render_es)]
def begin_node_collapser_es(stg : UpdateStageInfoBeforeRender; var animchar_render : AnimcharRendComponent&)
  assume scene = animchar_render.sceneInstance
  *scene |> scene_instance_clearNodeCollapser()

[soa_es, es(REQUIRE=watchedPlayerItem, before=animchar_before_render_es, after=begin_node_collapser_es)]
def on_animchar_node_collapser_item(evt : HideNodesEvent;
                                    var animchar_render : AnimcharRendComponent&;
                                    animchar_visbits : uint8;
                                    animchar_bsph : vec4f;
                                    animchar_node_collapser : AnimcharNodeCollapser_SOA;
                                    animchar_node_collapser__children : IntList;
                                    animchar_node_collapser__camThreshold : float = 0.3;
                                    animchar_node_collapser__origin_threshold_sq : float = 100.0)
  collapser(evt.at, animchar_render, animchar_visbits, float4(animchar_bsph).xyz,
            animchar_node_collapser, animchar_node_collapser__children,
            animchar_node_collapser__camThreshold, animchar_node_collapser__origin_threshold_sq)


[soa_es, es(REQUIRE=hero, before=animchar_before_render_es, after=begin_node_collapser_es)]
def on_animchar_node_collapser(evt : HideNodesEvent;
                               var animchar_render : AnimcharRendComponent&;
                               animchar_visbits : uint8;
                               animchar_bsph : vec4f;
                               animchar_node_collapser : AnimcharNodeCollapser_SOA;
                               animchar_node_collapser__children : IntList;
                               animchar_node_collapser__camThreshold : float = 0.3;
                               animchar_node_collapser__origin_threshold_sq : float = 100.0)
  collapser(evt.at, animchar_render, animchar_visbits, float4(animchar_bsph).xyz,
            animchar_node_collapser, animchar_node_collapser__children,
            animchar_node_collapser__camThreshold, animchar_node_collapser__origin_threshold_sq)