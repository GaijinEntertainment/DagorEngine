from "%darg/ui_imports.nut" import *
from "widgets/simpleComponents.nut" import mkCombo, menuBtn, normalCursor, headerTxt
from "json" import parse_json, object_to_json_string

from "%sqstd/functools.nut" import tryCatch
from "%sqstd/string.nut" import tostring_r
from "string" import format
from "widgets/msgbox.nut" import showWarning, showMsgbox
from "http_task.nut" import HttpPostTask, mkJsonHttpReq
from "backend_api.nut" import get_master_server_url
from "dagor.workcycle" import defer
from "dagor.time" import unixtime_to_local_timetbl, get_local_unixtime
let DngBhv = require("dng.behaviors")
let { setOfflineSessionParams, isInMainMenu, ulog, launch_network_session } = require("app_state.nut")
let { hardPersistWatched } = require("%sqstd/globalState.nut")
let { mainLogin, userUid, isOfflineMode, desiredUserName, userName, reloginUI, logout } = require("login.nut")
let { showGameMenu, gameMenu } = require("game_menu.nut")
//let {get_user_system_info=@() null} = require_optional("sysinfo")

let unused = @(...) null

let mkRoomInfo = kwarg(@(roomId, scene=null, sessionId=null,
    users=null, hostIp = null, hostPort=null, params=null, timeCreated=null,
    owner_uid=null, creater_nick=null, lastTimeUsed=null, timeSessionStarted=null, authKey=null, encKey=null
  )
    unused(lastTimeUsed, timeSessionStarted) ?? freeze({roomId, scene, sessionId, users, hostPort, hostIp, timeCreated, params, creater_nick, owner_uid, encKey, authKey})
)

let scenes = [{value = "gamedata/scenes/planet_orbit.blk", text = "Planet Orbit"}, {value = "gamedata/scenes/gaija_planet.blk", text = "Planet Surface"}]
let scenesTextMap = scenes.reduce(@(res, v) res.rawset(v.value, v.text), {})
let scenePathToSceneName = @(scenePath) scenesTextMap?[scenePath] ?? scenePath?.split("/").slice(-1)?[0].split(".").slice(-2)?[0].replace("_", " ") ?? "unknown"

let receivedRoomsInfo = hardPersistWatched("receivedRoomsInfo", [])
let selectedRoom = mkWatched(persist, "selectedRoom")
let joinedRoomInfo = hardPersistWatched("joinedRoomInfo")
let background = freeze({  rendObj = ROBJ_IMAGE
  image = Picture("launcher_back.png")
  color = Color(100,100,100)
  keepAspect = KEEP_ASPECT_FILL
  size = flex()
})
let roomsErrorTxt = Watched()
let isInCreateRoom = mkWatched(persist, "isInCreateRoom", false)
let selectedScene = mkWatched(persist, "selectedScene", scenes[0])
let offlineSession = mkWatched(persist, "offlineSession", false)
let roomParams = mkWatched(persist, "roomParams", {
  maxPlayers = 8
  sessionMaxTimeLimit = 10.0
})
let joinedRoomInfoError = Watched(null)

let headerAnimations = freeze([
  { prop=AnimProp.opacity, from=0, to=1, duration=1, play=true, easing=OutCubic }
  { prop=AnimProp.opacity, from=1, to=0, duration=1, playFadeOut=true, easing=OutCubic }
  { prop=AnimProp.translate,  from=[hdpx(200), 0], to=[0,0], duration=1.25, play=true, easing=OutQuintic }
  { prop=AnimProp.translate,  from=[0, 0], to=[-hdpx(300),0], duration=1.0, playFadeOut=true, easing=OutCubic }
])

let boxAnimations = freeze([
  { prop=AnimProp.translate,  from=[hdpx(200), 0], to=[0,0], duration=0.8, play=true, easing=OutQuintic }
  { prop=AnimProp.translate,  from=[0, 0], to=[-hdpx(300),-hdpx(100)], duration=0.25, playFadeOut=true, easing=OutCubic }
  { prop=AnimProp.opacity, from=1, to=0, duration=0.25, playFadeOut=true, easing=OutCubic }
])

