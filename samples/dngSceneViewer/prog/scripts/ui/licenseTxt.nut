from "%darg/ui_imports.nut" import *
from "widgets/simpleComponents.nut" import menuBtn
from "dagor.fs" import read_text_from_file_on_disk

let {makeVertScroll} = require("widgets/scrollbar.nut")

let rstTagsFormat = {
  h1 = {
    checkfunc = @(line) line.startswith("======") //replace with regexp
    headerTag = true
  }
  h2 = {
    checkfunc = @(line) line.startswith("-----")  //replace with regexp
    headerTag = true
  }
}
function checkRstFmt(line) { //-disable:-return-different-types
  foreach (tag, info in rstTagsFormat) {
    if (info.checkfunc(line))
      return [tag, info.headerTag]
  }
  return false
}
let tagsTable = {
  h1 = { fontSize = hdpx(30) }
  h2 = { fontSize = hdpx(30) }
}
function formatLicenseTxt(text){
  let lines = text.replace("\r\n", "\n").split("\n")
  let ret = []
  local openedTag = null
  local headerTag = false
  foreach (line in lines) {
    let checkRes = checkRstFmt(line)
    if (checkRes) {
      if (openedTag && checkRes[0]==openedTag) {
        openedTag = null
      }
      else {
        openedTag = checkRes[0]
        headerTag = checkRes[1]
      }
    }
    else if (openedTag && headerTag) {
      if (line=="") //regexp? whitespaces also should be skipped?
        continue
      else {
        ret.append($"<{openedTag}>{line}</{openedTag}>")
        headerTag = false
      }
    }
    else {
      ret.append(line)
    }
  }
  return "\n".join(ret)
}

let showLicense = mkWatched(persist, "showLicense", true)

const licenseTxtFname = "LICENSE.txt"
const acceptKey = "Accept"
function licenseWnd() {
  let text = formatLicenseTxt(read_text_from_file_on_disk(licenseTxtFname))
  return {
    rendObj = ROBJ_WORLD_BLUR
    size = flex()
    fillColor = Color(0, 0, 0, 200)
    flow = FLOW_VERTICAL
    halign = ALIGN_CENTER
    gap = hdpx(20)
    padding = sh(8)
    onAttach = @() move_mouse_cursor(acceptKey, false)
    children = [
      @() {rendObj = ROBJ_TEXT text = licenseTxtFname fontSize = hdpx(30) color = Color(220,200, 150) hplace = ALIGN_LEFT}
      {
        size = [sw(50), flex()]
        children = makeVertScroll({
           rendObj = ROBJ_TEXTAREA
           size = [flex(), SIZE_TO_CONTENT]
           behavior = Behaviors.TextArea
           text
           tagsTable
           preformatted = FMT_KEEP_SPACES
        })
      }
      menuBtn("Accept", @() showLicense.set(false), {
        key = acceptKey,
        hotkeys = [["Enter"]]
      })
    ]
  }
}
return {
  formatLicenseTxt
  licenseWnd
  showLicense
}