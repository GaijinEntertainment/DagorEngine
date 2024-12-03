import "%sqstd/rand.nut" as Rand
from "%darg/ui_imports.nut" import *

let {system} = require("system")
let {get_primary_screen_info, get_arg_value_by_name} = require("dagor.system")
let psi = {pixelsHeight = 720}
try{
  psi.pixelsHeight = get_primary_screen_info().pixelsHeight
}
catch(e) {
  println("get_primary_screen_info is not implemented?")
}
let mkWebui = @(idx=0) $"-config:debug/enableWebSocketStream:b=yes -config:debug/WebSocketPort:i={9113+idx} -config:debug/dedicatedWebSocketPort:i={9114+idx} -config:debug/dedicatedWebUIPort:i={23457+idx}"

let rand = Rand()
let defSId = rand.rint()
let useVroms = @(vromsMode) vromsMode == "vroms"
    ? "-config:debug/useAddonVromSrc:b=no"
    : vromsMode == "source"
      ? "-config:debug/useAddonVromSrc:b=yes"
      : ""

let is_windows_host = (["win32", "win64", "win-arm64"].contains(require("platform").get_platform_string_id()))
let defExePath = get_arg_value_by_name("exePath") ?? (is_windows_host ? "..\\..\\outerSpace\\game" : "../../outerSpace/game")

let {pixelsHeight} = psi

let clientWndSize = [pixelsHeight*0.8, pixelsHeight*0.8*(1080/1920.0)].map(@(v) v.tointeger())
let wndOffset =  (pixelsHeight * 0.05).tointeger()

let getWindowForClient = @(idx){
  x = wndOffset + (idx % 2) * (clientWndSize[0] + wndOffset)
  y = wndOffset + (idx / 2) * (clientWndSize[1] + wndOffset)
  width = clientWndSize[0]
  height = clientWndSize[1]
}

function window(idx, _total) {
  let wnd = getWindowForClient(idx)
  return $"-config:video/mode:t=windowed -config:video/position:ip2={wnd.x},{wnd.y} -config:video/resolution:t={wnd.width}x{wnd.height}"
}

let client = @(idx=0) $"-user_id:{idx+1} -user_name:Player_{idx+1} {window(idx, 1)} --das-no-linter"
function clnt(idx, playersNum) {
  return $"<<gameExe>> <<videoDriver>> <<useVroms>> {client(idx)} {window(idx, playersNum)} <<connect>> <<dasDebugStr>> <<noSoundStr>> <<quality>> <<runDetachedSuffix>>"
}

function mkClient(idx) {
  return @(playersNum=1) idx<playersNum ? clnt(idx, playersNum) : ""
}

let scheme = {
  formatters = {
    dasDebugStr = @(dasDebug = true) dasDebug ? "-es_tag:dasDebug" : "--das-no-debugger"
    noSoundStr = @(enableSound = true) enableSound ? "" : "-nosound"
    useVroms
    connect  =  @(sessionId=defSId) $"-session_id:{sessionId} --connect"
    common = @() $"<<dasDebugStr>> <<noSoundStr>> {mkWebui()} --das-no-linter"
    gameExe = function(platform, exePath=defExePath) {
      if (["win32", "win64", "win-arm64"].contains(platform))
        return $"@start {exePath}\\{platform}\\outer_space-dev.exe"
      if (platform == "linux64")
        return $"{exePath}/{platform}/outer_space-dev"
      if (platform == "macOS")
        return $"open {exePath}/OuterSpace.app --args"
      return ""
    }
    exePath = @(exePath = defExePath) exePath
    dedicatedExe = function(platform, exePath = defExePath, dedicatedPlatform = "win32") {
      if (["win32", "win64", "win-arm64"].contains(platform))
        return $"@start {exePath}\\{dedicatedPlatform}\\outer_space-ded-dev.exe"
      if (platform == "linux64")
        return $"{exePath}/{platform}/outer_space-ded-dev"
      if (platform == "macOS")
        return $"{exePath}/macosx/outer_space-ded-dev"
      return ""
    }
    runDetachedSuffix = @(platform) ["win32", "win64", "win-arm64"].contains(platform) ? "" : "&"
    platform = @(platform = "win32") platform
    scene       = @(scene) $"-scene:gamedata/scenes/{scene}"
    videoDriver = @(videoDriver="dx12") ["game defined","auto"].contains(videoDriver) ? "" : $"-config:video/driver:t={videoDriver}"
    quality = @(_quality="high") ""//$"-config:graphics/preset:t=\"{quality}\"" //not working if requires restart
    fullscreenwindowed  = "-config:video/mode:t=fullscreenwindowed -config:video/resolution:t=auto"
    maxTimeLimit = @(maxTimeLimit=12) $"-config:sessionLaunchParams/sessionMaxTimeLimit:r={maxTimeLimit}"
    windowMode  = function(windowMode="fullscreenwindowed") {
      if (["fullscreenwindowed","windowed","fullscreen"].contains(windowMode))
        return $"-config:video/mode:t={windowMode}"
      return ""
    }
    extraCmd    = @(extraCmd=null) extraCmd ? $"{extraCmd}" : ""
    listen  =  @(sessionId=defSId) $"--listen -nostatsd -session_id:{sessionId}"
    client1 = mkClient(0)
    client2 = mkClient(1)
    client3 = mkClient(2)
    client4 = mkClient(3)
    dedicatedArgs = @(dedicatedArgs="") dedicatedArgs
  }
  schema = [
    {
      id = "local"
      title = "local"
      commands = [
        "<<gameExe>> -config:disableMenu:b=yes <<scene>> <<videoDriver>> <<common>> <<useVroms>> <<quality>> <<maxTimeLimit>> <<runDetachedSuffix>>"
      ]
    }
    {
      id = "menu"
      title = "menu"
      commands = [
        "<<gameExe>> -scene:gamedata/scenes/menu.blk <<videoDriver>> <<common>> <<useVroms>> <<quality>> <<runDetachedSuffix>>"
      ]
    }
    {
      id = "client_server"
      title = "client & server"
      commands = [
        "<<dedicatedExe>> <<listen>> <<scene>> <<common>> <<useVroms>> <<maxTimeLimit>> <<dedicatedArgs>> -nonetenc -nopeerauth --das-no-linter <<runDetachedSuffix>>"
        "<<client1>>"
        "<<client2>>"
        "<<client3>>"
        "<<client4>>"
      ]
    }
  ]
}

let gameSchemaById = {}
scheme.schema.each(@(ss) gameSchemaById[ss.id] <- ss.__merge({formatters = scheme.formatters}))
return {
  scheme
  gameSchemaById
  killGame = function(game="outer_space") {
    let kill_cmd = is_windows_host ? "taskkill /f /im" : "pkill -SIGINT"
    let exe_suffix = is_windows_host ? "dev*" : "dev"
    system($"{kill_cmd} {game}-{exe_suffix}")
    system($"{kill_cmd} {game}-ded-{exe_suffix}")
  }
}
