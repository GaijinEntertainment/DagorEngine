module pufd_events shared
require ecs


[cpp_event(broadcast)]
struct ParallelUpdateFrameDelayed
  curTime : float
  dt : float