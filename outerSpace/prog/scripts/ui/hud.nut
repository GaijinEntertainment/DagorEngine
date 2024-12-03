from "%darg/ui_imports.nut" import *
from "math" import min
from "string" import format

let DngBhv = require("dng.behaviors")
let {mkActionText, getControlsByGroupTag} = require("input_hints.nut")
let {dtext, textBtn } = require("widgets/simpleComponents.nut")
let {mkChatUi} = require("chat.nut")
let {get_sync_time} = require("net")
let ecs = require("%dngscripts/ecs.nut")
let { exit_game, sessionResult, levelLoaded, isDisableMenu } = require("app_state.nut")
let { sound_play } = require("%dngscripts/sound_system.nut")
let { EventSessionFinishedOnTimeout=2,EventSessionFinishedOnWin = 3} = require_optional("dasevents")
let { showGameMenu, gameMenu } = require("game_menu.nut")

let spaceShipControls = getControlsByGroupTag("Spaceship")

let FONT_SZ_MAIN = hdpx(20)
let FONT_SZ_BIG = hdpx(40)
let FONT_SZ_GIANT= hdpx(120)

let spaceCraftControls = @() {
  flow = FLOW_VERTICAL
  halign = ALIGN_RIGHT
  gap  = hdpx(5)
  children = spaceShipControls.map(mkActionText)
}

let freeCamControls = dtext("F10 - toggle free camera (WASD + Mouse); Esc - Menu")
let controlsHints = {
  halign = ALIGN_RIGHT
  flow = FLOW_VERTICAL
  gap = hdpx(20)
  children = [spaceCraftControls, freeCamControls]
}

let hints = {children = controlsHints valign = ALIGN_BOTTOM size = flex(), halign = ALIGN_RIGHT padding = hdpx(10)}

let fpsBar = @(){
  behavior = Behaviors.FpsBar
  size = [sw(20), SIZE_TO_CONTENT]
  vplace = ALIGN_BOTTOM
  rendObj = ROBJ_TEXT
}

let countDownAnims = freeze([
  { prop=AnimProp.opacity, from=0, to=1, duration=0.3, play=true, easing=InCubic }
  { prop=AnimProp.scale,  from=[0, 0], to=[1,1], duration=0.5, play=true, easing=OutQuintic }
  { prop=AnimProp.opacity, from=1, to=0, duration=0.3, playFadeOut=true, easing=OutCubic }
  { prop=AnimProp.translate,  from=[0, 0], to=[0, -sh(25)], duration=0.3, playFadeOut=true, easing=InExp }
  { prop=AnimProp.scale,  from=[1, 1], to=[1.2, 1.2], duration=0.3, playFadeOut=true, easing=OutCubic }
])
let mainHudTxtStyle = {
  fontFxFactor = min(hdpx(64), 64)
  fontFx = FFT_GLOW
  fontFxColor = Color(0,0,0,80)
  color = Color(205,205,205,180)
}

let startCountDownTime = Watched(-1)
let startSessionTime = Watched(-1)
let endSessionTime = Watched(-1)
let countdownToStart = Watched(-1)

ecs.register_es("get_time_limits_ui", {
  [["onInit", "onChange"]] = function(_evt, _eid, comp) {
    startCountDownTime.set(comp["time_to_start_countdown_msec"])
    startSessionTime.set(comp["time_to_start_session_msec"])
    endSessionTime.set(comp["time_to_end_session_msec"])
    countdownToStart.set(comp["countdown_to_start_sec"])
  }
  onDestroy = function(...) {
    startCountDownTime.set(-1)
    startSessionTime.set(-1)
    endSessionTime.set(-1)
    countdownToStart.set(-1)
  }
}, {comps_track = [
  ["time_to_start_countdown_msec", ecs.TYPE_INT],
  ["time_to_start_session_msec", ecs.TYPE_INT],
  ["time_to_end_session_msec", ecs.TYPE_INT],
  ["countdown_to_start_sec", ecs.TYPE_INT],
]})

let players = Watched({})
let localPlayerEid = Watched(-1)

