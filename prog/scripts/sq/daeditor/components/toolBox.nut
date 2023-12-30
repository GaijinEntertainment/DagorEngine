from "%darg/ui_imports.nut" import *
from "%darg/laconic.nut" import *

let cursors = require("%daeditor/components/cursors.nut")
let textButton = require("%daeditor/components/textButton.nut")


let buttonStyle    = { boxStyle  = { normal = { fillColor = Color(0,0,0,120) } } }
let buttonStyleOn  = { textStyle = { normal = { color = Color(0,0,0,255) } }
                       boxStyle  = { normal = { fillColor = Color(255,255,255) }, hover = { fillColor = Color(245,250,255) } } }
let buttonStyleOff = { off = true,
                       textStyle = { normal = { color = Color(120,120,120,255) }, hover = { color = Color(120,120,120,255) } },
                       boxStyle  = { normal = { fillColor = Color(0,0,0,120)}, hover = {fillColor = Color(0,0,0,120) } } }

let panelW = hdpx(230)
let rowDiv = { size = [panelW, hdpx(5)] }
let tooltipDX = -hdpx(20)

function clearToolboxOptions(tb_shown, tb_state) {
  tb_state.options = []
  tb_shown.trigger()
}

function addToolboxOption(tb_shown, tb_state, opt_enabled, opt_name, opt_click, opt_content, opt_tooltip) {
  tb_state.options.append({
    on = opt_enabled,
    name = opt_name,
    click = opt_click,
    content = opt_content,
    tooltip = opt_tooltip
  })
  tb_shown.trigger()
}

function addToolboxNextRow(tb_state) {
  if (tb_state.options.len() % 2 == 0)
    return
  tb_state.options.append(null)
}

function clickToolboxOption(opt, opt_near, trig) {
  let opt_on = opt?.on ? opt.on() : false
  if (opt.click) {
    opt.click(!opt_on)
  }
  if (opt?.content != null && opt_near?.content != null) {
    let near_on = opt_near?.on ? opt_near.on() : false
    if (!opt_on && near_on && opt_near.click)
      opt_near.click(false)
  }
  trig.trigger()
}

function mkOption(opt, opt_near, trig, tb_state) {
  let restyle = opt.name?.style
  let name = typeof opt.name == "function" ? opt.name() : opt.name?.text ? opt.name.text : opt.name
  let func = @() clickToolboxOption(opt, opt_near, trig)
  let opt_on = opt?.on ? opt.on() : false
  let style = restyle != null ? restyle : (opt_on ? buttonStyleOn : buttonStyle)
  return textButton(name, func, style.__merge({
    onHover = function(on) {
      tb_state.tooltipElem = on ? opt : null
      tb_state.tooltipShow(on)
    }
  }))
}

let mkToolboxTooltip = function(tt_obj, tt_text, dx, tb_state) {
  if (typeof tt_text != "string")
    return null
  return @() {
    size = [0,0]
    watch = [tb_state.tooltipShow]
    children = tb_state.tooltipShow.value && tb_state.tooltipElem == tt_obj ? {
      pos = [dx, 0]
      hplace = ALIGN_RIGHT

      rendObj = ROBJ_BOX
      fillColor = Color(30, 30, 30, 160)
      borderColor = Color(50, 50, 50, 20)
      size = SIZE_TO_CONTENT
      borderWidth = hdpx(1)
      padding = fsh(1)

      children = {
        rendObj = ROBJ_TEXTAREA
        behavior = Behaviors.TextArea
        maxWidth = hdpx(500)
        text = tt_text
        color = Color(180, 180, 180, 120)
      }
    } : null
  }
}

let mkOptionsRow = @(opts, content, tooltip1, tooltip2) {
  size = [panelW, SIZE_TO_CONTENT]
  children = [
    tooltip1
    tooltip2
    {
      flow = FLOW_VERTICAL
      children = [
        opts
        content != null ? { flow = FLOW_VERTICAL, children = content } : null
      ]
    }
  ]
}

function mkOptions(opt1, opt2, trig, tb_state) {
  let show1 = opt1?.content != null && opt1?.on ? opt1.on() : false
  let tooltip1 = mkToolboxTooltip(opt1, opt1.tooltip, tooltipDX, tb_state)
  if (opt2 == null) {
    let opts = { flow = FLOW_HORIZONTAL, children = mkOption(opt1, null, trig, tb_state) }
    return mkOptionsRow(opts, show1 ? opt1.content : null, tooltip1, null)
  }
  let childs = [ mkOption(opt1, opt2, trig, tb_state),
                 mkOption(opt2, opt1, trig, tb_state) ]
  let opts = { flow = FLOW_HORIZONTAL, children = childs }
  let show2 = opt2?.content != null && opt2?.on ? opt2.on() : false
  let tooltip2 = mkToolboxTooltip(opt2, opt2.tooltip, tooltipDX, tb_state)
  return mkOptionsRow(opts, show1 ? opt1.content : show2 ? opt2.content : null, tooltip1, tooltip2)
}

function setToolboxPos(tb_state, px, py) {
  tb_state.px = px
  tb_state.py = py
}

function mkToolboxPanelContent(tb_shown, tb_state) {
  let trig = tb_shown
  return @() {
    pos = [tb_state.px, tb_state.py]
    hplace = ALIGN_RIGHT
    vplace = ALIGN_TOP
    size = [panelW, SIZE_TO_CONTENT]

    cursor = cursors.normal

    children = [
      {
        behavior = Behaviors.Button

        rendObj = ROBJ_BOX
        borderWidth = hdpx(1)
        borderRadius = hdpx(7)
        borderColor = Color(120,120,120, 160)
        fillColor = Color(40,40,40, 160)

        size = [panelW, SIZE_TO_CONTENT]
        padding = hdpx(10)

        cursor = cursors.normal
        hotkeys = [["Esc", @() tb_shown(false)]]

        flow = FLOW_VERTICAL
        children = [
          function() {
            local childs = []
            let numOptions = tb_state.options.len()
            for (local i = 0; i < numOptions; i += 2) {
              if (i > 0)
                childs.append(rowDiv)
              let opt1 = tb_state.options[i]
              let opt2 = tb_state.options?[i+1]
              childs.append(mkOptions(opt1, opt2, trig, tb_state));
            }
            return { flow = FLOW_VERTICAL, watch = [trig], children = childs }
          }
        ]
      }
    ]
  }
}

function mkToolboxButton(tb_state, text, func) {
  local button = null
  button = textButton(text, func, buttonStyle.__merge({
    onHover = function(on) {
      tb_state.tooltipElem = on ? button : null
      tb_state.tooltipShow(on)
    }
  }))
  return button
}

function mkToolbox(tb_shown) {
  local toolBoxState = {
    px = 0
    py = 0
    options = []
    tooltipShow = Watched(false)
    tooltipElem = null
  }
  return {
    clearOptions =  @() clearToolboxOptions(tb_shown, toolBoxState)
    addOption = @(on, name, click, content, tooltip) addToolboxOption(tb_shown, toolBoxState, on, name, click, content, tooltip)
    addNextRow = @() addToolboxNextRow(toolBoxState)

    redraw = @() tb_shown.trigger()
    setPos = @(px, py) setToolboxPos(toolBoxState, px, py)
    panel = mkToolboxPanelContent(tb_shown, toolBoxState)

    mkButton = @(text, func) mkToolboxButton(toolBoxState, text, func)
    mkTooltip = @(obj, text, dx) mkToolboxTooltip(obj, text, dx, toolBoxState)

    buttonStyle
    buttonStyleOn
    buttonStyleOff
    rowDiv
  }
}

return mkToolbox
