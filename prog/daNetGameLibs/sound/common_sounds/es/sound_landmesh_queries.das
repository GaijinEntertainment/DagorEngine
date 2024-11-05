require ecs
require ecs.safe
require soundSystem
require DagorMath
require math.base
require PhysMat
require Dacoll
require sound_utils_net.modules.sound_utils_net_events
require gpuReadbackQuery
require landMesh
require puddleQueryManager
require DagorSystem


let INVALID_QUERY_ID = -1
let BIOME_WEIGHT_THERSHOLD = 0.05


[es(tag=sound, on_appear)]
def sound_landmesh_queries_appear(evt : Event;
                                  var sound_landmesh_queries__biomeQueryId : int&;
                                  var sound_landmesh_queries__puddleQueryId : int&;
                                  sound_landmesh_queries__biomes : Object;
                                  var sound_landmesh_queries__biomeToMaterial : IPoint2List&)
  sound_landmesh_queries__biomeQueryId = INVALID_QUERY_ID
  sound_landmesh_queries__puddleQueryId = INVALID_QUERY_ID

  sound_landmesh_queries__biomeToMaterial |> resize(max(0, get_num_biome_groups()))
  for idx in iter_range(sound_landmesh_queries__biomeToMaterial)
    sound_landmesh_queries__biomeToMaterial[idx].x = PHYSMAT_INVALID
    let group = get_biome_group_name(idx)
    let obj = sound_landmesh_queries__biomes[group] ?as Object
    if obj != null
      let physmat = obj?.physmat ?? ""
      let pmid = get_material_id(physmat)
      if pmid == PHYSMAT_INVALID
        logerr("pmid for biome '{group}' is not valid: there is no physmat '{physmat}'")
      sound_landmesh_queries__biomeToMaterial[idx].x = pmid
      sound_landmesh_queries__biomeToMaterial[idx].y = (obj?.overrideWater ?? false) ? 1 : 0


def apply(biome_index : int; biome_weight : float;
          sound_landmesh_queries__biomeToMaterial : IPoint2List;
          var sound_landmesh_queries__biomePhysMatId : int&;
          var sound_landmesh_queries__biomeOverridesWater : bool&)
  if biome_weight >= BIOME_WEIGHT_THERSHOLD && uint(biome_index) < uint(length(sound_landmesh_queries__biomeToMaterial))
    let btm = sound_landmesh_queries__biomeToMaterial[biome_index]
    if btm.x != PHYSMAT_INVALID
      sound_landmesh_queries__biomePhysMatId = btm.x
      sound_landmesh_queries__biomeOverridesWater = btm.y != 0


