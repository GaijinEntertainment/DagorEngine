module motion_matching_common shared

require ecs
require DagorDataBlock
require DagorSystem
require MotionMatching
require daslib/strings_boost

def write_weights_to_block(var blk : DataBlock; weights : FeatureWeights)

  for p, v in weights.nodePositions, weights.nodeVelocities
    blk |> datablock_add_new_block("node") <| $(node)
      node |> add("position_weight", p)
      node |> add("velocity_weight", v)

  for p, v in weights.rootPositions, weights.rootDirections
    blk |> datablock_add_new_block("point") <| $(node)
      node |> add("position_weight", p)
      node |> add("velocity_weight", v)


def serialize_node_names(var blk : DataBlock; nodes_name : StringList)
  blk |> datablock_add_new_block("nodes_name") <| $(nodesNameBlk)
    for name in nodes_name
      nodesNameBlk |> add("node", "{name}")


def serialize_prediction_times(var blk : DataBlock; prediction_times : FloatList)
  blk |> datablock_add_new_block("prediction_times") <| $(predTimesBlk)
    for time in prediction_times
      predTimesBlk |> add("time", time)

def serialize_presets(path : string; presets : TagPresetVector; nodes_name : StringList; prediction_times : FloatList; presets_tag_names : StringList)

  if !empty(path)
    using() <| $(var blk : DataBlock)
      serialize_node_names(blk, nodes_name)
      serialize_prediction_times(blk, prediction_times)

      for preset, tagName in presets, presets_tag_names
        blk |> datablock_add_new_block("{tagName}") <| $(presetBlk)
          write_weights_to_block(presetBlk, preset.weights)
          presetBlk |> add("animationBlendTime", preset.animationBlendTime)
          presetBlk |> add("linearVelocityViscosity", preset.linearVelocityViscosity)
          presetBlk |> add("angularVelocityViscosity", preset.angularVelocityViscosity)

      datablock_save_to_text_file(blk, path)

  return true


def read_weights_from_block(var weights : FeatureWeights; blk : DataBlock; node_count, point_count : int)

  init_feature_weights(weights, node_count, point_count)

  for p, v, j in weights.nodePositions, weights.nodeVelocities, urange(0, node_count)
    datablock_get_block(blk, j) <| $(node)
      p = datablock_getReal(node, "position_weight", 0.0)
      v = datablock_getReal(node, "velocity_weight", 0.0)


  for p, d, j in weights.rootPositions, weights.rootDirections, urange(node_count, node_count + point_count)
    datablock_get_block(blk, j) <| $(point)
      p = datablock_getReal(point, "position_weight", 0.0)
      d = datablock_getReal(point, "direction_weight", 0.0)

  commit_feature_weights(weights)

def deserialize_node_names(blk : DataBlock; var nodes_name : StringList)
  nodes_name |> resize(int(blk.paramCount))
  for i in range(blk.paramCount)
    nodes_name[i] := blk |> datablock_getStr(i)

def deserialize_prediction_times(blk : DataBlock; var prediction_times : FloatList)
  prediction_times |> resize(int(blk.paramCount))
  for i in range(blk.paramCount)
    prediction_times[i] = blk |> datablock_getReal(i)

def deserialize_presets(path : string;
                        var presets : TagPresetVector;
                        var nodes_name : StringList;
                        var prediction_times : FloatList;
                        var presets_tags_name : StringList)
  presets |> clear()
  nodes_name |> clear()
  prediction_times |> clear()
  presets_tags_name |> clear()

  var result = false
  using() <| $(var blk : DataBlock)
    if !datablock_load(blk, path)
      return
    deserialize_node_names(*(blk |> datablock_get_block_by_name("nodes_name")), nodes_name)
    deserialize_prediction_times(*(blk |> datablock_get_block_by_name("prediction_times")), prediction_times)

    let nodeCount = nodes_name |> length()
    let pointCount = prediction_times |> length()
    let presetsCount = int(blk.blockCount)

    //two block for nodes_name and prediction_times
    presets |> resize(presetsCount - 2)
    for i, j in urange(2, presetsCount), range(presetsCount)
      datablock_get_block(blk, i) <| $(presetBlk)
        if (nodeCount + pointCount != int(presetBlk.blockCount))
          logerr("missmatch nodes_names and prediction_time {nodeCount}+{pointCount}!={presetBlk.blockCount} in weightPreset=[{presetBlk.getBlockName}] in {path}")
          return
        presets_tags_name |> push(presetBlk.getBlockName)
        read_weights_from_block(presets[j].weights, presetBlk, nodeCount, pointCount)
        presets[j].animationBlendTime = presetBlk |> datablock_getReal("animationBlendTime", 0.2f)
        presets[j].linearVelocityViscosity = presetBlk |> datablock_getReal("linearVelocityViscosity", 0.1f)
        presets[j].angularVelocityViscosity = presetBlk |> datablock_getReal("angularVelocityViscosity", 0.1f)
        presets[j].metricaToleranceMin = presetBlk |> datablock_getReal("metricaToleranceMin", 0f)
        presets[j].metricaToleranceMax = presetBlk |> datablock_getReal("metricaToleranceMax", 0f)
        presets[j].metricaToleranceDecayTime = presetBlk |> datablock_getReal("metricaToleranceDecayTime", 0f)
        presets[j].presetBlendTime = presetBlk |> datablock_getReal("presetBlendTime", 0f)

    result = true
  if presets |> length() == 0
    presets_tags_name |> resize(1)
    presets |> resize(1)
  return result

