from "%darg/ui_imports.nut" import *
import "%dngscripts/ecs.nut" as ecs

let { EventSqChatMessage, mkCmdChatMessage } = require("%scripts/globs/sqevents.nut")
let { userName } = require("%scripts/ui/login.nut")

let chatLines = persist("chatLines", @() [])
let linesGen = mkWatched(persist, "linesGen", 0)
let showChatInput = mkWatched(persist, "showChatInput", true)
let outMessage = mkWatched(persist, "outMessage", "")

let sendMessage = @(text) ecs.client_msg_sink(mkCmdChatMessage({name=userName.get(), text}))

const MAX_LINES = 10
const DEF_TTL = 25
function pushMsg(name_from, text) {
  if (text=="")
    return
  let rec = {
    name = name_from
    text
    ttl = DEF_TTL
  }
  chatLines.append(rec)
  if (chatLines.len() > MAX_LINES)
    chatLines.remove(0)
  linesGen.modify(@(v) v+1)
}

ecs.register_es("chat_client_es", {
    [EventSqChatMessage] = function onChatMessage(evt, _eid, _comp) {
      let data = evt?.data
      if (data==null)
        return
      let { name = "unknown", text = "" } = data
      pushMsg(name, text)
    }
  }, {comps_rq = ["msg_sink"]}
)

let itemAnim = freeze([
  { prop=AnimProp.opacity, from=1.0, to=0, duration=0.6, playFadeOut=true}
  { prop=AnimProp.scale, from=[1,1], to=[1,0.01], delay=0.4, duration=0.6, playFadeOut=true}
])

function chatItem(item) {
  let {name, text} = item
  let rowChildren = {
    size = FLEX_H
    rendObj = ROBJ_TEXTAREA
    behavior = Behaviors.TextArea
    text = $"{name}: {text}"
  }

  return @() {
    size = FLEX_H
    rendObj = ROBJ_SOLID
    color = Color(0,0,0,80)
    key = item
    children = {
      flow = FLOW_HORIZONTAL
      key = item
      size = FLEX_H
      children = rowChildren
    }
    padding = hdpx(2)
    transform = { pivot = [0, 0] }
    animations = itemAnim
  }
}

function chatLog() {
  let logLines = chatLines.map(@(line) chatItem(line))
  return {
    watch = [linesGen, showChatInput]
    size = flex()
    clipChildren = true
//    rendObj = showChatInput.get() ? ROBJ_WORLD_BLUR_PANEL : null
//    fillColor = Color(0,0,0,50)
    flow = FLOW_VERTICAL
    children = logLines
    key = showChatInput.get()
//    behavior = showChatInput.get() ? Behaviors.WheelScroll : null
    valign = ALIGN_BOTTOM
    vplace = ALIGN_BOTTOM
  }
}

let inputBoxHeight = fsh(3)
let inputBoxDummy = freeze({size=[flex(), inputBoxHeight]})

function inputBox() {
  return {
    rendObj = ROBJ_SOLID
    vplace = ALIGN_BOTTOM
    color = Color(0,0,0,200)
    size = [flex(), inputBoxHeight]

    children = @() {
      rendObj = ROBJ_TEXT
      size = FLEX_H
      margin = fsh(0.5)
      text = outMessage.get()
      watch = outMessage
      behavior = Behaviors.TextInput
      onChange = @(text) outMessage.set(text)
      onAttach = @(elem) capture_kb_focus(elem)
      function onReturn() {
        if (outMessage.get().len()>0) {
          sendMessage(outMessage.get())
        }
        outMessage.set("")
        showChatInput.set(false)
      }
      hotkeys = [
        [$"Esc", function() {
          outMessage.set("")
          showChatInput.set(false)
        }]
      ]
    }
  }
}

function chatUi() {
  let children = [chatLog, showChatInput.get() ? inputBox : inputBoxDummy]
  return {
    key = "chat"
    flow = FLOW_VERTICAL
    size = static [sw(20), fsh(20)]
//    rendObj = ROBJ_DEBUG
    vplace = ALIGN_BOTTOM
    watch = showChatInput
    children
    margin = hdpx(25)
  }
}


const ChatUpdatePeriod = 1.0

function updateChat() {
  local needToClear = false
  foreach (rec in chatLines){
    rec.ttl -= ChatUpdatePeriod
    if (rec.ttl < 0)
      needToClear = true
  }
  if (!needToClear)
    return
  chatLines.replace(chatLines.filter(@(rec) rec.ttl > 0))
  linesGen.modify(@(v) v+1)
}

function mkChatUi() {
  outMessage.set("")
  chatLines.clear()
  showChatInput.set(false)
  gui_scene.clearTimer(updateChat)
  gui_scene.setInterval(ChatUpdatePeriod, updateChat)

  return {chatUi, showChatInput}
}

return {
  mkChatUi
}
