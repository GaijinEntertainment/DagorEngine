options no_aot
require ecs
require imgui
require math.base
require AnimV20
require DagorImgui
require DagorMath
require DagorSystem
require GeomNodeTree


[es(tag=dev, on_appear, REQUIRE=anim_replay__animchars)]
def anim_replay_create_camera(evt : Event)
  let found = find_query() <| $ [es(REQUIRE=anim_replay_camera)] ()
    return true
  if !found
    createEntitySync("anim_replay_camera")

[es(tag=dev, on_disappear, REQUIRE=anim_replay__animchars)]
def anim_replay_remove_camera_target(evt : Event)
  query() <| $ [es(REQUIRE=anim_replay_camera)] (var camera__target : EntityId&)
    camera__target = INVALID_ENTITY_ID

[es(tag=dev, on_appear, track=(camera__target, camera__active))]
def anim_replay_toggle_camera(evt : Event;
                              var camera__active aka anim_replay_camera__active : bool&;
                              var anim_replay_camera__previousCameraEid : EntityId&;
                              var transform aka anim_replay_camera__transform : float3x4&;
                              var anim_replay_camera__centerAnimchar : bool&;
                              camera__target : EntityId)
  if camera__target != INVALID_ENTITY_ID && !anim_replay_camera__active
    // Treat free camera as higher priority than anim replay camera, so we shouldn't disable it here
    find_query() <| $ [es(REQUIRE_NOT=free_cam)] (eid : EntityId; var camera__active : bool&; transform : float3x4)
      if !camera__active
        return false
      camera__active = false
      anim_replay_camera__active = true
      anim_replay_camera__previousCameraEid = eid
      anim_replay_camera__transform = transform
      return true
  elif camera__target == INVALID_ENTITY_ID && anim_replay_camera__active
    if !doesEntityExist(anim_replay_camera__previousCameraEid)
      assume prevCamEid = anim_replay_camera__previousCameraEid
      logerr("Previously saved camera <{prevCamEid}> <{getEntityTemplateName(prevCamEid)}> is already destroyed")
    query(anim_replay_camera__previousCameraEid)  <| $ [es] (var camera__active : bool&)
      camera__active = true
    anim_replay_camera__active = false
    anim_replay_camera__previousCameraEid = INVALID_ENTITY_ID
  if camera__target != INVALID_ENTITY_ID
    anim_replay_camera__centerAnimchar = true

[es(tag=dev, on_appear, track=(camera__active), REQUIRE_NOT=(free_cam, anim_replay_camera))]
def anim_replay_enable_camera_after_freecam(evt : Event; eid : EntityId; var camera__active aka old_camera__active : bool&)
  // If we didn't enable anim replay camera because free camera was active, we can do it now
  if camera__active
    query() <| $ [es] (var camera__active aka anim_replay_camera__active : bool&;
                       var anim_replay_camera__previousCameraEid : EntityId&;
                       camera__target : EntityId)
      if camera__target != INVALID_ENTITY_ID
        assert(!anim_replay_camera__active)
        old_camera__active = false
        anim_replay_camera__active = true
        anim_replay_camera__previousCameraEid = eid

[es(tag=dev, before=anim_replay_camera_update)]
def anim_replay_camera_input(info : UpdateStageInfoAct;
                             var anim_replay_camera__move : float2&;
                             var anim_replay_camera__rotation : float2&;
                             var anim_replay_camera__zoom : float&)
  let imguiIO & = imgui::GetIO()
  if imgui_get_state() == ImGuiState.ACTIVE && !imguiIO.WantCaptureMouse
    anim_replay_camera__move = !imguiIO.KeyAlt && imgui::IsMouseDown(ImGuiMouseButton.Middle) ? imguiIO.MouseDelta : float2(0, 0)
    anim_replay_camera__rotation = (imgui::IsMouseDown(ImGuiMouseButton.Right) ||
        imguiIO.KeyAlt && imgui::IsMouseDown(ImGuiMouseButton.Middle)) ? imguiIO.MouseDelta : float2(0, 0)
    anim_replay_camera__zoom = imguiIO.MouseWheel
  else
    anim_replay_camera__move = float2(0, 0)
    anim_replay_camera__rotation = float2(0, 0)
    anim_replay_camera__zoom = 0f

[es(tag=dev, after=(anim_replay_camera_input, anim_replay_update_animchar), before=after_camera_sync)]
def anim_replay_camera_update(info : UpdateStageInfoAct;
                              var camera__accuratePos : DPoint3&;
                              var transform aka camera_transform : float3x4&;
                              var anim_replay_camera__targetOffset : float3&;
                              var anim_replay_camera__distanceToTarget : float&;
                              var anim_replay_camera__centerAnimchar : bool&;
                              camera__target : EntityId;
                              anim_replay_camera__move : float2;
                              anim_replay_camera__rotation : float2;
                              anim_replay_camera__zoom : float;
                              anim_replay_camera__moveSensitivity : float;
                              anim_replay_camera__rotationSensitivity : float;
                              anim_replay_camera__zoomSensitivity : float;
                              anim_replay_camera__zoomLimit : float2)
  var yaw, roll, pitch : float
  matrix_to_euler(camera_transform, yaw, roll, pitch);
  yaw += anim_replay_camera__rotation.x * anim_replay_camera__rotationSensitivity
  pitch += anim_replay_camera__rotation.y * anim_replay_camera__rotationSensitivity
  pitch = clamp(pitch, deg_to_rad(-90.f), deg_to_rad(90.f))
  var cameraOrientation : quat
  euler_to_quat(yaw, 0.f, pitch, cameraOrientation)
  make_tm(cameraOrientation, camera_transform[3], camera_transform)

  let cameraMove = -camera_transform[0] * anim_replay_camera__move.x + camera_transform[1] * anim_replay_camera__move.y
  let moveSpeed = anim_replay_camera__moveSensitivity * max(anim_replay_camera__distanceToTarget, 1f)
  anim_replay_camera__targetOffset += cameraMove * moveSpeed

  anim_replay_camera__distanceToTarget -= anim_replay_camera__zoom * anim_replay_camera__zoomSensitivity
  anim_replay_camera__distanceToTarget = clamp(anim_replay_camera__distanceToTarget,
      anim_replay_camera__zoomLimit.x, anim_replay_camera__zoomLimit.y)

  query(camera__target) <| $ [es] (animchar : AnimcharBaseComponent;
                                   transform aka target_transform : float3x4)
    if anim_replay_camera__centerAnimchar
      var skeletonBox : bbox3f
      assume nodeTree = *animchar.nodeTree
      geomtree_calcWorldBox(nodeTree, skeletonBox)
      let bboxCenter = (skeletonBox.bmin.xyz + skeletonBox.bmax.xyz) * 0.5f
      anim_replay_camera__targetOffset = bboxCenter + (geomtree_getWtmOfs(nodeTree) - transform[3])
      let bboxSize = length(skeletonBox.bmax.xyz - skeletonBox.bmin.xyz)
      anim_replay_camera__distanceToTarget = bboxSize
      anim_replay_camera__centerAnimchar = false

    let lookAtPos = target_transform[3] + anim_replay_camera__targetOffset
    let cameraOffset = -camera_transform[2] * anim_replay_camera__distanceToTarget
    camera_transform[3] = lookAtPos + cameraOffset
    camera__accuratePos = DPoint3(camera_transform[3])
