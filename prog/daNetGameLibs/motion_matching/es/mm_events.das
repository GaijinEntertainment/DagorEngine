module mm_events shared
require ecs

//to be moved to daNetGameLibs
[cpp_event(broadcast)]
struct ParallelUpdateFrameDelayed
  curTime : float
  dt : float

[cpp_event(broadcast)]
struct InvalidateAnimationDataBase {}

[cpp_event(unicast)]
struct AnimationDataBaseAssigned {}