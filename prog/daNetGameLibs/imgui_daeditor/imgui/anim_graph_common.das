options no_global_variables = false
module anim_graph_common shared

require imgui
require DagorImgui
require AnimV20
require DagorConsole
require DagorMath
require DagorResources
require math
require strings

require imgui/imgui_boost
require imgui/imgui_node_editor_boost

require daslib/static_let
require daslib/defer
require daslib/safe_addr

var private [[ignore_heap_usage]] context : EditorContext? // ignore heap usage because it is used c++ heap

[finalize]
def shutdown()
  DestroyEditor(context)

struct AnimgraphContext
  links : array<tuple<outIdx : int; inIdx : int>>
  params : array<tuple<paramId : int; inIdx : int>>

typedef
  BlendNodeRenderer = function<(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float) : void>

let
  nodeScale = 10000
  inputOffset = 1
  outputOffset = 1000
  nodeWidth = 220f


def has_children(links : array<tuple<int; int>>; id : int)
  for it in links
    if it._0 / nodeScale - 1 == id
      return true
  return false


def collect_children(links : array<tuple<int; int>>; id : int; var buffer : array<int>)
  for it in links
    if it._0 / nodeScale - 1 == id && buffer |> find_index(it._1 / nodeScale - 1) < 0
      buffer |> push(it._1 / nodeScale - 1)


def get_node_idx(idx : int)
  return nodeScale * (idx + 1)


def get_input_node_idx(idx : int; offset : int = 0)
  return nodeScale * (idx + 1) + inputOffset + offset


def get_output_node_idx(idx : int; offset : int)
  return nodeScale * (idx + 1) + outputOffset + offset


def in_pin(id : int; blk : block<() : void>)
  // imgui_node_editor::PushStyleVar(StyleVar StyleVar_PinArrowSize, 10f)
  // imgui_node_editor::PushStyleVar(StyleVar StyleVar_PinArrowWidth, 10f)
  // imgui_node_editor::PushStyleColor(StyleColor StyleColor_PinRect, float4(1f, 0f, 0f, 1f))
  // imgui_node_editor::PushStyleVar(StyleVar StyleVar_PinCorners, 3f)
  BeginPin(id, PinKind Input) <|
    Dummy(float2(10))

    let offset = float2(3, 3)
    let min = GetItemRectMin() - float2(8, 0)
    let max = GetItemRectMax() - float2(8, 0)
    var drawList = GetWindowDrawList()
    // *drawList |> AddRectFilled(min, max, 0xFFFFFFFF)
    *drawList |> AddTriangleFilled(min, float2(max.x, (min.y + max.y) * 0.5), float2(min.x, max.y), 0xFFFFFFFF)
    // *drawList |> AddCircleFilled((min + max) * 0.5, 5f, 0xFFFFFFFF)// 0xFFFFb032)
    PinRect(min - offset, max + offset)
    let pin = float2(min.x, (min.y + max.y) * 0.5) + float2(1, 0)
    PinPivotRect(pin, pin)

    blk |> invoke()

  // imgui_node_editor::PopStyleVar(2)
  // imgui_node_editor::PopStyleColor(1)


def out_pin(id : int; text : string implicit; blk : block<() : void>)
  out_pin(id, text, nodeWidth - CalcTextSize(text).x, blk)


def out_pin(id : int; text : string implicit; indent : float; blk : block<() : void>)

  ImGuiEx_BeginColumn()

  if indent > 0f
    imgui::Indent(indent)
  imgui::TextUnformatted(text)

  blk |> invoke()

  ImGuiEx_NextColumn()

  // imgui_node_editor::PushStyleVar(StyleVar StyleVar_PinArrowSize, 10f)
  // imgui_node_editor::PushStyleVar(StyleVar StyleVar_PinArrowWidth, 10f)
  // imgui_node_editor::PushStyleVar(StyleVar StyleVar_PinCorners, 3f)
  // let size = GetNodeSize()
  // SetNextItemWidth(-FLT_MIN)
  BeginPin(id, PinKind Output) <|
    // to center pin vertically against the pin text
    let pinSize = 10.0
    let pinOffset = float2(0.0, GetItemRectSize().y / 2.0 - pinSize)

    Dummy(float2(pinSize))

    let min = GetItemRectMin() + float2(8, 3) + pinOffset
    let max = GetItemRectMax() + float2(8, 3) + pinOffset
    var drawList = GetWindowDrawList()
    // *drawList |> AddRectFilled(min, max, 0xFFFFFFFF)
    // *drawList |> AddCircleFilled((min + max) * 0.5, 5f, 0xFFFFFFFF)
    *drawList |> AddTriangleFilled(min, float2(max.x, (min.y + max.y) * 0.5), float2(min.x, max.y), 0xFFFFFFFF)
    let margin = float2(3, 3)
    PinRect(min - margin, max + margin)
    let pin = float2(min.x, (min.y + max.y) * 0.5) + float2(9, 0)
    PinPivotRect(pin, pin)

    //blk |> invoke()

  // imgui_node_editor::PopStyleVar(2)
  ImGuiEx_EndColumn()


