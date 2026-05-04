from "%darg/ui_imports.nut" import *

/*
TODO:
  transtions
  scrollbar+pannable
  stdtabs
*/

let cursors = require("samples_prog/_cursors.nut")

function text(s, params={}){
  return {}.__update(params).__update({rendObj=ROBJ_TEXT, text=s ?? ""})
}

let anims = [
  { prop=AnimProp.opacity, from=0, to=1, duration=0.55, play=true, easing=OutCubic}
  { prop=AnimProp.translate, from=[0,-sh(20)], to=[0,0], duration=0.35, play=true, easing=OutQuart}

  { prop=AnimProp.opacity, from=1, to=0, duration=0.55, playFadeOut=true, easing=OutCubic}
  { prop=AnimProp.translate, from=[0,0], to=[0,sh(20)], duration=0.35, playFadeOut=true, easing=OutQuart}
]

let tab1 = {
  animations = anims key="tab1" size=flex() transform={pivot=[0.5,0.5]} flow=FLOW_VERTICAL gap=sh(1) padding=sh(1) children = [text("First tab"), text("2"), text("3")]
}
let tab2 = {
  animations = anims key="tab2" size=flex() transform={pivot=[0.5,0.5]} flow=FLOW_VERTICAL gap=sh(1) padding=sh(3) children = [text("Second tab content 21"), text("22"), text("23"), text("33")]
}

let tabs = [{title="tab1" content = tab1}, {content = tab2 title="tab2"}]

let current_tab = mkWatched(persist, "current_tab", tabs?[0])

function create_caption_button(tab) {
  let stateFlags = Watched(0)
  return function() {
    let sf = stateFlags.get()
    return {
      watch = [stateFlags, current_tab]
      onElemState = @(s) stateFlags.set(s)
      rendObj = ROBJ_BOX
      fillColor = (sf & S_HOVER) ? Color(20,80,80,100) : Color(0,0,0,0)
      borderWidth = 0
      size = FLEX_V
      halign = ALIGN_CENTER
      valign =ALIGN_CENTER
      padding = sh(1)
      children = {rendObj=ROBJ_TEXT text=tab?.title ?? "untitled" color = current_tab.get()?.title == tab?.title ? Color(255,255,255) : Color(180,180,180)}
      behavior = [Behaviors.Button]
      onClick = @() current_tab.set(tab)
    }
  }
}
function tabs_header() {
  return {
    rendObj = ROBJ_SOLID
    color=Color(80,120,120)
    size = const [flex(),sh(5)]
    flow=FLOW_HORIZONTAL
    valign = ALIGN_CENTER
    padding=sh(0.1)
    gap={size=[sh(2*100./1080),ph(50)] rendObj=ROBJ_SOLID color=Color(0,0,0,50)}
    children = tabs.map(@(tab) create_caption_button(tab))
    clipChildren=true
  }
}

function tabs_component (){
  return {
    watch = current_tab
    size = const [sh(30),sh(20)]
    rendObj=ROBJ_SOLID
    color = Color(70,70,70)
    flow=FLOW_VERTICAL
    gap=sh(1)
    children = [tabs_header, {size=flex() clipChildren=true children = current_tab.get()?.content}]
    animations = anims
  }
}

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  gap = sh(2)
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  children = [
    text("Tabs are very simple - captions are buttons that change one state value - current tab selected. Tab is component and caption")
    text("Tab body switch childs depends on current tab value")
    tabs_component
  ]
}

