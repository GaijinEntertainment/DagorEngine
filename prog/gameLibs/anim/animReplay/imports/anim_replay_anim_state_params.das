module anim_replay_anim_state_params shared
options no_aot
require daslib/math_bits
require ecs
require strings
require daslib/strings_boost
require AnimV20
require DagorSystem
require DagorDataBlock


// ParametricSwitcher controller uses pointers in some parameter names, like fifo_00007FF0078186F0 or :lastAnim_00007FF0078186F0
// We can replace this pointer to controller name, so it will be more robust between different launches. It will look
// like `fifo_~~weapon_aim:arms` and `:lastAnim_~~weapon_aim:arms` in saved blk.
def replace_pointer_in_name(name : string; anim_graph : AnimationGraph)
  let pointerSize = typeinfo sizeof(type<IAnimBlendNode?>)
  let hexCharactersNum = pointerSize * 2
  let len = length(name)
  if len < hexCharactersNum
    return name
  for i in range(len - hexCharactersNum, len)
    if !is_hex(character_at(name, i))
      return name
  let ptr = to_uint64(slice(name, -hexCharactersNum), true)
  let animNodeName = anim_graph_getBlendNodeName(anim_graph, unsafe(reinterpret<IAnimBlendNode?>(ptr)))
  if !empty(animNodeName)
    return "{slice(name, 0, -hexCharactersNum)}{animNodeName}"
  return name

def save_anim_state_parameters(anim_graph : AnimationGraph; anim_state : IAnimStateHolder; var dest_obj : Object)
  clear(dest_obj)
  using() <| $(var rawData : IntList)
    set(dest_obj, "rawData", rawData)
  using() <| $(var paramTypes : UInt8List)
    set(dest_obj, "paramTypes", paramTypes)
  using() <| $(var paramOffsets : IntList)
    set(dest_obj, "paramOffsets", paramOffsets)
  using() <| $(var paramIds : IntList)
    set(dest_obj, "paramIds", paramIds)
  assume rawData = *getRW_ecs_IntList(dest_obj, "rawData")
  assume paramTypes = *getRW_ecs_UInt8List(dest_obj, "paramTypes")
  assume paramOffsets = *getRW_ecs_IntList(dest_obj, "paramOffsets")
  assume paramIds = *getRW_ecs_IntList(dest_obj, "paramIds")

  anim_graph_getParamNames(anim_graph) <| $(params)
    for param in params
      let paramType = anim_graph_getParamType(anim_graph, param.id)
      // Some PT_InlinePtr (like `AnimPostBlendNodesFromAttachement::LocalData`) stores pointers to external memory
      // and can't be saved. But maybe we can whitelist other parameters?
      if paramType == PT_Reserved || paramType == PT_InlinePtr || paramType == PT_InlinePtrCTZ
        continue
      push(paramIds, param.id)
      push(paramTypes, uint8(paramType))
      push(paramOffsets, length(rawData))
      if paramType == PT_ScalarParam
        let paramValue = anim_state_holder_getParam(anim_state, param.id)
        push(rawData, float_bits_to_int(paramValue))
      elif paramType == PT_ScalarParamInt
        let paramValue = anim_state_holder_getParamInt(anim_state, param.id)
        push(rawData, paramValue)
      elif paramType == PT_TimeParam
        let paramValue = anim_state_holder_getParam(anim_state, param.id)
        push(rawData, float_bits_to_int(paramValue))
      elif paramType == PT_InlinePtr || paramType == PT_Fifo3 || paramType == PT_Effector
        let ptr = anim_state_holder_getInlinePtrConst(anim_state, param.id)
        let prevSize = length(rawData)
        let copySizeInBytes = int(anim_state_holder_getInlinePtrMaxSz(anim_state, param.id))
        resize(rawData, prevSize + copySizeInBytes / 4)
        unsafe
          memcpy(addr(rawData[prevSize]), ptr, copySizeInBytes)
      else
        logerr("unknown animState param type {paramType} ({param.name})")

