module attach_events shared
require ecs


[cpp_event(unicast)]
struct EventItemAttached
  attach_to : EntityId

[cpp_event(unicast)]
struct EventItemDetached
  detach_from : EntityId

[cpp_event(unicast)]
struct CmdInitSkeletonAttach {}

[event(unicast)]
struct CmdInitScaleSkeletonAttach {}