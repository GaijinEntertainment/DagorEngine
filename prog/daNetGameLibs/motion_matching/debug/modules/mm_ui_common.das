options no_aot
module mm_ui_common shared

require ecs
require DagorSystem
require imgui
require imgui/imgui_boost
require MotionMatching
require daslib/safe_addr
require daslib/strings_boost
require DagorConsole
//require stddlg
require danetlibs.motion_matching.es.motion_matching_common
require danetlibs.motion_matching.es.mm_events
require danetlibs.motion_matching.debug.modules.mm_ui_imgui_helpers
require danetlibs.motion_matching.debug.modules.mm_ui_play_clips
require danetlibs.motion_matching.debug.modules.mm_ui_edit_tags


def index(tags : StringList; tag : string) : int
  return find_index_if(each(tags)) <| $(s) { return eq(s, tag); }

def edit_weights(var weights : FeatureWeights; nodes_name : StringList; prediction_times : FloatList)
  let hash = unsafe(addr(weights))
  for p, v, name in weights.nodePositions, weights.nodeVelocities, nodes_name
    imgui::Text("{name}")
    safe_slider("position##{name}{hash}", p, 0f, 10f)
    safe_slider("velocity##{name}{hash}", v, 0f, 10f)

  for p, v, t in weights.rootPositions, weights.rootDirections, prediction_times
    imgui::Text("prediction time {t}")

    safe_slider("position##{t}{hash}", p, 0f, 10f)
    safe_slider("velocity##{t}{hash}", v, 0f, 10f)
  commit_feature_weights(weights)

def edit_preset(var preset : TagPreset)
  let hash = unsafe(addr(preset))

  safe_slider("animationBlendTime##{hash}", preset.animationBlendTime, 0f, 1f)
  safe_slider("linearVelocityViscosity##{hash}", preset.linearVelocityViscosity, 0f, 1f)
  safe_slider("angularVelocityViscosity##{hash}", preset.angularVelocityViscosity, 0f, 1f)
  safe_slider("metricaToleranceMin##{hash}", preset.metricaToleranceMin, 0f, 5f)
  safe_slider("metricaToleranceMax##{hash}", preset.metricaToleranceMax, 0f, 5f)
  safe_slider("metricaToleranceDecayTime##{hash}", preset.metricaToleranceDecayTime, 0f, 3f)

def edit_value(text : string; var value : das_string&)
  let buf <- ImGuiInputTextBuffer("{value}", 256)
  if InputText(text, buf, ImGuiInputTextFlags.AutoSelectAll)
    value := buf |> to_string()
    return true
  return false

def edit_value(text : string; var value : float&)
  var tmpValue = value
  let result = InputFloat(text, safe_addr(tmpValue))
  value = tmpValue
  return result

[generic]
def edit_list(var list : auto(TT)&; text : string) : tuple<edited : bool; enlarged : bool; eraseIdx : int>
  let listHash = unsafe(addr(list))
  var eraseIdx = -1
  var edited = false
  var enlarged = false
  imgui::Text("use Ctrl+C, Ctrl+V to edit these fields")
  for element, i in list, range(length(list))

    edited = edit_value("##{i}{listHash}", element) || edited
    imgui::SameLine()
    if imgui::Button("delete##{i}{listHash}")
      eraseIdx = i

  if eraseIdx >= 0
    erase(list, eraseIdx)
    edited = true
  if imgui::Button("add {text}##{listHash}")
    list |> resize(length(list) + 1)
    enlarged = true
    edited = true
  return (edited, enlarged, eraseIdx)

def edit_root_update_params(var controller : MotionMatchingController)
  safe_checkbox("Root synchronization", controller.rootSynchronization)
  safe_checkbox("Root adjustment", controller.rootAdjustment)
  if controller.rootAdjustment
    imgui::Indent()
    safe_slider("velocity ratio", controller.rootAdjustmentVelocityRatio, 0f, 1f)
    safe_slider("time to adjust position", controller.rootAdjustmentPosHalfLife, 0f, 1f)
    safe_slider("angular velocity ratio", controller.rootAdjustmentAngVelocityRatio, 0f, 1f)
    safe_slider("time to adjust rotation", controller.rootAdjustmentRotHalfLife, 0f, 1f)
    safe_checkbox("adjustment by future trajectory", controller.rootAdjustmentByFutureTrajectory)
    imgui::Unindent()
  safe_checkbox("Root clamping", controller.rootClamping)
  if controller.rootClamping
    imgui::Indent()
    safe_slider("distance", controller.rootClampingMaxDistance, 0f, 1f)
    safe_slider("angle", controller.rootClampingMaxAngle, 0f, PI)
    imgui::Unindent()