def copy_anim_state_parameters_to_blk(anim_graph : AnimationGraph; params_obj : Object; var blk : DataBlock)
  assume rawData = *get_ecs_IntList(params_obj, "rawData")
  assume paramTypes = *get_ecs_UInt8List(params_obj, "paramTypes")
  assume paramOffsets = *get_ecs_IntList(params_obj, "paramOffsets")
  assume paramIds = *get_ecs_IntList(params_obj, "paramIds")

  for paramId, pType, pOffset,  i in paramIds, paramTypes, paramOffsets, iter_range(paramTypes)
    let paramName = anim_graph_getParamName(anim_graph, paramId)
    let processedParamName = replace_pointer_in_name(string(paramName), anim_graph)
    datablock_add_block(blk, processedParamName) <| $(var paramBlk : DataBlock)
      let paramType = uint(pType)
      add(paramBlk, "type", int(pType))
      if paramType == PT_ScalarParam || paramType == PT_ScalarParamInt || paramType == PT_TimeParam
        add(paramBlk, "value", rawData[pOffset])
      elif paramType == PT_Fifo3
        unsafe
          var fifoQueuePtr = reinterpret<AnimFifo3Queue?>(addr(rawData[pOffset]))
          for j in range(3)
            add(paramBlk, "node{j}", anim_graph_getBlendNodeName(anim_graph, fifoQueuePtr.node[j]))
          add(paramBlk, "morphTime0", fifoQueuePtr.morphTime[0])
          add(paramBlk, "morphTime1", fifoQueuePtr.morphTime[1])
          add(paramBlk, "state", fifoQueuePtr.state)
          add(paramBlk, "t0", fifoQueuePtr.t0)
      elif paramType == PT_InlinePtr || paramType == PT_Effector
        let nextOffset = i + 1 < length(paramOffsets) ? paramOffsets[i + 1] : length(rawData)
        let dataSize = nextOffset - pOffset
        datablock_add_block(paramBlk, "raw") <| $(var raw : DataBlock)
          for j in range(dataSize)
            add(raw, "v", rawData[pOffset + j])

def push_zero_param(var raw_data : IntList; param_type : uint; anim_state : IAnimStateHolder; param_id : int)
  if param_type == PT_ScalarParam || param_type == PT_ScalarParamInt || param_type == PT_TimeParam
    push(raw_data, 0)
  elif param_type == PT_InlinePtr || param_type == PT_Fifo3 || param_type == PT_Effector
    let paramSize = anim_state_holder_getInlinePtrMaxSz(anim_state, param_id) / 4u
    for _ in range(paramSize)
      push(raw_data, 0)

def load_anim_state_parameters(anim_graph : AnimationGraph; anim_state : IAnimStateHolder; var dest_obj : Object; blk : DataBlock)
  clear(dest_obj)
  using() <| $(var rawData : IntList)
    set(dest_obj, "rawData", rawData)
  using() <| $(var paramTypes : UInt8List)
    set(dest_obj, "paramTypes", paramTypes)
  using() <| $(var paramOffsets : IntList)
    set(dest_obj, "paramOffsets", paramOffsets)
  using() <| $(var paramIds : IntList)
    set(dest_obj, "paramIds", paramIds)
  assume rawData = *getRW_ecs_IntList(dest_obj, "rawData")
  assume paramTypes = *getRW_ecs_UInt8List(dest_obj, "paramTypes")
  assume paramOffsets = *getRW_ecs_IntList(dest_obj, "paramOffsets")
  assume paramIds = *getRW_ecs_IntList(dest_obj, "paramIds")
  anim_graph_getParamNames(anim_graph) <| $(params)
    for param in params
      let paramType = anim_graph_getParamType(anim_graph, param.id)
      let paramName = replace_pointer_in_name(string(param.name), anim_graph)
      let paramBlk = datablock_get_block_by_name(blk, paramName)
      if paramBlk == null
        continue
      push(paramIds, param.id)
      push(paramTypes, uint8(paramType))
      push(paramOffsets, length(rawData))
      let loadedParamType = uint(datablock_getInt(*paramBlk, "type", -1))
      if paramType != loadedParamType
        logerr("param type mismatch: {paramType} != {loadedParamType} ({param.name})")
        push_zero_param(rawData, paramType, anim_state, param.id)
        continue
      if paramType == PT_ScalarParam || paramType == PT_ScalarParamInt || paramType == PT_TimeParam
        push(rawData, datablock_getInt(*paramBlk, "value", 0))
      elif paramType == PT_Fifo3
        let fifoQueueSize = typeinfo sizeof(type<AnimFifo3Queue>) / 4
        resize(rawData, length(rawData) + fifoQueueSize)
        unsafe
          var fifoQueuePtr = reinterpret<AnimFifo3Queue?>(addr(rawData[length(rawData) - fifoQueueSize]))
          for i in range(3)
            let animNodeName = datablock_getStr(paramBlk, "node{i}", "")
            let nodeId = anim_graph_getBlendNodeId(anim_graph, animNodeName)
            fifoQueuePtr.node[i] = anim_graph_getBlendNodePtr(anim_graph, nodeId)
          fifoQueuePtr.morphTime[0] = datablock_getReal(paramBlk, "morphTime0", 0f)
          fifoQueuePtr.morphTime[1] = datablock_getReal(paramBlk, "morphTime1", 0f)
          fifoQueuePtr.state = datablock_getInt(paramBlk, "state", 0)
          fifoQueuePtr.t0 = datablock_getReal(paramBlk, "t0", 0f)
      elif paramType == PT_InlinePtr || paramType == PT_Effector
        let expectedSize = anim_state_holder_getInlinePtrMaxSz(anim_state, param.id) / 4u
        datablock_get_block(*paramBlk, "raw") <| $(raw)
          let validSize = expectedSize == raw.paramCount
          if !validSize
            logerr("inline ptr param size mismatch: {expectedSize} != {raw.paramCount} ({param.name})")
          for i in range(expectedSize)
            push(rawData, validSize ? datablock_getInt(raw, i) : 0)