ecs.register_es("players_ui_es", {
  [["onInit", "onChange"]] = function(_evt, eid, comp) {
    let is_local = comp.is_local
    if (is_local)
      localPlayerEid.set(eid)
    players.mutate(function(v){
      v[eid] <- {name = comp.name, player_score=comp.player__score, player_last_checkpoint_time=comp.player__last_checkpoint_time, is_local=is_local, checkpoints = comp.player__checkpoints}
    })
  }
  onDestroy = function(_evt, eid, _comp) {
    if (localPlayerEid.get()==eid)
      localPlayerEid.set(-1)
    players.mutate(@(v) v.$rawdelete(eid))
  },
}, {comps_track = [
  ["name", ecs.TYPE_STRING], ["player__score", ecs.TYPE_FLOAT, 0.0],
  ["player__checkpoints", ecs.TYPE_INT, 0],
  ["player__last_checkpoint_time", ecs.TYPE_FLOAT, 0.0],
  ["is_local", ecs.TYPE_BOOL]
], comps_rq=["player"]})


ecs.register_es("debriefing_ui_es", {
  [EventSessionFinishedOnTimeout] = function(...){
     if (sessionResult.get()==null)
       sessionResult.set({})
     sessionResult.mutate(function(v) {
       v.reason <- "SESSION TIMEOUT"
       v.players <- players.get()
     })
  },
  [EventSessionFinishedOnWin] = function(evt, _eid, _comp) {
    let {byPlayer, scores} = evt
    if (sessionResult.get()==null)
      sessionResult.set({})
    let localPlEid = localPlayerEid.get().tostring()
    let plinfo = scores.getAll().filter(@(v) v?.name!=null).map(@(v, eid) v.__update({is_local = eid==localPlEid}))
    sessionResult.mutate(function(v) {
      v.winnerEid <- byPlayer
      v.winner <- players.get()?[byPlayer]
      v.players <- plinfo
    })
  }
})

let checkpoints = Watched({})

ecs.register_es("checkpoints_ui_es", {
  [["onInit", "onChange"]] = function(_evt, eid, comp) {
    let visited_by_players = {}
    foreach (i in comp.checkpoint__visited_by_player)
      visited_by_players[i]<-i
    checkpoints.mutate(function(v){
      v[eid] <- {score = comp.checkpoint__score, pos=comp.transform[3], visited_by_players}
    })
  }
  onDestroy = function(_, eid, _) {
    checkpoints.mutate(@(v) v.rawdelete(eid))
  }
}, {
  comps_track = [ ["checkpoint__visited_by_player", ecs.TYPE_EID_LIST]],
  comps_ro = [["checkpoint__score", ecs.TYPE_FLOAT, 0.0], ["transform", ecs.TYPE_MATRIX]], comps_rq=["check_pt"]
})

function mkAnimatedCT(i) {
  let text = i <= 0 ? "START" : i
  return {
    rendObj = ROBJ_TEXT text
    fontSize = FONT_SZ_GIANT
    color = Color(205,205,205,180)
    pos = [0, sh(20)]
    transform = {}
    key = text
    animations = countDownAnims
  }.__update(mainHudTxtStyle)
}
local oldt = null
function cdt() {
  let t = countdownToStart.get()
  if (t!= oldt) {
    sound_play(t > 0 ? "ui/timer_tick" : "ui/session_start")
    oldt = t
  }
  return {
    size = flex()
    watch = countdownToStart
    halign  = ALIGN_CENTER
    transform = {}
    children = t >= 0 ? mkAnimatedCT(t) : null
  }
}

function countDownTimer() {
  let sst = startSessionTime.get()
  let ctd = startCountDownTime.get()
  return {
    size = flex()
    watch = [startCountDownTime, startSessionTime]
    transform = {}
    children = ctd >= -0.0 && sst > 0 && ctd < sst ? cdt : null
  }
}

let hidetxt = {text = null, rendObj = null}

function mkTimer(et, cvt, digits=2, hide = false){
  let showColon = digits == 2
  return {
    rendObj = ROBJ_TEXT
    fontSize = FONT_SZ_BIG
    behavior = Behaviors.RtPropUpdate
    text = null

    update = function(){
      let ct = get_sync_time()*1000
      let num = cvt(et-ct)
      let text = showColon ? format($"%0{digits}d:", num) : format($"%0{digits}d", num)
      if (hide && num == 0)
        return hidetxt
      return {text}
    }
  }.__update(mainHudTxtStyle)
}

