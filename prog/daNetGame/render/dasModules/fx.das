require fx
require ecs
require DagorSystem


[cpp_event(broadcast)]
struct StartEffectEvent
  pos : float3
  fx : AcesEffect?
  fxType : int

[cpp_event(unicast)]
struct RecreateEffectEvent {}


[generic]
def get_type_by_name(name : string) : int
  return get_type_by_name(name, /*optional*/false)

[generic]
def start_effect(name : string;
                 emitter_tm : float3x4;
                 fx_tm : float3x4)
  let fxType = get_type_by_name(name)
  if fxType >= 0
    return start_effect(fxType, emitter_tm, fx_tm, false)
  else
    logerr("Can't start effect with name <{name}>")
    return null


[generic]
def start_effect(name : string;
                 emitter_tm : float3x4;
                 fx_tm : float3x4;
                 cb : block<(var fx : AcesEffect) : void>)
  let fxType = get_type_by_name(name)
  if fxType >= 0
    return start_effect(fxType, emitter_tm, fx_tm, false, cb)
  else
    logerr("Can't start effect with name <{name}>")
    return false


[generic]
def start_effect(fx_type : int;
                 emitter_tm : float3x4;
                 fx_tm : float3x4)
  return start_effect(fx_type, emitter_tm, fx_tm, false)


[generic]
def start_effect(fx_type : int;
                 emitter_tm : float3x4;
                 fx_tm : float3x4;
                 cb : block<(var fx : AcesEffect) : void>)
  return start_effect(fx_type, emitter_tm, fx_tm, false, cb)