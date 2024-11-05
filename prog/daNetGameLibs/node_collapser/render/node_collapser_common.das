module node_collapser_common shared
require ecs
require AnimV20
require DagorSystem
require math.base
require ecs.soa_template

let MAX_COLLAPSE_NODES = 4

[soa_template]
struct AnimcharNodeCollapser
  animchar_node_collapser__rootNode : int
  animchar_node_collapser__targetNode : int
  animchar_node_collapser__childrenCount : int


[soa_def]
def collapser(position : float3;
              var animchar_render : AnimcharRendComponent&;
              animchar_visbits : uint8;
              animchar_bsph : float3;
              animchar_node_collapser : AnimcharNodeCollapser_SOA;
              animchar_node_collapser__children : IntList;
              animchar_node_collapser__camThreshold : float;
              animchar_node_collapser__origin_threshold_sq : float)
  if animchar_visbits == uint8(0)
    return
  if distance_sq(animchar_bsph, position) > animchar_node_collapser__origin_threshold_sq
    return

  assume scene = animchar_render.sceneInstance
  var zeroTm = float3x4();
  var from = 0
  for node in animchar_node_collapser
    var collide = false
    for childIdx in range(from, from + node.animchar_node_collapser__childrenCount)
      let child = uint(animchar_node_collapser__children[childIdx])
      let childPos = scene_instance_getNodeWtm(*scene, child)[3]
      // wtm in DynamicRenderableSceneInstance are relative to camPos
      if length_sq(childPos) <= square(animchar_node_collapser__camThreshold)
        collide = true
        break

    if collide
      let rootPos = scene_instance_getNodeWtm(*scene, uint(node.animchar_node_collapser__rootNode))[3]
      zeroTm[3] = (node.animchar_node_collapser__targetNode > 0 ?
                   scene_instance_getNodeWtm(*scene, uint(node.animchar_node_collapser__targetNode))[3] :
                   rootPos)
      for childIdx in range(from, from + node.animchar_node_collapser__childrenCount)
        let child = uint(animchar_node_collapser__children[childIdx])
        *scene |> scene_instance_setNodeWtm(child, zeroTm)
    from += node.animchar_node_collapser__childrenCount
