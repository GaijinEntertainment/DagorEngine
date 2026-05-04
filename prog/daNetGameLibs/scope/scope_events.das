module scope_events shared
require ecs

[cpp_event(unicast)]
struct OnScopeCrosshairGuiRender
  lensWidth : float
  lensHeight : float
  fontHdScale : float
  outParallaxPlaneDistance : float?
