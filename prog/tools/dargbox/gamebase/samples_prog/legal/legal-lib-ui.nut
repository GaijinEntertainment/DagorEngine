from "%darg/ui_imports.nut" import *
from "dagor.localize" import loc
let {makeVertScroll} = require("samples_prog/_basic/components/scrollbar.nut")

let txt = @(text, style = null) {rendObj = ROBJ_TEXT text}.__update(style ?? {})
let textArea = @(text, style = null) {rendObj = ROBJ_TEXTAREA behavior = Behaviors.TextArea text size = [flex(), SIZE_TO_CONTENT]}.__update(style ?? {})

let txtBtn = function(text, handler=null, opts = null) {
  let stateFlags=Watched(0)
  return @() {
    rendObj = ROBJ_BOX
    watch = stateFlags
    padding = [hdpx(5), hdpx(10)]
    children = txt(text)
    onClick = handler
    onElemState = @(s) stateFlags(s)
    borderRadius = hdpx(5)
    behavior = Behaviors.Button
    fillColor = stateFlags.value & S_HOVER ? Color(60,80,80) : Color(40,40,40)
  }.__update(opts ?? {})
}

let curSelColor = Color(200,200,200)
let curSelDoc = {rendObj = ROBJ_SOLID size = [hdpx(10), flex()] color=curSelColor margin = [hdpx(5),0]}
let curNSelDoc = {size = [hdpx(10), flex()] margin = [hdpx(5),0]}

let docBtn = function(text, isCurrent, handler=null) {
  let stateFlags = Watched(0)
  return function() {
    let color =
      isCurrent.value
      ? curSelColor
      : stateFlags.value & S_HOVER
        ? Color(255,255,255)
        : Color(150,205,255)
    return {
      flow  = FLOW_HORIZONTAL
      size = SIZE_TO_CONTENT
      gap = hdpx(10)
      children = [
        isCurrent.value ? curSelDoc : curNSelDoc
        {
          rendObj = ROBJ_FRAME
          children = txt(text, {color})
          borderWidth = isCurrent.value ? 0 :[0,0,hdpx(1),0]
          color
        }
      ]
      watch = [isCurrent, stateFlags]
      onClick = handler
      onElemState = @(s) stateFlags(s)
      behavior = Behaviors.Button
    }
  }
}

let introTitle = loc("legal/introTitle", "Introduction")
let curChapter = mkWatched(persist, "curChapter", 0)

let protoIntro = freeze({title = introTitle, loaded=true})

let mkLegalWindow = kwarg(function(legal_docs_status, needApprove, isManuallyOpen, isNewUser, onConfirm, onCancel, legals_url, onDetach) {
  let newbieIntro = loc("legal/newbieConfirmEulaIntro", $"Confirm that you have read and agree with these documents (you can also read them on {legals_url})")
  let updateIntro = loc("legal/updateEulaIntrol", $"Documents have been updated, please read (you can also read them on {legals_url})")
  let manualIntro = loc("legal/manualLegalIntro", $"Legal documents. You can also read them on {legals_url} ")
  let closeWnd = @() isManuallyOpen(false)
  let needToShowLegals = Computed(@() needApprove.value || isManuallyOpen.value)
  let closeBtn = txtBtn(loc("legal/close", "Close"), closeWnd, {hotkeys = [["Esc | Enter"]]})
  let acceptAndCloseBtn = txtBtn(loc("legal/close", "Close"), onConfirm, {hotkeys = [["Esc | Enter"]]})
  let cancelBtn = onCancel != null ? txtBtn(loc("legal/cancel", "Cancel"), onCancel, {hotkeys = [["Esc | Enter"]]}) : null
  let acceptBtn = txtBtn(loc("legal/acceptAll", "Accept All"), onConfirm, {hotkeys = [["Enter"]]})
  let doc = function() {
    let chapters = []
    if (needApprove.value) {
      if (isNewUser.value)
        chapters.append(protoIntro.__merge({text = newbieIntro}))
      else
        chapters.append(protoIntro.__merge({text = updateIntro}))
    }
    else
      chapters.append(protoIntro.__merge({text = manualIntro}))
    chapters.extend(legal_docs_status.value.map(@(v) {title = loc(v.id)}.__merge(v)))
    let children = [txt(loc("legal/ToC", "Table of Contents"), {color = Color(129,129,129)})]
      .extend(chapters.map(@(v, idx) docBtn(v.title, Computed(@() idx==curChapter.value), @() curChapter(idx))))
    let textarea = function() {
      let chapter = chapters?[curChapter.value]
      let curText = chapter?.text ?? (
        chapter?.status != "FAILED" && !chapter?.loaded
          ? loc("legal/loading", "Loading. Please wait")
          : "visit legal.gaijin.net"
      )
      let t = {
        rendObj = ROBJ_TEXTAREA
        margin = hdpx(10)
        behavior = Behaviors.TextArea
        size=[flex(), SIZE_TO_CONTENT]
        text = curText
      }
      return {
        watch = curChapter
        size = flex()
        children  = {
          key = curChapter.value
          size = flex()
          children = makeVertScroll(t)
        }
      }
    }
    let toc = {
      flow = FLOW_VERTICAL
      padding = hdpx(10)
      gap = hdpx(5)
      children
    }
    return {
      watch = [legal_docs_status, isManuallyOpen, isNewUser]
      onDetach
      size=flex()
      flow = FLOW_HORIZONTAL
      gap = hdpx(5)
      children = [
        toc
        {rendObj=ROBJ_SOLID color = Color(50,50,50) size=[hdpx(1), flex()]}
        textarea
      ]
    }
  }
  let gap = freeze({size=[flex(),0]})
  return function() {
    if (!needToShowLegals.value)
      return {watch = needToShowLegals}
    else {
      return {
        size = [sw(80), sh(80)]
        hplace = ALIGN_CENTER
        vplace = ALIGN_CENTER
        watch = [needToShowLegals]
        rendObj = ROBJ_SOLID
        color = Color(30,30,30)
        padding = hdpx(10)
        flow = FLOW_VERTICAL
        gap = hdpx(10)
        children = [
          txt(loc("legal/title", "Legal Agreements and Policies"), {hplace = ALIGN_CENTER})
          doc
          @() {
            watch = [isManuallyOpen, isNewUser]
            flow = FLOW_HORIZONTAL
            size = [flex(), SIZE_TO_CONTENT]
            children = needApprove.value && isNewUser.value
              ? [cancelBtn, gap, acceptBtn]
              : needApprove.value && !isNewUser.value
                ? [gap, acceptAndCloseBtn]
                : [gap, closeBtn]
          }
        ]
      }
    }
  }
})

return {
  txtBtn
  txt
  textArea
  mkLegalWindow
}