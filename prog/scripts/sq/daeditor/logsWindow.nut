import "dagor.debug" as dagorDebug
from "dagor.clipboard" import set_clipboard_text
from "%darg/ui_imports.nut" import *
from "%sqstd/ecs.nut" import *
let { LogsWindowId } = require("state.nut")
let { hasNewLogerr } = require("%daeditor/state/logsWindow.nut")
let { colors } = require("components/style.nut")
let textButton = require("components/textButton.nut")
let { isWindowVisible } = require("components/window.nut")
let { makeVertScroll } = require("%daeditor/components/scrollbar.nut")

let scrollHandler = ScrollHandler()
let logExpandedTexScroll = ScrollHandler()

let logList = Watched([])
let selectedLogIndex = Watched(-1)
let logCount = Computed(@() logList.get().len())

let logTableColColor = Color(15,15,15,255)
let logTableBgColor = Color(15,15,15,140)
let logExpandedColor = Color(15,15,15,200)

let excludeLogByText = [
  "sync creation of entity"
]

dagorDebug.register_logerr_monitor([" "], function(_tags, msg, _timestamp) {
  foreach(excludeText in excludeLogByText)
    if (msg.contains(excludeText))
      return

  if (!isWindowVisible(LogsWindowId))
    hasNewLogerr.set(true)
  logList.mutate(@(v) v.append(msg))
})

function scrollBySelection() {
  scrollHandler.scrollToChildren(function(desc) {
    return desc?.idx == selectedLogIndex.get()
  }, 2, false, true)
}

function selectedLogCopy() {
  if (selectedLogIndex.get() == -1)
    return
  set_clipboard_text(logList.get()[selectedLogIndex.get()])
}

function statusLine() {
  return {
    watch = logCount
    size = FLEX_H
    flow = FLOW_HORIZONTAL
    children = [
      {
        rendObj = ROBJ_TEXT
        halign = ALIGN_RIGHT
        size = FLEX_H
        text = $"{logCount.get()} errors   "
        color = Color(170,170,170)
     }
    ]
  }
}

function listRow(msg, idx) {
  return watchElemState(function(sf) {
    let isSelected = selectedLogIndex.get() == idx
    let color = isSelected ? colors.GridRowHover
      : sf & S_TOP_HOVER ? colors.GridRowHover
      : logTableColColor

    return {
      rendObj = ROBJ_SOLID
      margin = [0, 0, hdpx(5)]
      size = FLEX_H
      idx
      color
      behavior = Behaviors.Button
      onClick = function(){
        selectedLogIndex.set(selectedLogIndex.get() != idx ? idx : -1)
      }
      children = {
        rendObj = ROBJ_TEXT
        text = msg
        fontSize = hdpx(14)
        margin = fsh(0.5)
      }
    }
  })
}

function listRowMoreLeft(num) {
  return watchElemState(function(sf) {
    let color = sf & S_TOP_HOVER ? colors.GridRowHover : logTableColColor
    return {
      rendObj = ROBJ_SOLID
      size = FLEX_H
      color
      children = {
        rendObj = ROBJ_TEXT
        text = $"{num} more ..."
        margin = [fsh(0.7), fsh(0.1), fsh(0.7), fsh(0.5)]
        color = Color(160,160,160,160)
      }
    }
  })
}

function selectedLogExpanded() {
  if (selectedLogIndex.get() == -1)
    return { watch = selectedLogIndex }

  return {
    rendObj = ROBJ_SOLID
    color = logExpandedColor
    size = static [flex(), hdpx(160)]
    watch = [logList, selectedLogIndex]
    children = makeVertScroll({
      margin = hdpx(10)
      rendObj = ROBJ_TEXTAREA
      behavior = Behaviors.TextArea
      size = FLEX_H
      text = logList.get()[selectedLogIndex.get()]
    }, {
      scrollHandler = logExpandedTexScroll
      rootBase = {
        size = flex()
        onAttach = scrollBySelection
      }
    })
  }
}


function listContent() {
  const maxVisibleItems = 500
  let rows = logList.get().slice(0, maxVisibleItems).map(@(msg, idx) listRow(msg, idx))
  if (rows.len() < logList.get().len())
    rows.append(listRowMoreLeft(logList.get().len() - rows.len()))

  return {
    rendObj = ROBJ_SOLID
    watch = [logList, selectedLogIndex]
    size = FLEX_H
    flow = FLOW_VERTICAL
    children = rows
    color = logTableBgColor
    behavior = Behaviors.Button
  }
}


let scrollList = makeVertScroll(listContent, {
  scrollHandler
  rootBase = {
    size = flex()
    onAttach = scrollBySelection
  }
})

function logsRoot() {
  return {
    flow = FLOW_VERTICAL
    gap = fsh(0.5)
    size = flex()
    children = [
      {
        size = FLEX_H
        flow = FLOW_HORIZONTAL
        children = static [
          { size = [sw(0.2), SIZE_TO_CONTENT] }
          {
            rendObj = ROBJ_TEXT
            size = FLEX_H
            margin = fsh(0.5)
            text = "Errors log"
          }
        ]
      }
      {
        size = flex()
        children = scrollList
      }
      selectedLogExpanded
      statusLine
      {
        flow = FLOW_HORIZONTAL
        size = FLEX_H
        halign = ALIGN_CENTER
        children = [
          textButton("Clear",  function(){
            logList.set([])
            selectedLogIndex.set(-1)
          }, {hotkeys=["^X"]})
        ]
      }
    ]
    hotkeys = [
      ["L.Ctrl !L.Alt C", { action = selectedLogCopy, ignoreConsumerCallback = true }]
    ]
  }
}
return {
  id = LogsWindowId
  content = logsRoot
  onAttach = @() hasNewLogerr.set(false)
  saveState=true
}
