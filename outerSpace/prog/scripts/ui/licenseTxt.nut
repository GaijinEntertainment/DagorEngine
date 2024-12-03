from "%darg/ui_imports.nut" import *
from "widgets/simpleComponents.nut" import normalCursor, menuBtn
from "dagor.fs" import read_text_from_file_on_disk

let { makeVertScroll } = require("widgets/scrollbar.nut")
let { hardPersistWatched } = require("%sqstd/globalState.nut")
let { isDisableMenu } = require("app_state.nut")
let { dgs_get_settings} = require("dagor.system")
let { set_setting_by_blk_path_and_save } = require("settings")

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
const LicenseAcceptedKey = "licenseAccepted"
let showLicense = hardPersistWatched("showLicense", isDisableMenu ? !(dgs_get_settings()?[LicenseAcceptedKey] ?? false) : true) //show in Menu each time

function acceptLicense() {
  set_setting_by_blk_path_and_save(LicenseAcceptedKey, true)
  showLicense.set(false)
}
const licenseTxtFname = "LICENSE.txt"
function licenseUi() {
  let text = formatLicenseTxt(read_text_from_file_on_disk(licenseTxtFname))
  return {
    rendObj = ROBJ_WORLD_BLUR
    size = flex()
    fillColor = Color(30, 30, 30, 255)
    flow = FLOW_VERTICAL
    halign = ALIGN_CENTER
    cursor = normalCursor
    gap = hdpx(20)
    padding = sh(5)
    stopMouse = true
    children = [
      @() {rendObj = ROBJ_TEXT text = licenseTxtFname fontSize = hdpx(30) color = Color(220,200, 150) hplace = ALIGN_LEFT}
      {
        size = [sw(60), flex()]
        children = makeVertScroll({
          size=[flex(), SIZE_TO_CONTENT]
          rendObj = ROBJ_SOLID
          padding = hdpx(20)
          color = Color(0,0,0,120)
          children = {
           rendObj = ROBJ_TEXTAREA
           size = [flex(), SIZE_TO_CONTENT]
           behavior = Behaviors.TextArea
           text
           tagsTable
           preformatted = FMT_KEEP_SPACES
        }})
      }
      menuBtn("Accept", acceptLicense)
    ]
  }
}
return {
  formatLicenseTxt
  licenseUi
  showLicense
}