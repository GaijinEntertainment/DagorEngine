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
require %gameLibs.imgui_daeditor.imgui.imgui_common

require daslib/static_let
require daslib/defer
require daslib/safe_addr

var private @ignore_heap_usage context : EditorContext? // ignore heap usage because it is used c++ heap

struct AnimCharViewVarSettings
  enumCls : string
  isBool : bool
  trackbar : bool
  minVal : float
  maxVal : float
  step : float

var private param_view_settings : table<string; AnimCharViewVarSettings>

[event(broadcast)]
struct AnimGraphViewerSetAnimState
  state_idx : int

[event(broadcast)]
struct AnimGraphViewerSelectAssetResource
  res_name : string

[finalize]
def shutdown()
  DestroyEditor(context)

enum NodeDrawMode
  FULL
  MINIFIED

enum UndoRedoActionKind
  HUB_ADD
  HUB_REMOVE
  LINEAR_POLY_ADD
  LINEAR_POLY_REMOVE
  LINEAR_POLY_SET_P0
  PARAM_SWITCH_ADD
  PARAM_SWITCH_REMOVE
  PARAM_SWITCH_SET_START
  PARAM_SWITCH_SET_END

struct UndoRedoAction
  kind : UndoRedoActionKind
  name : string
  needsStateReset : bool // applying this action may register new graph params, so all anim states using the graph must be reinited
  alreadyApplied : bool // value edits are applied live by the widget; on commit they are only recorded for undo, not executed again
  ctrlName : string
  childName : string
  childIdx : int
  oldValue : float
  newValue : float
  weight : float // HUB_ADD, HUB_REMOVE
  p0 : float // LINEAR_POLY_ADD, LINEAR_POLY_REMOVE
  start : float // PARAM_SWITCH_ADD, PARAM_SWITCH_REMOVE
  end : float
  baseVal : float

struct UndoRedoStacks
  undo : array<UndoRedoAction>
  redo : array<UndoRedoAction>

// keyed by anim graph resId so each graph keeps its own history while the selected entity changes
var private graph_edit_history : table<int; UndoRedoStacks>

struct AnimgraphContext
  nodeNames : array<string>
  nodeIds : array<int>
  weights : array<float>
  links : array<tuple<outIdx : int; inIdx : int>>
  animchar : AnimcharBaseComponent?
  drawMode : NodeDrawMode
  pendingActions : array<UndoRedoAction>

typedef
  BlendNodeRenderer = function<(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext) : void>

let
  nodeScale = 10000
  inputOffset = 1
  outputOffset = 1000
  nodeWidth = 220f


def has_children(links : array<tuple<outIdx : int; inIdx : int>>; id : int)
  for it in links
    if it.outIdx / nodeScale - 1 == id
      return true
  return false


def collect_children(links : array<tuple<outIdx : int; inIdx : int>>; id : int; var buffer : array<int>)
  for it in links
    if it.outIdx / nodeScale - 1 == id && buffer |> find_index(it.inIdx / nodeScale - 1) < 0
      buffer |> push(it.inIdx / nodeScale - 1)


def get_node_idx(idx : int)
  return nodeScale * (idx + 1)


def get_input_node_idx(idx : int; offset : int = 0)
  return nodeScale * (idx + 1) + inputOffset + offset


def get_output_node_idx(idx : int; offset : int)
  return nodeScale * (idx + 1) + outputOffset + offset


def get_link_node(link : int)
  return link / nodeScale - 1


def HelpMarkerInNode(desc : string)
  TextDisabled("(?)")
  if IsItemHovered()
    imgui_node_editor::Suspend()
    BeginTooltip()
    PushTextWrapPos(GetFontSize() * 35.0f)
    TextUnformatted(desc)
    PopTextWrapPos()
    EndTooltip()
    imgui_node_editor::Resume()


def in_pin(id : int)
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


def link_child(graph : AnimationGraph; var nod : IAnimBlendNode?; outId : int; var childNode : IAnimBlendNode?; var context : AnimgraphContext)
  if childNode == null
    return
  let childNodeId = *childNode |> anim_blend_node_getAnimNodeId()
  if childNodeId >= 0
    context.links |> emplace <|  (outIdx=outId, inIdx=get_input_node_idx(childNodeId))
    return

  if *childNode |> anim_blend_node_isSubOf(AnimBlendNodeNullCID) // remap to null node
    let nullNodeId = graph |> anim_graph_getBlendNodeId("null")
    if nullNodeId >= 0
      context.links |> emplace <|  (outIdx=outId, inIdx=get_input_node_idx(nullNodeId))
      return
  let name = graph |> anim_graph_getBlendNodeName(childNode)
  print("'{name}' : {*nod |> anim_blend_node_class_name()} {graph |> anim_graph_getAnimNodeName(*nod |> anim_blend_node_getAnimNodeId)} child mismatch {*childNode |> anim_blend_node_getAnimNodeId} {*childNode |> anim_blend_node_class_name()}")


def get_param_value_str(state : AnimGraphStateHolder; graph : AnimationGraph; paramId : int)
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


def set_anim_char_view_vars(vars_blk : DataBlock)
  param_view_settings |> clear()
  vars_blk |> datablock_for_each_block() <| $(paramBlk)
    var settings : AnimCharViewVarSettings
    settings.enumCls = "{paramBlk |> datablock_getStr("enum", "")}"
    settings.isBool = paramBlk |> datablock_getBool("bool", false)
    settings.trackbar = paramBlk |> datablock_getBool("trackbar", false)
    settings.minVal = paramBlk |> datablock_getReal("min", 0.0)
    settings.maxVal = paramBlk |> datablock_getReal("max", settings.trackbar ? 1.0 : 0.0)
    settings.step = paramBlk |> datablock_getReal("step", 0.01)
    param_view_settings |> insert("{paramBlk |> datablock_getBlockName()}", settings)


def get_enum_items(graph : AnimationGraph; enum_cls : string; var names : array<string>; var values : array<int>)
  let enumListBlk = graph |> anim_graph_getEnumList(enum_cls)
  if enumListBlk == null
    return
  var items : array<tuple<name : string; value : int>>
  *enumListBlk |> datablock_for_each_block() <| $(itemBlk)
    items |> emplace <| (name = "{itemBlk |> datablock_getBlockName()}", value = itemBlk |> datablock_getInt("_enumValue", -1))
  items |> sort() <| $(left; right)
    return left.name < right.name
  for item in items
    names |> push(item.name)
    values |> push(item.value)


def get_enum_name_of_parameter_value(graph : AnimationGraph; state : AnimGraphStateHolder; id : int)
  let paramType = graph |> anim_graph_getParamType(id)
  if paramType == PT_ScalarParamInt
    let value = state |> anim_state_holder_getParamInt(id)
    return animV20_get_enum_name_by_id(value)
  elif paramType == PT_ScalarParam
    let value = state |> anim_state_holder_getParam(id)
    return animV20_get_enum_name_by_id(roundi(value))
  else
    return ""


def draw_param_hint(graph : AnimationGraph; state : AnimGraphStateHolder; id : int; name : string)
  if IsItemHovered() && id >= 0
    imgui_node_editor::Suspend()
    BeginTooltip()
    imgui::Text("Name: \"{name}\"")
    imgui::Text("Param ID: {id}")
    let paramType = graph |> anim_graph_getParamType(id)
    imgui::Text("Param type: {paramType}")
    imgui::Text("Value: {get_param_value_str(state, graph, id)}")
    let enumName = get_enum_name_of_parameter_value(graph, state, id)
    if enumName != ""
      imgui::Text("Possible enum: {enumName}")
    EndTooltip()
    imgui_node_editor::Resume()


def draw_param_info(graph : AnimationGraph; state : AnimGraphStateHolder; paramId : int; name : string)
  let paramName = graph |> anim_graph_getParamName(paramId)
  let displayStr = paramId == -1 ? "---" : "{paramName} = {get_param_value_str(state, graph, paramId)}"
  imgui::Text("[P] {name} ({paramId}): {displayStr}")
  draw_param_hint(graph, state, paramId, paramName)


def get_node_name(graph : AnimationGraph; var nod : IAnimBlendNode?)
  if nod == null
    return "-"
  return graph |> anim_graph_getAnimNodeName(*nod |> anim_blend_node_getAnimNodeId())


