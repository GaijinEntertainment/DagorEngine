options no_aot
options no_global_variables = false
options persistent_heap

require ecs
require AnimV20
require DagorConsole
require DagorDataBlock
require DagorSystem
require EcsUtils
require %gameLibs/anim/animReplay/imports/anim_replay_anim_state_params
require %gameLibs/anim/animReplay/imports/anim_replay_create_animchars
require %gameLibs/anim/animReplay/imports/anim_replay_events
require %gameLibs/anim/animReplay/imports/anim_replay_motion_matching_file_io

def save_optional_component_to_blk(animchar_data : Object; comp_name : string; var blk : DataBlock)
  let childComp = get_child(animchar_data, comp_name)
  if childComp != null
    component_to_blk_param(comp_name, getEntityComponentRef(*childComp), blk)

def save_animchar_data_to_blk(var blk : DataBlock; animchar_data : Object)
  let eid = *get_Eid(animchar_data, "eid")
  query(eid) <| $ [es] (animchar : AnimcharBaseComponent)
    assume animGraph = *animchar.animGraph
    component_to_blk_param("position", getEntityComponentRef(*get_child(animchar_data, "position")), blk)
    component_to_blk_param("rotation", getEntityComponentRef(*get_child(animchar_data, "rotation")), blk)
    component_to_blk_param("animParams", getEntityComponentRef(*get_child(animchar_data, "animParams")), blk)
    datablock_add_block(blk, "_initialAnimStateParams") <| $(var initialAnimStateParams : DataBlock)
      copy_anim_state_parameters_to_blk(animGraph, *get_ecs_object(animchar_data, "initialAnimStateParams"), initialAnimStateParams)
    component_to_blk_param("animTreeStates", getEntityComponentRef(*get_child(animchar_data, "animTreeStates")), blk)
    component_to_blk_param("animcharRes", getEntityComponentRef(*get_child(animchar_data, "animcharRes")), blk)
    component_to_blk_param("originalTemplate", getEntityComponentRef(*get_child(animchar_data, "originalTemplate")), blk)
    component_to_blk_param("additionalTemplates", getEntityComponentRef(*get_child(animchar_data, "additionalTemplates")), blk)
  save_optional_component_to_blk(animchar_data, "deathTime", blk)
  save_mm_animchar_data_to_blk(blk, animchar_data)

[es(REQUIRE=anim_replay__animchars)]
def save_anim_replay_to_file(evt : AnimReplaySaveRecord;
                             anim_replay__animchars : Array;
                             eid : EntityId)
  using() <| $(var blk : DataBlock)
    let mgr & = getEntityManager()
    component_to_blk_param("anim_replay__timestamps", mgr |> getComponentRef(eid, "anim_replay__timestamps"), blk)
    datablock_add_block(blk, "anim_replay__loadingAnimchars:array") <| $(var animchars_blk : DataBlock)
      for animcharChildComp, i in anim_replay__animchars, iter_range(anim_replay__animchars)
        assume animcharData = *get_ecs_object(animcharChildComp)
        datablock_add_block(animchars_blk, "animchar{i}:object") <| $(var animchar_blk : DataBlock)
          save_animchar_data_to_blk(animchar_blk, animcharData)
    save_motion_matching_databases(blk)

    if evt.saveAsText
      datablock_save_to_text_file(blk, evt.filepath)
    else
      datablock_save_to_binary_file(blk, evt.filepath)
    visual_log("Successfully saved to {evt.filepath} " + (evt.saveAsText ? "(as text)" : "(as binary)"))

def patch_animchar_data(var animchar_data : Object; blk : DataBlock; animchar_idx : int; num_timestamps : int)
  let eid = *get_Eid(animchar_data, "eid")
  query(eid) <| $[es] (animchar : AnimcharBaseComponent)
    assume animGraph = *animchar.animGraph
    assume animState = *animchar.animState
    let initialParamsBlk = datablock_get_block_by_name(blk, "_initialAnimStateParams")
    patch_anim_graph_data(animchar_data, animGraph, animState, *initialParamsBlk)
  if get_float(animchar_data, "deathTime") == null
    var validData = true
    validData &&= length(*get_ecs_Point3List(animchar_data, "position")) == num_timestamps
    validData &&= length(*get_ecs_Point4List(animchar_data, "rotation")) == num_timestamps
    //NOTE: we can verify more data here, but I'm not sure that it is really necessary
    if !validData
      let templateName = string(*get_ecs_string(animchar_data, "originalTemplate"))
      logerr("Recorded animchar '{animchar_idx + 1}. {templateName}' has invalid size for position/rotation data" +
             " and 'deathTime' component is missing")
      set(animchar_data, "deathTime", 0.0f)

var replay_blk : DataBlock?


[es(on_event=AnimReplayAnimcharsCreated)]
def anim_replay_loading_animchars_created(evt : Event; var anim_replay__animcharsLoaded : bool&)
  anim_replay__animcharsLoaded = true

[es(no_order)]
def finalize_anim_replay_loading(evt : UpdateStageInfoAct;
                                 eid : EntityId;
                                 var anim_replay__loadingAnimchars : Array;
                                 var anim_replay__mmDataBases : Object;
                                 anim_replay__timestamps : FloatList;
                                 anim_replay__loadingFromFile : bool;
                                 anim_replay__animcharsLoaded : bool)
  if !anim_replay__animcharsLoaded || !are_motion_matching_databases_loaded(anim_replay__loadingAnimchars)
    return
  if anim_replay__loadingFromFile
    var i = 0
    let numTimestamps = length(anim_replay__timestamps)
    let animcharsBlk = datablock_get_block_by_name(*replay_blk, "anim_replay__loadingAnimchars:array")
    for animchar in anim_replay__loadingAnimchars
      assume animcharData = *getRW_ecs_object(animchar)
      patch_animchar_data(animcharData, *datablock_get_block(*animcharsBlk, uint(i)), i, numTimestamps)
      i++
    patch_motion_matching_data(anim_replay__loadingAnimchars, anim_replay__mmDataBases)
    unsafe
      delete replay_blk
  createEntity("anim_replay") <| $(var init : ComponentsInitializer)
    set(init, "anim_replay__timestamps", anim_replay__timestamps)
    set(init, "anim_replay__animchars", anim_replay__loadingAnimchars)
  destroyEntity(eid)

[es]
def load_anim_replay_from_file(evt : AnimReplayLoadRecord)
  if (find_query <| $ [es(REQUIRE=anim_replay__loadingAnimchars)] () => true)
    logerr("already loading another replay")
    return
  replay_blk = new DataBlock
  if !datablock_load(*replay_blk, evt.filepath, DataBlockReadFlag.ROBUST)
    logerr("failed to load '{evt.filepath}'")
    unsafe
      delete replay_blk
    return
  createEntitySync("anim_replay_load_context") <| $(var init : ComponentsInitializer)
    set(init, "anim_replay__loadingFromFile", true)
    using() <| $(var comps : ComponentsList)
      load_comp_list_from_blk(*replay_blk, comps)
      find_component(comps) <| $(comp_name : string; comp : ChildComponent const#)
        set(init, comp_name, comp)
        return false

[es(on_appear)]
def anim_replay_create_loading_animchars(evt : Event; var anim_replay__loadingAnimchars : Array)
  schedule_animchars_creation(anim_replay__loadingAnimchars)
