options no_aot
options no_global_variables = false
options persistent_heap

require ecs
require AnimV20
require AssetsImport
require DagorDataBlock
require %gameLibs.anim.animReplay.imports.anim_replay_anim_state_params
require gameLibs.assets_import.main.asset_manager_events

var persistent_anim_graph_data : DataBlock?

[es(before=track_animchar_assets_es)]
def save_anim_graph_data_before_reload(evt : AssetChangedEvent; anim_replay__animchars : Array)
  if evt.asset.typeName != "animTree"
    return
  if persistent_anim_graph_data != null
    unsafe
      delete persistent_anim_graph_data
  persistent_anim_graph_data = new DataBlock
  for animcharChildComp, i in anim_replay__animchars, count()
    let animcharData = get_ecs_object(animcharChildComp)
    let eid = *get_Eid(animcharData, "eid")
    query(eid) <| $ [es] (animchar : AnimcharBaseComponent)
      datablock_add_block(*persistent_anim_graph_data, "animchar{i}") <| $(var anim_state_params : DataBlock)
        copy_anim_state_parameters_to_blk(*animchar.animGraph, *get_ecs_object(*animcharData, "initialAnimStateParams"), anim_state_params)

[es(after=track_animchar_assets_es)]
def update_anim_graph_data_after_reload(evt : AssetChangedEvent; var anim_replay__animchars : Array)
  if evt.asset.typeName != "animTree"
    return
  for animcharChildComp, i in anim_replay__animchars, count()
    var animcharData = getRW_ecs_object(animcharChildComp)
    let eid = *get_Eid(animcharData, "eid")
    query(eid) <| $ [es] (animchar : AnimcharBaseComponent)
      datablock_get_block(*persistent_anim_graph_data, uint(i)) <| $(anim_state_params)
        patch_anim_graph_data(*animcharData, *animchar.animGraph, *animchar.animState, anim_state_params)
    var cachedAnimStateParams = getRW_ecs_array(animcharData, "cachedAnimStateParams")
    if cachedAnimStateParams != null
      clear(*cachedAnimStateParams)
  if persistent_anim_graph_data != null
    unsafe
      delete persistent_anim_graph_data
