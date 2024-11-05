module screen_frost_events shared
require ecs

[cpp_event(broadcast)]
struct SetResolutionEvent
  type_ : int
  displayResolution : int2
  renderingResolution : int2
  postFxResolution : int2