function clearRoom(){
  joinedRoomInfo.set(null)
  joinedRoomInfoError.set(null)
}

let noRoomsFound = freeze({
  size = flex()
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  children = {rendObj = ROBJ_TEXT, text= "No rooms found"}
})

function mkRoomCell(text, width = null, color=null){
  return {
    rendObj = ROBJ_TEXT
    size = width !=null ? [width, SIZE_TO_CONTENT] : null
    text
    color
    halign = ALIGN_CENTER
    fontSize = hdpx(20)
  }
}

let roomLineSizes = ["Sample gaija planet", "Players", "WWWW WWWW WWW", "31 Nov 8888 88:88:88"].map(@(v) calc_str_box(mkRoomCell(v))[0])

function mkRoomLine(children){
  return {
    gap = hdpx(30)
    children
    flow = FLOW_HORIZONTAL
  }
}
let roomsListColsHeaders = mkRoomLine(["Scene", "Players", "Creater", "Time"].map(@(text, i) mkRoomCell(text, roomLineSizes?[i], Color(105,135,135))))

function mkRoomsUi() {
  joinedRoomInfoError.set(null)
  let joinedRoomId = Computed(@() joinedRoomInfo.get()?["roomId"])
  let requestRooms = mkJsonHttpReq($"{get_master_server_url()}/rooms_list",
    function(err) {
      if (roomsErrorTxt.get()==null)
        showWarning($"request rooms: {err}")
      roomsErrorTxt.set(err)
      receivedRoomsInfo.set([])
      selectedRoom.set(null)
    },
    function(res) {
      let prevRooms = receivedRoomsInfo.get()
      receivedRoomsInfo.set(res.map(mkRoomInfo))
      roomsErrorTxt.set(null)
      try {
        if (prevRooms.len()==0 && res.len() > 0 ) {
          selectedRoom.set(res?[0]["roomId"])
          return
        }
        let roomsIds = receivedRoomsInfo.get().reduce(function(m, i) {m[i.roomId] <- i.roomId; return m;}, {})
        if (selectedRoom.get() not in roomsIds)
          selectedRoom.set(null)
      }
      catch(e) {
        selectedRoom.set(null)
      }
    }
  )

  let requestRoomInfo = mkJsonHttpReq($"{get_master_server_url()}/room_info",
    function(err) {
      if (isInMainMenu.get() && joinedRoomInfoError.get() == null)
        showWarning($"Get Room Info: {err}")
      joinedRoomInfoError.set(err)
      //joinedRoomInfo.set(null)
    },
    function(res) {
      let roomInfo = mkRoomInfo(res)
      if (userUid.get() not in roomInfo?.users) {
        joinedRoomInfo.set(null)
        joinedRoomInfoError.set(null)
        showWarning($"You was kicked from the room")
        return
      }
      joinedRoomInfo.set(roomInfo)
      joinedRoomInfoError.set(null)
    }
  )

  function requestCurRoomInfo() {
    let roomId = joinedRoomId.get()
    if ((roomId ?? -1) != -1 && isInMainMenu.get())
      requestRoomInfo({roomId, userid=userUid.get(), nick=userName.get()})
    else
      joinedRoomInfoError.set(null)
  }

  let getSelScene = @() selectedScene.get().value
  let requestCreateRoom = mkJsonHttpReq($"{get_master_server_url()}/create_room",
    @(res) showWarning($"Create Room Error: {res}"), @(res) joinedRoomInfo.set(mkRoomInfo(res)))
  let openCreateRoomBtn = menuBtn("Create Room", @() isInCreateRoom.set(true), {hotkeys=[["^Enter"]]})
  let requestCreateRoomNjoin = mkJsonHttpReq($"{get_master_server_url()}/create_room", @(res) showWarning($"Create Room Error: {res}"), @(res) println(res))

  let requestJoinRoom = mkJsonHttpReq($"{get_master_server_url()}/join_room", @(res) showWarning($"Join Room Error: {res}"), function(res) {
    joinedRoomInfo.set(mkRoomInfo(res))
    joinedRoomInfoError.set(null)
  })

  function joinRoom(roomId) {
    if (selectedRoom.get()!=null)
      requestJoinRoom({userid=userUid.get(), roomId, nick = userName.get()})
    else {
      showWarning("no room selected")
      selectedRoom.set(null)
    }
  }
  let joinRoomBtn = menuBtn("Join Room", @() joinRoom(selectedRoom.get()), {hotkeys = [["^Enter"]]})

  let refreshRoomsBtn = menuBtn("Refresh", @() requestRooms(), {hotkeys=[["^R"]]})

  let requestLeaveRoom = mkJsonHttpReq($"{get_master_server_url()}/leave_room", @(err) showWarning($"Leave Room Error: {err}"), @(v) println(v))
  let requestLeaveRoomNoError = mkJsonHttpReq($"{get_master_server_url()}/leave_room", log, ulog)

  function leaveRoom(roomId=null) {
    if (joinedRoomInfoError.get() == null && roomId)
      requestLeaveRoom({userid = userUid.get(), roomId})
    else if (roomId)
      requestLeaveRoomNoError({userid = userUid.get(), roomId})
    clearRoom()
    requestRooms()
  }
  const READY_STATUS = "ready"
  const NOT_READY_STATUS = "not_ready"

  let requestStatusInRoom = mkJsonHttpReq($"{get_master_server_url()}/set_status_in_room", @(res) showWarning($"Set Status Error: {res}"), @(res) joinedRoomInfo.set(mkRoomInfo(res)))
  function setStatusInRoom(status){
  //  ulog("Set Status", status)
    let roomId = joinedRoomId.get()
    if (roomId==null)
      showWarning("no room to set status")
    requestStatusInRoom({userid = userUid.get(), roomId, status, nick = userName.get()})
  }

  let setReady = @() setStatusInRoom(READY_STATUS)
  let setNotReady = @() setStatusInRoom(NOT_READY_STATUS)
  let readyBtn = menuBtn("Ready", setReady)
  let notReadyBtn = menuBtn("Not Ready", setNotReady)
  let curStatus = Computed(@() joinedRoomInfo.get()?.users[userUid.get()]?["status"])
  let leaveRoomOffline = clearRoom

  function leaveRoomAction() {
    if (isOfflineMode.get()) {
      leaveRoomOffline()
      return
    }
    showMsgbox({
      text="Do you want to leave room?",
      buttons = [
        {text = "Leave", onClick = @() leaveRoom(joinedRoomId.get()), customStyle={hotkeys = [["Enter"]]}}
        {text = "Stay", onClick = @() null, customStyle={hotkeys = [["Esc"]]}}
      ]
    })
  }
  let leaveRoomsBtn = menuBtn("Leave Room", leaveRoomAction)

  let requestStartSession = mkJsonHttpReq($"{get_master_server_url()}/start_session", @(res) showWarning($"Error Starting Session: {res}"),
    function(res) {
      let startedRoomInfo = mkRoomInfo(res)
      ulog("onRequestStart", startedRoomInfo)
      joinedRoomInfo.set(startedRoomInfo)
    }
  )
  function startSession() {
    let scene = joinedRoomInfo.get()["scene"]
    ulog("Start", scene, $"offline: {offlineSession.get()}")
//    isInMainMenu.set(false)
    if (offlineSession.get()) {
      launch_network_session({scene})

    }
    else {
      let roomId = joinedRoomId.get()
      let userid = userUid.get()
      if (roomId==null) {
        showWarning("No room to Start")
        return
      }
      if (userid==null) {
        showWarning("Not logged in")
        return
      }
      requestStartSession({userid, roomId})
    }
  }

  function startAction(){
    if (isOfflineMode.get()) {
      startSession()
      return
    }
    showMsgbox({text = "Start session?", buttons= [
      {text = "Start", onClick = startSession, customStyle={hotkeys = [["Enter"]]}}
      {text = "Wait", onClick = @() null, customStyle={hotkeys = [["Esc"]]}}
    ]})
  }
  let startSessionBtn = menuBtn("Start", startAction)

  function mkRoomStateUi(panel, buttons, header=null) {
    let headerUi = header!=null ? {children = headerTxt(header) animations = headerAnimations key = header transform = {}} : null
    let panelBox = {
      hplace = ALIGN_CENTER
      key = {}
      vplace = ALIGN_CENTER
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      flow = FLOW_VERTICAL
      gap = hdpx(40)
      rendObj = ROBJ_BOX
      borderWidth = 0
      borderColor = Color(30,30,30)
      fillColor = Color(0,0,0,190)
      padding = hdpx(40)
      animations = boxAnimations
      transform = {}
      children = [
        {size = [sw(50), sh(50)] children = panel, clipChildren=true}
        buttons
      ]
    }
    return freeze({
      padding = hdpx(40)
      size = flex()
      transform = {}
      children = [headerUi, panelBox]
    })
  }

  function hotkeysRoomHandler(){
    let hotkeys = []
    if (isOfflineMode.get())
      hotkeys.append(["^Esc", leaveRoomAction], ["^Enter", startAction])
    else if (curStatus.get()==READY_STATUS){
      hotkeys.append(["^Esc", setNotReady], ["^Enter", startAction])
    }
    else if (curStatus.get()==NOT_READY_STATUS){
      hotkeys.append(["^Esc", leaveRoomAction], ["^Enter", setReady])
    }
    return {
      watch = [curStatus, isOfflineMode]
      key = {}
      hotkeys
    }
  }

  function buttonsJoinedRoom(){
    return {
      gap = hdpx(40)
      watch = [isOfflineMode, curStatus, joinedRoomInfo, joinedRoomInfoError]
      flow = FLOW_HORIZONTAL
      children = [
        leaveRoomsBtn,
        joinedRoomInfoError.get()!=null || isOfflineMode.get() ? null : (curStatus.get()==READY_STATUS ? notReadyBtn : readyBtn),
        joinedRoomInfoError.get() ==null ? startSessionBtn : null,
        hotkeysRoomHandler
      ]
    }
  }

  function buttonsRoomsList(){
    return {
      gap = hdpx(40)
      watch = [joinedRoomId, selectedRoom, isOfflineMode]
      flow = FLOW_HORIZONTAL
      children = isOfflineMode.get()
        ? [openCreateRoomBtn]
        : [refreshRoomsBtn, openCreateRoomBtn, selectedRoom.get() != null ? joinRoomBtn : null]
    }
  }


  function formatTime(time) {
    try {
      let {day, month, year, min, sec, hour} = unixtime_to_local_timetbl(time.tointeger())
      let monthMap = ["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep", "Oct", "Nov", "Dec"]
        .reduce(function(res, m, i) {res[i+1]<-m; return res;}, {})
      return " ".concat($"{day} {monthMap?[month+1] ?? month} {year}", format("%02d:%02d:%02d", hour,min,sec))
    }
    catch(e) return ""
  }

  let errorTitle = @(text) {rendObj = ROBJ_TEXT text color = Color(180, 120, 120) fontSize=hdpx(30)}
  let errorTxt = @(text) {
    size = flex()
    rendObj = ROBJ_TEXTAREA behavior = Behaviors.TextArea
    color = Color(100, 80,80)
    text
  }

  function errorScrn(titleTxt, watchErrorTxt=null, tip=null) {
    let watch = watchErrorTxt instanceof Watched ? watchErrorTxt : null
    return @() {
      size = flex()
      flow = FLOW_VERTICAL
      gap = hdpx(20)
      watch
      children = [
        errorTitle(titleTxt)
        errorTxt(watch!=null ? watch?.get() : watchErrorTxt)
        type(tip) =="string" ? {rendObj = ROBJ_TEXT text = tip} : tip
      ]
    }
  }

  let roomsErrorComp = errorScrn("Error getting rooms info", roomsErrorTxt, menuBtn("Logout", logout))

  function mkRoomComp(roomInfo){
    let roomId = roomInfo["roomId"]
    let stateFlags = Watched(0)
    let onElemState = @(s) stateFlags.set(s)
    let onClick = @() selectedRoom.set(roomId)
    let onDoubleClick = @() joinRoom(roomId)
    return @() {
      watch = [selectedRoom, stateFlags]
      rendObj = ROBJ_BOX
      padding = hdpx(5)
      fillColor = stateFlags.get() & S_HOVER ? Color(30,30,30,30) : 0
      behavior = Behaviors.Button
      onDoubleClick
      onClick
      onElemState
      borderWidth = selectedRoom.get()==roomId ? [0,0, hdpx(4), 0] : 0
      children = mkRoomLine([
        scenePathToSceneName(roomInfo["scene"]),
        "".concat(roomInfo["users"].len(), "/", roomInfo["params"]?.maxPlayers ?? "??"),
        roomInfo["creater_nick"],
        formatTime(roomInfo["timeCreated"])
      ].map(@(text, i) mkRoomCell(text, roomLineSizes?[i])))
    }
  }
  function roomsPlainList() {
    let children = [roomsListColsHeaders].extend(receivedRoomsInfo.get().map(mkRoomComp))
    return {
      watch = receivedRoomsInfo
      children
      flow = FLOW_VERTICAL
      halign = ALIGN_CENTER
      size = flex()
    }
  }
  function mkRoomsList(){
    let doRoomsExist = Computed(@() receivedRoomsInfo.get().len() >0 )
    return function(){
      return {
        onDetach = function() {
          selectedRoom.set(null)
          gui_scene.clearTimer(requestRooms)
        }
        onAttach = function() {
          if (isOfflineMode.get())
            return
          requestRooms()
          gui_scene.setInterval(5, requestRooms)
        }
        watch = [doRoomsExist, roomsErrorTxt]
        size = flex()
        children = (roomsErrorTxt.get() ?? "") != "" ? roomsErrorComp : (doRoomsExist.get() ? roomsPlainList: noRoomsFound)
      }
    }
  }

  let mkRoomsListUi = @() mkRoomStateUi(mkRoomsList(), buttonsRoomsList, "Rooms List")

  let mkComboState = @(watch, key) {var=watch, setValue = @(val) watch.mutate(@(v) v[key]<-val), getValue = @() watch.get()?[key]}

  function mkCreateRoomUi(){
    let doJoinAfterCreation = Watched(true)
    let options = [
      ["Scene:", mkCombo({var=selectedScene, values=scenes})],
      ["Max time limit (min):", mkCombo(mkComboState(roomParams, "sessionMaxTimeLimit").__update({values = [1.0, 2.0, 3.0, 5.0, 10.0, 15.0]}))],
    ]
    if (!isOfflineMode.get()){
      options.append(
        ["Max Players:", mkCombo(mkComboState(roomParams, "maxPlayers").__update({values=[2,3,4,5,6,7,8]}))],
        ["Local game:", mkCombo({var=offlineSession, values = [true, false]})],
        ["Join room:", mkCombo({var=doJoinAfterCreation, values = [true, false]})],
      )
    }
    let maxTextWidth = calc_str_box({rendObj = ROBJ_TEXT text = "maximum allowed text size"})[0]
    function createRoomSettings(){
      return {
        flow = FLOW_VERTICAL
        size = flex()
        gap = hdpx(10)
        halign = ALIGN_CENTER
        valign = ALIGN_CENTER
        onDetach = @() isInCreateRoom.set(false)
        onAttach = function() {
          if (isOfflineMode.get())
            offlineSession.set(true)
        }
        children = options.map(@(v) {
            flow=FLOW_HORIZONTAL, gap = hdpx(5)
          children = [
            {size =[maxTextWidth, SIZE_TO_CONTENT] rendObj = ROBJ_TEXT text = v[0]},
            {size=[maxTextWidth/2, SIZE_TO_CONTENT] children = v[1]}
          ]
        })
      }
    }
    let createRoomBtn = menuBtn("Create", function() {
      if (isOfflineMode.get()) {
        joinedRoomInfo.set(mkRoomInfo({
          scene = getSelScene(), roomId=-1, users=[{nick=desiredUserName.get(), status=READY_STATUS}], timeCreated=get_local_unixtime(), creater_nick=desiredUserName.get()
          params = roomParams.get()
        }))
        let params = roomParams.get()
        setOfflineSessionParams({sessionMaxTimeLimit = (params?["sessionMaxTimeLimit"] ?? 10).tofloat(), maxPlayers = (params?["maxPlayers"] ?? 8).tointeger()})
        return
      }
      if (doJoinAfterCreation.get())
        requestCreateRoom({userid=userUid.get(), nick = userName.get(), scene = getSelScene(), params = object_to_json_string(roomParams.get())})
      else {
        requestCreateRoomNjoin({userid=userUid.get(), nick = userName.get(), scene = getSelScene(), params = object_to_json_string(roomParams.get())})
        isInCreateRoom.set(false)
      }
    }, {hotkeys=[["^Enter"]]})

    let buttonsCreateRoom = freeze({
      gap = hdpx(40)
      flow = FLOW_HORIZONTAL
      children = [menuBtn("Back", @() isInCreateRoom.set(false), {hotkeys=[["^Esc"]]}), createRoomBtn]
    })

    return mkRoomStateUi(createRoomSettings, buttonsCreateRoom, "Create Room")
  }

  let showDbgInfo = Watched(false)
  let dbgInfoBtn = menuBtn("?", @() showDbgInfo.modify(@(v) !v))

  function roomDbgInfoTxt(){
    return {
      watch = joinedRoomInfo
      size = [flex(), sh(40)]
      rendObj = ROBJ_TEXTAREA
      behavior = [Behaviors.TextArea,Behaviors.WheelScroll]
      preformatted = true
      text = tostring_r(joinedRoomInfo.get())
    }
  }
  function roomDbgInfo(){
    return {
      flow = FLOW_VERTICAL
      watch = showDbgInfo
      size = flex()
      children = [
        dbgInfoBtn
        showDbgInfo.get() ? roomDbgInfoTxt : null
    ]}
  }
  let playersInfoTitle = freeze({
    halign = ALIGN_CENTER margin = [0, 0, hdpx(10), 0]
    size = [flex(), SIZE_TO_CONTENT]
    children = {rendObj = ROBJ_TEXT text = "Players" fontSize=hdpx(30)}
  })

  let statusTexts = {[NOT_READY_STATUS] = "not ready"}
  function users(){
    let children = [playersInfoTitle]
    foreach (user in (joinedRoomInfo.get()?.users ?? {})){
      let status = user?.status==null ? "not in room" : (statusTexts?[user.status] ?? user.status)
      children.append({
        flow = FLOW_HORIZONTAL
        gap = hdpx(10)
        size = [flex(), SIZE_TO_CONTENT]
        children = [
          {rendObj = ROBJ_TEXT text = user?.nick}
          {rendObj = ROBJ_TEXT text = status color = status==READY_STATUS
            ? Color(120,180,140)
            : status==statusTexts.not_ready ? Color(140, 100, 100)  : Color(80,80,80)}
        ]
      })
    }
    return {
      watch = joinedRoomInfo
      flow = FLOW_VERTICAL
      size = flex()
      children
      gap = hdpx(10)
    }
  }

  function sceneInfo(){
    let info = joinedRoomInfo.get()
    let scenename = scenePathToSceneName(info?.scene)
    let createdBy = info?.creater_nick ?? "unknown"
    let createdTime = info?.timeCreated!=null ? formatTime(info?.timeCreated ) : "unknown"
    let sceneName = { text = $"Scene: {scenename}" rendObj = ROBJ_TEXT fontSize = hdpx(30) }
    let createrDetails = { text = $"Created by: {createdBy}" rendObj = ROBJ_TEXT color = Color(90,90,90,50)}
    let createTimed = { text = $"{createdTime}" rendObj = ROBJ_TEXT color = Color(90,90,90,50)}
    let players = { text = $"Players: {info?.users.len() ?? "?"}/{info?.params.maxPlayers ?? "?"}" rendObj = ROBJ_TEXT color = Color(90,90,90,50)}
    let params = (info?.params ?? {})
      .filter(@(_, k) k!="maxPlayers")
      .reduce(@(res, val, key) res.append($"{key}: {val}"), [])
      .map(@(text) { text rendObj = ROBJ_TEXT color = Color(90,90,90,50)})
    return {
      watch = [joinedRoomInfo, showDbgInfo]
      flow = FLOW_VERTICAL
      children = [sceneName, createrDetails, createTimed, players].extend(params).append(roomDbgInfo)
      gap = hdpx(10)
    }
  }

  function roomInfoNorm(){
    return {
      children = [sceneInfo, users]
      flow = FLOW_HORIZONTAL
      gap = hdpx(80)
      size = flex()
      onAttach = function() {
        gui_scene.resetTimeout(1, function() {
          gui_scene.clearTimer(requestCurRoomInfo)
          gui_scene.setInterval(0.9, requestCurRoomInfo)
        })
      }
      onDetach = function() {
        gui_scene.clearTimer(requestCurRoomInfo)
        clearRoom()
      }
    }
  }

  let roomInfoErrorUi = errorScrn("Error in room info", joinedRoomInfoError, "Probably room is destroyed or server is down")

  let roomInfoUi = function() {
    return {
      watch = joinedRoomInfoError
      size  = flex()
      children = (joinedRoomInfoError.get() ?? "") != "" ? roomInfoErrorUi : roomInfoNorm
    }
  }

  let joinedRoomUi = mkRoomStateUi(roomInfoUi, buttonsJoinedRoom, "Room Info")

  function tryConnectTo(room_info) {
    if (room_info && room_info?.hostIp != null) {
      println($"trying to connect to ip:{room_info?.hostIp}")
      gui_scene.clearTimer(requestCurRoomInfo)
      let connectTo = room_info?.hostPort ? $"{room_info.hostIp}:{room_info.hostPort}" : room_info.hostIp
      let sessionParams = {sessionId=room_info?.sessionId ?? 0, host_urls = [connectTo]}
      if (room_info?.authKey)
        sessionParams.authKey <- room_info?.authKey
      if (room_info?.encKey)
        sessionParams.encKey <- room_info?.encKey
      launch_network_session(sessionParams)
    }
  }

  joinedRoomInfo.subscribe(tryConnectTo)
  tryConnectTo(joinedRoomInfo.get())
  return @() {
    watch = [joinedRoomId, isOfflineMode, isInCreateRoom]
    size = flex()
    children = [
      {children = reloginUI, hplace = ALIGN_RIGHT}
      joinedRoomId.get()!=null ? joinedRoomUi : (isInCreateRoom.get() ? mkCreateRoomUi() : mkRoomsListUi())
    ]
  }

}

let mainMenu = function() {
  let children = [background]
   if (showGameMenu.get())
     children.append(gameMenu)
   else
     children.append(userUid.get()!=null ? mkRoomsUi() : mainLogin)
  children.append(menuBtn("Menu", @() showGameMenu.set(true)))
  return {
    watch = [userUid, showGameMenu]
    size = flex()
    rendObj = ROBJ_SOLID
    behavior = DngBhv.ActivateActionSet
    actionSet = "StopInput"
    color = Color(0,0,0)
    children
  }
}
isInMainMenu.subscribe(function(v){
  if (v)
    clearRoom()
})

return {
  mainMenu
  isInMainMenu
  background
}