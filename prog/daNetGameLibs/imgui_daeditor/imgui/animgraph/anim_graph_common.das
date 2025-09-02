options no_global_variables = false
module anim_graph_common shared

require imgui
require DagorImgui
require DagorDataBlock
require AnimV20
require DagorConsole
require DagorMath
require DagorResources
require GeomNodeTree
require math
require strings
require ecs

require imgui/imgui_boost
require imgui/imgui_node_editor_boost

require daslib/static_let
require daslib/defer
require daslib/safe_addr

var private @ignore_heap_usage context : EditorContext? // ignore heap usage because it is used c++ heap

[event(broadcast)]
struct AnimGraphViewerSetAnimState
  state_idx : int

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


def get_link_node(link : int)
  return link / nodeScale - 1


def in_pin(id : int; blk : block<() : void>)
  // imgui_node_editor::PushStyleVar(StyleVar StyleVar_PinArrowSize, 10f)
  // imgui_node_editor::PushStyleVar(StyleVar StyleVar_PinArrowWidth, 10f)
  // imgui_node_editor::PushStyleColor(StyleColor StyleColor_PinRect, float4(1f, 0f, 0f, 1f))
  // imgui_node_editor::PushStyleVar(StyleVar StyleVar_PinCorners, 3f)
  BeginPin(id, PinKind.Input) <|
    Dummy(float2(10))

    let offset = float2(3, 3)
    let min = GetItemRectMin() - float2(16, 0)
    let max = GetItemRectMax() - float2(16, 0)
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
  BeginPin(id, PinKind.Output) <|
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
    context.links |> emplace <|  (outId, get_input_node_idx(childNodeId))
    return

  if *childNode |> anim_blend_node_isSubOf(AnimBlendNodeNullCID) // remap to null node
    let nullNodeId = graph |> anim_graph_getBlendNodeId("null")
    if nullNodeId >= 0
      context.links |> emplace <|  (outId, get_input_node_idx(nullNodeId))
      return
  let name = graph |> anim_graph_getBlendNodeName(childNode)
  print("'{name}' : {*nod |> anim_blend_node_class_name()} {graph |> anim_graph_getAnimNodeName(*nod |> anim_blend_node_getAnimNodeId)} child mismatch {*childNode |> anim_blend_node_getAnimNodeId} {*childNode |> anim_blend_node_class_name()}")

def get_param_value_str(var state : IAnimStateHolder; var graph : AnimationGraph; paramId : int)
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

  return paramValue

def draw_param_hint(var graph : AnimationGraph; var state : IAnimStateHolder; id : int; name : string)
  if IsItemHovered() && id >= 0
    imgui_node_editor::Suspend()
    BeginTooltip()
    imgui::Text(name)
    imgui::Text("Param ID: {id}")
    let paramType = graph |> anim_graph_getParamType(id)
    imgui::Text("Param type: {paramType}")
    imgui::Text("Value: {get_param_value_str(state, graph, id)}")
    if paramType == PT_ScalarParamInt
      let value = state |> anim_state_holder_getParamInt(id)
      let enumName = animV20_get_enum_name_by_id(value)
      if enumName != ""
        imgui::Text("Possible enum: {enumName}")
    elif paramType == PT_ScalarParam
      let value = state |> anim_state_holder_getParam(id)
      let enumName = animV20_get_enum_name_by_id(roundi(value))
      if enumName != ""
        imgui::Text("Possible enum: {enumName}")
    EndTooltip()
    imgui_node_editor::Resume()

def add_input_param(paramId, idx : int; postfix : string; nodeId : int; var state : IAnimStateHolder; var graph : AnimationGraph; var context : AnimgraphContext)
  let paramIdx = get_input_node_idx(nodeId, idx)
  in_pin(paramIdx) <|
    SameLine()

    let paramValue = get_param_value_str(state, graph, paramId)

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
    draw_param_hint(graph, state, paramId, paramName)
    context.params |> push <|  (paramId, paramIdx)


def get_node_name(var graph : AnimationGraph; var nod : IAnimBlendNode?)
  if nod == null
    return "-"
  return graph |> anim_graph_getAnimNodeName(*nod |> anim_blend_node_getAnimNodeId())


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
    imgui::Text("time: {float(node.dt)/TIME_TicksPerSec}")
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
    imgui::Text("isAdditive: {node.isAdditive}")
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("time: {float(node.dt)/TIME_TicksPerSec}")
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
    imgui::Text("time: {float(node.dt)/TIME_TicksPerSec}")
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
        out_pin(outId, "{idx + 1}) {slice.start}..{slice.end} '{get_node_name(graph, childNode)}'") <|
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
      var childNode = *queue |> AnimFifo3Queue_get_node(idx)
      out_pin(outId, "{idx + 1}) '{get_node_name(graph, childNode)}'") <|
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
        var childNode = child.node |> get()
        out_pin(outId, "{idx + 1}) repeat:{child.maxRepeat} wt: {child.rndWt}\n'{get_node_name(graph, childNode)}'") <|
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

          var childNode = child |> get()
          let text = constWeights ? "{idx + 1}) wt:{wt} '{get_node_name(graph, childNode)}'" : "{idx + 1}) '{get_node_name(graph, childNode)}'"
          out_pin(outId, text) <|
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
        var childNode = child |> get()
        out_pin(outId, "{idx + 1}) '{get_node_name(graph, childNode)}'") <|
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
        var childNode = child |> get()
        out_pin(outId, "{idx + 1}) '{get_node_name(graph, childNode)}'") <|
          link_child(graph, nod, outId, childNode, context)

