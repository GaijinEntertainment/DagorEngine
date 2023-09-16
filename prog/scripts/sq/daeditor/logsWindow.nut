from "%darg/ui_imports.nut" import *

from "%sqstd/ecs.nut" import *

let {showLogsWindow, hasNewLogerr} = require("%daeditor/state/logsWindow.nut")
let {showEntitySelect} = require("state.nut")
let {colors} = require("components/style.nut")
let textButton = require("components/textButton.nut")
let closeButton = require("components/closeButton.nut")
let mkWindow = require("components/window.nut")
let scrollbar = require("%daeditor/components/scrollbar.nut")
let dagorDebug = require("dagor.debug")
let {set_clipboard_text} = require("dagor.clipboard")

let scrollHandler = ScrollHandler()
let logExpandedTexScroll = ScrollHandler()

let logList = Watched([])
let selectedLogIndex = Watched(-1)
let logCount = Computed(@() logList.value.len())

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

  if (!showLogsWindow.value)
    hasNewLogerr(true)
  logList.mutate(@(v) v.append(msg))
})

showEntitySelect.subscribe(@(v) v ? showLogsWindow(false) : null)
showLogsWindow.subscribe(function(v) {
  if (v) {
    hasNewLogerr(false)
    showEntitySelect(false)
  }
})

let function scrollBySelection() {
  scrollHandler.scrollToChildren(function(desc) {
    return desc?.idx == selectedLogIndex.value
  }, 2, false, true)
}

let function selectedLogCopy() {
  if (selectedLogIndex.value == -1)
    return
  set_clipboard_text(logList.value[selectedLogIndex.value])
}

let function statusLine() {
  return {
    watch = logCount
    size = [flex(), SIZE_TO_CONTENT]
    flow = FLOW_HORIZONTAL
    children = [
      {
        rendObj = ROBJ_TEXT
        halign = ALIGN_RIGHT
        size = [flex(), SIZE_TO_CONTENT]
        text = $"{logCount.value} errors   "
        color = Color(170,170,170)
     }
    ]
  }
}

let function listRow(msg, idx) {
  return watchElemState(function(sf) {
    let isSelected = selectedLogIndex.value == idx
    let color = isSelected ? colors.GridRowHover
      : sf & S_TOP_HOVER ? colors.GridRowHover
      : logTableColColor

    return {
      rendObj = ROBJ_SOLID
      margin = [0, 0, hdpx(5)]
      size = [flex(), SIZE_TO_CONTENT]
      idx
      color
      behavior = Behaviors.Button
      onClick = function(){
        selectedLogIndex(selectedLogIndex.value != idx ? idx : -1)
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

let function listRowMoreLeft(num) {
  return watchElemState(function(sf) {
    let color = sf & S_TOP_HOVER ? colors.GridRowHover : logTableColColor
    return {
      rendObj = ROBJ_SOLID
      size = [flex(), SIZE_TO_CONTENT]
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

let function selectedLogExpanded() {
  if (selectedLogIndex.value == -1)
    return { watch = selectedLogIndex }

  return {
    rendObj = ROBJ_SOLID
    color = logExpandedColor
    size = [flex(), hdpx(160)]
    watch = [logList, selectedLogIndex]
    children = scrollbar.makeVertScroll({
      margin = hdpx(10)
      rendObj = ROBJ_TEXTAREA
      behavior = Behaviors.TextArea
      size = [flex(), SIZE_TO_CONTENT]
      text = logList.value[selectedLogIndex.value]
    }, {
      scrollHandler = logExpandedTexScroll
      rootBase = class {
        size = flex()
        onAttach = scrollBySelection
      }
    })
  }
}


let function logsRoot() {

  let function listContent() {
    const maxVisibleItems = 500
    let rows = logList.value.slice(0, maxVisibleItems).map(@(msg, idx) listRow(msg, idx))
    if (rows.len() < logList.value.len())
      rows.append(listRowMoreLeft(logList.value.len() - rows.len()))

    return {
      rendObj = ROBJ_SOLID
      watch = [logList, selectedLogIndex]
      size = [flex(), SIZE_TO_CONTENT]
      flow = FLOW_VERTICAL
      children = rows
      color = logTableBgColor
      behavior = Behaviors.Button
    }
  }


  let scrollList = scrollbar.makeVertScroll(listContent, {
    scrollHandler
    rootBase = class {
      size = flex()
      onAttach = scrollBySelection
    }
  })

  let content = {
    flow = FLOW_VERTICAL
    gap = fsh(0.5)
    size = flex()
    children = [
      {
        size = [flex(), SIZE_TO_CONTENT]
        flow = FLOW_HORIZONTAL
        children = [
          { size = [sw(0.2), SIZE_TO_CONTENT] }
          {
            rendObj = ROBJ_TEXT
            size = [flex(), SIZE_TO_CONTENT]
            margin = fsh(0.5)
            text = "Errors log"
          }
          closeButton(@() showLogsWindow(false))
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
        size = [flex(), SIZE_TO_CONTENT]
        halign = ALIGN_CENTER
        children = [
          textButton("Clear",  function(){
            logList([])
            selectedLogIndex(-1)
          }, {hotkeys=["^X"]})
        ]
      }
    ]
    hotkeys = [
      ["L.Ctrl !L.Alt C", { action = selectedLogCopy, ignoreConsumerCallback = true }]
    ]
  }
  return mkWindow({
    id = "log_window"
    content
    saveState = true
  })()
}


return logsRoot
