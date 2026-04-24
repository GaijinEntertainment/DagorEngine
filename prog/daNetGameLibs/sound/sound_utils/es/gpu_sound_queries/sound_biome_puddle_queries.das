require ecs
require ecs.common
require ecs.safe
require soundSystem
require DagorMath
require math.base
require PhysMat
require Dacoll
require gameLibs.pufd_events.pufd_events
require gpuReadbackQuery
require landMesh
require puddleQueryManager
require DagorSystem


def invalid_query_id()
  return -1

def is_valid_query_id(query_id : int)
  return query_id != invalid_query_id()


[es(tag=sound, on_appear)]
def sound_biome_puddle_queries_appear(evt : Event;
                                      var sound_biome_puddle_queries__biomeQueryId : int&;
                                      var sound_biome_puddle_queries__puddleQueryId : int&)

  sound_biome_puddle_queries__biomeQueryId = invalid_query_id()
  sound_biome_puddle_queries__puddleQueryId = invalid_query_id()


[es(tag=sound, after=sound_begin_update_es, before=sound_end_update_es)]
def sound_biome_puddle_queries_update(info : ParallelUpdateFrameDelayed;
                                      is_watched_sound : bool = false;
                                      transform : float3x4;

                                      sound_biome_puddle_queries_vars__speedSq : float;
                                      sound_biome_puddle_queries_vars__isOnGround : bool;

                                      sound_biome_puddle_queries__lmeshDiff : float;
                                      sound_biome_puddle_queries__farawayThreshold : float;
                                      sound_biome_puddle_queries__zeroSpeedThreshold : float;
                                      sound_biome_puddle_queries__heroSpeedToQueryInterval : float4;
                                      sound_biome_puddle_queries__enemySpeedToQueryInterval : float4;
                                      sound_biome_puddle_queries__heroEnemyInAirInterval : float2;
                                      sound_biome_puddle_queries__puddleQueryToDepth : float4;

                                      var sound_biome_puddle_queries__lastQueryTime : float&;
                                      var sound_biome_puddle_queries__biomeQueryId : int&;
                                      var sound_biome_puddle_queries__puddleQueryId : int&;
                                      var sound_biome_puddle_queries__biomeId : int&;
                                      var sound_biome_puddle_queries__puddleDepth : float&)

  if is_valid_query_id(sound_biome_puddle_queries__biomeQueryId)

    var biomeResult = BiomeQueryResult()
    let biomeState = biome_query_result(sound_biome_puddle_queries__biomeQueryId, biomeResult)
    if is_gpu_readback_query_successful(biomeState) || is_gpu_readback_query_failed(biomeState)
      sound_biome_puddle_queries__biomeQueryId = invalid_query_id()
      sound_biome_puddle_queries__biomeId = -1
      if is_gpu_readback_query_successful(biomeState)
        if biomeResult.mostFrequentBiomeGroupWeight >= 0.05
          sound_biome_puddle_queries__biomeId = biomeResult.mostFrequentBiomeGroupIndex
          /*
          var test = false
          if sound_biome_puddle_queries__biomeId >= 0
            query() <| $ [es(REQUIRE=soundStepSettings)] (landmesh_biomes_to_sound_material__biomeIdToSound : IPoint2List;
                                                          sound_physmat__soundMaterials : StringList)
              if sound_biome_puddle_queries__biomeId < length(landmesh_biomes_to_sound_material__biomeIdToSound)
                let curBiomeSoundId = landmesh_biomes_to_sound_material__biomeIdToSound[sound_biome_puddle_queries__biomeId].x
                if curBiomeSoundId >= 0 && curBiomeSoundId < length(sound_physmat__soundMaterials)
                  logerr("biome={get_biome_group_name(sound_biome_puddle_queries__biomeId)} -> {sound_physmat__soundMaterials[curBiomeSoundId]}")
                  test = true
          if !test
            logerr("biome={get_biome_group_name(sound_biome_puddle_queries__biomeId)}")
          */

  elif is_valid_query_id(sound_biome_puddle_queries__puddleQueryId)

    var puddleResult = 0.
    let puddleState = puddle_query_value(sound_biome_puddle_queries__puddleQueryId, puddleResult)
    if is_gpu_readback_query_successful(puddleState) || is_gpu_readback_query_failed(puddleState)
      sound_biome_puddle_queries__puddleQueryId = invalid_query_id()
      sound_biome_puddle_queries__puddleDepth = 0.
      if is_gpu_readback_query_successful(puddleState)
        sound_biome_puddle_queries__puddleDepth = cvt(puddleResult,
          sound_biome_puddle_queries__puddleQueryToDepth.x, sound_biome_puddle_queries__puddleQueryToDepth.y,
          sound_biome_puddle_queries__puddleQueryToDepth.z, sound_biome_puddle_queries__puddleQueryToDepth.w)

  else
    var interval = -1.
    if sound_biome_puddle_queries_vars__isOnGround
      if sound_biome_puddle_queries_vars__speedSq >= square(sound_biome_puddle_queries__zeroSpeedThreshold)
        if is_watched_sound
          interval = cvt(sqrt(sound_biome_puddle_queries_vars__speedSq), sound_biome_puddle_queries__heroSpeedToQueryInterval.x, sound_biome_puddle_queries__heroSpeedToQueryInterval.y, sound_biome_puddle_queries__heroSpeedToQueryInterval.z, sound_biome_puddle_queries__heroSpeedToQueryInterval.w)
        else
          interval = cvt(sqrt(sound_biome_puddle_queries_vars__speedSq), sound_biome_puddle_queries__enemySpeedToQueryInterval.x, sound_biome_puddle_queries__enemySpeedToQueryInterval.y, sound_biome_puddle_queries__enemySpeedToQueryInterval.z, sound_biome_puddle_queries__enemySpeedToQueryInterval.w)
    else
      interval = is_watched_sound ? sound_biome_puddle_queries__heroEnemyInAirInterval.x : sound_biome_puddle_queries__heroEnemyInAirInterval.y

    if interval >= 0. && info.curTime >= sound_biome_puddle_queries__lastQueryTime + interval
      //logerr("{sound_biome_puddle_queries__biomeId}   biome={get_biome_group_name(sound_biome_puddle_queries__biomeId)}     {sound_biome_puddle_queries__puddleDepth}  {sound_biome_puddle_queries__lastQueryTime}")
      sound_biome_puddle_queries__lastQueryTime = info.curTime
      let pos = transform[3]
      if is_watched_sound || length_sq(get_listener_pos() - pos) < square(sound_biome_puddle_queries__farawayThreshold)
        let posXZ = pos.xz
        let ground = traceht_lmesh(posXZ)
        if pos.y - ground < sound_biome_puddle_queries__lmeshDiff
          sound_biome_puddle_queries__biomeQueryId = biome_query_start(float3(pos.x, ground, pos.z), 0.1)
          if sound_biome_puddle_queries__puddleQueryToDepth.w > 0. // puddle queries enabled
            sound_biome_puddle_queries__puddleQueryId = puddle_query_start(float3(pos.x, ground, pos.z))