[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_SetMotionMatchingTag(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    assume node = *reinterpret<AnimBlendCtrl_SetMotionMatchingTag?> nod
    imgui::Text("weight: {fmt(":.2f", weight)}")
    imgui::Text("tag: {node.tagName}")


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
        var childNode = child.node |> get()
        out_pin(outId, "{idx + 1}) p0:{child.p0} wtPid:{child.wtPid}\n'{get_node_name(graph, childNode)}'") <|
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
    imgui::PushStyleVar(ImGuiStyleVar.ItemSpacing, float2(0.0, 10.0))
    add_input_param(paramId, ++inNodeIdx, "", nodeId, state, graph, context)

    *node |> AnimBlendCtrl_ParametricSwitcher_getChildren <| $(nodeValuePairs)
      // We have some huge switches in which a ton of values connect to a handfull of different nodes,
      // i.e. deciding what body anim to use based on the weapon selected.
      // Displaying all of those make huge unredable nodes and is a mess in general.
      // So we group all values by output node and display only the output nodes.
      // We can still display values list in a hint since it is occasionally useful.
      // nvp = Node Value Pair
      var childNodeToPairIndex : array<array<int>> // Inner array contains indicies into nodeValuePairs array.
      for nvpIdx, nvp in range(length(nodeValuePairs)), nodeValuePairs
        // Find if some other value already referenced the node this pair is referencing.
        let nodeIdx = childNodeToPairIndex |> find_index_if() <| $(it)
          return nvp.node.get() == nodeValuePairs[it[0]].node.get() // Zero index since all nodes in the array should be the same.

        if nodeIdx < 0
          childNodeToPairIndex |> emplace(array(nvpIdx))
        else
          childNodeToPairIndex[nodeIdx] |> push(nvpIdx)

      for i, pairIndicies in range(length(childNodeToPairIndex)), childNodeToPairIndex
        let outId = get_output_node_idx(nodeId, i)
        var childNode = nodeValuePairs[pairIndicies[0]].node.get()
        out_pin(outId, "{get_node_name(graph, childNode)}") <|
          link_child(graph, nod, outId, childNode, context)

          if IsItemHovered()
            imgui_node_editor::Suspend()
            BeginTooltip()
            for j, nvpIdx in range(length(pairIndicies)), pairIndicies
              assume child = nodeValuePairs[nvpIdx]
              let start = child |> AnimBlendCtrl_ParametricSwitcherItemAnim_getStart()
              let end = child |> AnimBlendCtrl_ParametricSwitcherItemAnim_getEnd()
              // When animGraph generates param switch ranges from enums it generates ranges like [enumVal - 0.1, enumVal + 0.1]
              // Using this we can guess that ranges of length < 0.3 are probably enum values and we can try find one
              var paramName = ""
              if end - start < 0.3
                let possibleParamId = floori(end)
                let name = animV20_get_enum_name_by_id(possibleParamId)
                if name != ""
                  paramName = "'{name}' "
              TextUnformatted("{j}) {paramName}base: {child.baseVal} [{start};{end}]")
            EndTooltip()
            imgui_node_editor::Resume()

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


def draw_param_info(var graph : AnimationGraph; var state : IAnimStateHolder; paramId : int; name : string)
  imgui::Text("{name}: ({paramId}) {graph |> anim_graph_getParamName(paramId)} = {get_param_value_str(state, graph, paramId)}")


def render_AnimPostBlendParamFromNode(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    render_AnimPostBlendCtrl(nod, graph, state, context, weight)
    assume node = reinterpret<AnimPostBlendParamFromNode?> nod
    imgui::Spacing()
    // draw_param_info(graph, state, node.varId, "varId") // not really useful unless we get underlying inline ptr param
    imgui::Text("node: {node.nodeName}")
    imgui::Text("slotId: {node.slotId} ({animchar_getSlotName(node.slotId)})")
    draw_param_info(graph, state, node.destVarId, "destVarId")
    var localData = reinterpret<AnimPostBlendParamFromNodeLocalData?>(state |> anim_state_holder_getInlinePtr(node.varId))
    imgui::Text("lastRefUid: {EntityId(uint(localData.lastRefUid))}")
    imgui::Text("destVarWtId: {node.destVarWtId}")
    imgui::Text("defVal: {node.defVal}")
    imgui::Text("invertVal: {node.invertVal}")


def render_AttachGeomNodeCtrl(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : IAnimStateHolder; var context : AnimgraphContext; weight : float)
  unsafe
    render_AnimPostBlendCtrl(nod, graph, state, context, weight)
    assume node = reinterpret<AttachGeomNodeCtrl?> nod
    imgui::Spacing()
    imgui::Text("minNodeScale: {node.minNodeScale}")
    imgui::Text("maxNodeScale: {node.maxNodeScale}")
    draw_param_info(graph, state, node.perAnimStateDataVarId, "perAnimStateDataVarId")
    imgui::Spacing()

    AttachGeomNodeCtrl_getNodeNames(*node) <| $(nodeNames)
      for name, i in nodeNames, range(length(node.destVarId))
        imgui::Spacing()
        imgui::Text("Node: {name}")
        assume n = node.destVarId[i]
        draw_param_info(graph, state, n.wScale, "  wScale param")
        imgui::Text("  wScaleInverted: {n.wScaleInverted}")
        draw_param_info(graph, state, n.nodeWtm, "  nodeWtm param")
        if n.nodeWtm >= 0
          var AttachDesc : AttachGeomNodeCtrlAttachDesc? = reinterpret<AttachGeomNodeCtrlAttachDesc?>(state |> anim_state_holder_getInlinePtr(n.nodeWtm))
          var tm_pos, tm_scale : float3
          var tm_rot_q : float4
          decompose(float4x4(AttachDesc.wtm), tm_pos, tm_rot_q, tm_scale)
          let tm_rot = euler_from_quat(tm_rot_q)
          imgui::Text("    wtm Pos: X: {fmt(":.2f",tm_pos.x)} Y: {fmt(":.2f",tm_pos.y)} Z: {fmt(":.2f",tm_pos.z)}")
          imgui::Text("    wtm Rot: X: {fmt(":.2f",tm_rot.x)} Y: {fmt(":.2f",tm_rot.y)} Z: {fmt(":.2f",tm_rot.z)}")
          imgui::Text("    wtm Scale: X: {fmt(":.2f",tm_scale.x)} Y: {fmt(":.2f",tm_scale.y)} Z: {fmt(":.2f",tm_scale.z)}")
          imgui::Text("    weight: {AttachDesc.w}")


enum DrawNodesMode
  ALL
  ACTIVE
  ACTIVE_AND_HISTORY


[unused_argument(state)]
def render_anim_graph2(animchar : AnimcharBaseComponent; var graph : AnimationGraph; var state : IAnimStateHolder; var reset_state : bool&; var params_filter : ImGuiTextFilter&; var states_filter : ImGuiTextFilter&)

  var renderers : array<tuple<uint; BlendNodeRenderer>>
  push(renderers, tuple(AnimBlendNodeNullCID, @@render_AnimBlendNodeNull))
  push(renderers, tuple(AnimBlendNodeContinuousLeafCID, @@render_AnimBlendNodeContinuousLeaf))
  push(renderers, tuple(AnimBlendNodeParametricLeafCID, @@render_AnimBlendNodeParametricLeaf))
  push(renderers, tuple(AnimBlendNodeSingleLeafCID, @@render_AnimBlendNodeSingleLeaf))
  push(renderers, tuple(AnimBlendCtrl_1axisCID, @@render_AnimBlendCtrl_1axis))
  push(renderers, tuple(AnimBlendCtrl_Fifo3CID, @@render_AnimBlendCtrl_Fifo3))
  push(renderers, tuple(AnimBlendCtrl_RandomSwitcherCID, @@render_AnimBlendCtrl_RandomSwitcher))
  push(renderers, tuple(AnimBlendCtrl_HubCID, @@render_AnimBlendCtrl_Hub))
  push(renderers, tuple(AnimBlendCtrl_BinaryIndirectSwitchCID, @@render_AnimBlendCtrl_BinaryIndirectSwitch))
  push(renderers, tuple(AnimBlendCtrl_SetMotionMatchingTagCID, @@render_AnimBlendCtrl_SetMotionMatchingTag))
  push(renderers, tuple(AnimBlendCtrl_ParametricSwitcherCID, @@render_AnimBlendCtrl_ParametricSwitcher))
  push(renderers, tuple(AnimBlendCtrl_LinearPolyCID, @@render_AnimBlendCtrl_LinearPoly))
  // push(renderers, tuple(AnimBlendNodeLeafCID, @@render_AnimBlendNodeLeaf))
  push(renderers, tuple(AnimBlendNodeStillLeafCID, @@render_AnimBlendNodeStillLeaf))

  push(renderers, tuple(AnimPostBlendParamFromNodeCID, @@render_AnimPostBlendParamFromNode))
  push(renderers, tuple(AttachGeomNodeCtrlCID, @@render_AttachGeomNodeCtrl))
  push(renderers, tuple(AnimPostBlendCtrlCID, @@render_AnimPostBlendCtrl)) // should be last as this is a generic pbc renderer

  var ctx : AnimgraphContext

  ImGuiEx_BeginColumn()

  let paramsWidth = 250f
  imgui::BeginChild("__params__", float2(paramsWidth, -FLT_MIN), ImGuiChildFlags.Border, ImGuiWindowFlags.None)

  var autoAlign = imgui::Button("Align")
  imgui::SameLine()
  var focusContent = imgui::Button("Focus")
  imgui::SameLine()
  let focusRoot = imgui::Button("Focus root")

  static_let <|
    var lastGraphPtr : uint64
    var forcedState = -1
    var resetState = false
    var drawMode : DrawNodesMode = DrawNodesMode.ACTIVE
    var prevDrawMode : DrawNodesMode = DrawNodesMode.ACTIVE
    var nodesToShow : table<int; tuple<generation : int; active : bool>>
    var nodesToShowMaxInactiveCount = 50
    var graphHash = 0
    var bookmarksLoaded = false
    var bookmarkedStates : array<string>
    var bookmarkedParams : array<string>
    var pbcListShowActiveOnly = true
    var bnlListShowActiveOnly = true

  static_let_finalize <|
    var skeletonFilter = new ImGuiTextFilter

  if !bookmarksLoaded
    bookmarksLoaded = true
    bookmarkedStates |> clear()
    bookmarkedParams |> clear()

    let load = $ <| (name : string; var bookmarks : array<string>)
      imgui_get_blk() |> datablock_get_block(name) <| $(blk)
        blk |> datablock_for_each_block() <| $(stateBlk)
          let bookmarkName = stateBlk |> datablock_getBlockName()
          if bookmarkName != "" && bookmarks |> find_index(bookmarkName) < 0
            bookmarks |> push_clone(bookmarkName)

    invoke(load, "animgraph_viewer_bookmarked_states", bookmarkedStates)
    invoke(load, "animgraph_viewer_bookmarked_params", bookmarkedParams)

  let graphPtr = intptr(unsafe(addr(graph)))
  if lastGraphPtr != graphPtr
    lastGraphPtr = graphPtr
    forcedState = -1
    resetState = false
    drawMode = DrawNodesMode.ACTIVE
    nodesToShow.clear()
    graphHash = 0
    // reset focus on to the content when opening the editor or changing graph
    autoAlign = true
    focusContent = true
    pbcListShowActiveOnly = true
    bnlListShowActiveOnly = true

  var recalcShownNodes = false
  if prevDrawMode != drawMode
    prevDrawMode = drawMode
    recalcShownNodes = true
    autoAlign = true
    focusContent = true

  graph |> anim_graph_getAnimNodeNames <| $(nodes)
    graph |> animchar_getAnimBlendNodeWeights(state) <| $(abn_wt; bnl_wt; pbc_wt)

      imgui::Checkbox("Apply selected state", safe_addr(resetState))
      reset_state = resetState

      // graph |> anim_graph_getStRec <| $(stateRecs)
      //   for stateRec in stateRecs
      //     imgui::SameLine()
      //     imgui::Text("{stateRec}")

      if imgui::CollapsingHeader("Force States")
        let statesCount = graph.stateCount

        var isSelected = forcedState == -1
        if imgui::Selectable("None", isSelected, ImGuiSelectableFlags.SpanAllColumns)
          forcedState = -1

        for stateId in range(statesCount)
          let stateName = graph |> anim_graph_getStateName(stateId)
          let stateIdx = graph |> anim_graph_getStateIdx(stateName)
          isSelected = forcedState == stateIdx
          if imgui::Selectable("{stateName} ({stateIdx})", isSelected, ImGuiSelectableFlags.SpanAllColumns)
            forcedState = stateIdx

      if resetState && forcedState >= 0
        graph |> anim_graph_enqueueState(state, forcedState)

      var saveBookmarks = false

      if imgui::CollapsingHeader("States")
        imgui::Text("States filter:")
        states_filter |> Draw("##statesFilter")
        Spacing()

        for stateId in range(graph.stateCount)
          let stateName = graph |> anim_graph_getStateName(stateId)
          let stateIdx = graph |> anim_graph_getStateIdx(stateName)

          if !(states_filter |> PassFilter(stateName))
            continue

          if imgui::Button("{stateName} ({stateIdx})##states_{stateName}")
            graph |> anim_graph_enqueueState(state, stateIdx)
            broadcastEvent(AnimGraphViewerSetAnimState(state_idx = stateIdx))

          if imgui::BeginPopupContextItem("StateContextMenu##states_{stateName}")
            let bookmarkIndex = bookmarkedStates |> find_index("{stateName}")
            if bookmarkIndex < 0 && imgui::MenuItem("Bookmark")
              bookmarkedStates |> push_clone(stateName)
              saveBookmarks = true
            elif bookmarkIndex >= 0 && imgui::MenuItem("Remove Bookmark")
              bookmarkedStates |> erase(bookmarkIndex)
              saveBookmarks = true
            imgui::EndPopup()

      var eraseBookmarkParamId = -1

      let drawParam = $ <| (id : int; prefix : string; name : string; showHiddenParams : bool)
        let popup = $ <| ()
          if imgui::BeginPopupContextItem("ParamContextMenu##{prefix}params_{name}")
            let bookmarkIndex = bookmarkedParams |> find_index(name)
            if bookmarkIndex < 0 && imgui::MenuItem("Bookmark")
              bookmarkedParams |> push_clone(name)
              saveBookmarks = true
            elif bookmarkIndex >= 0 && imgui::MenuItem("Remove Bookmark")
              eraseBookmarkParamId = bookmarkIndex
              saveBookmarks = true
            imgui::EndPopup()

        let paramType = graph |> anim_graph_getParamType(id)
        if paramType == PT_Reserved
          return

        if paramType == PT_ScalarParam
          imgui::Text("{name} ({id})")
          invoke(popup)
          var value = state |> anim_state_holder_getParam(id)
          let min = -1e8
          let max = 1e8
          SetNextItemWidth(-FLT_MIN)
          if imgui::DragScalar("##{name}", ImGuiDataType.Float, safe_addr(value), 0.01, safe_addr(min), safe_addr(max), "%.2f", ImGuiSliderFlags.None)
            state |> anim_state_holder_setParam(id, value)
          Spacing()

        elif paramType == PT_ScalarParamInt
          imgui::Text("{name} ({id})")
          invoke(popup)
          let value = state |> anim_state_holder_getParamInt(id)
          let step = 1
          let fastStep = 5
          SetNextItemWidth(-FLT_MIN)
          if imgui::InputScalar("##{name}", ImGuiDataType.S32, safe_addr(value), safe_addr(step), safe_addr(fastStep), "%d", ImGuiInputTextFlags.None)
            state |> anim_state_holder_setParamInt(id, value)
          Spacing()

        elif showHiddenParams
          if paramType == PT_TimeParam
            invoke(popup)
            imgui::Text("{name} ({id})")
            let timeValue = state |> anim_state_holder_getParam(id)
            let min = -1e8
            let max = 1e8
            let availableWidth = imgui::GetContentRegionAvail().x
            SetNextItemWidth(availableWidth * 0.5f)
            if imgui::DragScalar("##{name}_time", ImGuiDataType.Float, safe_addr(timeValue),  0.01, safe_addr(min), safe_addr(max), "%.2f", ImGuiSliderFlags.None)
              state |> anim_state_holder_setParam(id, timeValue)
            imgui::SameLine()
            let value = state |> anim_state_holder_getTimeScaleParamId(id)
            let step = 1
            let fastStep = 5
            SetNextItemWidth(availableWidth * 0.5f)
            if imgui::InputScalar("##{name}_scale", ImGuiDataType.S32, safe_addr(value), safe_addr(step), safe_addr(fastStep), "%d", ImGuiInputTextFlags.None)
              state |> anim_state_holder_setTimeScaleParamId(id, value)
            Spacing()

          elif paramType == PT_InlinePtr
            imgui::Text("{name} ({id})")
            invoke(popup)
            imgui::TextUnformatted("  <PT_InlinePtr>")
            Spacing()

          elif paramType == PT_InlinePtrCTZ
            imgui::Text("{name} ({id})")
            invoke(popup)
            imgui::TextUnformatted("  <PT_InlinePtrCTZ>")
            Spacing()

          elif paramType == PT_Fifo3
            imgui::Text("{name} ({id})")
            invoke(popup)
            imgui::TextUnformatted("  <PT_Fifo3>")
            Spacing()

          elif paramType == PT_Effector
            imgui::Text("{name} ({id})")
            invoke(popup)
            imgui::TextUnformatted("  <PT_Effector>")
            Spacing()

          else
            imgui::Text("{name} ({id})")
            Spacing()

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
            if !showHiddenParams && param.name |> character_at(0) == ':'
              continue

            if !(params_filter |> PassFilter(param.name))
              continue

            invoke(drawParam, param.id, "", "{param.name}", showHiddenParams)

      if imgui::CollapsingHeader("Bookmarked states and params")
        if length(bookmarkedStates) + length(bookmarkedParams) == 0
          imgui::Text("No bookmarks found.\nRight click on param\\state to add\\remove.")

        var eraseBookmarkedStatedx = -1
        imgui::Text("States:")
        for stateName in bookmarkedStates
          let stateIdx = graph |> anim_graph_getStateIdx(stateName)
          if stateIdx < 0
            continue

          if imgui::Button("{stateName} ({stateIdx})##bookmarkStates_{stateName}")
            graph |> anim_graph_enqueueState(state, stateIdx)
            broadcastEvent(AnimGraphViewerSetAnimState(state_idx = stateIdx))

          if imgui::BeginPopupContextItem("StateContextMenu##bookmarkStates_{stateIdx}")
            let bookmarkIndex = bookmarkedStates |> find_index(stateName)
            if bookmarkIndex >= 0 && imgui::MenuItem("Remove Bookmark") // bookmarkIndex should always be >= 0 here, but lets check just to be sure
              eraseBookmarkedStatedx = bookmarkIndex
            imgui::EndPopup()

        if eraseBookmarkedStatedx >= 0
          bookmarkedStates |> erase(eraseBookmarkedStatedx)
          saveBookmarks = true

        imgui::Text("Params:")
        for paramName in bookmarkedParams
          let paramId = graph.anim_graph_getParamId(paramName)
          if paramId < 0
            continue

          invoke(drawParam, paramId, "bookmark", paramName, true)

      if eraseBookmarkParamId >= 0
        bookmarkedParams.erase(eraseBookmarkParamId)
        saveBookmarks = true

      if saveBookmarks
        let save = $ <| (name : string; bookmarks : array<string>)
          if !(imgui_get_blk() |> datablock_block_exists(name))
            imgui_get_blk() |> datablock_add_new_block(name)
          imgui_get_blk() |> datablock_get_block(name) <| $(blk)
            blk |> datablock_clear_data()
            for bookmarkName in bookmarks
              blk |> datablock_add_new_block(bookmarkName)

        invoke(save, "animgraph_viewer_bookmarked_states", bookmarkedStates)
        invoke(save, "animgraph_viewer_bookmarked_params", bookmarkedParams)
        imgui_save_blk()

      if imgui::CollapsingHeader("Slots")
        for i in range(animchar |> animchar_getAttachmentSlotsCount())
          let slotId = animchar |> animchar_getAttachmentSlotId(i)
          imgui::Text("{slotId}: {animchar_getSlotName(slotId)}")

      if imgui::CollapsingHeader("Enums")
        var enumId = 0
        var enumName = animV20_get_enum_name_by_id(enumId)
        // animV20_get_enum_name_by_id returns NULL (which is "" string in das) when enumId is out of range.
        while enumName != ""
          imgui::Text("{enumId}: {enumName}")
          enumId += 1
          enumName = animV20_get_enum_name_by_id(enumId)

      if imgui::CollapsingHeader("Post Blend Controllers ({length(graph.blender.pbCtrl)})")
        imgui::Checkbox("Show active only##pbc", safe_addr(pbcListShowActiveOnly))
        for i, pbcPtr in range(length(graph.blender.pbCtrl)), graph.blender.pbCtrl
          if pbcListShowActiveOnly && pbc_wt[i] == 0.0
            continue
          let pbc = get(pbcPtr)
          imgui::Text("{graph |> anim_graph_getBlendNodeName(pbc)}")

      if imgui::CollapsingHeader("Blend Node Leafs ({length(graph.blender.bnl)})")
        imgui::Checkbox("Show active only##bnl", safe_addr(bnlListShowActiveOnly))
        for i, bnlPtr in range(length(graph.blender.bnl)), graph.blender.bnl
          if bnlListShowActiveOnly && bnl_wt[i] == 0.0
            continue
          let bnl = get(bnlPtr)
          imgui::Text("{graph |> anim_graph_getBlendNodeName(bnl)}")

      imgui::EndChild()
      ImGuiEx_NextColumn()

      imgui::BeginTabBar("Tabs")

      let graphViewControlBarHeight = 24.0f
      let graphSize = float2(
        imgui::GetContentRegionAvail().x,
        imgui::GetContentRegionAvail().y - graphViewControlBarHeight
      )

      if imgui::BeginTabItem("Graph")
        defer <|
          imgui::EndTabItem()

        if context == null
          var config = Config()
          // config.SettingsFile = "Simple.json"
          context = CreateEditor(safe_addr(config))

        SetCurrentEditor(context)

        var style & = unsafe(imgui_node_editor::GetStyle())
        style.NodeRounding = 0f
        style.GroupRounding = 0f
        style.PinRounding = 0f
        style.NodePadding = float4(12, 4, 12, 8)
        imgui::PushStyleColor(ImGuiCol.Text, float4(1))
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


        Begin("Anim graph", graphSize) <|
          animNodesCount = abn_wt |> length()
          pbcNodesCount = pbc_wt |> length()
          bnlNodesCount = bnl_wt |> length()

          var weights : array<float>
          weights |> reserve(length(nodes))

          var newGraphHash = 0
          for node in nodes
            var nodePtr = graph |> anim_graph_getBlendNodePtr(node.id)
            var weight = abn_wt[node.id]

            // pbcs and bnls save their weights to different pbc_wt and bnl_wt respectively, abn_wt of those will always be 0.0
            if weight > 0.0
              activeBlendNodesCount += 1

            if nodePtr == null
              weights |> push(weight)
              continue

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

            weights |> push(weight)

            if weight > 0.0
              newGraphHash ^= node.id * 16777619

          // We calculate graph hash based on what nodes are active.
          // When it changes we assume some nodes became active and some inactive.
          // If there are more inactive nodes on the screen than user wants, we want to delete nodes
          // which were shown least.
          if graphHash != newGraphHash || recalcShownNodes
            graphHash = newGraphHash
            autoAlign = true
            if drawMode != DrawNodesMode.ACTIVE
              var activeNodesCount = 0
              var maxGeneration = 0
              for idx, w in range(length(weights)), weights
                var wasActive = false
                nodesToShow |> get(idx) <| $(var val)
                  wasActive = true
                  if w > 0.0
                    activeNodesCount += 1
                    val.active = true
                    val.generation += 1 // We are showing some node again, bump it's generation so it has less chance to be hidden after
                    maxGeneration = max(maxGeneration, val.generation)
                  else
                    val.active = false
                if !wasActive && w > 0.0
                  nodesToShow |> insert(idx, tuple(0, true))
                  activeNodesCount += 1

              if length(nodesToShow) - activeNodesCount > nodesToShowMaxInactiveCount
                var keysToDelete : array<int>
                var keysToDeleteCount = length(nodesToShow) - activeNodesCount - nodesToShowMaxInactiveCount
                for genToDelete in range(maxGeneration - 1)
                  for val, key in values(nodesToShow), keys(nodesToShow)
                    if val.generation <= genToDelete && weights[key] == 0.0
                      keysToDelete |> push(key)
                      keysToDeleteCount -= 1

                  if keysToDeleteCount <= 0
                    break

                for key in keysToDelete
                  nodesToShow |> erase(key)


          for idx, node in range(length(nodes)), nodes
            let weight = weights[idx]
            if drawMode == DrawNodesMode.ACTIVE && weight == 0.0
              continue
            if drawMode == DrawNodesMode.ACTIVE_AND_HISTORY && !key_exists(nodesToShow, idx)
              continue

            let activeNodeBorder = drawMode != DrawNodesMode.ACTIVE && weight > 0.0
            if activeNodeBorder
              imgui_node_editor::PushStyleColor(StyleColor.NodeBorder, float4(0.58, 0.96, 0.23f, 1f))

            var nodePtr = graph |> anim_graph_getBlendNodePtr(node.id)

            BeginNode(get_node_idx(node.id)) <|
              Dummy(float2(nodeWidth, 0f))

              let inId = get_input_node_idx(node.id)
              in_pin(inId) <|
                pass

              if nodePtr == null
                imgui::Text("\nThis node has failed to load.\nProbably there was in error in this char's animTree.")
                return

              SameLine()
              imgui::TextUnformatted(node.name)
              // SameLine()
              // imgui::TextUnformatted("{node.id}")

              imgui::Spacing()

              let className = *nodePtr |> anim_blend_node_class_name()
              imgui::Text(className)

              var foundRender = false
              for render in renderers
                if *nodePtr |> anim_blend_node_isSubOf(render._0)
                  unsafe
                    imgui::Text("ID: {reinterpret<uint64> nodePtr}")
                  render._1 |> invoke(nodePtr, graph, state, ctx, weight)
                  foundRender = true
                  break
              if !foundRender
                imgui::Text("Error: no renderer")

            if activeNodeBorder
              imgui_node_editor::PopStyleColor()

            // Colored node header
            let color = nodePtr == null ? 0x600000F0 : \
                        *nodePtr |> anim_blend_node_isSubOf(AnimBlendNodeLeafCID) ? 0x40b032FF : \
                        *nodePtr |> anim_blend_node_isSubOf(AnimPostBlendCtrlCID) ? 0x30b0FF32 : 0x30FFb032
            let min = GetItemRectMin() + float2(1)
            let max = GetItemRectMax()
            *drawList |> AddRectFilled(min, float2(max.x - 1f, min.y + 26f), color)

          for idx, lnk in iter_range(ctx.links), ctx.links
            var color = float4(1, 1, 1, 1)
            if drawMode != DrawNodesMode.ACTIVE
              let active = weights[get_link_node(lnk.outIdx)] > 0.0 && weights[get_link_node(lnk.inIdx)] > 0.0
              if active
                color = float4(0.58, 0.96, 0.23f, 1f)
            imgui_node_editor::Link(idx, lnk.outIdx, lnk.inIdx, color)

          let animNodeCount = graph.animNodeCount
          let startParamsIdx = get_node_idx(animNodeCount)
          var paramsOffset = startParamsIdx
          for idx, it in range(length(ctx.links), length(ctx.links) + length(ctx.params)), ctx.params
            if it.paramId < 0
              continue
            let paramName = graph |> anim_graph_getParamName(it.paramId)
            BeginNode(++paramsOffset) <|
              out_pin(++paramsOffset, "{paramName} ({it.paramId}): {get_param_value_str(state, graph, it.paramId)}", 0f) <|
                draw_param_hint(graph, state, it.paramId, paramName)
              imgui_node_editor::Link(idx, paramsOffset, it.inIdx, float4(1, 1, 1, 1))


          assume rootNode = *graph.root
          let rootNodeId = rootNode |> anim_blend_node_getAnimNodeId()
          if autoAlign
            var children : array<int>
            children |> push(rootNodeId)
            var allNodes : table<int; bool>
            for i in range(graph.animNodeCount)
              allNodes.insert(i, has_children(ctx.links, i))

            let HGAP = 100f
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
            NavigateToSelection(false, 0.2f)

          if focusContent
            NavigateToContent(0.2f)

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

        imgui::PopStyleColor()

        SetCurrentEditor(null)

        imgui::Text("Total Node Count: {animNodesCount} (active: {activeBlendNodesCount + activePbcNodesCount + activeBnlNodesCount})  |  PBCs: {pbcNodesCount} (active: {activePbcNodesCount})  |  BNLs: {bnlNodesCount} (active: {activeBnlNodesCount})  |")
        imgui::SameLine()
        if imgui::Button("Draw all{drawMode == DrawNodesMode.ALL ? " (X)" : ""}")
          drawMode = DrawNodesMode.ALL
        imgui::SameLine()
        if imgui::Button("Draw active{drawMode == DrawNodesMode.ACTIVE ? " (X)" : ""}")
          drawMode = DrawNodesMode.ACTIVE
        imgui::SameLine()
        if imgui::Button("Draw active and history{drawMode == DrawNodesMode.ACTIVE_AND_HISTORY ? " (X)" : ""}")
          drawMode = DrawNodesMode.ACTIVE_AND_HISTORY
        if drawMode == DrawNodesMode.ACTIVE_AND_HISTORY
          imgui::SameLine()
          let step = 1
          imgui::Text("Max inactive nodes: ")
          imgui::SameLine()
          imgui::PushItemWidth(120.0f)
          InputScalar("##Max inactive nodes", ImGuiDataType.S32, safe_addr(nodesToShowMaxInactiveCount), safe_addr(step), safe_addr(step), "%d", ImGuiInputTextFlags.None)
          imgui::PopItemWidth()


      if imgui::BeginTabItem("Skeleton")
        defer <|
          imgui::EndTabItem()
        imgui::BeginChild("__skeleton__", float2(-FLT_MIN, -FLT_MIN), ImGuiChildFlags.Border, ImGuiWindowFlags.HorizontalScrollbar)
        defer <|
          imgui::EndChild()

        var expandAll = false
        expandAll = imgui::Button("Expand all")

        var skeletonNodesToShow : array<bool> // Parallel to geom node tree array, stores if a node should be shown when filter is not empty. If filter is empty array is empty too.
        var filterChanged = false
        filterChanged = *skeletonFilter |> Draw("##skeletonNodesFilter")
        imgui::SameLine()
        if imgui::Button("Clear")
          *skeletonFilter |> Clear()

        // if a node is passed by the filter mark all parent nodes as shown too
        if *skeletonFilter |> IsActive()
          skeletonNodesToShow |> reserve(int(animchar.nodeTree.nodeCount))
          for i in range(int(animchar.nodeTree.nodeCount))
            if *skeletonFilter |> PassFilter(*animchar.nodeTree |> geomtree_getNodeName(i))
              skeletonNodesToShow |> push(true)
              var parentNode = *animchar.nodeTree |> geomtree_getParentNodeIdx(i)
              while parentNode != -1
                skeletonNodesToShow[parentNode] = true
                parentNode = *animchar.nodeTree |> geomtree_getParentNodeIdx(parentNode)
            else
              skeletonNodesToShow |> push(false)

        draw_skeleton_node_in_tree_recursive(graph, *animchar.nodeTree, skeletonNodesToShow, 0, expandAll, filterChanged)

      imgui::EndTabBar()
      ImGuiEx_EndColumn()

        // for stateIdx in range(graph.stateCount)
        //   let stateName = graph |> anim_graph_getStateName(stateIdx)
        //   BeginNode(stateIdx + statesOffset) <|
        //     BeginNodeTitleBar <|
        //       imgui::TextUnformatted("State: {stateName}")
        //     imgui::Text(stateName)


def draw_skeleton_node_in_tree_recursive(var graph : AnimationGraph;
                                         tree : GeomNodeTree;
                                         nodesFilter : array<bool>&;
                                         node_idx : int;
                                         expand_all : bool;
                                         filter_changed : bool)
  if expand_all || filter_changed
    imgui::SetNextItemOpen(true)

  let node_name = "{node_idx == 0 ? "::root" : tree |> geomtree_getNodeName(node_idx)}"
  if (length(nodesFilter) == 0 || nodesFilter[node_idx]) && imgui::TreeNodeEx("{node_name}##skel_node_{node_idx}", ImGuiTreeNodeFlags.None)
    let posIdent = 6.0f // just to make pos node a bit more noticable, because it is not a skeleton child
    imgui::Indent(posIdent)

    if imgui::TreeNodeEx("positions##skel_pose_{node_idx}", ImGuiTreeNodeFlags.None)
      var tm : float3x4
      tree |> geomtree_getNodeTmScalar(node_idx, tm)

      var tm_pos, tm_scale : float3
      var tm_rot_q : float4
      decompose(float4x4(tm), tm_pos, tm_rot_q, tm_scale)
      let tm_rot = euler_from_quat(tm_rot_q)
      imgui::Text("Pos: X: {fmt(":.2f",tm_pos.x)} Y: {fmt(":.2f",tm_pos.y)} Z: {fmt(":.2f",tm_pos.z)}")
      imgui::Text("Rot: X: {fmt(":.2f",tm_rot.x)} Y: {fmt(":.2f",tm_rot.y)} Z: {fmt(":.2f",tm_rot.z)}")
      imgui::Text("Scale: X: {fmt(":.2f",tm_scale.x)} Y: {fmt(":.2f",tm_scale.y)} Z: {fmt(":.2f",tm_scale.z)}")

      var wtm : float3x4
      tree |> geomtree_getNodeWtmScalar(node_idx, wtm)

      var wtm_pos, wtm_scale : float3
      var wtm_rot_q : float4
      decompose(float4x4(wtm), wtm_pos, wtm_rot_q, wtm_scale)
      let wtm_rot = euler_from_quat(wtm_rot_q)
      imgui::Text("WPos: X: {fmt(":.2f", wtm_pos.x)} Y: {fmt(":.2f", wtm_pos.y)} Z: {fmt(":.2f", wtm_pos.z)}")
      imgui::Text("WRot: X: {fmt(":.2f", wtm_rot.x)} Y: {fmt(":.2f", wtm_rot.y)} Z: {fmt(":.2f", wtm_rot.z)}")
      imgui::Text("WScale: X: {fmt(":.2f", wtm_scale.x)} Y: {fmt(":.2f", wtm_scale.y)} Z: {fmt(":.2f", wtm_scale.z)}")

      imgui::TreePop()
    imgui::Unindent(posIdent)

    let childCount = tree |> geomtree_getChildCount(node_idx)
    for i in 0u..childCount
      let childNodeIdx = tree |> geomtree_getChildNodeIdx(node_idx, i)
      draw_skeleton_node_in_tree_recursive(graph, tree, nodesFilter, childNodeIdx, expand_all, filter_changed)

    imgui::TreePop()

