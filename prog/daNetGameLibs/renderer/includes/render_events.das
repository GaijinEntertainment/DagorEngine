module render_events shared
require ecs
require DagorDriver3D
require DagorMath
require fx

[cpp_event(broadcast)]
struct SetFxQuality
  fxQualityConfig : FxQuality

[cpp_event(broadcast)]
struct SetResolutionEvent
  type_ : int
  displayResolution : int2
  renderingResolution : int2
  postFxResolution : int2

[cpp_event(broadcast)]
struct QueryUnexpectedAltitudeChange
  enabled : bool = false

[cpp_event(broadcast)]
struct RenderEventUI
  viewTm : TMatrix
  viewItm : TMatrix
  globtm : mat44f
  persp : Driver3dPerspective

[cpp_event(broadcast)]
struct RenderXray
  viewTm : TMatrix
  viewItm : TMatrix
  globtm : mat44f
  persp : Driver3dPerspective

// Be sure that field order is same as on C++ side
[cpp_event(broadcast)]
struct UpdateStageInfoBeforeRender
  viewTm : TMatrix
  projTm : mat44f
  globTm : mat44f
  mainCullingFrustum : Frustum
  froxelFogCullingFrustum : Frustum
  dt : float
  actDt : float
  realDt : float
  mainInvHkSq : float
  mainWkHk : float2
  camPos : float3
  dirFromSun : float3
  viewItm : TMatrix
  negRoundedCamPos : float4
  negRemainderCamPos : float4
  mainOcclusion : void?
  persp : Driver3dPerspective


[cpp_event(broadcast)]
struct AfterDeviceReset
  fullReset : bool

[cpp_event(broadcast)]
struct OnRenderDecals
  viewTm : float3x4
  viewItm : float3x4
  cameraWorldPos : float3
  texCtx : TexStreamingContext
  rendinstMainVisibility : void?

[cpp_event(broadcast)]
struct BeforeDraw
  persp : Driver3dPerspective
  frustum : Frustum
  camPos : float3
  dt : float