def apply_anim_state_parameters(var anim_state : IAnimStateHolder; src_obj : Object)
  assume rawData = *get_ecs_IntList(src_obj, "rawData")
  assume paramTypes = *get_ecs_UInt8List(src_obj, "paramTypes")
  assume paramOffsets = *get_ecs_IntList(src_obj, "paramOffsets")
  assume paramIds = *get_ecs_IntList(src_obj, "paramIds")
  for pType, paramId, paramOffset, i in paramTypes, paramIds, paramOffsets, iter_range(paramTypes)
    let paramType = uint(pType)
    if paramType == PT_ScalarParam
      anim_state_holder_setParam(anim_state, paramId, int_bits_to_float(rawData[paramOffset]))
    elif paramType == PT_ScalarParamInt
      anim_state_holder_setParamInt(anim_state, paramId, rawData[paramOffset])
    elif paramType == PT_TimeParam
      anim_state_holder_setParam(anim_state, paramId, int_bits_to_float(rawData[paramOffset]))
    elif paramType == PT_InlinePtr || paramType == PT_Fifo3 || paramType == PT_Effector
      var ptr = anim_state_holder_getInlinePtr(anim_state, paramId)
      let nextOffset = i + 1 < length(paramOffsets) ? paramOffsets[i + 1] : length(rawData)
      let copySizeInBytes = (nextOffset - paramOffset) * 4
      unsafe
        memcpy(ptr, addr(rawData[paramOffset]), copySizeInBytes)

def erase_if(var arr : Array; blk : block<(key : ChildComponent) : bool>)
  var i = length(arr) - 1
  while i >= 0
    if invoke(blk, arr[i])
      erase(arr, i)
    i--

def patch_anim_graph_data(var animchar_data : Object; anim_graph : AnimationGraph; anim_state : IAnimStateHolder; anim_state_params : DataBlock)
  var animParams & = unsafe(*getRW_ecs_array(animchar_data, "animParams"))
  for animParamChildComp in animParams
    assume animParam = *getRW_ecs_object(animParamChildComp)
    let paramName = string(*get_ecs_string(animParam, "name"))
    let paramId = anim_graph_getParamId(anim_graph, paramName, int(PT_ScalarParam))
    set(animParam, "id", paramId)
    if paramId < 0
      logwarn("anim param '{paramName}' not found")
  erase_if(animParams) <| $(animParamChildComp)
    assume animParam = *get_ecs_object(animParamChildComp)
    return *get_int(animParam, "id") < 0
  using() <| $(var initialAnimStateParams : Object)
    load_anim_state_parameters(anim_graph, anim_state, initialAnimStateParams, anim_state_params)
    set(animchar_data, "initialAnimStateParams", initialAnimStateParams)
  assume animTreeStates = *getRW_ecs_object(animchar_data, "animTreeStates")
  assume stateNames = *get_ecs_StringList(animTreeStates, "stateNames")
  for combinedState in *getRW_ecs_array(animTreeStates, "state")
    for state in *getRW_ecs_object(combinedState)
      assume stateIdx = *getRW_int(state.value)
      let stateName = string(stateNames[stateIdx])
      stateIdx = anim_graph_getStateIdx(anim_graph, stateName)
      if stateIdx < 0
        logerr("state '{stateName}' ({state.key}) not found")
        stateIdx = 0
