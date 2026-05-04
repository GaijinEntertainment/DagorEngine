module anim_replay_motion_matching_file_io shared
options no_aot

require ecs
require ecs.safe
require strings
require DagorDataBlock
require DagorSystem
require EcsUtils
require MotionMatching

def save_mm_animchar_data_to_blk(var blk : DataBlock; animchar_data : Object)
  for elem in animchar_data
    if starts_with(elem.key, "mm")
      component_to_blk_param(elem.key, getEntityComponentRef(elem.value), blk)

def save_motion_matching_databases(var blk : DataBlock)
  datablock_add_block(blk, "anim_replay__mmDataBases:object") <| $(var mm_databases : DataBlock)
    let mgr & = getEntityManager()
    query() <| $ [es] (eid : EntityId;
                       dataBase : AnimationDataBase)
      //todo: ignore unused databases
      let databaseName = getEntityTemplateName(eid)
      datablock_add_block(mm_databases, "{databaseName}:object") <| $(var database_blk : DataBlock)
        component_to_blk_param("availableTags", mgr |> getComponentRef(eid, "main_database__availableTags"), database_blk)
        datablock_add_block(database_blk, "clips:list<t>") <| $(var clips_list : DataBlock)
          for clip in dataBase.clips
            add(clips_list, "item", string(clip.name))
        set(database_blk, "trajectoryFeaturesCount", dataBase.trajectorySize * 2)
        set(database_blk, "poseFeaturesCount", dataBase.nodeCount * 2)

def patch_motion_matching_data(var anim_replay__loadingAnimchars : Array;
                               var anim_replay__mmDataBases : Object)
  for originalDataBase in anim_replay__mmDataBases
    let eid = getSingletonEntity(originalDataBase.key)
    var dbObj = getRW_ecs_object(originalDataBase.value)
    query(eid) <| $ [es] (dataBase : AnimationDataBase)
      assume originalClipNames = *get_ecs_StringList(*dbObj, "clips")
      using() <| $(var clipsRemap : IntList)
        for clipName in originalClipNames
          var clipIdx = -1
          for i, clip in iter_range(dataBase.clips), dataBase.clips
            if clipName == clip.name
              clipIdx = i
              break
          push(clipsRemap, clipIdx)
        set(*dbObj, "clipsRemap", clipsRemap)

  for animchar in anim_replay__loadingAnimchars
    assume animcharData = *getRW_ecs_object(animchar)
    assume mmTransitionClipIdx = *getRW_ecs_IntList(animcharData, "mmTransitionClipIdx")
    assume mmTagsMask = *getRW_ecs_IntList(animcharData, "mmTagsMask")
    let eid = *get_Eid(animcharData, "eid")
    query(eid) <| $[es] (motion_matching__dataBaseEid : EntityId;
                         motion_matching__dataBaseTemplateName : string)
      assume originalDataBase = *getRW_ecs_object(anim_replay__mmDataBases, motion_matching__dataBaseTemplateName)
      assume clipsRemap = *getRW_ecs_IntList(originalDataBase, "clipsRemap")
      assume clipNames = *get_ecs_StringList(originalDataBase, "clips")
      for clipIdx in mmTransitionClipIdx
        let idx = clipsRemap[clipIdx]
        if idx >= 0
          clipIdx = idx
        elif idx == -1
          logerr("animation '{clipNames[clipIdx]}' not found")
          clipsRemap[clipIdx] = -2
      assume availableTags = *get_ecs_StringList(originalDataBase, "availableTags")
      let originalTrajectoryFeaturesCount = *get_int(originalDataBase, "trajectoryFeaturesCount")
      let originalPoseFeaturesCount = get_int(originalDataBase, "poseFeaturesCount") ?? 0
      query(motion_matching__dataBaseEid) <| $[es] (dataBase : AnimationDataBase;
                                                    main_database__availableTags : StringList)
        var sameTags = length(main_database__availableTags) == length(availableTags)
        for originalTag, currentTag in availableTags, main_database__availableTags
          if originalTag != currentTag
            sameTags = false
        if !sameTags
          logerr("tags are different")
          for mask in mmTagsMask
            mask = 0
        if originalTrajectoryFeaturesCount != dataBase.trajectorySize * 2
          logerr("trajectory features count mismatch")
          assume mmTrajectoryFeatures = *getRW_ecs_Point2List(animcharData, "mmTrajectoryFeatures")
          clear(mmTrajectoryFeatures)
        if originalPoseFeaturesCount != dataBase.nodeCount * 2
          if originalPoseFeaturesCount > 0
            logerr("pose features count mismatch")
            assume mmPoseFeatures = *getRW_ecs_Point3List(animcharData, "mmPoseFeatures")
            clear(mmPoseFeatures)
          else
            using() <| $(var mmPoseFeatures : Point3List)
              set(animcharData, "mmPoseFeatures", mmPoseFeatures)
    // Outdated parameter, should be completely removed in future
    let mmTransitionTrajectoryFeatures = get_ecs_Point2List(animcharData, "mmTransitionTrajectoryFeatures")
    if mmTransitionTrajectoryFeatures != null
      verify(get_ecs_Point2List(animcharData, "mmSearchTrajectoryFeatures") == null &&
        get_ecs_FloatList(animcharData, "mmSearchTime") == null)
      set(animcharData, "mmSearchTrajectoryFeatures", *mmTransitionTrajectoryFeatures)
      set(animcharData, "mmSearchTime", *get_ecs_FloatList(animcharData, "mmTransitionTime"))

def are_motion_matching_databases_loaded(animchars : Array)
  for animchar in animchars
    assume animcharData = *get_ecs_object(animchar)
    let eid = *get_Eid(animcharData, "eid")
    var dataBaseLoaded = false
    query(eid) <| $[es] (motion_matching__dataBaseEid : EntityId)
      if motion_matching__dataBaseEid != INVALID_ENTITY_ID
        dataBaseLoaded = true
    if !dataBaseLoaded
      return false
  return true
