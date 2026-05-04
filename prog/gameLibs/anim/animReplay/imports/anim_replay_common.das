module anim_replay_common shared
options no_aot
require ecs
require daslib/algorithm

def make_tm(quat : float4; pos : float3)
  return float3x4(compose(pos, quat, float3(1)))

def sample_timestamps(t : float; timestamps : FloatList; blk : block<(prev_idx, next_idx : int) : void>)
  let nextIdx = clamp(lower_bound(timestamps, t), 1, length(timestamps) - 1)
  let prevIdx = nextIdx - 1
  invoke(blk, prevIdx, nextIdx)

def sample_timestamps(t : float; timestamps : FloatList; blk : block<(prev_idx, next_idx : int; interp_k : float) : void>)
  let nextIdx = min(lower_bound(timestamps, t), length(timestamps) - 1)
  let prevIdx = max(nextIdx - 1, 0)
  let interpK = (prevIdx == nextIdx ? 0.f :
    (t - timestamps[prevIdx]) / (timestamps[nextIdx] - timestamps[prevIdx]))
  invoke(blk, prevIdx, nextIdx, interpK)

def get_current_timestamp_idx(t : float; timestamps : FloatList)
  return max(lower_bound(timestamps, t, $(a, b) => a <= b) - 1, 0)

def get_next_timestamp_idx(t : float; timestamps : FloatList)
  return min(lower_bound(timestamps, t, $(a, b) => a <= b), length(timestamps) - 1)

def get_next_timestamp(t : float; timestamps : FloatList)
  let timeIdx = get_next_timestamp_idx(t, timestamps)
  return t < timestamps[timeIdx] ? timestamps[timeIdx] : FLT_MAX

def is_animchar_dead(animchar_data : Object; t : float)
  let deathTime = get_float(animchar_data, "deathTime")
  return deathTime != null && t >= *deathTime

def iterate_alive_replay_animchars(animchars : Array; cur_time : float; blk : block<(animchar_data : Object) : void>)
  for animcharChildComp in animchars
    unsafe
      let animcharData & = *get_ecs_object(animcharChildComp)
      if is_animchar_dead(animcharData, cur_time)
        continue
      invoke(blk, animcharData)