[unused_argument(nod, graph, state, context)]
def render_AnimBlendNodeNull(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    assume node = reinterpret<AnimBlendNodeNull?> nod
    if context.drawMode != NodeDrawMode.MINIFIED
      imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
      imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

      let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
      draw_param_info(graph, state, timeScaleParam, "Time scale")


def get_leaf_anim_res_ids(var nodePtr : IAnimBlendNode?) : tuple< id : int; sourceId : int>
  if !*nodePtr |> anim_blend_node_isSubOf(AnimBlendNodeLeafCID)
    return (id = -1, sourceId = -1)

  assume node = unsafe(reinterpret<AnimBlendNodeLeaf?> nodePtr)
  let anim = *node |> AnimBlendNodeLeaf_get_anim()

  let resId = anim.resId
  var sourceResId = -1

  if anim.resId == -1
    let sourceAnim = *anim |> AnimData_get_source_anim_data()
    if sourceAnim != null
      sourceResId = sourceAnim.resId

  return (id = resId, sourceId = sourceResId)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendNodeStillLeaf(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    assume node = reinterpret<AnimBlendNodeStillLeaf?> nod

    let ids = get_leaf_anim_res_ids(nod)
    let resName = get_game_resource_name(ids.id)
    Text("a2d: {resName}")
    if ids.id == -1
      let sourceResName = get_game_resource_name(ids.sourceId)
      imgui::Text("  source a2d: {sourceResName}")

    imgui::Text("isAdditive: {node.isAdditive}")
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    draw_param_info(graph, state, timeScaleParam, "Time scale")

    let pos = *node |> anim_blend_node_getPos()
    imgui::Text("pos: {pos}")


[unused_argument(nod, graph, state, context)]
def render_AnimBlendNodeContinuousLeaf(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    assume node = reinterpret<AnimBlendNodeContinuousLeaf?> nod

    let ids = get_leaf_anim_res_ids(nod)
    let resName = get_game_resource_name(ids.id)
    Text("a2d: {resName}")
    if ids.id == -1
      let sourceResName = get_game_resource_name(ids.sourceId)
      imgui::Text("  source a2d: {sourceResName}")

    imgui::Text("isAdditive: {node.isAdditive}")
    imgui::Text("time: {float(node.dt)/TIME_TicksPerSec}")
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    draw_param_info(graph, state, timeScaleParam, "Time scale")
    let paramId = *node |> anim_blend_node_getParamId()
    draw_param_info(graph, state, paramId, "Param")


[unused_argument(nod, graph, state, context)]
def render_AnimBlendNodeParametricLeaf(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    assume node = reinterpret<AnimBlendNodeParametricLeaf?> nod

    let ids = get_leaf_anim_res_ids(nod)
    let resName = get_game_resource_name(ids.id)
    Text("a2d: {resName}")
    if ids.id == -1
      let sourceResName = get_game_resource_name(ids.sourceId)
      imgui::Text("  source a2d: {sourceResName}")

    imgui::Text("isAdditive: {node.isAdditive}")
    imgui::Text("time: {float(node.dt)/TIME_TicksPerSec}")
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    draw_param_info(graph, state, timeScaleParam, "Time scale")
    let paramId = *node |> anim_blend_node_getParamId()
    draw_param_info(graph, state, paramId, "Param")


[unused_argument(nod, graph, state, context)]
def render_AnimBlendNodeSingleLeaf(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    assume node = reinterpret<AnimBlendNodeSingleLeaf?> nod

    let ids = get_leaf_anim_res_ids(nod)
    let resName = get_game_resource_name(ids.id)
    Text("a2d: {resName}")
    if ids.id == -1
      let sourceResName = get_game_resource_name(ids.sourceId)
      imgui::Text("  source a2d: {sourceResName}")

    imgui::Text("isAdditive: {node.isAdditive}")
    imgui::Text("isAnimationIgnored: {node.isAnimationIgnored}")
    imgui::Text("time: {float(node.dt)/TIME_TicksPerSec}")
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    draw_param_info(graph, state, timeScaleParam, "Time scale")
    let paramId = *node |> anim_blend_node_getParamId()
    draw_param_info(graph, state, paramId, "Param")


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_1axis(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    assume node = reinterpret<AnimBlendCtrl_1axis?> nod
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    draw_param_info(graph, state, timeScaleParam, "Time scale")
    let paramId = *node |> anim_blend_node_getParamId()
    draw_param_info(graph, state, paramId, "Param")

    *node |> AnimBlendCtrl_1axis_getChildren <| $(children)
      for idx, slice in iter_range(children), children
        var childNode = slice.node |> get()
        let outId = get_output_node_idx(nodeId, idx)
        out_pin(outId, "{idx + 1}) {slice.start}..{slice.end} '{get_node_name(graph, childNode)}'") <|
          link_child(graph, nod, outId, childNode, context)


def render_AnimFifo3QueueParam(paramId : int; graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext; drawNodeCb : block<(var nod : IAnimBlendNode?; index : int; weight : float) : void>) // nolint:LINT014
  unsafe
    var queue : AnimFifo3Queue? = reinterpret<AnimFifo3Queue?>(state |> anim_state_holder_getInlinePtr(paramId))

    let gtime = state.anim_state_holder_getParam(graph.anim_graph_getParamId("::GlobalTime"))
    if context.drawMode != NodeDrawMode.MINIFIED
      let stateText = queue.state == 0 ? "No animation" : queue.state == 1 ? "Stable" : "Blending"
      imgui::Text("State: {stateText} ({queue.state})")
      imgui::Text("Blend start time: {queue.t0:.2f}")
      imgui::Text("Current time: {gtime:.2f}")
      imgui::Text("Morph time 1 to 2: {queue.morphTime[0]}")
      imgui::Text("Morph time 2 to 3: {queue.morphTime[1]}")
    let fifoBlendWeight = saturate(safediv((gtime - queue.t0), queue.morphTime[0]))

    for idx in range(3)
      var childNode = *queue |> AnimFifo3Queue_get_node(idx)
      let weights = queue.state == 0 ? fixed_array(0.0, 0.0, 0.0) : queue.state == 1 ? fixed_array(1.0, 0.0, 0.0) : fixed_array(1.0 - fifoBlendWeight, fifoBlendWeight, 0.0)
      invoke(drawNodeCb, childNode, idx, weights[idx])


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_Fifo3(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    assume node = reinterpret<AnimBlendCtrl_Fifo3?> nod
    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    let paramId = *node |> anim_blend_node_getParamId()
    if context.drawMode != NodeDrawMode.MINIFIED
      imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
      imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

      let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
      draw_param_info(graph, state, timeScaleParam, "Time scale")
      var queue : AnimFifo3Queue? = reinterpret<AnimFifo3Queue?>(state |> anim_state_holder_getInlinePtr(paramId))
      draw_param_info(graph, state, paramId, "Param")

    render_AnimFifo3QueueParam(paramId, graph, state, context) <| $(childNode, idx, childWeight)
      let outId = get_output_node_idx(nodeId, idx)
      if context.drawMode != NodeDrawMode.MINIFIED || childWeight > 0.0
        out_pin(outId, "{idx + 1}) '{get_node_name(graph, childNode)}' (rel weight: {childWeight:.2f})") <|
          link_child(graph, nod, outId, childNode, context)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_RandomSwitcher(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    assume node = reinterpret<AnimBlendCtrl_RandomSwitcher?> nod
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    draw_param_info(graph, state, timeScaleParam, "Time scale")
    let paramId = *node |> anim_blend_node_getParamId()
    draw_param_info(graph, state, paramId, "Param")
    let repParamId = *node |> anim_blend_node_getRepParamId()
    draw_param_info(graph, state, repParamId, "Repeat")

    *node |> AnimBlendCtrl_RandomSwitcher_getChildren <| $(children)
      for idx, child in iter_range(children), children
        let outId = get_output_node_idx(nodeId, idx)
        var childNode = child.node |> get()
        out_pin(outId, "{idx + 1}) repeat:{child.maxRepeat} wt: {child.rndWt}\n'{get_node_name(graph, childNode)}'") <|
          link_child(graph, nod, outId, childNode, context)


def private undoable_drag_float(widget_id : string; value : float; speed : float) : tuple<changed : bool; committed : bool; newValue : float; oldValue : float>
  static_let <|
    var edit_start_value = 0.0
  var v = value
  let minVal = -1e8
  let maxVal = 1e8
  let changed = imgui::DragScalar(widget_id, ImGuiDataType.Float, safe_addr(v), speed, safe_addr(minVal), safe_addr(maxVal), "%.3f", ImGuiSliderFlags.None)
  if imgui::IsItemActivated()
    edit_start_value = value
  let committed = imgui::IsItemDeactivatedAfterEdit()
  return (changed=changed, committed=committed, newValue=v, oldValue=edit_start_value)


def private node_select_combo(context : AnimgraphContext; id_suffix : string; var selected_index : int&)
  static_let_finalize <|
    var nodeSelectFilter = new ImGuiTextFilter

  if length(context.nodeNames) == 0
    return

  // Hacky way to not crash when selected anim tree changes
  if selected_index >= length(context.nodeNames)
    selected_index = 0

  let maxComboSize = 200.0
  imgui::SetNextWindowSizeConstraints(ImVec2(0.0, 0.0), ImVec2(FLT_MAX, FLT_MAX))
  if imgui::BeginCombo("Node##nodesel_{id_suffix}", context.nodeNames[selected_index])
    *nodeSelectFilter |> Draw("##nodeselfilter_{id_suffix}")

    var filteredNodeIndexes : array<int>
    filteredNodeIndexes |> reserve(length(context.nodeNames))
    for i in range(length(context.nodeNames))
      if *nodeSelectFilter |> PassFilter(context.nodeNames[i])
        filteredNodeIndexes |> push(i)

    imgui::SetNextWindowSizeConstraints(ImVec2(0.0, 0.0), ImVec2(FLT_MAX, maxComboSize))
    if imgui::BeginChild("##nodeseloptions_{id_suffix}", ImVec2(-FLT_MIN, 0.0), ImGuiChildFlags.AutoResizeY | ImGuiChildFlags.AlwaysAutoResize)
      using() <| $(var clipper : ImGuiListClipper)
        Begin(clipper, length(filteredNodeIndexes), imgui::GetTextLineHeightWithSpacing())
        while Step(clipper)
          for i in range(clipper.DisplayStart, clipper.DisplayEnd)
            let isSelected = selected_index == filteredNodeIndexes[i]
            if imgui::Selectable(context.nodeNames[filteredNodeIndexes[i]], isSelected)
              selected_index = filteredNodeIndexes[i]
              imgui::CloseCurrentPopup()

      imgui::EndChild()
    imgui::EndCombo()


def private hub_find_child_idx_from_end(var hub : AnimBlendCtrl_Hub; child : IAnimBlendNode?) : int
  var i = length(hub.nodes) - 1
  while i >= 0
    if hub.nodes[i] |> get() == child
      return i
    i -= 1
  return -1


// Finds the child by identity (node + p0), sorting by p0 moves items around so a stored index is only a fast path guess
def private linear_poly_find_child_idx(var ctrl : AnimBlendCtrl_LinearPoly; child : IAnimBlendNode?; p0 : float; guess_idx : int) : int
  if guess_idx >= 0 && guess_idx < length(ctrl.poly) && ctrl.poly[guess_idx].node |> get() == child && ctrl.poly[guess_idx].p0 == p0
    return guess_idx
  for i in range(length(ctrl.poly))
    if ctrl.poly[i].node |> get() == child && ctrl.poly[i].p0 == p0
      return i
  return -1


def private param_switcher_find_child_idx_from_end(var ctrl : AnimBlendCtrl_ParametricSwitcher; child : IAnimBlendNode?) : int
  var i = length(ctrl.list) - 1
  while i >= 0
    if ctrl.list[i].node |> get() == child
      return i
    i -= 1
  return -1


def private apply_graph_action(act : UndoRedoAction; is_redo : bool; var graph : AnimationGraph)
  unsafe
    // actions address nodes by name, resolve them against the currently viewed graph and skip actions which no longer resolve
    var ctrl = graph |> anim_graph_getBlendNodePtr(graph |> anim_graph_getBlendNodeId(act.ctrlName))
    var child = act.childName != "" ? graph |> anim_graph_getBlendNodePtr(graph |> anim_graph_getBlendNodeId(act.childName)) : null
    if ctrl == null || (act.childName != "" && child == null)
      visual_log("animgraph undo/redo: can't resolve '{act.ctrlName}'/'{act.childName}', skipping '{act.name}'")
      return

    if act.kind == UndoRedoActionKind.HUB_ADD || act.kind == UndoRedoActionKind.HUB_REMOVE
      assume hub = *reinterpret<AnimBlendCtrl_Hub?> ctrl
      if (act.kind == UndoRedoActionKind.HUB_ADD) == is_redo
        hub |> anim_blend_node_addBlendNode(child, true, act.weight)
      else
        var idx = act.childIdx
        if idx < 0 || idx >= length(hub.nodes) || hub.nodes[idx] |> get() != child
          idx = hub_find_child_idx_from_end(hub, child)
        if idx >= 0
          hub.nodes |> erase(idx)
          hub.defNodeWt |> erase(idx)

    elif act.kind == UndoRedoActionKind.LINEAR_POLY_ADD || act.kind == UndoRedoActionKind.LINEAR_POLY_REMOVE
      assume lp = *reinterpret<AnimBlendCtrl_LinearPoly?> ctrl
      if (act.kind == UndoRedoActionKind.LINEAR_POLY_ADD) == is_redo
        lp |> anim_blend_node_addBlendNode(child, act.p0, graph, act.ctrlName)
        lp |> anim_blend_node_sortBlendNodes()
      else
        let idx = linear_poly_find_child_idx(lp, child, act.p0, act.childIdx)
        if idx >= 0
          lp.poly |> erase(idx)

    elif act.kind == UndoRedoActionKind.LINEAR_POLY_SET_P0
      assume lp = *reinterpret<AnimBlendCtrl_LinearPoly?> ctrl
      let curValue = is_redo ? act.oldValue : act.newValue
      let idx = linear_poly_find_child_idx(lp, child, curValue, act.childIdx)
      if idx >= 0
        lp.poly[idx].p0 = is_redo ? act.newValue : act.oldValue
        lp |> anim_blend_node_sortBlendNodes()

    elif act.kind == UndoRedoActionKind.PARAM_SWITCH_ADD || act.kind == UndoRedoActionKind.PARAM_SWITCH_REMOVE
      assume ps = *reinterpret<AnimBlendCtrl_ParametricSwitcher?> ctrl
      if (act.kind == UndoRedoActionKind.PARAM_SWITCH_ADD) == is_redo
        ps |> anim_blend_node_addBlendNode(child, act.start, act.end, act.baseVal, graph, act.ctrlName)
      else
        var idx = act.childIdx
        if idx < 0 || idx >= length(ps.list) || ps.list[idx].node |> get() != child
          idx = param_switcher_find_child_idx_from_end(ps, child)
        if idx >= 0
          ps.list |> erase(idx)

    elif act.kind == UndoRedoActionKind.PARAM_SWITCH_SET_START
      assume ps = *reinterpret<AnimBlendCtrl_ParametricSwitcher?> ctrl
      if act.childIdx >= 0 && act.childIdx < length(ps.list)
        let curEnd = ps.list[act.childIdx] |> AnimBlendCtrl_ParametricSwitcherItemAnim_getEnd()
        ps.list[act.childIdx] |> AnimBlendCtrl_ParametricSwitcherItemAnim_setRange(is_redo ? act.newValue : act.oldValue, curEnd)

    elif act.kind == UndoRedoActionKind.PARAM_SWITCH_SET_END
      assume ps = *reinterpret<AnimBlendCtrl_ParametricSwitcher?> ctrl
      if act.childIdx >= 0 && act.childIdx < length(ps.list)
        let curStart = ps.list[act.childIdx] |> AnimBlendCtrl_ParametricSwitcherItemAnim_getStart()
        ps.list[act.childIdx] |> AnimBlendCtrl_ParametricSwitcherItemAnim_setRange(curStart, is_redo ? act.newValue : act.oldValue)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_Hub(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    assume node = *reinterpret<AnimBlendCtrl_Hub?> nod
    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    let paramId = node |> anim_blend_node_getParamId()
    let constWeights = paramId < 0
    if context.drawMode != NodeDrawMode.MINIFIED
      imgui::Text("duration: {node |> anim_blend_node_getDuration(state)}")
      imgui::Text("avg speed: {node |> anim_blend_node_getAvgSpeed(state)}")

      let timeScaleParam = node |> anim_blend_node_getTimeScaleParamId(state)
      draw_param_info(graph, state, timeScaleParam, "Time scale")
      draw_param_info(graph, state, paramId, "Param")

    imgui::Text("const: {constWeights}")
    imgui::SameLine()
    if constWeights
      imgui::PushStyleColor(ImGuiCol.Text, float4(0.0, 1.0, 0.0, 1.0))
      imgui::Text("(editable)")
      imgui::PopStyleColor()
    else
      imgui::PushStyleColor(ImGuiCol.Text, float4(1.0, 0.0, 0.0, 1.0))
      imgui::Text("(not editable)")
      imgui::PopStyleColor()

    imgui::SameLine()
    HelpMarkerInNode("When hub has const:b=false (default) a parameter is registered which contains the weight for each child node.
This allows to overwrite each node weight in a hub externally.
For example, this is used to disable some IK nodes when soldiers are far away(optimisation).
Adding and removing nodes through anim graph viewer is only supported for const hubs.
")


    var removeNodeId = -1

    node |> AnimBlendCtrl_Hub_getDefNodeWt <| $(weights)
      node |> AnimBlendCtrl_Hub_getChildren <| $(children)
        for idx, child, wt in iter_range(children), children, weights
          let outId = get_output_node_idx(nodeId, idx)

          var childNode = child |> get()
          let text = constWeights ? "{idx + 1}) wt:{wt} '{get_node_name(graph, childNode)}'" : "{idx + 1}) '{get_node_name(graph, childNode)}'"
          out_pin(outId, text) <|
            if constWeights
              imgui::SameLine()
              if imgui::Button("X##hub_{nodeId}_remove_node_{idx}")
                removeNodeId = idx
            link_child(graph, nod, outId, childNode, context)

    if removeNodeId != -1
      var childPtr = node.nodes[removeNodeId] |> get()
      var act : UndoRedoAction
      act.kind = UndoRedoActionKind.HUB_REMOVE
      act.ctrlName = "{graph |> anim_graph_getBlendNodeName(nod)}"
      act.childName = "{graph |> anim_graph_getBlendNodeName(childPtr)}"
      act.childIdx = removeNodeId
      act.weight = node.defNodeWt[removeNodeId]
      act.name = "hub: remove '{get_node_name(graph, childPtr)}'"
      context.pendingActions |> push(act)

    if constWeights
      let addNewButtonClicked = imgui::Button("Add Node##hubaddnodepoupup_{nodeId}")

      imgui_node_editor::Suspend()

      if addNewButtonClicked
        imgui::OpenPopup("hub_add_node_{nodeId}")

      // imgui node editor can not handle combo creation inside a node so we suspend the editor, create a popup and add node combo there
      if imgui::BeginPopup("hub_add_node_{nodeId}")
        static_let <|
          var newNodeSelectedIndex = 0
          var newNodeWeight = 1.0

        node_select_combo(context, "hubadd{nodeId}", newNodeSelectedIndex)

        imgui::InputFloat("Weight##hubaddnodeweight{nodeId}", safe_addr(newNodeWeight))

        if imgui::Button("Add##hubaddnode{nodeId}")
          var childPtr = graph |> anim_graph_getBlendNodePtr(context.nodeIds[newNodeSelectedIndex])
          if childPtr != null
            var act : UndoRedoAction
            act.kind = UndoRedoActionKind.HUB_ADD
            act.name = "hub: add '{context.nodeNames[newNodeSelectedIndex]}'"
            act.ctrlName = "{graph |> anim_graph_getBlendNodeName(nod)}"
            act.childName = "{graph |> anim_graph_getBlendNodeName(childPtr)}"
            act.childIdx = -1
            act.weight = newNodeWeight
            context.pendingActions |> push(act)

        imgui::EndPopup()

      imgui_node_editor::Resume()

[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_Blender(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    assume node = *reinterpret<AnimBlendCtrl_Blender?> nod
    imgui::Text("blendTime: {node.blendTime}")
    imgui::Text("duration: {node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    let timeScaleParam = node |> anim_blend_node_getTimeScaleParamId(state)
    draw_param_info(graph, state, timeScaleParam, "Time scale")
    let paramId = node |> anim_blend_node_getParamId()
    draw_param_info(graph, state, paramId, "Param")

    node |> AnimBlendCtrl_Blender_getChildren <| $(children)
      for idx, child in iter_range(children), children
        let outId = get_output_node_idx(nodeId, idx)
        var childNode = child |> get()
        out_pin(outId, "{idx + 1}) '{get_node_name(graph, childNode)}'") <|
          link_child(graph, nod, outId, childNode, context)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_BinaryIndirectSwitch(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    assume node = *reinterpret<AnimBlendCtrl_BinaryIndirectSwitch?> nod
    imgui::Text("duration: {node |> anim_blend_node_getDuration(state)}")
    imgui::Text("time scale param: {node |> anim_blend_node_getTimeScaleParamId(state)}")
    imgui::Text("avg speed: {node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    let timeScaleParam = node |> anim_blend_node_getTimeScaleParamId(state)
    draw_param_info(graph, state, timeScaleParam, "Time scale")
    let paramId = node |> anim_blend_node_getParamId()
    draw_param_info(graph, state, paramId, "Param")

    node |> AnimBlendCtrl_BinaryIndirectSwitch_getChildren <| $(children)
      for idx, child in iter_range(children), children
        let outId = get_output_node_idx(nodeId, idx)
        var childNode = child |> get()
        out_pin(outId, "{idx + 1}) '{get_node_name(graph, childNode)}'") <|
          link_child(graph, nod, outId, childNode, context)


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_SetMotionMatchingTag(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    assume node = *reinterpret<AnimBlendCtrl_SetMotionMatchingTag?> nod
    imgui::Text("tag: {node.tagName}")


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_LinearPoly(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    assume node = *reinterpret<AnimBlendCtrl_LinearPoly?> nod
    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    if context.drawMode != NodeDrawMode.MINIFIED
      imgui::Text("duration: {node |> anim_blend_node_getDuration(state)}")
      imgui::Text("avg speed: {node |> anim_blend_node_getAvgSpeed(state)}")

      let timeScaleParam = node |> anim_blend_node_getTimeScaleParamId(state)
      draw_param_info(graph, state, timeScaleParam, "Time scale")
      let paramId = node |> anim_blend_node_getParamId()
      draw_param_info(graph, state, paramId, "Param")

    var removeIdx = -1
    var sortNeeded = false
    for idx in range(length(node.poly))
      let outId = get_output_node_idx(nodeId, idx)
      var childNode = node.poly[idx].node |> get()
      if context.drawMode == NodeDrawMode.MINIFIED && nodeId != -1 && context.weights[nodeId] == 0.0
        continue
      if context.drawMode == NodeDrawMode.MINIFIED
        out_pin(outId, "{idx + 1}) p0:{node.poly[idx].p0} wtPid:{node.poly[idx].wtPid}\n'{get_node_name(graph, childNode)}'") <|
          link_child(graph, nod, outId, childNode, context)
      else
        out_pin(outId, "{idx + 1}) wtPid:{node.poly[idx].wtPid} '{get_node_name(graph, childNode)}'") <|
          link_child(graph, nod, outId, childNode, context)
          imgui::Text("p0:")
          imgui::SameLine()
          SetNextItemWidth(80.0)
          let r = undoable_drag_float("##lp_p0_{nodeId}_{idx}", node.poly[idx].p0, 0.01)
          if r.changed
            node.poly[idx].p0 = r.newValue
          if r.committed
            var act : UndoRedoAction
            act.kind = UndoRedoActionKind.LINEAR_POLY_SET_P0
            act.name = "linearPoly: p0 {r.oldValue} -> {r.newValue}"
            act.alreadyApplied = true
            act.ctrlName = "{graph |> anim_graph_getBlendNodeName(nod)}"
            act.childName = "{graph |> anim_graph_getBlendNodeName(childNode)}"
            act.childIdx = idx
            act.oldValue = r.oldValue
            act.newValue = r.newValue
            context.pendingActions |> push(act)
            sortNeeded = true
          imgui::SameLine()
          if imgui::Button("X##lp_remove_{nodeId}_{idx}")
            removeIdx = idx

    if removeIdx != -1
      var childPtr = node.poly[removeIdx].node |> get()
      var act : UndoRedoAction
      act.kind = UndoRedoActionKind.LINEAR_POLY_REMOVE
      act.needsStateReset = true // undo re-adds the node which may register a new weight param
      act.ctrlName = "{graph |> anim_graph_getBlendNodeName(nod)}"
      act.childName = "{graph |> anim_graph_getBlendNodeName(childPtr)}"
      act.childIdx = removeIdx
      act.p0 = node.poly[removeIdx].p0
      act.name = "linearPoly: remove '{get_node_name(graph, childPtr)}'"
      context.pendingActions |> push(act)

    // sorting during an active drag would move items under the cursor, so only sort once the edit is committed
    if sortNeeded
      node |> anim_blend_node_sortBlendNodes()

    if context.drawMode != NodeDrawMode.MINIFIED
      let addNewButtonClicked = imgui::Button("Add Node##lpaddnode_{nodeId}")

      imgui_node_editor::Suspend()

      if addNewButtonClicked
        imgui::OpenPopup("lp_add_node_{nodeId}")

      if imgui::BeginPopup("lp_add_node_{nodeId}")
        static_let <|
          var lpNewNodeSelectedIndex = 0
          var lpNewNodeP0 = 0.0

        node_select_combo(context, "lpadd{nodeId}", lpNewNodeSelectedIndex)
        imgui::InputFloat("p0##lpaddnodep0_{nodeId}", safe_addr(lpNewNodeP0))

        if imgui::Button("Add##lpaddnodebtn_{nodeId}")
          var childPtr = graph |> anim_graph_getBlendNodePtr(context.nodeIds[lpNewNodeSelectedIndex])
          if childPtr != null
            var act : UndoRedoAction
            act.kind = UndoRedoActionKind.LINEAR_POLY_ADD
            act.name = "linearPoly: add '{context.nodeNames[lpNewNodeSelectedIndex]}'"
            act.needsStateReset = true // addBlendNode registers a new weight param
            act.ctrlName = "{graph |> anim_graph_getBlendNodeName(nod)}"
            act.childName = "{graph |> anim_graph_getBlendNodeName(childPtr)}"
            act.childIdx = -1
            act.p0 = lpNewNodeP0
            context.pendingActions |> push(act)

        imgui::EndPopup()

      imgui_node_editor::Resume()


[unused_argument(nod, graph, state, context)]
def render_AnimBlendCtrl_ParametricSwitcher(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    var node = reinterpret<AnimBlendCtrl_ParametricSwitcher?> nod
    if context.drawMode != NodeDrawMode.MINIFIED
      imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
      imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let nodeId = *nod |> anim_blend_node_getAnimNodeId()
    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    let paramId = *node |> anim_blend_node_getParamId()
    if context.drawMode != NodeDrawMode.MINIFIED
      draw_param_info(graph, state, timeScaleParam, "Time scale")
      draw_param_info(graph, state, paramId, "Param")
      draw_param_info(graph, state, node.residualParamId, "Residual")
      draw_param_info(graph, state, node.lastAnimParamId, "Last Anim")
      draw_param_info(graph, state, node.fifoParamId, "Fifo")
      imgui::Text("Morph time: {node.morphTime}")
      imgui::Text("Continuous anim mode: {node.continuousAnimMode}")

      imgui::Text("Fifo:")
      if node.fifoParamId >= 0
        let indent = 10.0
        imgui::Indent(indent)
        render_AnimFifo3QueueParam(node.fifoParamId, graph, state, context) <| $(childNode, idx, childWeight)
          imgui::Text("{idx + 1}) '{get_node_name(graph, childNode)}' (rel weight: {childWeight:.2f})")
        imgui::Unindent(indent)

    imgui::Text("Children:")
    imgui::PushStyleVar(ImGuiStyleVar.ItemSpacing, float2(0.0, 10.0))
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
        // For some reason we have null nodes with abnId = -1 which is veery strange, so check that
        if context.drawMode == NodeDrawMode.MINIFIED && (childNode == null || *childNode |> anim_blend_node_getAnimNodeId() == -1 || context.weights[*childNode |> anim_blend_node_getAnimNodeId()] == 0.0)
          continue
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
              TextUnformatted("{j}) {paramName} param range: [{start}; {end}]")
            EndTooltip()
            imgui_node_editor::Resume()

    imgui::PopStyleVar()

    if context.drawMode != NodeDrawMode.MINIFIED
      let editItemsClicked = imgui::Button("Edit Items##psedititems_{nodeId}")

      imgui_node_editor::Suspend()

      if editItemsClicked
        imgui::OpenPopup("ps_edit_items_{nodeId}")

      if imgui::BeginPopup("ps_edit_items_{nodeId}")
        var removeIdx = -1
        for idx in range(length(node.list))
          var childNode = node.list[idx].node |> get()
          imgui::Text("{idx + 1}) '{get_node_name(graph, childNode)}'")
          imgui::SameLine()

          let curStart = node.list[idx] |> AnimBlendCtrl_ParametricSwitcherItemAnim_getStart()
          let curEnd = node.list[idx] |> AnimBlendCtrl_ParametricSwitcherItemAnim_getEnd()

          SetNextItemWidth(70.0)
          let rs = undoable_drag_float("start##ps_start_{nodeId}_{idx}", curStart, 0.01)
          if rs.changed
            node.list[idx] |> AnimBlendCtrl_ParametricSwitcherItemAnim_setRange(rs.newValue, curEnd)
          if rs.committed
            var act : UndoRedoAction
            act.kind = UndoRedoActionKind.PARAM_SWITCH_SET_START
            act.name = "paramSwitch: start {rs.oldValue} -> {rs.newValue}"
            act.alreadyApplied = true
            act.ctrlName = "{graph |> anim_graph_getBlendNodeName(nod)}"
            act.childIdx = idx
            act.oldValue = rs.oldValue
            act.newValue = rs.newValue
            context.pendingActions |> push(act)

          imgui::SameLine()
          SetNextItemWidth(70.0)
          let re = undoable_drag_float("end##ps_end_{nodeId}_{idx}", curEnd, 0.01)
          if re.changed
            node.list[idx] |> AnimBlendCtrl_ParametricSwitcherItemAnim_setRange(node.list[idx] |> AnimBlendCtrl_ParametricSwitcherItemAnim_getStart(), re.newValue)
          if re.committed
            var act : UndoRedoAction
            act.kind = UndoRedoActionKind.PARAM_SWITCH_SET_END
            act.name = "paramSwitch: end {re.oldValue} -> {re.newValue}"
            act.alreadyApplied = true
            act.ctrlName = "{graph |> anim_graph_getBlendNodeName(nod)}"
            act.childIdx = idx
            act.oldValue = re.oldValue
            act.newValue = re.newValue
            context.pendingActions |> push(act)

          imgui::SameLine()
          if imgui::Button("X##ps_remove_{nodeId}_{idx}")
            removeIdx = idx

        if removeIdx != -1
          var childPtr = node.list[removeIdx].node |> get()
          var act : UndoRedoAction
          act.kind = UndoRedoActionKind.PARAM_SWITCH_REMOVE
          act.needsStateReset = true // undo re-adds the item which may register a rewind bitmap param
          act.ctrlName = "{graph |> anim_graph_getBlendNodeName(nod)}"
          act.childName = "{graph |> anim_graph_getBlendNodeName(childPtr)}"
          act.childIdx = removeIdx
          act.start = node.list[removeIdx] |> AnimBlendCtrl_ParametricSwitcherItemAnim_getStart()
          act.end = node.list[removeIdx] |> AnimBlendCtrl_ParametricSwitcherItemAnim_getEnd()
          act.baseVal = node.list[removeIdx].baseVal
          act.name = "paramSwitch: remove '{get_node_name(graph, childPtr)}'"
          context.pendingActions |> push(act)

        imgui::Separator()
        imgui::Text("Add Item:")
        static_let <|
          var psNewNodeSelectedIndex = 0
          var psNewStart = 0.0
          var psNewEnd = 1.0

        node_select_combo(context, "psadd{nodeId}", psNewNodeSelectedIndex)
        imgui::InputFloat("start##psaddstart_{nodeId}", safe_addr(psNewStart))
        imgui::InputFloat("end##psaddend_{nodeId}", safe_addr(psNewEnd))

        if imgui::Button("Add##psaddbtn_{nodeId}")
          var childPtr = graph |> anim_graph_getBlendNodePtr(context.nodeIds[psNewNodeSelectedIndex])
          if childPtr != null
            var act : UndoRedoAction
            act.kind = UndoRedoActionKind.PARAM_SWITCH_ADD
            act.name = "paramSwitch: add '{context.nodeNames[psNewNodeSelectedIndex]}'"
            act.needsStateReset = true // addBlendNode may register a rewind bitmap param
            act.ctrlName = "{graph |> anim_graph_getBlendNodeName(nod)}"
            act.childName = "{graph |> anim_graph_getBlendNodeName(childPtr)}"
            act.childIdx = -1
            act.start = psNewStart
            act.end = psNewEnd
            act.baseVal = 0.0
            context.pendingActions |> push(act)

        imgui::EndPopup()

      imgui_node_editor::Resume()


// [unused_argument(nod, graph, state, context)]
// def render_AnimBlendNodeLeaf(var nod: IAnimBlendNode?; var graph: AnimationGraph; var state: AnimGraphStateHolder; var context: AnimgraphContext)
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


[unused_argument(context)]
def render_general_post_blend_controller(var nod : IAnimBlendNode?; graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  unsafe
    assume node = reinterpret<AnimPostBlendCtrl?> nod
    var debugContext = context.animchar.animcharDebugContext
    var hasWtOverride = animchar_debug_getPbcOverrideEnabled(*debugContext, node.pbcId)
    if imgui::Checkbox("Force disable##pbcWtOverride_{node.pbcId}", safe_addr(hasWtOverride))
      if hasWtOverride
        animchar_debug_setPbcOverride(*debugContext, node.pbcId, 0.0)
      else
        animchar_debug_disablePbcOverride(*debugContext, node.pbcId)
    imgui::Text("duration: {*node |> anim_blend_node_getDuration(state)}")
    imgui::Text("avg speed: {*node |> anim_blend_node_getAvgSpeed(state)}")

    let timeScaleParam = *node |> anim_blend_node_getTimeScaleParamId(state)
    draw_param_info(graph, state, timeScaleParam, "Time scale")


[unused_argument(nod, graph, state, context)]
def render_AnimPostBlendCtrl(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  if context.drawMode == NodeDrawMode.MINIFIED
    return
  render_general_post_blend_controller(nod, graph, state, context)
  imgui::Spacing()
  imgui::Spacing()
  imgui::Text("--- Missing node specific render ---\nThis node has more parameters than displayed")


def render_DasAnimPostBlendCtrl(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  if context.drawMode == NodeDrawMode.MINIFIED
    return
  unsafe
    render_general_post_blend_controller(nod, graph, state, context)
    assume node = reinterpret<DasAnimPostBlendCtrl?> nod
    *node |> das_pbc_renderDebug(state)


def render_AnimPostBlendSetParam(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  if context.drawMode == NodeDrawMode.MINIFIED
    return
  unsafe
    render_general_post_blend_controller(nod, graph, state, context)
    assume node = reinterpret<AnimPostBlendSetParam?> nod
    imgui::Spacing()
    let step = 0.1f
    imgui::Text("Min Weight:")
    imgui::SameLine()
    SetNextItemWidth(150.0f)
    InputScalar("##{node}_minWeight", ImGuiDataType.Float, unsafe(addr(node.minWeight)), safe_addr(step), safe_addr(step), "%.4f", ImGuiInputTextFlags.None)
    imgui::Text("Source Value: {node.val}")
    draw_param_info(graph, state, node.srcVarId, "Source Param")
    imgui::Text("Destination Var Name: {node.destVarName}")
    imgui::Text("Destination Slot: {node.destSlotId} ({animchar_getSlotName(node.destSlotId)})")
    draw_param_info(graph, state, node.destVarId, "Destination Param:")
    imgui::Text("Is Pull: {node.pullParamValueFromSlot}")


def render_AnimPostBlendParamFromNode(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  if context.drawMode == NodeDrawMode.MINIFIED
    return
  unsafe
    render_general_post_blend_controller(nod, graph, state, context)
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


def render_AttachGeomNodeCtrl(var nod : IAnimBlendNode?; var graph : AnimationGraph; var state : AnimGraphStateHolder; var context : AnimgraphContext)
  if context.drawMode == NodeDrawMode.MINIFIED
    return
  unsafe
    render_general_post_blend_controller(nod, graph, state, context)
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


def param_type_to_text(typ : uint)
  let text = fixed_array<string>(
    "PT_Reserved",
    "PT_ScalarParam",
    "PT_ScalarParamInt",
    "PT_TimeParam",
    "PT_InlinePtr",
    "PT_InlinePtrCTZ",
    "PT_Fifo3",
    "PT_Effector"
  )
  if int(typ) > length(text)
    return "Unknown type id: {typ}"
  return text[typ]

let anim_tree_class_names = fixed_array<string>(
    // "continuous",
    // "single",
    // "still",
    // "parametric",
    "fifo3",
    "linear",
    "linearPoly",
    "randomSwitch",
    "paramSwitch",
    "paramSwitchS",
    "setMotionMatchingTag",
    "hub",
    "null",
    "stub",
    "blender",
    "BIS",
    "deltaAnglesCalc",
    "alignNode",
    "alignEx",
    "rotateNode",
    "rotateAroundNode",
    "scaleNode",
    "moveNode",
    "paramsCtrl",
    "defClampCtrl",
    "condHide",
    "animateNode",
    "legsIK",
    "twoBonesIK",
    "multiChainFABRIK",
    "attachNode",
    "aim",
    "lookat",
    "lookatNode",
    "effFromAttachment",
    "nodesFromAttachement",
    "paramFromNode",
    "matFromNode",
    "compoundRotateShift",
    "setParam",
    "twistCtrl",
    "eyeCtrl",
    "effectorFromChildIK",
    "deltaRotateShiftCalc",
    "footLockerIK",
    "hasAttachment",
    "humanAim",
  )

def class_name_to_anim_tree_class_name(class_name : string)
  // bnl
  if class_name == "AnimBlendNodeContinuousLeaf"
    return "continuous"

  if class_name == "AnimBlendNodeSingleLeaf"
    return "single"

  if class_name == "AnimBlendNodeStillLeaf"
    return "still"

  if class_name == "AnimBlendNodeParametricLeaf"
    return "parametric"

  // bn
  if class_name == "AnimBlendCtrl_Fifo3"
    return "fifo3"

  if class_name == "AnimBlendCtrl_1axis"
    return "linear"

  if class_name == "AnimBlendCtrl_LinearPoly"
    return "linearPoly"

  if class_name == "AnimBlendCtrl_RandomSwitcher"
    return "randomSwitch"

  if class_name == "AnimBlendCtrl_ParametricSwitcher"
    return "paramSwitch"

  if class_name == "AnimBlendCtrl_ParametricSwitcher"
    return "paramSwitchS"

  if class_name == "AnimBlendCtrl_SetMotionMatchingTag"
    return "setMotionMatchingTag"

  if class_name == "AnimHubCtrl"
    return "hub"

  if class_name == "AnimBlendNodeNull"
    return "null"

  if class_name == "AnimBlendNodeNull"
    return "stub"

  if class_name == "AnimBlendCtrl_Blender"
    return "blender"

  if class_name == "AnimBISCtrl"
    return "BIS"

  if class_name == "DeltaAnglesCtrl"
    return "deltaAnglesCalc"

  if class_name == "AnimPostBlendAlignCtrl"
    return "alignNode"

  if class_name == "AnimPostBlendAlignExCtrl"
    return "alignEx"

  if class_name == "AnimPostBlendRotateCtrl"
    return "rotateNode"

  if class_name == "AnimPostBlendRotateAroundCtrl"
    return "rotateAroundNode"

  if class_name == "AnimPostBlendScaleCtrl"
    return "scaleNode"

  if class_name == "AnimPostBlendMoveCtrl"
    return "moveNode"

  if class_name == "ApbParamCtrl"
    return "paramsCtrl"

  if class_name == "DefClampParamCtrl"
    return "defClampCtrl"

  if class_name == "AnimPostBlendCondHideCtrl"
    return "condHide"

  if class_name == "ApbAnimateCtrl"
    return "animateNode"

  if class_name == "LegsIKCtrl"
    return "legsIK"

  if class_name == "AnimPostBlendTwoBonesIK"
    return "twoBonesIK"

  if class_name == "MultiChainFABRIKCtrl"
    return "multiChainFABRIK"

  if class_name == "AttachGeomNodeCtrl"
    return "attachNode"

  if class_name == "AnimPostBlendAimCtrl"
    return "aim"

  if class_name == "AnimPostBlendNodeLookatCtrl"
    return "lookat"

  if class_name == "AnimPostBlendNodeLookatNodeCtrl"
    return "lookatNode"

  if class_name == "AnimPostBlendEffFromAttachement"
    return "effFromAttachment"

  if class_name == "AnimPostBlendNodeEffectorFromChildIK"
    return "nodesFromAttachement"

  if class_name == "AnimPostBlendParamFromNode"
    return "paramFromNode"

  if class_name == "AnimPostBlendMatVarFromNode"
    return "matFromNode"

  if class_name == "AnimPostBlendCompoundRotateShift"
    return "compoundRotateShift"

  if class_name == "AnimPostBlendSetParam"
    return "setParam"

  if class_name == "AnimPostBlendTwistCtrl"
    return "twistCtrl"

  if class_name == "AnimPostBlendEyeCtrl"
    return "eyeCtrl"

  if class_name == "AnimPostBlendNodeEffectorFromChildIK"
    return "effectorFromChildIK"

  if class_name == "DeltaRotateShiftCtrl"
    return "deltaRotateShiftCalc"

  if class_name == "FootLockerIKCtrl"
    return "footLockerIK"

  if class_name == "AnimPostBlendHasAttachment"
    return "hasAttachment"

  if class_name == "AnimPostBlendHumanAimCtrl"
    return "humanAim"

  return "??? (Missing information about this class name)"

def render_anim_graph(var animchar : AnimcharBaseComponent; var reset_state : bool&)
  assume graph = *animchar.animGraph;
  assume state = *animchar.animState;
  assume history = graph_edit_history[graph.resId]

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

  push(renderers, tuple(AnimPostBlendSetParamCID, @@render_AnimPostBlendSetParam))
  push(renderers, tuple(AnimPostBlendParamFromNodeCID, @@render_AnimPostBlendParamFromNode))
  push(renderers, tuple(AttachGeomNodeCtrlCID, @@render_AttachGeomNodeCtrl))
  push(renderers, tuple(DasAnimPostBlendCtrlCID, @@render_DasAnimPostBlendCtrl))
  push(renderers, tuple(AnimPostBlendCtrlCID, @@render_AnimPostBlendCtrl)) // should be last as this is a generic pbc renderer

  var ctx : AnimgraphContext
  ctx.animchar = unsafe(addr(animchar))

  imgui::Columns(2, "__params__")

  var autoAlign = false
  var focusContent = false
  var focusRoot = false

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
    var pbcListShowNodeClass = false
    var pbcListShowWeight = false
    var pbcListShowActiveOnly = true
    var bnlListShowActiveOnly = true
    var disableAutoAlign = false
    var paramsWidth = 280f
    var nodeDrawMode = NodeDrawMode.FULL

  static_let_finalize <|
    var skeletonFilter = new ImGuiTextFilter
    var animGraphNodesFilter = new ImGuiTextFilter
    var paramsFilter = new ImGuiTextFilter
    var statesFilter = new ImGuiTextFilter
    var slotsFilter = new ImGuiTextFilter
    var enumsFilter = new ImGuiTextFilter
    var enumParamComboFilter = new ImGuiTextFilter

  if !bookmarksLoaded
    bookmarksLoaded = true
    bookmarkedStates |> clear()
    bookmarkedParams |> clear()

    let load = $ <| (name : string; var bookmarks : array<string>)
      imgui_get_blk() |> datablock_get_block(name) <| $(blk)
        blk |> datablock_for_each_block() <| $(stateBlk)
          let bookmarkName = stateBlk |> datablock_getBlockName()
          if bookmarkName != "" && bookmarks |> find_index(bookmarkName) < 0
            bookmarks |> push(clone_string(bookmarkName))

    invoke(load, "animgraph_viewer_bookmarked_states", bookmarkedStates)
    invoke(load, "animgraph_viewer_bookmarked_params", bookmarkedParams)

    // ImGui legacy Columns width does not persist between launches, so we store our own value in the imgui config blk.
    paramsWidth = imgui_get_blk_val("animgraph_viewer", "paramsWidth", paramsWidth)
    imgui::SetColumnWidth(0, paramsWidth)

  let curParamsWidth = imgui::GetColumnWidth(0)
  if abs(curParamsWidth - paramsWidth) > 0.5
    paramsWidth = curParamsWidth
    imgui_set_blk_val("animgraph_viewer", "paramsWidth", paramsWidth)
    imgui_save_blk()

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

  if context == null
    var config = Config()
    // config.SettingsFile = "Simple.json"
    context = CreateEditor(safe_addr(config))

  SetCurrentEditor(context)

  imgui::BeginChild("__params__", float2(-FLT_MIN, -FLT_MIN), ImGuiChildFlags.Borders, ImGuiWindowFlags.None)

  autoAlign ||= imgui::Button("Align")
  imgui::SameLine()
  focusContent ||= imgui::Button("Focus")
  imgui::SameLine()
  focusRoot ||= imgui::Button("Focus root")
  imgui::Checkbox("Disable auto align", safe_addr(disableAutoAlign))
  var nodeDrawModeIsMinified = nodeDrawMode == NodeDrawMode.MINIFIED
  autoAlign ||= imgui::Checkbox("Minified node info", safe_addr(nodeDrawModeIsMinified))
  nodeDrawMode = nodeDrawModeIsMinified ? NodeDrawMode.MINIFIED : NodeDrawMode.FULL
  ctx.drawMode = nodeDrawMode

  var newNodesToCreate : array<tuple<string; int>>
  var doUndo = false
  var doRedo = false

  graph |> anim_graph_getAnimNodeNames <| $(nodes)
    ctx.nodeNames |> reserve(length(nodes))
    ctx.nodeIds |> reserve(length(nodes))
    for n in nodes
      ctx.nodeNames |> push("{n.name}")
      ctx.nodeIds |> push(n.id)

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
        *statesFilter |> Draw("##statesFilter")
        Spacing()

        for stateId in range(graph.stateCount)
          let stateName = graph |> anim_graph_getStateName(stateId)
          let stateIdx = graph |> anim_graph_getStateIdx(stateName)

          if !(*statesFilter |> PassFilter(stateName))
            continue

          if imgui::Button("{stateName} ({stateIdx})##states_{stateName}")
            graph |> anim_graph_enqueueState(state, stateIdx)
            broadcastEvent(AnimGraphViewerSetAnimState(state_idx = stateIdx))

          if imgui::BeginPopupContextItem("StateContextMenu##states_{stateName}")
            let bookmarkIndex = bookmarkedStates |> find_index("{stateName}")
            if bookmarkIndex < 0 && imgui::MenuItem("Bookmark")
              bookmarkedStates |> push(clone_string(stateName))
              saveBookmarks = true
            elif bookmarkIndex >= 0 && imgui::MenuItem("Remove Bookmark")
              bookmarkedStates |> erase(bookmarkIndex)
              saveBookmarks = true
            imgui::EndPopup()

      var eraseBookmarkParamId = -1

      let drawParam = $ <| (id : int; prefix : string; name : string; showHiddenParams : bool)
        let paramType = graph |> anim_graph_getParamType(id)
        let isBasicType = paramType == PT_ScalarParam || paramType == PT_ScalarParamInt || paramType == PT_TimeParam
        let isParamPaused = isBasicType && state |> anim_state_holder_getParamFlags(id, int(PF_Paused)) == int(PF_Paused)
        let isParamChanged = isBasicType && state |> anim_state_holder_getParamFlags(id, int(PF_Changed)) == int(PF_Changed)
        let typeText = param_type_to_text(paramType)
        let typeTextFormatted = "  <{typeText}>"
        let nameText = "{name} ({id}){isParamPaused ? " [P]" : ""}{isParamChanged ? " [C]" : ""}"

        let popup = $ <| ()
          if imgui::BeginPopupContextItem("ParamContextMenu##{prefix}params_{name}")
            imgui::Text("Name: {nameText}")
            imgui::Text("Type: {typeText} ({int(paramType)})")
            if paramType == PT_TimeParam
              let timeScaleParamId = state |> anim_state_holder_getTimeScaleParamId(id)
              if timeScaleParamId > 0
                let timeScaleParamName = graph |> anim_graph_getParamName(timeScaleParamId)
                let timeScaleParamValue = state |> anim_state_holder_getParam(timeScaleParamId)
                imgui::Text("Time Scale Param: {timeScaleParamName} ({timeScaleParamId}) = {timeScaleParamValue:.2f}")
              else
                imgui::Text("Time Scale Param: NONE (0)")
            if isBasicType
              imgui::Text("PF_Paused: {isParamPaused}")
              imgui::Text("PF_Changed: {isParamChanged}")
            let enumName = get_enum_name_of_parameter_value(graph, state, id)
            if enumName != ""
              imgui::Text("Possible Enum: {enumName}")
            imgui::Separator()
            let bookmarkIndex = bookmarkedParams |> find_index(name)
            if bookmarkIndex < 0 && imgui::MenuItem("Bookmark")
              bookmarkedParams |> push(clone_string(name))
              saveBookmarks = true
            elif bookmarkIndex >= 0 && imgui::MenuItem("Remove Bookmark")
              eraseBookmarkParamId = bookmarkIndex
              saveBookmarks = true

            if (paramType == PT_ScalarParam || paramType == PT_ScalarParamInt || paramType == PT_TimeParam) && imgui::MenuItem("Reset to 0.0")
              if paramType == PT_ScalarParamInt
                state |> anim_state_holder_setParamInt(id, 0)
              else
                state |> anim_state_holder_setParam(id, 0.0)

            if imgui::MenuItem("Copy name ({name})")
              imgui::SetClipboardText(name)
            if imgui::MenuItem("Copy type ({typeText})")
              imgui::SetClipboardText(typeText)
            imgui::EndPopup()

        if paramType == PT_Reserved
          return

        var hasViewSettings = false
        var viewSettings : AnimCharViewVarSettings
        if paramType == PT_ScalarParam || paramType == PT_ScalarParamInt
          param_view_settings |> get(name) <| $(var s)
            hasViewSettings = true
            viewSettings = s

        if viewSettings.enumCls != ""
          var enumNames : array<string>
          var enumValues : array<int>
          get_enum_items(graph, viewSettings.enumCls, enumNames, enumValues)
          if length(enumNames) > 0
            let intValue = paramType == PT_ScalarParamInt ? anim_state_holder_getParamInt(state, id) : roundi(anim_state_holder_getParam(state, id))
            let selIdx = enumValues |> find_index(intValue)
            let valueName = animV20_get_enum_name_by_id(intValue)
            let previewText = selIdx >= 0 ? enumNames[selIdx] : valueName != "" ? valueName : "non enum value: {intValue}"
            imgui::BeginGroup()
            imgui::Text(nameText)
            SetNextItemWidth(-FLT_MIN)
            imgui::SetNextWindowSizeConstraints(ImVec2(0.0, 0.0), ImVec2(FLT_MAX, FLT_MAX))
            if imgui::BeginCombo("##{prefix}enum_{name}", previewText)
              if imgui::IsWindowAppearing()
                *enumParamComboFilter |> Clear()
              *enumParamComboFilter |> Draw("##{prefix}enum_filter_{name}")

              var filteredIndexes : array<int>
              filteredIndexes |> reserve(length(enumNames))
              for i in range(length(enumNames))
                if *enumParamComboFilter |> PassFilter(enumNames[i])
                  filteredIndexes |> push(i)

              let maxComboSize = 200.0
              imgui::SetNextWindowSizeConstraints(ImVec2(0.0, 0.0), ImVec2(FLT_MAX, maxComboSize))
              if imgui::BeginChild("##{prefix}enum_options_{name}", ImVec2(-FLT_MIN, 0.0), ImGuiChildFlags.AutoResizeY | ImGuiChildFlags.AlwaysAutoResize)
                using() <| $(var clipper : ImGuiListClipper)
                  Begin(clipper, length(filteredIndexes), imgui::GetTextLineHeightWithSpacing())
                  while Step(clipper)
                    for i in range(clipper.DisplayStart, clipper.DisplayEnd)
                      let idx = filteredIndexes[i]
                      if imgui::Selectable(enumNames[idx], idx == selIdx)
                        if paramType == PT_ScalarParamInt
                          state |> anim_state_holder_setParamInt(id, enumValues[idx])
                        else
                          state |> anim_state_holder_setParam(id, float(enumValues[idx]))
                        imgui::CloseCurrentPopup()
                imgui::EndChild()
              imgui::EndCombo()
            imgui::EndGroup()
            invoke(popup)
            Spacing()
            return

        if viewSettings.isBool
          imgui::BeginGroup()
          var boolValue = paramType == PT_ScalarParamInt ? anim_state_holder_getParamInt(state, id) >= 1 : anim_state_holder_getParam(state, id) > 0.5
          if imgui::Checkbox("{nameText}##{prefix}bool_{name}", safe_addr(boolValue))
            if paramType == PT_ScalarParamInt
              state |> anim_state_holder_setParamInt(id, boolValue ? 1 : 0)
            else
              state |> anim_state_holder_setParam(id, boolValue ? 1.0 : 0.0)
          imgui::EndGroup()
          invoke(popup)
          Spacing()
          return

        if paramType == PT_ScalarParam
          imgui::BeginGroup()
          imgui::Text(nameText)
          var value = state |> anim_state_holder_getParam(id)
          SetNextItemWidth(-FLT_MIN)
          if hasViewSettings && viewSettings.trackbar
            if imgui::SliderFloat("##{name}", safe_addr(value), viewSettings.minVal, viewSettings.maxVal)
              state |> anim_state_holder_setParam(id, value)
          else
            let hasRange = hasViewSettings && viewSettings.minVal != viewSettings.maxVal
            let min = hasRange ? viewSettings.minVal : -1e8
            let max = hasRange ? viewSettings.maxVal : 1e8
            let speed = hasViewSettings ? viewSettings.step : 0.01
            if imgui::DragScalar("##{name}", ImGuiDataType.Float, safe_addr(value), speed, safe_addr(min), safe_addr(max), "%.2f", ImGuiSliderFlags.None)
              state |> anim_state_holder_setParam(id, value)
          imgui::EndGroup()
          invoke(popup)
          Spacing()

        elif paramType == PT_ScalarParamInt
          imgui::BeginGroup()
          imgui::Text(nameText)
          var value = state |> anim_state_holder_getParamInt(id)
          SetNextItemWidth(-FLT_MIN)
          if hasViewSettings && viewSettings.trackbar
            if imgui::SliderInt("##{name}", safe_addr(value), int(viewSettings.minVal), int(viewSettings.maxVal))
              state |> anim_state_holder_setParamInt(id, value)
          else
            let step = 1
            let fastStep = 5
            if imgui::InputScalar("##{name}", ImGuiDataType.S32, safe_addr(value), safe_addr(step), safe_addr(fastStep), "%d", ImGuiInputTextFlags.None)
              state |> anim_state_holder_setParamInt(id, value)
          imgui::EndGroup()
          invoke(popup)
          Spacing()

        elif showHiddenParams
          if paramType == PT_TimeParam
            imgui::BeginGroup()
            imgui::Text(nameText)
            let timeValue = state |> anim_state_holder_getParam(id)
            let min = -1e8
            let max = 1e8
            SetNextItemWidth(-FLT_MIN)
            if imgui::DragScalar("##{name}_time", ImGuiDataType.Float, safe_addr(timeValue),  0.01, safe_addr(min), safe_addr(max), "%.2f", ImGuiSliderFlags.None)
              state |> anim_state_holder_setParam(id, timeValue)
            imgui::EndGroup()
            invoke(popup)
            Spacing()
          else
            imgui::BeginGroup()
            imgui::Text(nameText)
            imgui::TextUnformatted(typeTextFormatted)
            imgui::EndGroup()
            invoke(popup)
            Spacing()

      if imgui::CollapsingHeader("Parameters")
        static_let <|
          var showHiddenParams = false
        imgui::Checkbox("Show hidden params", safe_addr(showHiddenParams))
        imgui::Text("Params filter:")
        *paramsFilter |> Draw("##paramsFilter")

        Spacing()
        graph |> anim_graph_getParamNames <| $(params)
          imgui::Text("Total params count: {length(params)}")
          imgui::Separator()

          for param in params
            if !showHiddenParams && param.name |> character_at(0) == ':'
              continue

            if !(*paramsFilter |> PassFilter(param.name))
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
        imgui::Text("Slots filter:")
        *slotsFilter |> Draw("##slotsFilter")
        for i in range(animchar |> animchar_getAttachmentSlotsCount())
          let slotId = animchar |> animchar_getAttachmentSlotId(i)
          let slotName = animchar_getSlotName(slotId)
          if !(*slotsFilter |> PassFilter(slotName)) && !(*slotsFilter |> PassFilter("{slotId}"))
            continue
          imgui::Text("{slotId}: {slotName}")

      if imgui::CollapsingHeader("Enums")
        imgui::Text("Enums filter:")
        *enumsFilter |> Draw("##enumsFilter")
        var enumId = 0
        var enumName = animV20_get_enum_name_by_id(enumId)
        // animV20_get_enum_name_by_id returns NULL (which is "" string in das) when enumId is out of range.
        while enumName != ""
          if (*enumsFilter |> PassFilter(enumName)) || (*enumsFilter |> PassFilter("{enumId}"))
            imgui::Text("{enumId}: {enumName}")
          enumId += 1
          enumName = animV20_get_enum_name_by_id(enumId)

      if imgui::CollapsingHeader("Post Blend Controllers ({length(graph.blender.pbCtrl)})")
        imgui::Checkbox("Show node types##pbc", safe_addr(pbcListShowNodeClass))
        imgui::Checkbox("Show node weight##pbc", safe_addr(pbcListShowWeight))
        imgui::Checkbox("Show active only##pbc", safe_addr(pbcListShowActiveOnly))
        let doCopyList = imgui::Button("Copy List To Clipboard")
        var copyText = ""
        var debugContext = animchar.animcharDebugContext

        for i, pbcPtr in range(length(graph.blender.pbCtrl)), graph.blender.pbCtrl
          let hasWtOverride = animchar_debug_getPbcOverrideEnabled(*debugContext, i)
          if pbcListShowActiveOnly && pbc_wt[i] == 0.0 && !hasWtOverride
            continue
          let pbc = get(pbcPtr)
          let name = graph |> anim_graph_getBlendNodeName(pbc)
          let className = pbcListShowNodeClass ? " ({*pbc |> anim_blend_node_class_name()})" : ""
          let weightText = pbcListShowWeight ? "(w: {pbc_wt[i]:.2f})" : ""
          let weightOverrideText = hasWtOverride ? " (weight override)" : ""
          let text = "{name}{className}{weightOverrideText}{weightText}"
          imgui::Text(text)
          if doCopyList
            copyText += "{text}\n"
          let nodeId = anim_blend_node_getAnimNodeId(*pbc)
          if imgui::BeginPopupContextItem("PostBlendControllersContext##PostBlendControllersContext_{i}")
            if imgui::Selectable("Focus")
              imgui_node_editor::SelectNode(get_node_idx(nodeId))
              NavigateToSelection(true, -1f)
            if imgui::Selectable("Copy name ({name})")
              imgui::SetClipboardText(name)
            imgui::EndPopup()

        if doCopyList
          imgui::SetClipboardText(copyText)

      if imgui::CollapsingHeader("Blend Node Leafs ({length(graph.blender.bnl)})")
        imgui::Checkbox("Show active only##bnl", safe_addr(bnlListShowActiveOnly))
        for i, bnlPtr in range(length(graph.blender.bnl)), graph.blender.bnl
          if bnlListShowActiveOnly && bnl_wt[i] == 0.0
            continue
          let bnl = get(bnlPtr)
          let name = graph |> anim_graph_getBlendNodeName(bnl)
          imgui::Text("{name}")
          let nodeId = anim_blend_node_getAnimNodeId(*bnl)
          if imgui::BeginPopupContextItem("BlendNodeLeafsContext##BlendNodeLeafsContext_{i}")
            if imgui::Selectable("Focus")
              imgui_node_editor::SelectNode(get_node_idx(nodeId))
              NavigateToSelection(true, -1f)
            if imgui::Selectable("Copy name ({name})")
              imgui::SetClipboardText(name)
            imgui::EndPopup()

      imgui::EndChild()
      imgui::NextColumn()

      imgui::BeginTabBar("Tabs")

      let graphViewControlBarHeight = 52.0f
      let graphSize = float2(
        imgui::GetContentRegionAvail().x,
        imgui::GetContentRegionAvail().y - graphViewControlBarHeight
      )

      if imgui::BeginTabItem("Graph")
        defer <|
          imgui::EndTabItem()

        imgui::Text("Anim Node Name Filter: ")
        imgui::SameLine()
        if *animGraphNodesFilter |> Draw("##animGraphNodesFilter")
          autoAlign = true
          focusContent = true
        imgui::SameLine()
        if imgui::Button("Clear")
          *animGraphNodesFilter |> Clear()
          autoAlign = true
          focusContent = true

        imgui::SameLine()
        let undoCount = length(history.undo)
        let redoCount = length(history.redo)
        if imgui::Button("Undo##animgraph_undo")
          doUndo = true
        if imgui::IsItemHovered() && undoCount > 0
          BeginTooltip()
          let undoName = history.undo[undoCount - 1].name
          imgui::Text("Undo: {undoName}")
          EndTooltip()
        imgui::SameLine()
        if imgui::Button("Redo##animgraph_redo")
          doRedo = true
        if imgui::IsItemHovered() && redoCount > 0
          BeginTooltip()
          let redoName = history.redo[redoCount - 1].name
          imgui::Text("Redo: {redoName}")
          EndTooltip()
        imgui::SameLine()
        imgui::TextDisabled("({undoCount}:{redoCount})")

        var style & = unsafe(imgui_node_editor::GetStyle())
        style.NodeRounding = 0f
        style.GroupRounding = 0f
        style.PinRounding = 0f
        style.NodePadding = float4(12, 4, 12, 8)
        imgui::PushStyleColor(ImGuiCol.Text, float4(1))
        imgui::PushStyleColor(ImGuiCol.PopupBg, float4(0.12, 0.12, 0.12, 1.0))
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

          var isNodeRendered : array<bool>
          ctx.weights |> reserve(length(nodes))
          isNodeRendered |> reserve(length(nodes))

          var newGraphHash = 0
          for node in nodes
            var nodePtr = graph |> anim_graph_getBlendNodePtr(node.id)
            var weight = abn_wt[node.id]

            // pbcs and bnls save their weights to different pbc_wt and bnl_wt respectively, abn_wt of those will always be 0.0
            if weight > 0.0
              activeBlendNodesCount += 1

            if nodePtr == null
              ctx.weights |> push(weight)
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

            ctx.weights |> push(weight)

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
              for idx, w in range(length(ctx.weights)), ctx.weights
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
                  nodesToShow |> insert(idx, (generation=0, active=true))
                  activeNodesCount += 1

              if length(nodesToShow) - activeNodesCount > nodesToShowMaxInactiveCount
                var keysToDelete : array<int>
                var keysToDeleteCount = length(nodesToShow) - activeNodesCount - nodesToShowMaxInactiveCount
                for genToDelete in range(maxGeneration - 1)
                  for val, key in values(nodesToShow), keys(nodesToShow)
                    if val.generation <= genToDelete && ctx.weights[key] == 0.0
                      keysToDelete |> push(key)
                      keysToDeleteCount -= 1

                  if keysToDeleteCount <= 0
                    break

                for key in keysToDelete
                  nodesToShow |> erase(key)


          for idx, node in range(length(nodes)), nodes
            isNodeRendered |> push(false)
            let weight = ctx.weights[idx]
            if drawMode == DrawNodesMode.ACTIVE && weight == 0.0
              continue
            if drawMode == DrawNodesMode.ACTIVE_AND_HISTORY && !key_exists(nodesToShow, idx)
              continue

            var nodePtr = graph |> anim_graph_getBlendNodePtr(node.id)
            let className = *nodePtr |> anim_blend_node_class_name()

            if *animGraphNodesFilter |> IsActive() && !(*animGraphNodesFilter |> PassFilter(node.name)) && !(*animGraphNodesFilter |> PassFilter("{node.id}")) && !(*animGraphNodesFilter |> PassFilter(className))
              continue

            let activeNodeBorder = drawMode != DrawNodesMode.ACTIVE && weight > 0.0
            if activeNodeBorder
              imgui_node_editor::PushStyleColor(StyleColor.NodeBorder, float4(0.58, 0.96, 0.23f, 1f))


            BeginNode(get_node_idx(node.id)) <|
              Dummy(float2(nodeWidth, 0f))

              isNodeRendered |> back() = true

              let inId = get_input_node_idx(node.id)
              in_pin(inId)

              if nodePtr == null
                imgui::Text("\nThis node has failed to load.\nProbably there was in error in this char's animTree.")
                return

              SameLine()
              imgui::TextUnformatted(node.name)
              SameLine()
              imgui::TextUnformatted("(id: {node.id})")

              imgui::Spacing()
              if nodeDrawMode == NodeDrawMode.MINIFIED
                imgui::Text("{className}, Weight: {weight:.2f}")
              else
                imgui::Text("{className}, ID: {unsafe(reinterpret<uint64> nodePtr)}")
                imgui::Text("weight: {weight:.2f}")

              var foundRender = false
              for render in renderers
                if *nodePtr |> anim_blend_node_isSubOf(render._0)
                  render._1 |> invoke(nodePtr, graph, state, ctx)
                  foundRender = true
                  break
              if !foundRender
                imgui::Text("Error: no renderer")

            imgui_node_editor::Suspend()

            var nodeId = -1
            if imgui_node_editor::ShowNodeContextMenu(safe_addr(nodeId))
              if node.id == get_link_node(nodeId)
                imgui::OpenPopup("node_context_menu##node_context_menu_{idx}")

            if imgui::BeginPopup("node_context_menu##node_context_menu_{idx}")
              if imgui::Selectable("Copy name ({node.name})")
                imgui::SetClipboardText(node.name)
              if nodePtr != null
                let blkClassName = class_name_to_anim_tree_class_name(className)
                if imgui::Selectable("Copy class ({className})")
                  imgui::SetClipboardText(className)
                if imgui::Selectable("Copy blk class ({blkClassName})")
                  imgui::SetClipboardText(blkClassName)
                let resIds = get_leaf_anim_res_ids(nodePtr)
                let resId = resIds._0 == -1 ? resIds._1 : resIds._0
                if resId != -1
                  imgui::Separator()
                  let resName = get_game_resource_name(resId)
                  if imgui::Selectable("Open A2D Resource ({resName})")
                    broadcastEvent(AnimGraphViewerSelectAssetResource(res_name = resName))
                  if imgui::Selectable("Copy A2D Resource Name ({resName})")
                    imgui::SetClipboardText(resName)
              imgui::EndPopup()

            imgui_node_editor::Resume()

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
              let active = ctx.weights[get_link_node(lnk.outIdx)] > 0.0 && ctx.weights[get_link_node(lnk.inIdx)] > 0.0
              if active
                color = float4(0.58, 0.96, 0.23f, 1f)
            imgui_node_editor::Link(idx, lnk.outIdx, lnk.inIdx, color)


          assume rootNode = *graph.root
          let rootNodeId = rootNode |> anim_blend_node_getAnimNodeId()
          if autoAlign && !disableAutoAlign
            var children : array<int>
            children |> push(rootNodeId)
            var allNodes : table<int; bool>
            for i in range(graph.animNodeCount)
              allNodes |> insert(i, has_children(ctx.links, i))

            let HGAP = 100f
            let VGAP = 50f
            let PADDING = 10f
            var maxWidth = 0f
            var offset = float2(PADDING)

            while length(children) > 0 || length(allNodes) > 0
              var copyChildren := children
              delete children

              for childId in copyChildren
                if childId == -1 || !key_exists(allNodes, childId) || !isNodeRendered[childId]
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
              if !isNodeRendered[childId]
                continue
              SetNodePosition(get_node_idx(childId), offset)
              let dim = GetNodeSize(get_node_idx(childId))
              offset.y += dim.y + VGAP
              maxWidth = max(dim.x, maxWidth)
              if offset.y > 2000f
                offset.x += maxWidth + HGAP
                offset.y = PADDING
                maxWidth = 0f

            if GetSelectedObjectCount() > 0
              NavigateToSelection(false, 0.0f)

          if focusRoot
            imgui_node_editor::ClearSelection()
            imgui_node_editor::SelectNode(get_node_idx(rootNodeId), true)
            NavigateToSelection(true, 0.2f)

          if focusContent
            if GetSelectedObjectCount() > 0
              NavigateToSelection(false, 0.0f)
            else
              NavigateToContent(0.2f)

          imgui_node_editor::Suspend()

          static_let <|
            var contextMenuLinkId = -1
          if imgui_node_editor::ShowLinkContextMenu(safe_addr(contextMenuLinkId))
            imgui::OpenPopup("link_context_menu")
          elif imgui_node_editor::ShowBackgroundContextMenu()
            imgui::OpenPopup("background_context_menu")

          if imgui::BeginPopup("link_context_menu")
            var startPinId = -1
            var endPinId = -1
            GetLinkPins(contextMenuLinkId, safe_addr(startPinId), safe_addr(endPinId))
            if startPinId >= 0
              let nodeId = get_link_node(startPinId)
              if imgui::Selectable("Go to start ({nodes[nodeId].name})")
                imgui_node_editor::SelectNode(get_node_idx(nodeId))
                NavigateToSelection(false, -1f)
            if endPinId >= 0
              let nodeId = get_link_node(endPinId)
              if imgui::Selectable("Go to end ({nodes[nodeId].name})")
                imgui_node_editor::SelectNode(get_node_idx(nodeId))
                NavigateToSelection(false, -1f)
            imgui::EndPopup()

          if imgui::BeginPopup("background_context_menu")
            if imgui::BeginMenu("Create Node")
              imgui::Text("Create New Anim Node")
              static_let <|
                var newNodeNameBuffer = ImGuiInputTextBuffer("new_name", 128)
                var newNodeSelectedType = 0
                var newNodeTypeFilterBuffer = ImGuiInputTextBuffer("", 128)
              static_let_finalize <|
                var newNodeTypeFilter = new ImGuiTextFilter
              InputText("Node Name", newNodeNameBuffer, ImGuiInputTextFlags.AutoSelectAll)

              let maxComboSize = 200.0
              imgui::SetNextWindowSizeConstraints(ImVec2(0.0, 0.0), ImVec2(FLT_MAX, FLT_MAX))
              if imgui::BeginCombo("Node Type", anim_tree_class_names[newNodeSelectedType])
                *newNodeTypeFilter |> Draw("##newNodeTypeFilter")

                var filteredNodeTypeIndexes : array<int>
                filteredNodeTypeIndexes.reserve(length(anim_tree_class_names))
                for i in range(length(anim_tree_class_names))
                  if *newNodeTypeFilter |> PassFilter(anim_tree_class_names[i])
                    filteredNodeTypeIndexes.push(i)

                imgui::SetNextWindowSizeConstraints(ImVec2(0.0, 0.0), ImVec2(FLT_MAX, maxComboSize))
                if imgui::BeginChild("##ComboOptions", ImVec2(-FLT_MIN, 0.0), ImGuiChildFlags.AutoResizeY | ImGuiChildFlags.AlwaysAutoResize)
                  using() <| $(var clipper : ImGuiListClipper)
                    Begin(clipper, length(filteredNodeTypeIndexes), imgui::GetTextLineHeightWithSpacing())
                    while Step(clipper)
                      for i in range(clipper.DisplayStart, clipper.DisplayEnd)
                        let isSelected = newNodeSelectedType == filteredNodeTypeIndexes[i]
                        if imgui::Selectable(anim_tree_class_names[filteredNodeTypeIndexes[i]], isSelected)
                          newNodeSelectedType = filteredNodeTypeIndexes[i]
                          imgui::CloseCurrentPopup()

                  imgui::EndChild()
                imgui::EndCombo()

              if imgui::Button("Create")
                newNodesToCreate |> push(tuple(to_string(newNodeNameBuffer), newNodeSelectedType))
              imgui::EndMenu()

            if imgui::Button("Test Node Creation")
              for i in range(length(anim_tree_class_names))
                let nodeName = "test_name_{i}"
                newNodesToCreate |> push(tuple(nodeName, i))
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
        imgui::BeginChild("__skeleton__", float2(-FLT_MIN, -FLT_MIN), ImGuiChildFlags.Borders, ImGuiWindowFlags.HorizontalScrollbar)
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

        // for stateIdx in range(graph.stateCount)
        //   let stateName = graph |> anim_graph_getStateName(stateIdx)
        //   BeginNode(stateIdx + statesOffset) <|
        //     BeginNodeTitleBar <|
        //       imgui::TextUnformatted("State: {stateName}")
        //     imgui::Text(stateName)

  // We should not create nodes or apply graph edits while drawing a graph cause it adds a lot of questions
  // about the system which I don't have answers to. Also animchar_getAnimBlendNodeWeights creates a copy of
  // animstate for the duration of the drawing and we can't easily update that one on the fly so it crashes on
  // desctruction because the graph is updated for new state.
  // That's why we are doing all of this node creation and edits as late as possible, hopefully correctly
  // updating all animchars.
  let hasPendingActions = length(ctx.pendingActions) > 0
  // Ignore undo\redo clicks if no actions are present.
  doUndo &&= length(history.undo) > 0
  doRedo &&= length(history.redo) > 0

  if length(newNodesToCreate) > 0 || hasPendingActions || doUndo || doRedo
    let gameresFactory = find_gameres_factory(CharacterGameResClassId)

    let for_each_char_with_graph_res_id = $ <| (resId : int; cb : block<(var animchar : AnimcharBaseComponent) : void>)
      // Anim state holds state for a specific graph. If graphs paramNames count changes the anim state becomes invalid and we need
      // to handle that. Otherwise we get oob and crash.
      // Right now anim states seems to be always stored inside AnimcharBaseComponent (except the copy in animchar_getAnimBlendNodeWeights)
      // AnimcharBaseComponent themselves are either stored in ecs (normal animchar entities) or inside CharacterGameResFactory (icon rendering)
      // If there are any others which we will edit and forget to clear - we will crash. But it does not seem like there are any right now.
      iterate_gameres_factory_resources(gameresFactory) <| $(var res : void?)
        var other_animchar & = unsafe(reinterpret<IAnimCharacter2?>(res)).base
        if other_animchar.animGraph != null && other_animchar.animState != null && other_animchar.animGraph.resId == resId
          invoke(cb, other_animchar)
      query() <| $ [es] (var animchar aka other_animchar : AnimcharBaseComponent&)
        if other_animchar.animGraph != null && other_animchar.animState != null && other_animchar.animGraph.resId == resId
          invoke(cb, other_animchar)

    var needStateReset = length(newNodesToCreate) > 0
    for act in ctx.pendingActions
      needStateReset ||= act.needsStateReset
    if doUndo
      let undoLast = length(history.undo) - 1
      needStateReset ||= history.undo[undoLast].needsStateReset
    if doRedo
      let redoLast = length(history.redo) - 1
      needStateReset ||= history.redo[redoLast].needsStateReset

    // Clear out anim state of all chars which use this graph. AnimState might have allocated memory
    // so we need to clear it.
    if needStateReset
      for_each_char_with_graph_res_id(graph.resId) <| $(var other_animchar : AnimcharBaseComponent)
        anim_state_holder_term(*other_animchar.animState)

    for node in newNodesToCreate
      let nodeName = node._0
      let classIdx = node._1
      visual_log("Creating node {nodeName} of type {anim_tree_class_names[classIdx]}")
      // Create a minimal node blk and init a new node with it
      using() <| $(var nodeBlk : DataBlock)
        datablock_change_block_name(nodeBlk, anim_tree_class_names[classIdx])
        nodeBlk |> set("name", nodeName)
        AnimResManagerV20_add_bn(graph, nodeBlk, "")

    if hasPendingActions
      for i in range(length(ctx.pendingActions))
        if !ctx.pendingActions[i].alreadyApplied
          apply_graph_action(ctx.pendingActions[i], true, graph)
      history.redo |> clear()
      for i in range(length(ctx.pendingActions))
        history.undo |> push(ctx.pendingActions[i])

    if doUndo
      let last = length(history.undo) - 1
      apply_graph_action(history.undo[last], false, graph)
      let poppedAction = history.undo[last]
      history.redo |> push(poppedAction)
      history.undo |> erase(last)

    if doRedo
      let last = length(history.redo) - 1
      apply_graph_action(history.redo[last], true, graph)
      let poppedAction = history.redo[last]
      history.undo |> push(poppedAction)
      history.redo |> erase(last)

    // After new nodes are added to the graph init anim states of all affected chars.
    if needStateReset
      for_each_char_with_graph_res_id(graph.resId) <| $(var other_animchar : AnimcharBaseComponent)
        // This resizes old animStates to be correct for new graph nodeParams size
        anim_state_holder_init(*other_animchar.animState)
        // TODO? we are doing extra work here because somewhere down the call line it allso calls
        // anim state reset which calls term and init. But i'm not sure if it is worth adding extra
        // code passes which don't do that just for the developer editor for a non performance heavy
        // thing. This inits the new state for all nodes correctly.
        other_animchar |> animchar_reset()


def draw_skeleton_node_in_tree_recursive(var graph : AnimationGraph;
                                         tree : GeomNodeTree;
                                         nodesFilter : array<bool>&;
                                         node_idx : int;
                                         expand_all : bool;
                                         filter_changed : bool)
  if expand_all || filter_changed
    imgui::SetNextItemOpen(true)

  let node_name = "{node_idx == 0 ? "::root" : tree |> geomtree_getNodeName(node_idx)}"
  if (length(nodesFilter) == 0 || nodesFilter[node_idx]) && imgui::TreeNodeEx("\"{node_name}\" ({node_idx})##skel_node_{node_idx}", ImGuiTreeNodeFlags.None)
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

