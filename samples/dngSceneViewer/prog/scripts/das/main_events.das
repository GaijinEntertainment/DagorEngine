module main_events shared
require ecs

[cpp_event(broadcast)]
struct ParallelUpdateFrameDelayed
  curTime : float
  dt : float

[event(broadcast)]
struct PhysUpdateEvent
  tick : int
  dt : float

[event(unicast)]
struct EventLutSwitcherAliveState {}

[event(broadcast)]
struct CmdSetCameraFov
  fov : float

[event(broadcast)]
struct CmdSetCameraDofEnabled
  enabled : bool

[event(broadcast)]
struct CmdSetFreeTpsCameraSpeed
  speed : float

[event(broadcast)]
struct CmdSetRain
  enabled : bool

[event(broadcast)]
struct CmdSetSnow
  enabled : bool

[event(broadcast)]
struct CmdSetLightning
  enabled : bool

[event(broadcast)]
struct CmdSetLenseFlareIntensity
  intensity : float

[event(broadcast)]
struct CmdSetCinematicModeEnabled
  enabled : bool

[event(broadcast)]
struct CmdSetMotionBlurScale
  scale : float

[event(broadcast)]
struct CmdSetBloomThreshold
  threshold : float

[event(broadcast)]
struct CmdSetChromaticAberrationOffset
  offset : float

[event(broadcast)]
struct CmdSetFilmGrain
  strength : float

[event(broadcast)]
struct CmdSetVignetteStrength
  strength : float

[event(broadcast)]
struct CmdSetDofIsFilmic
  isFilmic : bool

[event(broadcast)]
struct CmdSetDofFocusDistance
  focusDistance : float

[event(broadcast)]
struct CmdSetDofFocalLength
  focalLength : float

[event(broadcast)]
struct CmdSetDofFStop
  fStop : float

[event(broadcast)]
struct CmdSetDofBokehCorners
  bokehCorners : float

[event(broadcast)]
struct CmdSetDofBokehSize
  bokehSize : float

[event(broadcast)]
struct CmdChangeTimeOfDay
  timeOfDay : float

[event(broadcast)]
struct CmdWeather
  preset : string

[event(broadcast)]
struct CmdSetCameraLerpFactor
  lerpFactor : float

[event(broadcast)]
struct CmdSetCameraStopLerpFactor
  stopLerpFactor : float

[event(broadcast)]
struct CmdSetCinemaRecording
  enabled : bool

[event(broadcast)]
struct CmdSetCinematicPostFxBloom
  enabled : bool

[event(broadcast)]
struct CmdSetCinematicSetSuperPixels
  super_pixels : int

[event(broadcast)]
struct CmdSetCinematicCustomSettings
  enabled : bool