let mkMins = @(timeLeft) (timeLeft/60000).tointeger()
let mkSeconds = @(timeLeft) ((timeLeft%60000)/1000).tointeger()
let mkms = @(timeLeft) (timeLeft%1000)

function timeToSessionFinish() {
  let et = endSessionTime.get()
  return {
    halign = ALIGN_RIGHT
    watch = endSessionTime
    flow = FLOW_VERTICAL
    children = et <= 0 ? null : [
      {rendObj = ROBJ_TEXT text = "Time left"}.__update(mainHudTxtStyle)
      {
        flow = FLOW_HORIZONTAL
        children = [
          mkTimer(et, mkMins, 2, true)
          mkTimer(et, mkSeconds)
          mkTimer(et, mkms, 3)
        ]
      }
    ]
  }
}

function mkTextFromPlayerInfo(info){
  let color = info?.is_local ? Color(250,220,80, 220) : null
  return {
    flow = FLOW_HORIZONTAL
    gap = hdpx(10)
    children = [
      {text=info.name, rendObj = ROBJ_TEXT fontSize = FONT_SZ_MAIN}.__update(mainHudTxtStyle, {color})
      {text = info.player_score, rendObj = ROBJ_TEXT fontSize = FONT_SZ_MAIN}.__update(mainHudTxtStyle, {color})
    ]
  }
}
let mkScoreTxt= @(text_total, text=null) { rendObj = ROBJ_TEXT, text = text!=null ? $"{text} / {text_total}" : text_total, fontSize = FONT_SZ_MAIN }.__update(mainHudTxtStyle)

function scoresInfo(){
  local totalCheckpoints = checkpoints.get().len()
  local totalScore = 0
  let plEid = localPlayerEid.get()
  local myScore = players.get()?[plEid].player_score
  local myCheckPoints = 0
  foreach (v in checkpoints.get()) {
    totalScore += v.score
    if ( plEid in v?.visited_by_players)
      myCheckPoints+=1
  }
  return {
    watch = [checkpoints, localPlayerEid, players]
    flow = FLOW_VERTICAL
    halign = ALIGN_RIGHT
    children = [
      totalScore > 0 ? {flow = FLOW_HORIZONTAL gap = hdpx(10) children = [mkScoreTxt("SCORE:"), mkScoreTxt(totalScore, myScore)] } : null,
      totalCheckpoints > 0 ? {flow = FLOW_HORIZONTAL gap = hdpx(10) children = [mkScoreTxt("CHECKPOIHTS:"), mkScoreTxt(totalCheckpoints, myCheckPoints)] } : null,
    ]
  }

}
function mkPlayersInfo(pinfo){
  let res = pinfo.reduce(@(res, info) res.append(info), [])
  return res.sort(@(a,b) b.player_score <=> a.player_score || a.player_last_checkpoint_time <=> b.player_last_checkpoint_time)
}
function playersInfo(){
  return {
    watch = players
    flow = FLOW_VERTICAL
    gap = hdpx(10)
    halign = ALIGN_RIGHT
    children = mkPlayersInfo(players.get()).map(mkTextFromPlayerInfo)
  }
}
function sessionInfo(){
  return {
    flow = FLOW_VERTICAL
    children = [timeToSessionFinish, scoresInfo, playersInfo]
    hplace = ALIGN_RIGHT
    halign = ALIGN_RIGHT
    padding = hdpx(20)
    gap = hdpx(10)
  }
}

let markerAnim = freeze([
  { prop=AnimProp.scale,  from=[1, 1], to=[1.4, 1.4], duration=1.5, play=true, loop=true, easing=CosineFull}
])
let markerIcon = freeze({
  rendObj = ROBJ_TEXT
  text = "O"
  fontSize = hdpx(30)
}.__update(mainHudTxtStyle, {color = Color(100,100,100,40)}))

function marker(eid) {
  return {
    data = {
      eid
      minDistance = 5.0
      maxDistance = 10000
      yOffs = 0
      distScaleFactor = 0.6
      clampToBorder = true
    }
    transform = {}
    key = eid
    sortOrder = eid
    halign = ALIGN_CENTER
    valign = ALIGN_BOTTOM
    animations = markerAnim
    children = markerIcon
  }
}

