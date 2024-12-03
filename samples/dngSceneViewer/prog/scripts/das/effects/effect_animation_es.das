require ecs
require ecs.common
require das.main_events
require das.effects.effect_animation_common
require DagorSystem
require DagorMath


[es(tag=render, REQUIRE=effect_animation__activated, before=effect_anim_es)]
def effect_animation_update_es(info : ParallelUpdateFrameDelayed;
                               eid : EntityId;
                               var effect_animation__stageStartedTime : float&;
                               var effect_animation__currentStageId : int&;
                               effect_animation__stages : Array;
                               transform : float3x4;
                               var effect_animation__transform : float3x4&;
                               var effect__colorMult : E3DCOLOR?;
                               var effect__velocity : float3?;
                               var effect__spawnRate : float?)
  assume stages = effect_animation__stages
  assume currentStageId = effect_animation__currentStageId

  if currentStageId < 0 // begin animation
    effect_animation__currentStageId = 0
    effect_animation__stageStartedTime = info.curTime

  while currentStageId < length(stages) // skip finished stages
    let currentStage = get_ecs_object(stages[currentStageId])
    let duration = get_float(currentStage, "duration") ?? 0.0 // duration 0.0 means infinity stage
    let endTime = effect_animation__stageStartedTime + duration
    if duration > 0.0 && info.curTime > endTime
      effect_animation__currentStageId += 1
      effect_animation__stageStartedTime = endTime
    else
      break

  if currentStageId < length(stages) // apply props
    let currentStage = get_ecs_object(stages[currentStageId])

    let duration = get_float(currentStage, "duration") ?? 0.0
    let nextStageId = duration > 0.0 ? min(currentStageId + 1, length(stages) - 1) : currentStageId
    let nextStage = get_ecs_object(stages[nextStageId])
    let progress = duration > 0.0 ? (info.curTime - effect_animation__stageStartedTime) / duration : 0.0

    if effect__velocity != null
      var found = false
      var velocity = float3(0, 0, 0)
      dereference_both(get_Point3(currentStage, "velocity"), get_Point3(nextStage, "velocity")) <| $(a, b)
        velocity = lerp(a, b, float3(progress))
        found = true
      dereference_both(get_Point3(currentStage, "localVelocity"), get_Point3(nextStage, "localVelocity")) <| $(a, b)
        var m = transform
        m[3] = float3(0, 0, 0)
        velocity += m * lerp(a, b, float3(progress))
        found = true
      if found
        *effect__velocity = velocity

    if effect__colorMult != null
      dereference_both(get_Point4(currentStage, "colorMult"), get_Point4(nextStage, "colorMult")) <| $(a, b)
        *effect__colorMult = E3DCOLOR(lerp(a, b, float4(progress)))

    if effect__spawnRate != null
      dereference_both(get_float(currentStage, "spawnRate"), get_float(nextStage, "spawnRate")) <| $(a, b)
        *effect__spawnRate = lerp(a, b, progress)

    effect_animation__transform = transform
    dereference_both(get_Point3(currentStage, "areaScale"), get_Point3(nextStage, "areaScale")) <| $(a, b)
      let areaScale = lerp(a, b, float3(progress))
      effect_animation__transform[0] *= areaScale.x
      effect_animation__transform[1] *= areaScale.y
      effect_animation__transform[2] *= areaScale.z

  if currentStageId >= length(stages) // end animation
    if has(eid, "effect_animation__destroyOnAnimationEnd")
      destroyEntity(eid)
      return
    stop_effect_animation(eid)


def dereference_both(a : auto(Ta)?; b : auto(Tb)?; c : block<(arg1 : Ta; arg2 : Tb)>)
  if a != null && b != null
    invoke(c, *a, *b);

def E3DCOLOR(v : float4)
  let c = v * 255.0
  return E3DCOLOR(uint4(c.x, c.y, c.z, c.w))