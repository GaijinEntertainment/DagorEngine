require ecs
require ecs.common
require fx
require app
require math.base
require DagorRandom

[es(tag=render, no_order)]
def burst_fx_player(act : UpdateStageInfoAct; eid : EntityId;
                    transform, burst_fx__transform : float3x4;
                    burst_fx__template : string;
                    burst_fx__period : float2;
                    var burst_fx__nextUpdate, burst_fx__currentPeriod : float&)
  if act.curTime < burst_fx__nextUpdate
    return

  let updateTimeOveflow = act.curTime - burst_fx__nextUpdate
  if updateTimeOveflow < burst_fx__currentPeriod
    burst_fx__currentPeriod = rnd_float(burst_fx__period.x, burst_fx__period.y)
    burst_fx__nextUpdate += burst_fx__currentPeriod
  else
    burst_fx__currentPeriod = rnd_float(burst_fx__period.x, burst_fx__period.y)
    burst_fx__nextUpdate = act.curTime + burst_fx__currentPeriod * eid_frnd(eid)
    return // skip huge dt

  let resTm = transform * burst_fx__transform
  start_effect(burst_fx__template, resTm, IDENT_TM)