[es(tag=sound, after=sound_begin_update_es, before=sound_end_update_es)]
def sound_landmesh_queries_update(info : ParallelUpdateFrameDelayed;
                                  is_watched_sound : bool;
                                  transform : float3x4;

                                  sound_landmesh_queries_vars__speedSq : float;
                                  sound_landmesh_queries_vars__isOnGround : bool;

                                  sound_landmesh_queries__lmeshDiff : float;
                                  sound_landmesh_queries__farawayThreshold : float;
                                  sound_landmesh_queries__zeroSpeedThreshold : float;
                                  sound_landmesh_queries__heroSpeedToQueryInterval : float4;
                                  sound_landmesh_queries__enemySpeedToQueryInterval : float4;
                                  sound_landmesh_queries__heroEnemyInAirInterval : float2;
                                  sound_landmesh_queries__puddleQueryToDepth : float4;
                                  sound_landmesh_queries__biomeToMaterial : IPoint2List;

                                  var sound_landmesh_queries__lastQueryTime : float&;
                                  var sound_landmesh_queries__biomeQueryId : int&;
                                  var sound_landmesh_queries__puddleQueryId : int&;
                                  var sound_landmesh_queries__biomePhysMatId : int&;
                                  var sound_landmesh_queries__biomeOverridesWater : bool&;
                                  var sound_landmesh_queries__puddleDepth : float&)

  if sound_landmesh_queries__biomeQueryId != INVALID_QUERY_ID

    var biomeResult = BiomeQueryResult()
    let biomeState = biome_query_result(sound_landmesh_queries__biomeQueryId, biomeResult)
    if is_gpu_readback_query_successful(biomeState) || is_gpu_readback_query_failed(biomeState)
      sound_landmesh_queries__biomeQueryId = INVALID_QUERY_ID
      sound_landmesh_queries__biomePhysMatId = PHYSMAT_INVALID
      sound_landmesh_queries__biomeOverridesWater = false
      if is_gpu_readback_query_successful(biomeState)
        apply(biomeResult.secondMostFrequentBiomeGroupIndex, biomeResult.secondMostFrequentBiomeGroupWeight,
            sound_landmesh_queries__biomeToMaterial, sound_landmesh_queries__biomePhysMatId, sound_landmesh_queries__biomeOverridesWater)
        apply(biomeResult.mostFrequentBiomeGroupIndex, biomeResult.mostFrequentBiomeGroupWeight,
            sound_landmesh_queries__biomeToMaterial, sound_landmesh_queries__biomePhysMatId, sound_landmesh_queries__biomeOverridesWater)

  elif sound_landmesh_queries__puddleQueryId != INVALID_QUERY_ID

    var puddleResult = 0.
    let puddleState = puddle_query_value(sound_landmesh_queries__puddleQueryId, puddleResult)
    if is_gpu_readback_query_successful(puddleState) || is_gpu_readback_query_failed(puddleState)
      sound_landmesh_queries__puddleQueryId = INVALID_QUERY_ID
      sound_landmesh_queries__puddleDepth = 0.
      if is_gpu_readback_query_successful(puddleState)
        sound_landmesh_queries__puddleDepth = cvt(puddleResult,
          sound_landmesh_queries__puddleQueryToDepth.x, sound_landmesh_queries__puddleQueryToDepth.y,
          sound_landmesh_queries__puddleQueryToDepth.z, sound_landmesh_queries__puddleQueryToDepth.w)

  else
    var interval = -1.
    if sound_landmesh_queries_vars__isOnGround
      if sound_landmesh_queries_vars__speedSq >= square(sound_landmesh_queries__zeroSpeedThreshold)
        if is_watched_sound
          interval = cvt(sqrt(sound_landmesh_queries_vars__speedSq), sound_landmesh_queries__heroSpeedToQueryInterval.x, sound_landmesh_queries__heroSpeedToQueryInterval.y, sound_landmesh_queries__heroSpeedToQueryInterval.z, sound_landmesh_queries__heroSpeedToQueryInterval.w)
        else
          interval = cvt(sqrt(sound_landmesh_queries_vars__speedSq), sound_landmesh_queries__enemySpeedToQueryInterval.x, sound_landmesh_queries__enemySpeedToQueryInterval.y, sound_landmesh_queries__enemySpeedToQueryInterval.z, sound_landmesh_queries__enemySpeedToQueryInterval.w)
    else
      interval = is_watched_sound ? sound_landmesh_queries__heroEnemyInAirInterval.x : sound_landmesh_queries__heroEnemyInAirInterval.y

    if interval >= 0. && info.curTime >= sound_landmesh_queries__lastQueryTime + interval
      //logerr("{sound_landmesh_queries__biomePhysMatId}   {sound_landmesh_queries__puddleDepth}  {sound_landmesh_queries__lastQueryTime}")
      sound_landmesh_queries__lastQueryTime = info.curTime
      let pos = transform[3]
      if is_watched_sound || length_sq(get_listener_pos() - pos) < square(sound_landmesh_queries__farawayThreshold)
        let posXZ = pos.xz
        let ground = traceht_lmesh(posXZ)
        if pos.y - ground < sound_landmesh_queries__lmeshDiff
          sound_landmesh_queries__biomeQueryId = biome_query_start(float3(pos.x, ground, pos.z), 0.1)
          if sound_landmesh_queries__puddleQueryToDepth.w > 0. // puddle queries enabled
            sound_landmesh_queries__puddleQueryId = puddle_query_start(float3(pos.x, ground, pos.z))
