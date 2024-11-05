from "%darg/ui_imports.nut" import *
let textInput = require("samples_prog/_basic/components/textInput.nut")
let autoScrollTextArea = require("samples_prog/_basic/components/autoScrollTextArea.nut")
let textLog  = require("textlog.nut")

let { matching_listen_notify, matching_login, matching_logout, matching_call } = require("matching.api")
let matching_errors = require("matching.errors")
let { eventbus_subscribe, eventbus_subscribe_onehit } = require("eventbus")
let { object_to_json_string, parse_json } = require("json")
let {format_unixtime, get_local_unixtime} = require("dagor.time")

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
  if (loginState.value == LoginState.Disconnected)
    return "Login"
  if (loginState.value == LoginState.Connecting)
    return "Cancel"
  return "Logout"
})

local loginStatusText = Computed(function() {
  if (loginState.value == LoginState.Disconnected)
    return "Disconnected"
  else if (loginState.value == LoginState.Connecting)
    return "Connecting"
  else
    return "Logged In"
})

local loginStatusColor = Computed(function() {
  if (loginState.value == LoginState.LoggedIn)
    return Color(50, 128, 50)
  return Color(200, 200, 200)
})

local lastResult = Watched("")
local serverNotify = Watched("")

local resultLog = Watched([])
lastResult.subscribe(function(val) {
  local newKey = resultLog.value.len() ? resultLog.value.top().key + 1 : 0
  resultLog.mutate(@(v)
    v.append(
      {key = null, text = $"> {format_unixtime("%H:%M:%S", get_local_unixtime())}:", color=Color(255, 197, 17)}, //-format-arguments-count
      {key = newKey, text = val}
    )
  )
})

local notifyLog = Watched([])
serverNotify.subscribe(function(val) {
  local newKey = notifyLog.value.len() ? notifyLog.value.top().key + 1 : 0
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
  let reqInfo = matching_call(rpcCmd.value, parse_json(rpcParams.value))
  if (reqInfo?.reqId != null) {
    eventbus_subscribe_onehit($"{rpcCmd.value}.{reqInfo.reqId}",
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
  if (loginState.value == LoginState.Disconnected) {
    loginState(LoginState.Connecting)
    matching_login(loginInfo)
  }
  else if (loginState.value == LoginState.LoggedIn)  {
    matching_logout()
  }
}

let loginButton = @() {
  rendObj = ROBJ_SOLID
  color = Color(0,25,205)
  size = [pw(100), SIZE_TO_CONTENT]
  behavior = Behaviors.Button
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  padding = hdpx(5)
  onClick = onLoginButtonClick
  watch = loginBtnText
  children = {
    rendObj = ROBJ_TEXT
    text = loginBtnText.value
  }
}

let loginStatus =  @() {
  rendObj = ROBJ_TEXT
  watch = loginStatusText
  text = loginStatusText.value
  color = loginStatusColor.value
}

let loginInfoC = @() {
  flow = FLOW_VERTICAL
  rendObj = ROBJ_BOX
  borderWidth = hdpx(1)
  padding = hdpx(5)
  children = loginInfo.map(@(value, key) {
    flow = FLOW_HORIZONTAL
    size = [hdpx(500), SIZE_TO_CONTENT]
    children = [
      autoScrollTextArea({text = key})
      autoScrollTextArea({text = value})
    ]
  }).values()
}

let logsBlock = @() {
  flow = FLOW_HORIZONTAL
  size = [pw(100), flex()]
  watch = [lastResult, serverNotify]
  children = [
    {
      flow = FLOW_VERTICAL
      size = [pw(40), flex()]
      children = [
        {
          size = [pw(100), SIZE_TO_CONTENT]
          rendObj = ROBJ_TEXT
          text = "Result"
        }
        {
          rendObj = ROBJ_SOLID
          color = Color(0, 0, 0)
          size = [flex(), flex()]
          children = textLog(resultLog, {size = [pw(100), ph(100)]})
        }
      ]
    }
    {
      size = [flex(), 0]
    }
    {
      flow = FLOW_VERTICAL
      size = [pw(40), flex()]
      children = [
        {
          size = [pw(100), SIZE_TO_CONTENT]
          rendObj = ROBJ_TEXT
          text = "Notifications"
        }
        {
          rendObj = ROBJ_SOLID
          color = Color(0, 0, 0)
          size = [flex(), flex()]
          children = textLog(notifyLog, {size = [pw(100), ph(100)]})
        }
      ]
    }
  ]
}

local subscribeVal = Watched("")

let rpcForm = {
  flow = FLOW_VERTICAL
  valign = ALIGN_TOP
  size = [pw(100), ph(30)]
  gap = hdpx(6)
  children = [
    { rendObj = ROBJ_TEXT text = "Send command" }
    {
      flow = FLOW_HORIZONTAL
      size = [pw(100), SIZE_TO_CONTENT]
      gap = sh(3)
      children = [
        {
          flow = FLOW_HORIZONTAL
          size = flex()
          gap = sh(1)
          children = [
            { rendObj = ROBJ_TEXT text = "Cmd:" }
            {
              size = [flex(), flex()]
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
    { size = [0, flex()] }
    { rendObj = ROBJ_TEXT text = "Subscribe" }
    {
      size = [pw(50), SIZE_TO_CONTENT]
      children = textInput(subscribeVal, { onReturn = @() subscribeNotify(subscribeVal.value)
                                margin = 0 textmargin = hdpx(2)})
    }
  ]
}


return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = [pw(100), ph(100)]
  children = {
    flow = FLOW_VERTICAL
    pos = [pw(5), ph(5)]
    size = [pw(90), ph(90)]
    gap = sh(10)
    children = [{
        flow = FLOW_HORIZONTAL
        size = [pw(100), SIZE_TO_CONTENT]
        children = [
          {
            flow = FLOW_VERTICAL
            gap = sh(1)
            valign = ALIGN_CENTER
            halign = ALIGN_CENTER
            size = [pw(10), SIZE_TO_CONTENT]
            children = [
              loginStatus
              loginButton
            ]
          }
          {size = [flex(), SIZE_TO_CONTENT]}
          loginInfoC
        ]
      }
      rpcForm
      logsBlock
    ]
  }
}
