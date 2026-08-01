module node_collapser_common shared
require ecs
require AnimV20
require GeomNodeTree
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
              animchar_visbits : animchar_visbits_t;
              animchar_bsph : float3;
              animchar_node_collapser : AnimcharNodeCollapser_SOA;
              animchar_node_collapser__children : IntList;
              animchar_node_collapser__camThreshold : float;
              animchar_node_collapser__origin_threshold_sq : float)
  if animchar_visbits == animchar_visbits_t(0)
    return
  if distance_sq(animchar_bsph, position) > animchar_node_collapser__origin_threshold_sq
    return

  assume scene = animchar_render.sceneInstance

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
      for childIdx in range(from, from + node.animchar_node_collapser__childrenCount)
        let child = uint(animchar_node_collapser__children[childIdx])
        *scene |> scene_instance_markNodeCollapserNode(child)

    from += node.animchar_node_collapser__childrenCount


def node_collapser_fill_bits(var bits : NodeCollapserBits&;
                             nodes : Object const?;
                             animchar : AnimcharBaseComponent;
                             animchar_render : AnimcharRendComponent)
  bits |> node_collapser_bits_clear()
  if nodes == null
    return

  let scene = animchar_render.sceneInstance
  let geomtree = animchar.nodeTree

  for iter in *nodes
    let rootBoneIdx = *geomtree |> geomtree_findNodeIndex(iter.key)
    if rootBoneIdx <= 0
      continue
    var bones : array<int>
    bones |> push(rootBoneIdx)
    var i = 0
    while i < length(bones)
      let boneName = *geomtree |> geomtree_getNodeName(bones[i])
      if i > 0 && has(*nodes, boneName)
        i += 1
        continue
      let nodeId = *scene |> scene_instance_getNodeId(boneName)
      if nodeId >= 0
        let sceneBoneId = *scene |> scene_instance_getBoneForNode(nodeId)
        if sceneBoneId >= 0
          bits |> node_collapser_bits_markBone(uint(sceneBoneId))
      let childCount = *geomtree |> geomtree_getChildCount(bones[i])
      for j in range(childCount)
        bones |> push(*geomtree |> geomtree_getChildNodeIdx(bones[i], uint(j)))
      i += 1