module mm_events shared
require ecs


[cpp_event(broadcast)]
struct InvalidateAnimationDataBase {}

[cpp_event(unicast)]
struct AnimationDataBaseAssigned {}

[cpp_event(broadcast)]
struct MotionMatchingStartSearchJob
  dt : float
