options no_aot
require ecs
require AnimV20
require AssetsImport
require DagorConsole
require MotionMatching
require %gameLibs.anim.motionMatching.das.mm_events
require gameLibs.assets_import.main.asset_manager_events

[es(tag=dev, on_appear, before=init_animations_es)]
def motion_matching_reload_animchar_resources(evt : Event;
                                              eid aka appeared_animchar_eid : EntityId;
                                              animchar : AnimcharBaseComponent;
                                              motion_matching__dataBaseTemplateName : string)
  if get_asset_manager() == null
    return
  let dataBaseEid = getSingletonEntity(motion_matching__dataBaseTemplateName)
  assume dataBaseName = motion_matching__dataBaseTemplateName
  query(dataBaseEid) <| $ [es] (main_database__loaded : bool; dataBase : AnimationDataBase)
    // After asset manager is initialized all new animchars will use different resources (animTree and skeleton), which
    // loaded from sources with in-game dabuild. MM database can work only with same resources, so we need to reload them
    // for all exisiting characters and also invalidate database to remove outdated references
    if (main_database__loaded && dataBase.referenceSkeleton == animchar.originalNodeTree &&
        dataBase.referenceAnimGraph == animchar.animGraph)
      return
    var reloadCount = 0
    query() <| $ [es] (eid : EntityId; motion_matching__dataBaseTemplateName : string)
      if eid != appeared_animchar_eid && dataBaseName == motion_matching__dataBaseTemplateName
        reload_animchar_resources(eid, true, false);
        reloadCount++
    if reloadCount > 0
      visual_log("Reloaded animTree and skeleton for {reloadCount} animchars with '{dataBaseName}' MM database")
    if main_database__loaded
      broadcastEventImmediate(InvalidateAnimationDataBase())

[es(tag=dev, after=track_animchar_assets_es)]
def motion_matching_invalidate_database_on_anim_tree_changed(evt : AssetChangedEvent)
  if evt.asset.typeName == "animTree" || evt.asset.typeName == "a2d"
    broadcastEventImmediate(InvalidateAnimationDataBase())
