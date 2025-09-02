require ecs

//These events will be removed after all DNG-based games use ParallelUpdateFrameDelayed for dng systems
[event(broadcast)] // here for inability to use anywhere
struct UpdatePhysEvent
  curTime : float
  dt : float

[cpp_event(broadcast)]
struct UpdateAnimcharEvent
  curTime : float
  dt : float

[cpp_event(broadcast)]
struct UpdateActionsEvent
  dt : float


[es(after=before_net_phys_sync, before=after_net_phys_sync)]
def update_phys_es(info : UpdateStageInfoAct)
  broadcastEventImmediate(UpdatePhysEvent(curTime = info.curTime, dt = info.dt))

[es(after=before_animchar_update_sync, before=after_animchar_update_sync)]
def animchar_es(info : UpdateStageInfoAct) // same name for save order of es
  broadcastEventImmediate(UpdateAnimcharEvent(curTime = info.curTime, dt = info.dt))

[es]
def actions_es(info : UpdateStageInfoAct) // same name for save order of es (it was actions_es in Cpp)
  broadcastEventImmediate(UpdateActionsEvent(dt = info.dt))