def edit_params(characterEid, dataBaseEid : EntityId)
  query(characterEid) <| $ [es] (motion_matching__presetIdx : int;
                                 var motion_matching__controller : MotionMatchingController;
                                 var mm_trajectory__linearVelocityViscosity : float&;
                                 var mm_trajectory__angularVelocityViscosity : float&)
    if imgui::BeginTabItem("params")
      query(dataBaseEid) <| $ [es] (main_database__footLockerCtrlName : string;
                                    var dataBase : AnimationDataBase)
        safe_slider("blend time", dataBase.tagsPresets[max(motion_matching__presetIdx, 0)].animationBlendTime, 0f, 1f)
        safe_slider("velocity viscosity", mm_trajectory__linearVelocityViscosity, 0f, 1f)
        safe_slider("rotation viscosity", mm_trajectory__angularVelocityViscosity, 0f, 1f)
        imgui::Text("weights preset index = {motion_matching__presetIdx}")

        edit_root_update_params(motion_matching__controller)

        query() <| $ [es] (var mm_params__footLockerEnabled : bool&)
          if dataBase.footLockerParamId >= 0 && safe_checkbox("Foot IK locker (animTree)", mm_params__footLockerEnabled)
            let footLockerPBCIdx = get_post_blend_controller_idx(dataBase, main_database__footLockerCtrlName)
            dataBase |> changePBCWeightOverride(footLockerPBCIdx, mm_params__footLockerEnabled ? 1f : 0f)

      imgui::EndTabItem()

def debug_visualization_tab(imgui_state_eid : EntityId)
  if imgui::BeginTabItem("visualization")
    query(imgui_state_eid) <| $ [es] (var mm_visualization_show_feature_nodes : bool&;
                                      var mm_visualization_show_trajectory : bool&;
                                      var mm_visualization_show_skeleton : bool&;
                                      var mm_visualization_show_skeleton_original : bool&;
                                      var mm_visualization_show_skeleton_node_labels : bool&;
                                      var mm_visualization_show_foot_locker_positions : bool&)
      safe_checkbox("Show featured nodes", mm_visualization_show_feature_nodes)
      safe_checkbox("Show trajectory", mm_visualization_show_trajectory)
      safe_checkbox("Show skeleton", mm_visualization_show_skeleton)
      if mm_visualization_show_skeleton
        imgui::Indent()
        safe_checkbox("Show original skeleton", mm_visualization_show_skeleton_original)
        safe_checkbox("Show node labels", mm_visualization_show_skeleton_node_labels)
        imgui::Unindent()
      imgui::Separator()
      imgui::Text("Foot locker IK")
      safe_checkbox("Show locked position", mm_visualization_show_foot_locker_positions)
    imgui::EndTabItem()

def edit_presets_weights(dataBaseEid : EntityId)
  var invalidateDatabase = false
  if imgui::BeginTabItem("weights")

    query(dataBaseEid) <| $ [es] (main_database__presetsTagsName : StringList;
                                  var dataBase : AnimationDataBase&;
                                  weightsPath : string)

      let editedNode = edit_list(dataBase.nodesName, "node")
      let editedTime = edit_list(dataBase.predictionTimes, "prediction time")
      if editedNode.edited || editedTime.edited
        for preset in dataBase.tagsPresets
          if editedNode.enlarged
            preset.weights.nodePositions |> push(0f)
            preset.weights.nodeVelocities |> push(0f)
          if editedNode.eraseIdx >= 0
            preset.weights.nodePositions |> erase(editedNode.eraseIdx)
            preset.weights.nodeVelocities |> erase(editedNode.eraseIdx)

          if editedTime.enlarged
            preset.weights.rootPositions |> push(0f)
            preset.weights.rootDirections |> push(0f)
          if editedTime.eraseIdx >= 0
            preset.weights.rootPositions |> erase(editedTime.eraseIdx)
            preset.weights.rootDirections |> erase(editedTime.eraseIdx)
          commit_feature_weights(preset.weights)

        dataBase.nodeCount = dataBase.nodesName |> length()
        dataBase.trajectorySize = dataBase.predictionTimes |> length()
        dataBase.featuresSize = get_features_sizes(dataBase.nodeCount, dataBase.trajectorySize)

        invalidateDatabase = true

      imgui::Separator()

      var first = true
      for preset, tagName in dataBase.tagsPresets, main_database__presetsTagsName
        if first
          edit_weights(preset.weights, dataBase.nodesName, dataBase.predictionTimes)
          edit_preset(preset)
        elif imgui::TreeNode("{tagName}, tag index = {preset.requiredTagIdx}")
          edit_weights(preset.weights, dataBase.nodesName, dataBase.predictionTimes)
          edit_preset(preset)
          imgui::TreePop()
        first = false

      if imgui::Button("save##weights")
        serialize_presets(weightsPath, dataBase.tagsPresets, dataBase.nodesName, dataBase.predictionTimes, main_database__presetsTagsName)
    imgui::EndTabItem()

  return invalidateDatabase

