require ecs
require soundEvent


[es(tag=sound, before=burst_fx_player)]
def burst_fx_sound(info : UpdateStageInfoAct;
                   burst_fx__soundPath : string;
                   transform, burst_fx__transform : float3x4;
                   burst_fx__nextUpdate : float)
  if info.curTime < burst_fx__nextUpdate && info.curTime + info.dt >= burst_fx__nextUpdate
    let resTm = transform * burst_fx__transform
    oneshot(burst_fx__soundPath, resTm[3])
