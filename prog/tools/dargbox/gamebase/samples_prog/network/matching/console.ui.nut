import "matching.errors" as matching_errors
from "matching.api" import matching_listen_notify, matching_login, matching_logout, matching_call
from "eventbus" import eventbus_subscribe, eventbus_subscribe_onehit
from "json" import object_to_json_string, parse_json
from "dagor.time" import format_unixtime, get_local_unixtime
from "%darg/ui_imports.nut" import *

let textInput = require("samples_prog/_basic/components/textInput.nut")
let autoScrollTextArea = require("samples_prog/_basic/components/autoScrollTextArea.nut")
let textLog  = require("textlog.nut")


enum LoginState {
  Disconnected,
  Connecting,
  LoggedIn
}

let mgateAddrMoon = "match-moon-gj-msk-01.gaijin.ops:7853"
//let mgateAddrSun = "mgate-lw-nl-02.enlisted.net:7853"

let loginInfo = {
  mgateAddr = mgateAddrMoon
  userId = 100
  userName = "dargbox"
  versionStr = "0.0.0.0"
}

local loginState = Watched(LoginState.Disconnected)

local rpcCmd = Watched("")
local rpcParams = Watched("{}")

local loginBtnText = Computed(function() {
  if (loginState.get() == LoginState.Disconnected)
    return "Login"
  if (loginState.get() == LoginState.Connecting)
    return "Cancel"
  return "Logout"
})

local loginStatusText = Computed(function() {
  if (loginState.get() == LoginState.Disconnected)
    return "Disconnected"
  else if (loginState.get() == LoginState.Connecting)
    return "Connecting"
  else
    return "Logged In"
})

local loginStatusColor = Computed(function() {
  if (loginState.get() == LoginState.LoggedIn)
    return Color(50, 128, 50)
  return Color(200, 200, 200)
})

local lastResult = Watched("")
local serverNotify = Watched("")

local resultLog = Watched([])
lastResult.subscribe(function(val) {
  local newKey = resultLog.get().len() ? resultLog.get().top().key + 1 : 0
  resultLog.mutate(@(v)
    v.append(
      {key = null, text = $"> {format_unixtime("%H:%M:%S", get_local_unixtime())}:", color=Color(255, 197, 17)}, //-format-arguments-count
      {key = newKey, text = val}
    )
  )
})

local notifyLog = Watched([])
serverNotify.subscribe(function(val) {
  local newKey = notifyLog.get().len() ? notifyLog.get().top().key + 1 : 0
  notifyLog.mutate(@(v) v.append(
    {key = null, text = $"> {format_unixtime("%H:%M:%S", get_local_unixtime())}:", color=Color(255, 197, 17)} //-format-arguments-count
    {key = newKey, text = val}
  ))
})

let printNotify = @(name, notify) serverNotify(object_to_json_string(notify.__merge({name}), true))

function subscribeNotify(name, handler = printNotify) {
  matching_listen_notify(name)
  eventbus_subscribe(name, @(obj) handler(name, obj))
  lastResult($"subscribed to {name}")
}

function sendRPC() {
  let reqInfo = matching_call(rpcCmd.get(), parse_json(rpcParams.get()))
  if (reqInfo?.reqId != null) {
    eventbus_subscribe_onehit($"{rpcCmd.get()}.{reqInfo.reqId}",
      function(result) {
        if (result.error != 0)
          lastResult(matching_errors.error_string(result.error))
        else
          lastResult(object_to_json_string(result, true))
      })
  } else {
    lastResult(matching_errors.error_string(reqInfo.error))
  }
}

eventbus_subscribe("matching.login_finished", function(result) {
  lastResult(object_to_json_string(result, true))
  if (result.status == 0) {
    subscribeNotify("mpresence.notify_presence_update")
    loginState(LoginState.LoggedIn)
  }
  else
    loginState(LoginState.Disconnected)
})

eventbus_subscribe("matching.on_disconnect", function(reason) {
  lastResult(object_to_json_string(reason, true))
  loginState(LoginState.Disconnected)
})

function onLoginButtonClick() {
  if (loginState.get() == LoginState.Disconnected) {
    loginState(LoginState.Connecting)
    matching_login(loginInfo)
  }
  else if (loginState.get() == LoginState.LoggedIn)  {
    matching_logout()
  }
}

