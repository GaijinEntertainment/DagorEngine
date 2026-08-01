require ecs
require ecs.common
require ecs.safe
require soundSystem
require DagorMath
require math.base
require PhysMat
require Dacoll
require gameLibs.pufd_events.pufd_events
require sound_utils.modules.sound_trace_common
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
                                      human_net_phys__isInAir : bool = false;

                                      sound_biome_puddle_queries__maxDistToFloor : float;
                                      sound_biome_puddle_queries__farawayThreshold : float;
                                      sound_biome_puddle_queries__zeroSpeedThreshold : float;
                                      sound_biome_puddle_queries__watchedSpeedToQueryInterval : float4;
                                      sound_biome_puddle_queries__enemySpeedToQueryInterval : float4;
                                      sound_biome_puddle_queries__watchedOtherInAirInterval : float2;
                                      sound_biome_puddle_queries__puddleQueryToDepth : float4;

                                      var sound_biome_puddle_queries__lastQueryTime : float&;
                                      var sound_biome_puddle_queries__biomeQueryId : int&;
                                      var sound_biome_puddle_queries__puddleQueryId : int&;
                                      var sound_biome_puddle_queries__biomeId : int&;
                                      var sound_biome_puddle_queries__puddleDepth : float&)

  assume isInAir = human_net_phys__isInAir

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
    if !isInAir
      if sound_biome_puddle_queries_vars__speedSq >= square(sound_biome_puddle_queries__zeroSpeedThreshold)
        if is_watched_sound
          interval = cvt(sqrt(sound_biome_puddle_queries_vars__speedSq), sound_biome_puddle_queries__watchedSpeedToQueryInterval.x, sound_biome_puddle_queries__watchedSpeedToQueryInterval.y, sound_biome_puddle_queries__watchedSpeedToQueryInterval.z, sound_biome_puddle_queries__watchedSpeedToQueryInterval.w)
        else
          interval = cvt(sqrt(sound_biome_puddle_queries_vars__speedSq), sound_biome_puddle_queries__enemySpeedToQueryInterval.x, sound_biome_puddle_queries__enemySpeedToQueryInterval.y, sound_biome_puddle_queries__enemySpeedToQueryInterval.z, sound_biome_puddle_queries__enemySpeedToQueryInterval.w)
    else
      interval = is_watched_sound ? sound_biome_puddle_queries__watchedOtherInAirInterval.x : sound_biome_puddle_queries__watchedOtherInAirInterval.y

    if interval >= 0. && info.curTime >= sound_biome_puddle_queries__lastQueryTime + interval
      sound_biome_puddle_queries__lastQueryTime = info.curTime
      var pos = transform[3]

      if is_watched_sound || length_sq(get_listener_pos() - pos) < square(sound_biome_puddle_queries__farawayThreshold)

        let floorIsDown = is_floor_down(transform[1])
        if isInAir && floorIsDown
          let dirToFloor = make_dir_to_floor(transform[1], floorIsDown)
          var distToFloor = sound_biome_puddle_queries__maxDistToFloor
          var floorPMID = PHYSMAT_INVALID
          if trace_floor(pos, 0.5/*offset*/, distToFloor, floorPMID, dirToFloor, floorIsDown, null, get_trace_flags_for_biome_and_puddle_queries())
            pos += dirToFloor * distToFloor // bring pos to the "floor" to have proper values under feet even if in the air
          else
            sound_biome_puddle_queries__biomeId = -1
            sound_biome_puddle_queries__puddleDepth = 0.
            return  // too far above "floor"

        sound_biome_puddle_queries__biomeQueryId = biome_query_start(pos, 0.1)
        if sound_biome_puddle_queries__puddleQueryToDepth.w > 0. // puddle queries enabled
          sound_biome_puddle_queries__puddleQueryId = puddle_query_start(pos)

      else
        sound_biome_puddle_queries__biomeId = -1
        sound_biome_puddle_queries__puddleDepth = 0.
