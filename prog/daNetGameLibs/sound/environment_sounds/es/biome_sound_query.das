require ecs
require landMesh
require DagorMath
require soundEvent
require soundSystem
require gpuReadbackQuery
require danetlibs.renderer.includes.pufd_events


def invalid_query_id()
  return -1


[es(tag=sound, on_appear)]
def biome_sound_query_appear(evt : Event;
                             biome_sound_query__biomeNames : StringList;
                             var biome_sound_query__values : Point3List;
                             var biome_sound_query__id : int&;
                             var biome_sound_query__timeRange : float2&;
                             var biome_sound_query__nextQueryTime : float&)
  assert(length(biome_sound_query__biomeNames) > 0)
  let count = length(biome_sound_query__biomeNames)
  biome_sound_query__values |> resize(count)
  for i in range(count)
    biome_sound_query__values[i] = float3(0., 0., 0.)
  biome_sound_query__id = invalid_query_id()
  biome_sound_query__timeRange = float2(0., 0.)
  biome_sound_query__nextQueryTime = 0.


[es(tag=sound, after=sound_begin_update_es, before=sound_end_update_es)]
def biome_sound_query_update(info : ParallelUpdateFrameDelayed;
                             biome_sound_query__range : float;
                             biome_sound_query__period : float;
                             biome_sound_query__varNames : StringList;
                             biome_sound_query__biomeNames : StringList;
                             var biome_sound_query__values : Point3List;
                             var biome_sound_query__id : int&;
                             var biome_sound_query__timeRange : float2&;
                             var biome_sound_query__nextQueryTime : float&)

  if biome_sound_query__id == invalid_query_id() && info.curTime >= biome_sound_query__nextQueryTime
    biome_sound_query__id = biome_query_start(get_listener_pos(), biome_sound_query__range)
    biome_sound_query__nextQueryTime = info.curTime + biome_sound_query__period

  if biome_sound_query__id >= 0
    var result = BiomeQueryResult()
    let state = biome_query_result(biome_sound_query__id, result)

    if is_gpu_readback_query_successful(state)
      for value, name in biome_sound_query__values, biome_sound_query__biomeNames
        let groupId = get_biome_group_id(string(name))

        value.x = value.z // .x - lerp from; .y - lerp to; .z - current
        if groupId == result.mostFrequentBiomeGroupIndex
          value.y = result.mostFrequentBiomeGroupWeight
        elif groupId == result.secondMostFrequentBiomeGroupIndex
          value.y = result.secondMostFrequentBiomeGroupWeight
        else
          value.y = 0.

      //sound_debug("{biome_sound_query__nextQueryTime}: {result.mostFrequentBiomeGroupIndex}('{get_biome_group_name(result.mostFrequentBiomeGroupIndex)}')={result.mostFrequentBiomeGroupWeight}; {result.secondMostFrequentBiomeGroupIndex}('{get_biome_group_name(result.secondMostFrequentBiomeGroupIndex)}')={result.secondMostFrequentBiomeGroupWeight}")

      biome_sound_query__timeRange = float2(info.curTime, info.curTime + biome_sound_query__period)
      biome_sound_query__nextQueryTime = info.curTime + biome_sound_query__period
      biome_sound_query__id = invalid_query_id()

    elif is_gpu_readback_query_failed(state)
      biome_sound_query__nextQueryTime = info.curTime + biome_sound_query__period
      biome_sound_query__id = invalid_query_id()

  for value, varName in biome_sound_query__values, biome_sound_query__varNames
    value.z = cvt(info.curTime, biome_sound_query__timeRange.x, biome_sound_query__timeRange.y, value.x, value.y)

    if sound_banks_is_master_preset_loaded()
      set_var_global(string(varName), value.z)
