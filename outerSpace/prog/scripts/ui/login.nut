from "%darg/ui_imports.nut" import *
from "system" import getenv
from "http_task.nut" import HttpPostTask, mkJsonHttpReq
from "widgets/simpleComponents.nut" import textBtn, textInput, mkWatchedText, headerTxt, menuBtn, menuBtnTextColorNormal, menuBtnTextColorHover

from "backend_api.nut" import get_master_server_url, set_master_server_url
from "widgets/msgbox.nut" import showWarning
import "ecs" as ecs

let { hardPersistWatched } = require("%sqstd/globalState.nut")
let {EventUserLoggedIn, EventUserLoggedOut} = require("gameevents")
let userUid = hardPersistWatched("userId") //get after, login
let getDefUserName = @() getenv("username")
let userName = hardPersistWatched("userName", getDefUserName())
let desiredUserName = hardPersistWatched("desiredUserName", getDefUserName())
let loginInProgress = Watched(false)

const OfflineUid = -1

let mkText = @(text) {rendObj = ROBJ_TEXT, text, fontSize = hdpx(30)}

let offlineLogin = @() userUid.set(OfflineUid)
let isOfflineMode = Computed(@() userUid.get() == OfflineUid)

function logout() {
  userUid.set(null)
  userName.set(getDefUserName())
  ecs.g_entity_mgr.broadcastEvent(EventUserLoggedOut())
}

let currentUserName = function() {
  let stateFlags = Watched(0)
  return @() {
    watch=[userName, stateFlags], rendObj = ROBJ_TEXT, text=userName.get(),
    behavior = Behaviors.Button, onClick = logout, fontSize=hdpx(20)
    onElemState = @(s) stateFlags.set(s)
    color = stateFlags.get() & S_HOVER ? menuBtnTextColorHover : menuBtnTextColorNormal
  }
}()

let loginRequest = @() mkJsonHttpReq($"{get_master_server_url()}/login", function(err) {
    showWarning($"login error:{err}")
    logout()
    loginInProgress.set(false)
  },
  function(res){
    loginInProgress.set(false)
    userUid.set(res["userid"])
    userName.set(res["username"])
    ecs.g_entity_mgr.broadcastEvent(EventUserLoggedIn(res["userid"].tointeger(), res["username"]))
  }
)

function login(username = null){
  if (username == null)
    username = desiredUserName.get()
  loginInProgress.set(true)
  loginRequest()({username})
}

let reloginUI = @() {
  padding =hdpx(20)
  flow = FLOW_HORIZONTAL
  valign = ALIGN_CENTER
  gap = hdpx(5)
  children = [{rendObj = ROBJ_TEXT, text="Player:" fontSize=hdpx(20)}, currentUserName]
}

let continueOfflineBtn = menuBtn("Continue Offline", offlineLogin)

let desiredUserNameUi = {
  flow = FLOW_HORIZONTAL
  children = [
    mkText("Nickname: ")
    textInput({state=desiredUserName, placeholder="nickname"})
  ]
}
let loginInProgressUi = @() {
  size = flex()
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  children = {
    behavior = Behaviors.Button
    rendObj = ROBJ_TEXT
    text = "Trying to login..."
    fontSize = hdpx(30)
  }
}

let title = headerTxt("Outer Space")

function mkLoginScreen() {
  let masterServerUrl = Watched(get_master_server_url())
  let loginBtn = textBtn("Login", function() {
    set_master_server_url(masterServerUrl.get())
    login()
  }, {hotkeys = [["^Enter"]]})

  let masterServerUrlComp = @() {
    flow = FLOW_VERTICAL
    halign = ALIGN_CENTER
    gap = hdpx(10)
    children = [
      {rendObj = ROBJ_TEXT, text ="Master Server Url"}
      textInput({state=masterServerUrl, fontSize = hdpx(20), placeholder = "http://127.0.0.0:5000"})
    ]
  }
  let runMasterServerTip = {
    size = [sw(60), SIZE_TO_CONTENT]
    rendObj = ROBJ_TEXTAREA behavior = Behaviors.TextArea
    preformatted=true
    text = "To run local master server, run 'python master_server_sample.py' in prog/master_server_sample folder.\n You need python3.11 and flask installed."
    color = Color(100,100,100)
  }

  let serverAddresTip = @() {
    size = [sw(60), SIZE_TO_CONTENT]
    rendObj = ROBJ_TEXTAREA behavior = Behaviors.TextArea
    watch = masterServerUrl
    text = " ".concat($"Current master server url: {masterServerUrl.get()}.",
      "You can configure url with settings.blk or config.blk, by setting 'masterServerUrl:t=<https://your_url_here>'.",
      "Config also can be set via commandline, add '-config:masterServerUrl:t=<https://your_url_here>' to commandline"
    )
    color = Color(100,100,100)
  }

  return {
    padding = sh(10)
    size = flex()
    children = [
      title
      {
        flow = FLOW_VERTICAL
        gap = hdpx(40)
        children = [
          {size = [0, sh(20)]},
          desiredUserNameUi,
          loginBtn,
          masterServerUrlComp,
          continueOfflineBtn,
          {size = [0, sh(5)]},
          serverAddresTip, runMasterServerTip]
        hplace = ALIGN_CENTER
        halign = ALIGN_CENTER
        valign = ALIGN_CENTER
      }
    ]
  }
}
let mainLogin = @() {
  watch =  loginInProgress
  size = flex()
  children = loginInProgress.get() ? loginInProgressUi : mkLoginScreen()
}

return {
  login
  logout
  isOfflineMode
  userUid
  reloginUI
  mainLogin
  desiredUserName
  userName
}