def link_child(var graph : AnimationGraph; var nod : IAnimBlendNode?; outId : int; var childNode : IAnimBlendNode?; var context : AnimgraphContext)
  if childNode == null
    return
  let childNodeId = *childNode |> anim_blend_node_getAnimNodeId()
  if childNodeId >= 0
    context.links |> emplace <| [[auto outId, get_input_node_idx(childNodeId)]]
    return

  if *childNode |> anim_blend_node_isSubOf(AnimBlendNodeNullCID) // remap to null node
    let nullNodeId = graph |> anim_graph_getBlendNodeId("null")
    if nullNodeId >= 0
      context.links |> emplace <| [[auto outId, get_input_node_idx(nullNodeId)]]
      return
  let name = graph |> anim_graph_getBlendNodeName(childNode)
  print("'{name}' : {*nod |> anim_blend_node_class_name()} {graph |> anim_graph_getAnimNodeName(*nod |> anim_blend_node_getAnimNodeId)} child mismatch {*childNode |> anim_blend_node_getAnimNodeId} {*childNode |> anim_blend_node_class_name()}")


def add_input_param(paramId, idx : int; postfix : string; nodeId : int; var state : IAnimStateHolder; var graph : AnimationGraph; var context : AnimgraphContext)
  let paramIdx = get_input_node_idx(nodeId, idx)
  in_pin(paramIdx) <|
    SameLine()
    var paramValue = "-"
    if paramId >= 0
      let paramType = graph |> anim_graph_getParamType(paramId)
      if paramType == PT_ScalarParam
        let value = state |> anim_state_holder_getParam(paramId)
        paramValue = fmt(":.2f", value)
      elif paramType == PT_ScalarParamInt
        let value = state |> anim_state_holder_getParamInt(paramId)
        paramValue = "{value}"
      elif paramType == PT_TimeParam
        let value = state |> anim_state_holder_getParam(paramId)
        paramValue = fmt(":.2f", value)

    let paramName = graph |> anim_graph_getParamName(paramId)
    let isEmptyParamName = empty(paramName)
    let isEmptyPostfix = empty(postfix)
    var text = ""
    if !isEmptyParamName && !isEmptyPostfix
      text = "\"{paramName}\" (id: {paramId}, val: '{paramValue}'). {postfix}"
    elif !isEmptyParamName
      text = "\"{paramName}\" (id: {paramId}, val: '{paramValue}')"
    elif !isEmptyPostfix
      text = "{postfix} (id: {paramId}, val: '{paramValue}')"
    else
      text = "Param: id={paramId}"
    Text(text)
    context.params |> push <| [[auto paramId, paramIdx]]