def edit_presets_root_motion(dataBaseEid : EntityId)
  var invalidateDatabase = false
  if imgui::BeginTabItem("root motion")
    query(dataBaseEid) <| $ [es] (var main_database__root_node : das_string&;
                                  var main_database__direction_nodes : StringList;
                                  var main_database__direction_weights : FloatList;
                                  var main_database__center_of_mass_nodes : StringList;
                                  var main_database__center_of_mass_params : Point4List;
                                  rootMotionPath : string)
      imgui::Text("use Ctrl+C, Ctrl+V to edit the node name fields")
      invalidateDatabase = imgui_input_text("##root_node", main_database__root_node) || invalidateDatabase
      imgui::Separator()
      imgui::Text("Direction")
      var eraseIdx = -1
      for name, weight, i in main_database__direction_nodes, main_database__direction_weights, range(length(main_database__direction_nodes))
        invalidateDatabase = imgui_input_text("Node##{i}dirName", name) || invalidateDatabase
        imgui::SameLine()
        if imgui::Button("delete##{i}dir")
          eraseIdx = i
        safe_slider("Weight##{i}dirweight", weight, 0f, 100f)
      if eraseIdx >= 0
        erase(main_database__direction_nodes, eraseIdx)
        erase(main_database__direction_weights, eraseIdx)
        invalidateDatabase = true
        eraseIdx = -1
      if imgui::Button("add node##dir")
        main_database__direction_nodes |> resize(length(main_database__direction_nodes) + 1)
        main_database__direction_weights |> resize(length(main_database__direction_weights) + 1)
        invalidateDatabase = true
      imgui::Separator()
      imgui::Text("Center of mass")
      for name, params, i in main_database__center_of_mass_nodes, main_database__center_of_mass_params, range(length(main_database__center_of_mass_nodes))
        invalidateDatabase = imgui_input_text("Node##{i}COMName", name) || invalidateDatabase
        imgui::SameLine()
        if imgui::Button("delete##{i}COM")
          eraseIdx = i

        imgui::Text("{name} Offset")
        imgui::SameLine()
        var step = 0.025f
        var fastStep = 0.25f
        invalidateDatabase = imgui::InputScalar("x##{i}paramsx", ImGuiDataType.Float, safe_addr(params.x), safe_addr(step), safe_addr(fastStep), "%.2f", ImGuiInputTextFlags.None) || invalidateDatabase
        imgui::SameLine()
        invalidateDatabase = imgui::InputScalar("y##{i}paramsy", ImGuiDataType.Float, safe_addr(params.y), safe_addr(step), safe_addr(fastStep), "%.2f", ImGuiInputTextFlags.None) || invalidateDatabase
        imgui::SameLine()
        invalidateDatabase = imgui::InputScalar("z##{i}paramsz", ImGuiDataType.Float, safe_addr(params.z), safe_addr(step), safe_addr(fastStep), "%.2f", ImGuiInputTextFlags.None) || invalidateDatabase

        imgui::Text("{name} Weight")
        imgui::SameLine()
        invalidateDatabase = safe_slider("Weight##{i}comweight", params.w, 0f, 10f) || invalidateDatabase
      if eraseIdx >= 0
        erase(main_database__center_of_mass_nodes, eraseIdx)
        erase(main_database__center_of_mass_params, eraseIdx)
        invalidateDatabase = true
      if imgui::Button("add node##COM")
        main_database__center_of_mass_nodes |> resize(length(main_database__center_of_mass_nodes) + 1)
        main_database__center_of_mass_params |> resize(length(main_database__center_of_mass_params) + 1)
        invalidateDatabase = true
      if imgui::Button("save##rootMotions")
        serialize_root_motions(rootMotionPath, main_database__root_node, main_database__direction_nodes, main_database__direction_weights, main_database__center_of_mass_nodes, main_database__center_of_mass_params)
    imgui::EndTabItem()
  return invalidateDatabase

def edit_presets(dataBaseEid : EntityId)
  // add sub tabs for weights and root motion
  if imgui::BeginTabItem("presets")
    imgui::BeginTabBar("presets_Tabs")
    var invalidateDatabase = false
    invalidateDatabase = edit_presets_weights(dataBaseEid)
    invalidateDatabase = edit_presets_root_motion(dataBaseEid) || invalidateDatabase
    imgui::EndTabBar()

    if invalidateDatabase
      broadcastEvent(InvalidateAnimationDataBase())

    imgui::EndTabItem()

def motion_matching_imgui(eid : EntityId; imgui_state_eid : EntityId)

  imgui::PushItemWidth(150f)
  imgui::BeginTabBar("Tabs")

  query(eid) <| $ [es] (var motion_matching__controller : MotionMatchingController&;
                        motion_matching__dataBaseEid : EntityId)
    var dataBase = motion_matching__controller.dataBase
    if dataBase == null
      imgui::Text("MM database is not loaded")
      return
    edit_params(eid, motion_matching__dataBaseEid)

    debug_visualization_tab(imgui_state_eid)
    edit_tags(eid, *dataBase, motion_matching__controller)
    edit_presets(motion_matching__dataBaseEid)
    play_clips(eid, motion_matching__dataBaseEid, imgui_state_eid)

  imgui::EndTabBar()
