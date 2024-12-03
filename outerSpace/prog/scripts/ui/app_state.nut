from "%darg/ui_imports.nut" import *
from "widgets/msgbox.nut" import showWarning, showMsgbox
import "%dngscripts/ecs.nut" as ecs

let { has_network, DC_CONNECTION_CLOSED, EventOnNetworkDestroyed, EventOnConnectedToServer, EventOnDisconnectedFromServer} = require("net")
let { EventGameSessionFinished = 0, EventGameSessionStarted = 1, EventSessionFinishedOnTimeout = 2, EventSessionFinishedOnWin = 3} = require_optional("dasevents")
let { get_arg_value_by_name, dgs_get_settings} = require("dagor.system")
let { userUid } = require("login.nut")
let { hardPersistWatched } = require("%sqstd/globalState.nut")
let app = require("app")
let { exit_game= @() println("exit")} = app

let isDisableMenu = dgs_get_settings()?["disableMenu"] || get_arg_value_by_name("connect")!=null


function launch_network_session(params){
  log("launch_network_session:", params)
  app?.launch_network_session(params)
}

function switch_to_menu_scene(){
  if (app.is_app_terminated())
    return
  launch_network_session({scene="gamedata/scenes/menu.blk"})
}

const SessionLaunchOfflineParamsId = "sessionLaunchParams"

function setOfflineSessionParams(params) {
  let settings = dgs_get_settings()
  println(settings.formatAsString())
  if (!(SessionLaunchOfflineParamsId in settings))
    settings.addNewBlock(SessionLaunchOfflineParamsId)
  foreach (k,v in params)
    settings[SessionLaunchOfflineParamsId][k] <- v
  println(settings.formatAsString())
}
function getOfflineSessionParams() {
  let ret = {}
  let s = dgs_get_settings()?[SessionLaunchOfflineParamsId] ?? {}
  foreach (k, v in s) {
    ret[k] <- v
  }
  return ret
}

println($"isDisableMenu: {isDisableMenu}")

let sessionResult = mkWatched(persist, "sessionResult", null)

let isInMainMenu = hardPersistWatched("isInMainMenu", !isDisableMenu && (ecs == null || userUid.get()==null))
let levelLoaded = Watched(false)
const DO_VISUAL_LOG = true

let ulog = DO_VISUAL_LOG ? log_for_user : log

let showExitGameMsgbox = @(text, onClose=exit_game) showMsgbox({
  text
  onClose
  buttons = [ {text = "Exit game", onClick = @() exit_game(), customStyle={hotkeys = [["Enter"]]}} ]
})

let showExitMsgboxOnDisconnect = @() showExitGameMsgbox("Connecton lost!")
let showExitMsgboxOnTimeout = @() showExitGameMsgbox("Session timed out!")
let showExitMsgboxOnSessionEnd = @() showExitGameMsgbox("Session Finished", @() sessionResult.set(null))


let inMenuQuery = ecs.SqQuery("inMenuQuery", {comps_ro = [["in_menu", ecs?.TYPE_BOOL]]})
isInMainMenu.set(inMenuQuery.perform(@(_eid, comp) comp["in_menu"]) ?? false)

ecs.register_es("isInMainMenu", {
  [EventGameSessionStarted] = @(...) isInMainMenu.set(false),
  [EventGameSessionFinished] = @(...) isInMainMenu.set(true),
  onInit = @(_evt, _eid, comp) log("InMenu:", comp["in_menu"]) ?? isInMainMenu.set(comp["in_menu"]),
  onDestroy = @(...) isInMainMenu.set(false)
}, {comps_ro = [["in_menu", ecs.TYPE_BOOL]]})

local sessionFinished = true

ecs.register_es("on_event_session_finished", {
  [EventSessionFinishedOnTimeout] = function(...){
    sessionFinished = true
    println("EventSessionFinishedOnTimeout")
    if (!isInMainMenu.get() && has_network()) {
      if (isDisableMenu) {
        showExitMsgboxOnTimeout()
      }
      else {
        switch_to_menu_scene()
      }
    }
  },

  [EventSessionFinishedOnWin] = function(_evt, _eid, _comp) {
    println("EventSessionFinishedOnWin")
    sessionFinished = true
    const timeToFinish = 5.0
    if (isInMainMenu.get() || !has_network())
      return
    if (!isDisableMenu)
      gui_scene.resetTimeout(timeToFinish, switch_to_menu_scene)
  }
})

ecs.register_es("level_state_ui_es",
  {
    [["onChange","onInit"]] = @(_eid, comp)  levelLoaded.set(comp["level__loaded"]),
    onDestroy = @() levelLoaded.set(false)
  },
  {comps_track = [["level__loaded", ecs.TYPE_BOOL]]}
)
local isDisconnecting = false
function onDisconnectFromServer(evt) {
  let reason = evt?.last_client_dc ?? evt[0]
  println($"reason = {reason} {reason == DC_CONNECTION_CLOSED ? "DC_CONNECTION_CLOSED" : ""}")
  println($"isDisconnecting = {isDisconnecting}")
  if (isDisableMenu) {
    if (!sessionFinished)
      showExitMsgboxOnDisconnect()
    else
      showExitMsgboxOnSessionEnd()
  }
  else if (!isInMainMenu.get() && !isDisconnecting) {
    if (!sessionFinished)
      showWarning("Connection Lost")
    switch_to_menu_scene()
  }
}

local wasConnectedToServer = false
ecs.register_es("net_server_ui_es", {
  [EventOnDisconnectedFromServer] = function onDisconnectedFromServer(evt, _eid, _comp) {
    println("EventOnDisconnectedFromServer")
    onDisconnectFromServer(evt)
    isDisconnecting = true
  },
  [EventOnNetworkDestroyed] = function onDisconnectedFromServer(evt, _eid, _comp) {
    println("EventOnNetworkDestroyed")
    if (!wasConnectedToServer)
      return
    println($"isDisconnecting = {isDisconnecting}")
    onDisconnectFromServer(evt)
    wasConnectedToServer = false
    isDisconnecting = true
  },
  [EventOnConnectedToServer] = function() {
    wasConnectedToServer = true
    sessionFinished = false
    println($"isDisconnecting = {isDisconnecting}")
    isDisconnecting = false
    println("EventOnConnectedToServer")
  }
})


let isLoadingState = Computed(@() !levelLoaded.get() && !isInMainMenu.get())

function loadingWatchdog(){
  switch_to_menu_scene()
  showWarning("Error joining session: loading took too much time")
}
function checkLoading(v) {
  if (v)
    gui_scene.resetTimeout(45, loadingWatchdog)
  else
    gui_scene.clearTimer(loadingWatchdog)
}
isLoadingState.subscribe(checkLoading)
checkLoading(isLoadingState.get())

return {
  isInMainMenu
  isStubMode = ecs==null
  ulog
  levelLoaded
  sessionResult
  isDisableMenu
  launch_network_session
  isLoadingState
  switch_to_menu_scene
  setOfflineSessionParams
  getOfflineSessionParams
  exit_game
}