def deserealize_root_motion_directions(blk : DataBlock;
                                       var directionNodes : StringList;
                                       var directionWeights : FloatList)
  directionNodes |> resize(int(blk.blockCount))
  directionWeights |> resize(int(blk.blockCount))
  for i in urange(0, int(blk.blockCount))
    datablock_get_block(blk, i) <| $(node)
      directionNodes[i] := datablock_getStr(node, "name", "")
      directionWeights[i] = datablock_getReal(node, "weight", 0.0)

def deserealize_root_motion_center_of_mass(blk : DataBlock;
                                           var centerOfMassNodes : StringList;
                                           var centerOfMassParams : Point4List)
  centerOfMassNodes |> resize(int(blk.blockCount))
  centerOfMassParams |> resize(int(blk.blockCount))
  for i in urange(0, int(blk.blockCount))
    datablock_get_block(blk, i) <| $(node)
      centerOfMassNodes[i] := datablock_getStr(node, "name", "")
      centerOfMassParams[i] = datablock_getPoint4(node, "params", float4(0, 0, 0, 1))

def deserialize_root_motions(path : string;
                             var rootNode : das_string;
                             var rootMotionA2dNode : das_string;
                             var directionNodes : StringList;
                             var directionWeights : FloatList;
                             var centerOfMassNodes : StringList;
                             var centerOfMassParams : Point4List)
  rootNode := ""
  rootMotionA2dNode := ""
  directionNodes |> clear()
  directionWeights |> clear()
  centerOfMassNodes |> clear()
  centerOfMassParams |> clear()

  var result = false
  using() <| $(var blk : DataBlock)
    if !datablock_load(blk, path)
      return
    rootNode := blk |> datablock_getStr("root_node", "")
    rootMotionA2dNode := blk |> datablock_getStr("root_motion_a2d_node", "")
    datablock_get_block(blk, "direction_nodes") <| $(directionsBlk)
      deserealize_root_motion_directions(directionsBlk, directionNodes, directionWeights)
    datablock_get_block(blk, "center_of_mass") <| $(conterOfMassBlk)
      deserealize_root_motion_center_of_mass(conterOfMassBlk, centerOfMassNodes, centerOfMassParams)
    result = true
  return result


def serialize_root_motions(path : string;
                           rootNode : das_string;
                           directionNodes : StringList;
                           directionWeights : FloatList;
                           centerOfMassNodes : StringList;
                           centerOfMassParams : Point4List)
  var result = false
  if !empty(path)
    using() <| $(var blk : DataBlock)
      blk |> add("root_node", string(rootNode))
      blk |> datablock_add_new_block("direction_nodes") <| $(directionsBlk)
        for name, weight in directionNodes, directionWeights
          directionsBlk |> datablock_add_new_block("node") <| $(node)
            node |> add("name", string(name))
            node |> add("weight", weight)
      blk |> datablock_add_new_block("center_of_mass") <| $(conterOfMassBlk)
        for name, params in centerOfMassNodes, centerOfMassParams
          conterOfMassBlk |> datablock_add_new_block("node") <| $(node)
            node |> add("node", string(name))
            node |> add("params", params)
      datablock_save_to_text_file(blk, path)
      result = true
  return result