let loginButton = @() {
  rendObj = ROBJ_SOLID
  color = Color(0,25,205)
  size = static [pw(100), SIZE_TO_CONTENT]
  behavior = Behaviors.Button
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  padding = hdpx(5)
  onClick = onLoginButtonClick
  watch = loginBtnText
  children = {
    rendObj = ROBJ_TEXT
    text = loginBtnText.get()
  }
}

let loginStatus =  @() {
  rendObj = ROBJ_TEXT
  watch = loginStatusText
  text = loginStatusText.get()
  color = loginStatusColor.get()
}

let loginInfoC = @() {
  flow = FLOW_VERTICAL
  rendObj = ROBJ_BOX
  borderWidth = hdpx(1)
  padding = hdpx(5)
  children = loginInfo.map(@(value, key) {
    flow = FLOW_HORIZONTAL
    size = static [hdpx(500), SIZE_TO_CONTENT]
    children = [
      autoScrollTextArea({text = key})
      autoScrollTextArea({text = value})
    ]
  }).values()
}

let logsBlock = @() {
  flow = FLOW_HORIZONTAL
  size = static [pw(100), flex()]
  watch = [lastResult, serverNotify]
  children = [
    {
      flow = FLOW_VERTICAL
      size = static [pw(40), flex()]
      children = [
        {
          size = static [pw(100), SIZE_TO_CONTENT]
          rendObj = ROBJ_TEXT
          text = "Result"
        }
        {
          rendObj = ROBJ_SOLID
          color = Color(0, 0, 0)
          size = flex()
          children = textLog(resultLog, {size = static [pw(100), ph(100)]})
        }
      ]
    }
    {
      size = static [flex(), 0]
    }
    {
      flow = FLOW_VERTICAL
      size = static [pw(40), flex()]
      children = [
        {
          size = static [pw(100), SIZE_TO_CONTENT]
          rendObj = ROBJ_TEXT
          text = "Notifications"
        }
        {
          rendObj = ROBJ_SOLID
          color = Color(0, 0, 0)
          size = flex()
          children = textLog(notifyLog, {size = static [pw(100), ph(100)]})
        }
      ]
    }
  ]
}

local subscribeVal = Watched("")

let rpcForm = {
  flow = FLOW_VERTICAL
  valign = ALIGN_TOP
  size = static [pw(100), ph(30)]
  gap = hdpx(6)
  children = [
    { rendObj = ROBJ_TEXT text = "Send command" }
    {
      flow = FLOW_HORIZONTAL
      size = static [pw(100), SIZE_TO_CONTENT]
      gap = sh(3)
      children = [
        {
          flow = FLOW_HORIZONTAL
          size = flex()
          gap = sh(1)
          children = [
            { rendObj = ROBJ_TEXT text = "Cmd:" }
            {
              size = flex()
              children = textInput(rpcCmd, {
                onReturn = sendRPC
                margin=0
                textmargin = hdpx(4)
              })
            }]
        }
        {
          flow = FLOW_HORIZONTAL
          size = flex()
          gap = sh(1)
          children = [
            { rendObj = ROBJ_TEXT text = "Params JSON:" }
            {
              size = flex()
              children = textInput(rpcParams, {
                onReturn = sendRPC
                margin=0
                textmargin = hdpx(4)
              })
            }]
        }
      ]
    }
    { size = static [0, flex()] }
    { rendObj = ROBJ_TEXT text = "Subscribe" }
    {
      size = static [pw(50), SIZE_TO_CONTENT]
      children = textInput(subscribeVal, { onReturn = @() subscribeNotify(subscribeVal.get())
                                margin = 0 textmargin = hdpx(2)})
    }
  ]
}


return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = static [pw(100), ph(100)]
  children = {
    flow = FLOW_VERTICAL
    pos = [pw(5), ph(5)]
    size = static [pw(90), ph(90)]
    gap = sh(10)
    children = [{
        flow = FLOW_HORIZONTAL
        size = static [pw(100), SIZE_TO_CONTENT]
        children = [
          {
            flow = FLOW_VERTICAL
            gap = sh(1)
            valign = ALIGN_CENTER
            halign = ALIGN_CENTER
            size = static [pw(10), SIZE_TO_CONTENT]
            children = [
              loginStatus
              loginButton
            ]
          }
          {size = FLEX_H}
          loginInfoC
        ]
      }
      rpcForm
      logsBlock
    ]
  }
}
