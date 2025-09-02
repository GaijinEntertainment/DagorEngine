import "eventbus" as eventbus
import "dagor.udp" as udp
from "json" import object_to_json_string, parse_json
from "dagor.time" import format_unixtime, get_local_unixtime, get_time_msec
from "%darg/ui_imports.nut" import *

let textInput = require("samples_prog/_basic/components/textInput.nut")
//let autoScrollTextArea = require("samples_prog/_basic/components/autoScrollTextArea.nut")
let textLog  = require("textlog.nut")



local logMessages = Watched([])
local echoPort = Watched("8192")

function print_log(msg) {
  local newKey = logMessages.get().len() ? logMessages.get().top().key + 1 : 0
  logMessages.mutate(@(v)
    v.append(
      {key = newKey, text = msg}
    ))
}

function error_log(msg) {
  local newKey = logMessages.get().len() ? logMessages.get().top().key + 1 : 0
  logMessages.mutate(@(v)
    v.append(
      {key = newKey, text = msg, color=Color(255, 17, 17)}
    ))
}

eventbus.subscribe("udp.on_packet", function(evt) {
  print_log($"[{evt.socketId}] <- '{evt.data.as_string()}' from {evt.host}:{evt.port}")

  if (evt.socketId == "echo-server-socket") {
    print_log("send echo response")
    udp.send(evt.socketId, evt.host, evt.port, evt.data)
  }
  else if (evt.socketId == "client-socket") {
    let {recvTime} = evt
    let {sendTime} = parse_json(evt.data.as_string())
    print_log($"{recvTime}")
    print_log($"Echo RTT {recvTime - sendTime}ms")
  }
})

function onStartButtonClick() {
  print_log("Asynchronous UDP echo test starting.")
  udp.close_socket("echo-server-socket")
  udp.close_socket("client-socket")
  let echoPortInt = echoPort.get().tointeger()

  if (!udp.bind("echo-server-socket", "127.0.0.1", echoPortInt))  {
    error_log($"Failed to bind socket to port {echoPortInt}. {udp.last_error()}")
  }
  else {
    print_log($"Echo socket bound to port {echoPortInt}.")
  }
  if (!udp.send("client-socket", "127.0.0.1", echoPortInt, object_to_json_string({sendTime=get_time_msec()}))) {
    error_log($"Failed to send udp packet: {udp.last_error()}")
  }
}

let startButton = @() {
  rendObj = ROBJ_SOLID
  color = Color(0,25,205)
  size = static [pw(20), SIZE_TO_CONTENT]
  behavior = Behaviors.Button
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  padding = hdpx(5)
  onClick = onStartButtonClick
  children = {
    rendObj = ROBJ_TEXT
    text = "Start"
  }
}

let ctrlPanel = @() {
  flow = FLOW_HORIZONTAL
  size = static [pw(100), SIZE_TO_CONTENT]
  children = [
    startButton
    {
      size = static [pw(10), flex()]
    }
    {
      flow = FLOW_HORIZONTAL
      size = flex()
      children = [
        {
          rendObj = ROBJ_TEXT
          margin = hdpx(4)
          text = "Echo UDP port:"
        }
        {
          size = static [pw(20), flex()]
          children = textInput(echoPort, {
            margin=0
            textmargin = hdpx(4)
          })
        }
      ]
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
    gap = sh(2)
    children = [
        ctrlPanel
        {
          rendObj = ROBJ_SOLID
          color = Color(0, 0, 0)
          size = flex()
          children = textLog(logMessages, {size = static [pw(100), ph(100)]})
        }
    ]
  }
}
