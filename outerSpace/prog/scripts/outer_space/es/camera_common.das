module camera_common shared
require daslib/safe_addr
require ecs
require ecs.safe
require math.base
require DngCamera
require DngNet
require outer_space.events_outer_space
require player public

def create_camera_by_name(cam_name : string;
                          target : EntityId;
                          itm : float3x4 const? | #;
                          cflags : uint)
  let camEid = createEntitySync(cam_name) <| $(var init : ComponentsInitializer)
    if itm != null
      init |> set("transform", *itm)
    init |> set("camera__target", target)
    if (cflags & uint(CreateCameraFlags CCF_SET_TARGET_TEAM)) != 0u
      init |> set("team", target |> get_int("team") ?? TEAM_UNASSIGNED)
    if (cflags & uint(CreateCameraFlags CCF_SET_ENTITY)) != 0u
      init |> set("entity", target)
  assert(camEid != INVALID_ENTITY_ID)
  return camEid

def get_cam_itm()
  return get_cur_cam_entity() |> get_TMatrix("transform") ?? IDENT_TM

def bind_camera_to_entity(cam : EntityId; target : EntityId)
  sendEvent(cam, [[CmdSetCameraTarget target=target]])
  query(target) <| $ [es] (team aka target_team : int = TEAM_UNASSIGNED)
    query(cam) <| $ [es] (var team aka cam_team : int&)
      cam_team = target_team

def possess_entity_camera(actor_eid : EntityId;
                          cam_idx : int;
                          player_eid : EntityId;
                          var cam_names_eid : EntityId)
  query(actor_eid) <| $ [es] (var isTpsView : bool&;
                              var activeCamIdx : int&;
                              var bindedCamera : EntityId&;
                              isInVehicleHidden : bool = true)
    query(player_eid) <| $ [es] (var activeCamEid : EntityId&;
                                 var activeCamName : das_string&)
      if !!activeCamEid && !doesEntityExist(activeCamEid)
        activeCamEid = INVALID_ENTITY_ID

      if !!activeCamEid
        removeSubTemplate(activeCamEid, "spectator")

      if !cam_names_eid
        cam_names_eid = actor_eid

      query(actor_eid) <| $ [es] (human_anim__vehicleSelected : EntityId;
                                  isInVehicle : bool)
        if isInVehicle && has(human_anim__vehicleSelected, "camNames")
          cam_names_eid = human_anim__vehicleSelected

      activeCamIdx = 0
      var camName = "shooter_cam"
      query(cam_names_eid) <| $ [es] (camNames : Array)
        if !camNames.empty
          activeCamIdx = cam_idx % int(camNames.size)
          camName = camNames[activeCamIdx] ?? ""

      if !!activeCamEid && activeCamName == camName
        bindedCamera = activeCamEid
        let isBindToVehicle = get_bool(activeCamEid, "bindToVehicle") ?? false
        if !isInVehicleHidden || !isBindToVehicle
          bind_camera_to_entity(activeCamEid, actor_eid)
        else
          query(cam_names_eid) <| $ [es] (var bindedCamera : EntityId&)
            bindedCamera = activeCamEid
          bind_camera_to_entity(activeCamEid, cam_names_eid)
      else
        if !!activeCamEid
          destroyEntity(activeCamEid)

        let cameraTm = get_cam_itm()
        activeCamEid = create_camera_by_name(camName,
                                             actor_eid,
                                             safe_addr(cameraTm),
                                             uint(CreateCameraFlags CCF_SET_TARGET_TEAM))
        bindedCamera = activeCamEid
        activeCamName := camName

        set_scene_camera_entity(activeCamEid)

        isTpsView = get_bool(activeCamEid, "isTpsView") ?? false

        sendEvent(activeCamEid, [[CmdSetCameraTarget target=actor_eid]])

        if !is_server()
          send_net_event(player_eid, [[RequestCamSwitch isTpsView=isTpsView]])