[unused_argument(nod, graph, state, context)]
def render_AnimBlendNodeNull(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    assume node = reinterpret<AnimBlendNodeNull?> nod
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    var inNodeIdx = 0
    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    add_input_param(timeScaleParam, ++inNodeIdx, "time scale", nodeId, state, graph, context)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendNodeStillLeaf(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    assume node = reinterpret<AnimBlendNodeStillLeaf?> nod
    let anim = *node |> AnimBlendNodeStillLeaf_get_anim()
    let res = get_game_resource_name(anim.resId)
    Text("a2d: {res}")
    if anim.resId == -1
      let sourceAnim = *anim |> AnimData_get_source_anim_data()
      if sourceAnim != null
        let sourceRes = get_game_resource_name(sourceAnim.resId)
        imgui::Text("  source a2d: {sourceRes}")
    imgui::Text("isAdditive: {node.isAdditive}")
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    var inNodeIdx = 0
    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    add_input_param(timeScaleParam, ++inNodeIdx, "time scale", nodeId, state, graph, context)

    let pos = *node |> anim_blend_node_getPos()
    imgui::Text("pos: {pos}")


[unused_argument(nod, graph, state, context)]
def render_AnimBlendNodeContinuousLeaf(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    assume node = reinterpret<AnimBlendNodeContinuousLeaf?> nod
    let anim = *node |> AnimBlendNodeContinuousLeaf_get_anim()
    let res = get_game_resource_name(anim.resId)
    Text("a2d: {res}")
    if anim.resId == -1
      let sourceAnim = *anim |> AnimData_get_source_anim_data()
      if sourceAnim != null
        let sourceRes = get_game_resource_name(sourceAnim.resId)
        imgui::Text("  source a2d: {sourceRes}")
    imgui::Text("isAdditive: {node.isAdditive}")
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    var inNodeIdx = 0
    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    add_input_param(timeScaleParam, ++inNodeIdx, "time scale", nodeId, state, graph, context)
    let paramId = *node |> anim_blend_node_getParamId()
    add_input_param(paramId, ++inNodeIdx, "", nodeId, state, graph, context)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendNodeParametricLeaf(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    assume node = reinterpret<AnimBlendNodeParametricLeaf?> nod
    let anim = *node |> AnimBlendNodeParametricLeaf_get_anim()
    let res = get_game_resource_name(anim.resId)
    Text("a2d: {res}")
    if anim.resId == -1
      let sourceAnim = *anim |> AnimData_get_source_anim_data()
      if sourceAnim != null
        let sourceRes = get_game_resource_name(sourceAnim.resId)
        imgui::Text("  source a2d: {sourceRes}")
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    var inNodeIdx = 0
    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    add_input_param(timeScaleParam, ++inNodeIdx, "time scale", nodeId, state, graph, context)
    let paramId = *node |> anim_blend_node_getParamId()
    add_input_param(paramId, ++inNodeIdx, "", nodeId, state, graph, context)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendNodeSingleLeaf(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    assume node = reinterpret<AnimBlendNodeSingleLeaf?> nod
    let anim = *node |> AnimBlendNodeSingleLeaf_get_anim()
    let res = get_game_resource_name(anim.resId)
    Text("a2d: {res}")
    if anim.resId == -1
      let sourceAnim = *anim |> AnimData_get_source_anim_data()
      if sourceAnim != null
        let sourceRes = get_game_resource_name(sourceAnim.resId)
        imgui::Text("  source a2d: {sourceRes}")
    imgui::Text("isAdditive: {node.isAdditive}")
    imgui::Text("isAnimationIgnored: {node.isAnimationIgnored}")
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    var inNodeIdx = 0
    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    add_input_param(timeScaleParam, ++inNodeIdx, "time scale", nodeId, state, graph, context)
    let paramId = *node |> anim_blend_node_getParamId()
    add_input_param(paramId, ++inNodeIdx, "", nodeId, state, graph, context)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_1axis(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    assume node = reinterpret<AnimBlendCtrl_1axis?> nod
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    var inNodeIdx = 0
    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    add_input_param(timeScaleParam, ++inNodeIdx, "time scale", nodeId, state, graph, context)
    let paramId = *node |> anim_blend_node_getParamId()
    add_input_param(paramId, ++inNodeIdx, "", nodeId, state, graph, context)

    *node |> AnimBlendCtrl_1axis_getChildren <| $(children)
      for idx, slice in iter_range(children), children
        var childNode = slice.node |> get()
        let outId = get_output_node_idx(nodeId, idx)
        out_pin(outId, "{idx + 1}: {slice.start}..{slice.end}") <|
          link_child(graph, nod, outId, childNode, context)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_Fifo3(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    assume node = reinterpret<AnimBlendCtrl_Fifo3?> nod
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    var inNodeIdx = 0
    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    add_input_param(timeScaleParam, ++inNodeIdx, "time scale", nodeId, state, graph, context)
    let paramId = *node |> anim_blend_node_getParamId()
    var queue : AnimFifo3Queue? = reinterpret<AnimFifo3Queue?>(state |> anim_state_holder_getInlinePtr(paramId))
    add_input_param(paramId, ++inNodeIdx, "state: {queue.state} t0: {queue.t0}", nodeId, state, graph, context)

    for idx in range(3)
      let outId = get_output_node_idx(nodeId, idx)
      out_pin(outId, "{idx + 1}:") <|
        var childNode = *queue |> AnimFifo3Queue_get_node(idx)
        link_child(graph, nod, outId, childNode, context)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_RandomSwitcher(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    assume node = reinterpret<AnimBlendCtrl_RandomSwitcher?> nod
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    var inNodeIdx = 0
    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    add_input_param(timeScaleParam, ++inNodeIdx, "time scale", nodeId, state, graph, context)
    let paramId = *node |> anim_blend_node_getParamId()
    add_input_param(paramId, ++inNodeIdx, "", nodeId, state, graph, context)
    let repParamId = *node |> anim_blend_node_getRepParamId()
    add_input_param(repParamId, ++inNodeIdx, "", nodeId, state, graph, context)

    *node |> AnimBlendCtrl_RandomSwitcher_getChildren <| $(children)
      for idx, child in iter_range(children), children
        let outId = get_output_node_idx(nodeId, idx)
        out_pin(outId, "{idx + 1}: repeat:{child.maxRepeat} wt: {child.rndWt}") <|
          var childNode = child.node |> get()
          link_child(graph, nod, outId, childNode, context)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_Hub(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    assume node = *reinterpret<AnimBlendCtrl_Hub?> nod
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("duration: {node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    var inNodeIdx = 0
    let timeScaleParam = node |> anim_blend_node_getTimeScaleParamId(state)
    add_input_param(timeScaleParam, ++inNodeIdx, "time scale", nodeId, state, graph, context)
    let paramId = node |> anim_blend_node_getParamId()
    let constWeights = paramId < 0
    if !constWeights
      add_input_param(paramId, ++inNodeIdx, "", nodeId, state, graph, context)

    node |> AnimBlendCtrl_Hub_getDefNodeWt <| $(weights)
      node |> AnimBlendCtrl_Hub_getChildren <| $(children)
        for idx, child, wt in iter_range(children), children, weights
          let outId = get_output_node_idx(nodeId, idx)

          let text = constWeights ? "{idx + 1}: wt:{wt}" : "{idx + 1}:"
          out_pin(outId, text) <|
            var childNode = child |> get()
            link_child(graph, nod, outId, childNode, context)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_Blender(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    assume node = *reinterpret<AnimBlendCtrl_Blender?> nod
    imgui::Text("blendTime: {node.blendTime}")
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("duration: {node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    var inNodeIdx = 0
    let timeScaleParam = node |> anim_blend_node_getTimeScaleParamId(state)
    add_input_param(timeScaleParam, ++inNodeIdx, "time scale", nodeId, state, graph, context)
    let paramId = node |> anim_blend_node_getParamId()
    add_input_param(paramId, ++inNodeIdx, "", nodeId, state, graph, context)

    node |> AnimBlendCtrl_Blender_getChildren <| $(children)
      for idx, child in iter_range(children), children
        let outId = get_output_node_idx(nodeId, idx)
        out_pin(outId, "{idx + 1}:") <|
          var childNode = child |> get()
          link_child(graph, nod, outId, childNode, context)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_BinaryIndirectSwitch(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    assume node = *reinterpret<AnimBlendCtrl_BinaryIndirectSwitch?> nod
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("duration: {node |> anim_blend_node_getDuration(state)}")
    imgui::Text("time scale param: {node |> anim_blend_node_getTimeScaleParamId(state)}")
    imgui::Text("avg speed: {node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    var inNodeIdx = 0
    let timeScaleParam = node |> anim_blend_node_getTimeScaleParamId(state)
    add_input_param(timeScaleParam, ++inNodeIdx, "time scale", nodeId, state, graph, context)
    let paramId = node |> anim_blend_node_getParamId()
    add_input_param(paramId, ++inNodeIdx, "", nodeId, state, graph, context)

    node |> AnimBlendCtrl_BinaryIndirectSwitch_getChildren <| $(children)
      for idx, child in iter_range(children), children
        let outId = get_output_node_idx(nodeId, idx)
        out_pin(outId, "{idx + 1}:") <|
          var childNode = child |> get()
          link_child(graph, nod, outId, childNode, context)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_LinearPoly(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    assume node = *reinterpret<AnimBlendCtrl_LinearPoly?> nod
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("duration: {node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    var inNodeIdx = 0
    let timeScaleParam = node |> anim_blend_node_getTimeScaleParamId(state)
    add_input_param(timeScaleParam, ++inNodeIdx, "time scale", nodeId, state, graph, context)
    let paramId = node |> anim_blend_node_getParamId()
    add_input_param(paramId, ++inNodeIdx, "", nodeId, state, graph, context)

    node |> AnimBlendCtrl_LinearPoly_getChildren <| $(children)
      for idx, child in iter_range(children), children
        let outId = get_output_node_idx(nodeId, idx)
        out_pin(outId, "{idx + 1}: p0:{child.p0} wtPid:{child.wtPid}") <|
          var childNode = child.node |> get()
          link_child(graph, nod, outId, childNode, context)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_ParametricSwitcher(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    var node = reinterpret<AnimBlendCtrl_ParametricSwitcher?> nod
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    var inNodeIdx = 0
    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    add_input_param(timeScaleParam, ++inNodeIdx, "time scale", nodeId, state, graph, context)
    let paramId = *node |> anim_blend_node_getParamId()
    imgui::PushStyleVar(ImGuiStyleVar ItemSpacing, float2(0.0, 10.0))
    add_input_param(paramId, ++inNodeIdx, "", nodeId, state, graph, context)

    *node |> AnimBlendCtrl_ParametricSwitcher_getChildren <| $(children)
      for idx, child in iter_range(children), children
        let outId = get_output_node_idx(nodeId, idx)
        let start = child |> AnimBlendCtrl_ParametricSwitcherItemAnim_getStart()
        let end = child |> AnimBlendCtrl_ParametricSwitcherItemAnim_getEnd()
        var paramName = ""
        // When animGraph generates param switch ranges from enums it generates ranges like [enumVal - 0.1, enumVal + 0.1]
        // Using this we can guess that ranges of length < 0.3 are probably enum values and we can try find one
        if end - start < 0.3
          let possibleParamId = floori(end)
          let name = animV20_get_enum_name_by_id(possibleParamId)
          if name != ""
            paramName = "\n({name})"
        out_pin(outId, "{idx + 1}: base:{child.baseVal} {start}..{end}{paramName}") <|
          var childNode = child.node |> get()
          link_child(graph, nod, outId, childNode, context)

    imgui::PopStyleVar()


// [unused_argument(nod, graph, state, context)]
// def render_AnimBlendNodeLeaf(var nod: IAnimBlendNode?; var graph: AnimationGraph; var state: IAnimStateHolder; var context: AnimgraphContext)
//   unsafe
//     var node = reinterpret<AnimBlendNodeLeaf?> nod
//     let anim = *node |> AnimBlendNodeLeaf_get_anim()
//     let res = get_game_resource_name(anim.resId)
//     Text("a2d: {res}")

//     imgui::Text("isAdditive: {node.isAdditive}")
//     imgui::Text("isAnimationIgnored: {node.isAnimationIgnored}")

//     imgui::Text("duration: {*node |> AnimBlendNodeLeaf_getDuration(state)}")
//     imgui::Text("time scale param: {*node |> AnimBlendNodeLeaf_getTimeScaleParamId(state)}")
//     imgui::Text("avg speed: {*node |> AnimBlendNodeLeaf_getAvgSpeed(state)}")


[unused_argument(nod, graph, state, context)]
def render_AnimPostBlendCtrl(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    assume node = reinterpret<AnimPostBlendCtrl?> nod
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    var inNodeIdx = 0
    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    add_input_param(timeScaleParam, ++inNodeIdx, "time scale", nodeId, state, graph, context)


[unused_argument(state)]
def render_anim_graph2(animchar : AnimcharBaseComponent; var graph : AnimationGraph; var state : IAnimStateHolder; var reset_state : bool&; var params_filter : ImGuiTextFilter&)

  var renderers : table<uint; BlendNodeRenderer>
  insert(renderers, AnimBlendNodeNullCID, @@render_AnimBlendNodeNull)
  insert(renderers, AnimBlendNodeContinuousLeafCID, @@render_AnimBlendNodeContinuousLeaf)
  insert(renderers, AnimBlendNodeParametricLeafCID, @@render_AnimBlendNodeParametricLeaf)
  insert(renderers, AnimBlendNodeSingleLeafCID, @@render_AnimBlendNodeSingleLeaf)
  insert(renderers, AnimBlendCtrl_1axisCID, @@render_AnimBlendCtrl_1axis)
  insert(renderers, AnimBlendCtrl_Fifo3CID, @@render_AnimBlendCtrl_Fifo3)
  insert(renderers, AnimBlendCtrl_RandomSwitcherCID, @@render_AnimBlendCtrl_RandomSwitcher)
  insert(renderers, AnimBlendCtrl_HubCID, @@render_AnimBlendCtrl_Hub)
  insert(renderers, AnimBlendCtrl_BinaryIndirectSwitchCID, @@render_AnimBlendCtrl_BinaryIndirectSwitch)
  insert(renderers, AnimBlendCtrl_ParametricSwitcherCID, @@render_AnimBlendCtrl_ParametricSwitcher)
  insert(renderers, AnimBlendCtrl_LinearPolyCID, @@render_AnimBlendCtrl_LinearPoly)
  // insert(renderers, AnimBlendNodeLeafCID, @@render_AnimBlendNodeLeaf)
  insert(renderers, AnimPostBlendCtrlCID, @@render_AnimPostBlendCtrl)
  insert(renderers, AnimBlendNodeStillLeafCID, @@render_AnimBlendNodeStillLeaf)

  var ctx : AnimgraphContext

  ImGuiEx_BeginColumn()

  let paramsWidth = 250f
  imgui::BeginChild("__params__", float2(paramsWidth, -FLT_MIN), ImGuiChildFlags Border, ImGuiWindowFlags None)

  let autoAlign = imgui::Button("Align")
  imgui::SameLine()
  let focusRoot = imgui::Button("Focus root")

  static_let <|
    var lastGraphPtr : uint64
    var forcedState = -1
    var resetState = false
    var drawActiveOnly = true

  let graphPtr = intptr(unsafe(addr(graph)))
  if lastGraphPtr != graphPtr
    lastGraphPtr = graphPtr
    forcedState = -1
    resetState = false
    drawActiveOnly = true

  imgui::Checkbox("Apply selected state", safe_addr(resetState))
  reset_state = resetState

  // graph |> anim_graph_getStRec <| $(stateRecs)
  //   for stateRec in stateRecs
  //     imgui::SameLine()
  //     imgui::Text("{stateRec}")

  if imgui::CollapsingHeader("States")
    let statesCount = graph.stateCount

    var isSelected = forcedState == -1
    if imgui::Selectable("None", isSelected, ImGuiSelectableFlags SpanAllColumns)
      forcedState = -1

    for stateId in range(statesCount)
      let stateName = graph |> anim_graph_getStateName(stateId)
      let stateIdx = graph |> anim_graph_getStateIdx(stateName)
      isSelected = forcedState == stateIdx
      if imgui::Selectable("{stateName} ({stateIdx})", isSelected, ImGuiSelectableFlags SpanAllColumns)
        forcedState = stateIdx

  if resetState && forcedState >= 0
    graph |> anim_graph_enqueueState(state, forcedState)

  if imgui::CollapsingHeader("Parameters")
    static_let <|
      var showHiddenParams = false
    imgui::Checkbox("Show hidden params", safe_addr(showHiddenParams))
    imgui::Text("Params filter:")
    params_filter |> Draw("##paramsFilter")

    Spacing()
    graph |> anim_graph_getParamNames <| $(params)
      imgui::Text("Total params count: {length(params)}")
      imgui::Separator()
      for param in params
        let paramType = graph |> anim_graph_getParamType(param.id)
        if paramType == PT_Reserved
          continue

        if !showHiddenParams && param.name |> character_at(0) == ':'
          continue

        if !(params_filter |> PassFilter(param.name))
          continue

        if paramType == PT_ScalarParam
          imgui::Text("({param.id}) {param.name}")
          var value = state |> anim_state_holder_getParam(param.id)
          let min = -1e8
          let max = 1e8
          SetNextItemWidth(-FLT_MIN)
          if imgui::DragScalar("##{param.name}", ImGuiDataType Float, safe_addr(value), 0.01, safe_addr(min), safe_addr(max), "%.2f", ImGuiSliderFlags None)
            state |> anim_state_holder_setParam(param.id, value)
          Spacing()

        elif paramType == PT_ScalarParamInt
          imgui::Text("({param.id}) {param.name}")
          let value = state |> anim_state_holder_getParamInt(param.id)
          let step = 1
          let fastStep = 5
          SetNextItemWidth(-FLT_MIN)
          if imgui::InputScalar("##{param.name}", ImGuiDataType S32, safe_addr(value), safe_addr(step), safe_addr(fastStep), "%d", ImGuiInputTextFlags None)
            state |> anim_state_holder_setParamInt(param.id, value)
          Spacing()

        elif showHiddenParams
          if paramType == PT_TimeParam
            imgui::Text("({param.id}) {param.name}")
            let timeValue = state |> anim_state_holder_getParam(param.id)
            let min = -1e8
            let max = 1e8
            let availableWidth = imgui::GetContentRegionAvail().x
            SetNextItemWidth(availableWidth * 0.5f)
            if imgui::DragScalar("##{param.name}_time", ImGuiDataType Float, safe_addr(timeValue),  0.01, safe_addr(min), safe_addr(max), "%.2f", ImGuiSliderFlags None)
              state |> anim_state_holder_setParam(param.id, timeValue)
            imgui::SameLine()
            let value = state |> anim_state_holder_getTimeScaleParamId(param.id)
            let step = 1
            let fastStep = 5
            SetNextItemWidth(availableWidth * 0.5f)
            if imgui::InputScalar("##{param.name}_scale", ImGuiDataType S32, safe_addr(value), safe_addr(step), safe_addr(fastStep), "%d", ImGuiInputTextFlags None)
              state |> anim_state_holder_setTimeScaleParamId(param.id, value)
            Spacing()

          elif paramType == PT_InlinePtr
            imgui::Text("({param.id}) {param.name}")
            imgui::TextUnformatted("  <PT_InlinePtr>")
            Spacing()

          elif paramType == PT_InlinePtrCTZ
            imgui::Text("({param.id}) {param.name}")
            imgui::TextUnformatted("  <PT_InlinePtrCTZ>")
            Spacing()

          elif paramType == PT_Fifo3
            imgui::Text("({param.id}) {param.name}")
            imgui::TextUnformatted("  <PT_Fifo3>")
            Spacing()

          elif paramType == PT_Effector
            imgui::Text("({param.id}) {param.name}")
            imgui::TextUnformatted("  <PT_Effector>")
            Spacing()

          else
            imgui::Text("({param.id}) {param.name}")
            Spacing()

  if imgui::CollapsingHeader("Slots")
    for i in range(animchar |> animchar_getAttachmentSlotsCount())
      let slotId = animchar |> animchar_getAttachmentSlotId(i)
      imgui::Text("({slotId}) {animchar_getSlotName(slotId)}")

  imgui::EndChild()
  ImGuiEx_NextColumn()


  if context == null
    var config = Config()
    // config.SettingsFile = "Simple.json"
    context = CreateEditor(safe_addr(config))

  SetCurrentEditor(context)

  var style & = unsafe(imgui_node_editor::GetStyle())
  style.NodeRounding = 0f
  style.GroupRounding = 0f
  style.PinRounding = 0f
  style.NodePadding = float4(4)
  // style.Colors[int(StyleColor StyleColor_Bg)].w = 1f
  // style.Colors[int(StyleColor StyleColor_NodeBg)] = float4(0.12, 0.12, 0.12, 1.0)
  // style.Colors[int(StyleColor StyleColor_NodeBg)].w = 1f

  var drawList = imgui::GetWindowDrawList()

  var animNodesCount = 0 // includes pbcNodesCount and bnlNodesCount
  var pbcNodesCount = 0
  var bnlNodesCount = 0
  var activeBlendNodesCount = 0 // excludes activePbcNodesCount and activeBnlNodesCount
  var activePbcNodesCount = 0
  var activeBnlNodesCount = 0

  let graphViewControlBarHeight = 30.0f
  let availableGraphHeight = imgui::GetContentRegionAvail().y

  Begin("Anim graph", float2(0.0, availableGraphHeight - graphViewControlBarHeight)) <|

    let animNodeCount = graph.animNodeCount

    graph |> anim_graph_getAnimNodeNames <| $(nodes)
      graph |> animchar_getAnimBlendNodeWeights(state) <| $(abn_wt; bnl_wt; pbc_wt)
        animNodesCount = abn_wt |> length()
        pbcNodesCount = pbc_wt |> length()
        bnlNodesCount = bnl_wt |> length()

        for node in nodes
          var nodePtr = graph |> anim_graph_getBlendNodePtr(node.id)
          var weight = abn_wt[node.id]

          // pbcs and bnls save their weights to different pbc_wt and bnl_wt respectively, abn_wt of those will always be 0.0
          if weight > 0.0
            activeBlendNodesCount += 1
          if *nodePtr |> anim_blend_node_isSubOf(AnimBlendNodeLeafCID)
            unsafe
              let bnlNode = reinterpret<AnimBlendNodeLeaf?> nodePtr
              weight = bnl_wt[(*bnlNode).bnlId]
              if weight > 0.0
                activeBnlNodesCount += 1
          elif *nodePtr |> anim_blend_node_isSubOf(AnimPostBlendCtrlCID)
            unsafe
              let pbcNode = reinterpret<AnimPostBlendCtrl?> nodePtr
              weight = pbc_wt[(*pbcNode).pbcId]
              if weight > 0.0
                activePbcNodesCount += 1

          if drawActiveOnly && weight <= 0.0
            continue

          BeginNode(get_node_idx(node.id)) <|
            Dummy(float2(nodeWidth, 0f))

            let inId = get_input_node_idx(node.id)
            let className = *nodePtr |> anim_blend_node_class_name()

            in_pin(inId) <|
              pass

            SameLine()
            imgui::TextUnformatted(node.name)
            // SameLine()
            // imgui::TextUnformatted("{node.id}")

            imgui::Spacing()

            imgui::Text(className)

            var foundRender = false
            for uid, render in keys(renderers), values(renderers)
              if *nodePtr |> anim_blend_node_isSubOf(uid)
                unsafe
                  imgui::Text("ID: {reinterpret<uint64> nodePtr}")
                render |> invoke(nodePtr, graph, state, ctx, weight)
                foundRender = true
                break
            if !foundRender
              imgui::Text("Error: no renderer")

          // Colored node header
          let color = *nodePtr |> anim_blend_node_isSubOf(AnimBlendNodeLeafCID) ? 0x40b032FF : \
                      *nodePtr |> anim_blend_node_isSubOf(AnimPostBlendCtrlCID) ? 0x30b0FF32 : 0x30FFb032
          let min = GetItemRectMin() + float2(1)
          let max = GetItemRectMax()
          *drawList |> AddRectFilled(min, float2(max.x - 1f, min.y + 26f), color)

    for idx, lnk in iter_range(ctx.links), ctx.links
      imgui_node_editor::Link(idx, lnk.outIdx, lnk.inIdx, float4(1, 1, 1, 1)) // float4(0.58, 0.96, 0.23f, 1f)


    let startParamsIdx = get_node_idx(animNodeCount)
    var paramsOffset = startParamsIdx
    for idx, it in range(length(ctx.links), length(ctx.links) + length(ctx.params)), ctx.params
      if it.paramId < 0
        continue
      let paramName = graph |> anim_graph_getParamName(it.paramId)
      BeginNode(++paramsOffset) <|
        out_pin(++paramsOffset, paramName, 0f) <|
          pass
        imgui_node_editor::Link(idx, paramsOffset, it.inIdx, float4(0.58, 0.96, 0.23f, 1f))


    assume rootNode = *graph.root
    let rootNodeId = rootNode |> anim_blend_node_getAnimNodeId()
    if autoAlign
      var children : array<int>
      children |> push(rootNodeId)
      var allNodes : table<int; bool>
      for i in range(graph.animNodeCount)
        allNodes.insert(i, has_children(ctx.links, i))

      let HGAP = 200f
      let VGAP = 50f
      let PADDING = 10f
      var maxWidth = 0f
      var offset = float2(PADDING)

      while length(children) > 0 || length(allNodes) > 0
        var copyChildren := children
        delete children

        for childId in copyChildren
          if childId == -1 || !key_exists(allNodes, childId)
            continue
          allNodes |> erase(childId)
          collect_children(ctx.links, childId, children)

          SetNodePosition(get_node_idx(childId), offset)
          let dim = GetNodeSize(get_node_idx(childId))
          offset.y += dim.y + VGAP
          maxWidth = max(dim.x, maxWidth)

        offset.y = PADDING
        offset.x += maxWidth + HGAP
        if length(children) == 0
          var found = false
          for it in keys(allNodes)
            allNodes |> get(it) <| $(val)
              if val
                children |> push(it)
                found = true
            if found
              break
          if !found
            break


      maxWidth = 0f
      for childId in keys(allNodes)
        SetNodePosition(get_node_idx(childId), offset)
        let dim = GetNodeSize(get_node_idx(childId))
        offset.y += dim.y + VGAP
        maxWidth = max(dim.x, maxWidth)
        if offset.y > 2000f
          offset.x += maxWidth + HGAP
          offset.y = PADDING
          maxWidth = 0f

      var paramIdx = startParamsIdx + 1
      for _idx, it in range(length(ctx.links), length(ctx.links) + length(ctx.params)), ctx.params
        if it.paramId < 0
          continue
        let selfDim = GetNodeSize(paramIdx)
        let nodeIdx = get_node_idx(it.inIdx / nodeScale - 1)
        let nodeDim = GetNodeSize(nodeIdx)
        let pos = GetNodePosition(nodeIdx)
        let inNodeOffset = it.inIdx - nodeIdx - inputOffset - 1
        SetNodePosition(paramIdx, pos + float2(-selfDim.x - 50f, nodeDim.y * 0.5 + float(inNodeOffset) * 30f))
        paramIdx += 2

    if focusRoot
      imgui_node_editor::ClearSelection()
      imgui_node_editor::SelectNode(get_node_idx(rootNodeId), true)
      NavigateToSelection(false, -1f)

    imgui_node_editor::Suspend()
    static_let <|
      var contextMenuLinkId = -1
    if imgui_node_editor::ShowLinkContextMenu(safe_addr(contextMenuLinkId))
      imgui::OpenPopup("link_context_menu")

    if imgui::BeginPopup("link_context_menu")
      var startPinId = -1
      var endPinId = -1
      GetLinkPins(contextMenuLinkId, safe_addr(startPinId), safe_addr(endPinId))
      if startPinId >= 0 && imgui::Selectable("Go to start")
        imgui_node_editor::SelectNode(get_node_idx(startPinId / nodeScale - 1))
        NavigateToSelection(false, -1f)
      if endPinId >= 0 && imgui::Selectable("Go to end")
        imgui_node_editor::SelectNode(get_node_idx(endPinId / nodeScale - 1))
        NavigateToSelection(false, -1f)
      imgui::EndPopup()
    imgui_node_editor::Resume()

    //imgui_node_editor::Group(float2(100))
    // BeginGroupHint(0)
    // imgui::Text("foo")
    // EndGroupHint()


// Handle creation action, returns true if editor want to create new object (node or link)
    BeginCreate(float4(1), 1.0f) <|
      var inputPinId, outputPinId : PinId
      if QueryNewLink(safe_addr(inputPinId), safe_addr(outputPinId))
        pass
        // QueryNewLink returns true if editor want to create new link between pins.
        //
        // Link can be created only for two valid pins, it is up to you to
        // validate if connection make sense. Editor is happy to make any.
        //
        // Link always goes from input to output. User may choose to drag
        // link from output pin or input pin. This determine which pin ids
        // are valid and which are not:
        //   * input valid, output invalid - user started to drag new ling from input pin
        //   * input invalid, output valid - user started to drag new ling from output pin
        //   * input valid, output valid   - user dragged link over other pin, can be validated
        if inputPinId != 0 && outputPinId != 0// both are valid, let's accept link
          // ed::AcceptNewItem() return true when user release mouse button.
          if AcceptNewItem()
            // Since we accepted new link, lets add one to our list of links.
            // let newLinkId = g_NextLinkId++
            // g_Links |> push([[LinkInfo Id=newLinkId, InputId=inputPinId, OutputId=outputPinId]])
            // Draw new link.
            pass
            visual_log("link! {inputPinId} {outputPinId}")
            // Link(10005000, inputPinId, outputPinId)
          // You may choose to reject connection between these nodes
          // by calling ed::RejectNewItem(). This will allow editor to give
          // visual feedback by changing link thickness and color.

  // let selectedNum = GetSelectedObjectCount()
  // var selection: array<int>
  // selection |> resize(selectedNum)
  // unsafe
  //   GetSelectedNodes(reinterpret<int?> addr(selection), selectedNum)
  // visual_log("{selectedNum} {length(selection)} {selection}")

  SetCurrentEditor(null)

  imgui::Text("Total Node Count: {animNodesCount} (active: {activeBlendNodesCount + activePbcNodesCount + activeBnlNodesCount})  |  PBCs: {pbcNodesCount} (active: {activePbcNodesCount})  |  BNLs: {bnlNodesCount} (active: {activeBnlNodesCount})  |")
  imgui::SameLine()
  imgui::Checkbox("Draw active nodes only", safe_addr(drawActiveOnly))

  ImGuiEx_EndColumn()

    // for stateIdx in range(graph.stateCount)
    //   let stateName = graph |> anim_graph_getStateName(stateIdx)
    //   BeginNode(stateIdx + statesOffset) <|
    //     BeginNodeTitleBar <|
    //       imgui::TextUnformatted("State: {stateName}")
    //     imgui::Text(stateName)
