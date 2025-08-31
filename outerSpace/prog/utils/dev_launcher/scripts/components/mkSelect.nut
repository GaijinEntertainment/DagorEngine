from "%darg/ui_imports.nut" import *
import "style.nut" as style

let {makeVertScroll} = require("scrollbar.nut")
let {mkButton} = require("button.nut")
let {dtext} = require("text.nut")
let {mkTextInput} = require("textInput.nut")
let {chunk} = require("%sqstd/underscore.nut")
let {addModalWindow, removeModalWindow} = require("modalWindows.nut")
let fa = require("%darg/helpers/fontawesome.map.nut")

const SELECT_UID = "SELECT"
let hvrCl = Color(0,0,0)
let nrmClr = Color(180,180,180)

let mkSelect = kwarg(function(options, selected, title = "SELECT", label = null, viewOpt = @(v) v.tostring(), multiselect=null, size=null){
  multiselect = multiselect ?? (type(selected.get())=="array")
  let filterOptionsStr = Watched("")
  options = ("value" in options) ? options : Watched(options)
  let scrollHandler = ScrollHandler()

  let filteredOptions = Computed( function() {
    let fltr = (filterOptionsStr.get() ?? "")
    if (fltr=="")
      return options.get()
    else
     return options.get().filter(@(v) v.contains(fltr))
  })

  let closeSelectMenu = @() removeModalWindow(SELECT_UID)
  function mkSelectOptionBtn(opt) {
    let name = viewOpt(opt)
    function onClick() {
      if (multiselect) {
        let idx = selected.get()?.findindex(@(v) v==opt)
        if (idx!=null)
          selected(selected.get().filter(@(v) v!=opt))
        else
          selected.mutate(@(v) v.append(opt))
      }
      else
        selected(opt)
      closeSelectMenu()
    }
    return watchElemState(@(sf) {
      clipChildren = true
      behavior = [Behaviors.Button]
      onClick
      children = @() {
        behavior = [Behaviors.Marquee]
        scrollOnHover = true
        padding = [hdpx(2), hdpx(8)]
        children = dtext(name, {color = sf & S_HOVER ? hvrCl : (selected.get() == opt ? Color(255,255,255) : nrmClr)})
        rendObj = ROBJ_BOX
        fillColor = sf & S_HOVER
          ? nrmClr
          : selected.get() == opt || (multiselect && selected.get()?.contains(opt))
            ? Color(60,60,60,60)
            : 0
        borderColor = Color(250, 200, 100)
        borderWidth = selected.get() == opt || (multiselect && selected.get()?.contains(opt)) ? hdpx(1) : 0
        size = size ?? [sh(35), SIZE_TO_CONTENT]
      }
    })
  }

  let mkColumn = @(opts) {
    flow = FLOW_VERTICAL
    children = opts.map(mkSelectOptionBtn)
  }

  let filterTextInput = mkTextInput(filterOptionsStr, {placeholder = "Enter text to filter list", size = [sw(30), SIZE_TO_CONTENT]})

  let optionsContainer = @() {
    watch = filteredOptions
    gap = hdpx(10)
    behavior = Behaviors.Button
    onClick = @() set_kb_focus(null)
    flow = FLOW_HORIZONTAL
    children = chunk(filteredOptions.get(), filteredOptions.get().len()/4 + 1).map(mkColumn)
  }

  let optionsScrolled = makeVertScroll(optionsContainer, {scrollHandler})

  function optionsSelectWindow() {
    return  {
      size = [sw(90), sh(90)]
      hplace = ALIGN_CENTER
      vplace = ALIGN_CENTER
      color = Color(0,0,0,120)
      stopMouse = true
      key = SELECT_UID
      behavior = Behaviors.Button
      onClick = @() set_kb_focus(null)
      watch = selected
      flow = FLOW_VERTICAL
      rendObj = ROBJ_FRAME
      gap = hdpx(5)
      children = [
        {size= [flex(), SIZE_TO_CONTENT] children = [
            dtext(title, {hplace = ALIGN_CENTER})
            mkButton({txtIcon = fa["close"]  tooltip = "Close Select Window", onClick = closeSelectMenu, hotkeys = [["Esc"]]
                       override = {hplace = ALIGN_RIGHT}
                     })
          ]
        }
        filterTextInput
        optionsScrolled
      ]
      padding = hdpx(20)
    }
  }

  let openSelect = function() {
    filterOptionsStr.set("")
    addModalWindow({
      size = flex()
      key = SELECT_UID
      color = Color(0,0,0)
      rendObj = ROBJ_SOLID
      children = optionsSelectWindow
    })
  }

  let selectBtn = @() {
    watch = selected
    flow = FLOW_HORIZONTAL
    valign = ALIGN_CENTER
    size = [flex(), SIZE_TO_CONTENT]
    children = [
      label ? dtext(label, {color = style.LABEL_TXT}) : null
      {size = [flex(), 0]}
      mkButton({
        text=multiselect ? ",".join(selected.get()) : (selected.get()?.tostring() ?? ""),
        onClick = openSelect,
        override = {size = size ?? [style.SelectWidth, SIZE_TO_CONTENT] clipChildren = true},
      })
    ]
  }
  return {selectBtn, openSelect}
})

return {mkSelect}