function markerDist(eid) {
  return {
    data = {
      eid
      minDistance = 8.0
      maxDistance = 1000
      distScaleFactor = 0.1
      yOffs = -0.2
      clampToBorder = true
    }
    transform = {}
    key = {}
    pos = [0, sh(2.5)]
    targetEid = eid
    rendObj = ROBJ_TEXT
    text = "100"
    sortOrder = eid
    halign = ALIGN_CENTER
    behavior = [DngBhv.DistToPriority, Behaviors.OverlayTransparency, DngBhv.DistToEntity]
    markerFlags = DngBhv.MARKER_SHOW_ONLY_IN_VIEWPORT
    fontSize =hdpx(15)
  }.__update(mainHudTxtStyle)
}

let viewport = freeze({
  size = [sw(98), sh(98)]
  data = {
    isViewport = true
  }
})

function markers() {
  let children = [viewport]
  let plEid = localPlayerEid.get()
  foreach (eid, v in checkpoints.get()){
    if (plEid not in v.visited_by_players)
      children.append(marker(eid), markerDist(eid))
  }
  return {
    size = [sw(100), sh(100)]
    behavior = DngBhv.Projection
    sortChildren = true
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    watch = [checkpoints, localPlayerEid]
    children
  }
}

let winnerTxt = freeze({text = "WINNER" fontSize = FONT_SZ_BIG rendObj = ROBJ_TEXT}.__update(mainHudTxtStyle))
let debrSizes = []
let mkMkDebrText = @(color=null) @(text, i) {text rendObj = ROBJ_TEXT fontSize = FONT_SZ_BIG minWidth = debrSizes?[i] halign = ALIGN_CENTER}.__update(mainHudTxtStyle, {color})
let mkDebrText = mkMkDebrText()
let debrHeaders = ["Player Name", "Score", "Last checkpoint time (sec)"].map(mkMkDebrText(Color(180,170,120,140)))
debrSizes.replace(debrHeaders.map(@(v) calc_str_box(v)[0]))

function mkDebriefingFromPlayerInfo(info) {
  return {
    flow = FLOW_HORIZONTAL
    gap = hdpx(100)
    children = [info.name, info.player_score, info.player_last_checkpoint_time].map(mkDebrText)
  }
}

function mkDebriefing(show_btn=false) {
  let gap = freeze({size = [0, hdpx(20)]})
  return function() {
    let res = sessionResult.get()
    let winner = res?.winner.name
    let reason = res?.reason
    let children = []
    if (reason)
      children.append({text = reason fontSize = FONT_SZ_GIANT rendObj = ROBJ_TEXT}.__update(mainHudTxtStyle), gap)
    if (winner)
      children.append(winnerTxt, {text = winner fontSize = FONT_SZ_GIANT rendObj = ROBJ_TEXT}.__update(mainHudTxtStyle), gap)
    let plscore = sessionResult.get()?.players
    if (plscore && plscore.len() > 1) {
      children.append({flow = FLOW_HORIZONTAL gap = hdpx(100) children=debrHeaders})
      children.extend(mkPlayersInfo(sessionResult.get().players).map(mkDebriefingFromPlayerInfo))
    }
    if (show_btn)
      children.append({size = flex()}, textBtn("CLOSE", function() {
        sessionResult.set(null)
        if (isDisableMenu)
          exit_game()
      }))
    else
      children.append({size = flex()})
    return {
      watch = sessionResult
      size = flex()
      gap = hdpx(10)
      padding = [sh(10), hdpx(40), sh(10), hdpx(40)]
      rendObj = res && show_btn? ROBJ_SOLID : null
      color = Color(0,0,0,120)
      hplace = ALIGN_CENTER
      vplace = ALIGN_CENTER
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      flow = FLOW_VERTICAL
      children
    }
  }
}

function mkHud(){
  let {chatUi, showChatInput} = mkChatUi()
  return function(){
    let children = !levelLoaded.get()
      ? []
      : showGameMenu.get()
        ? gameMenu
        : [mkDebriefing(), countDownTimer, chatUi, sessionInfo, hints, markers, fpsBar]
    return {
      watch = [levelLoaded, showGameMenu]
      size = flex()
      eventHandlers = {
        ["HUD.GameMenu"] = @(_event) showGameMenu.set(true),
        ["HUD.ChatInput"] = @(_evt) showChatInput.set(true)
      }
      children
    }
  }
}
return { mkHud, sessionResult, mkDebriefing, showGameMenu }