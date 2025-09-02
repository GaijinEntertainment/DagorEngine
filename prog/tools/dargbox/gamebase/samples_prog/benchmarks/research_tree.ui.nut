from "%darg/ui_imports.nut" import *
import "dagor.profiler" as profiler
from "datetime" import clock

let cursors = require("samples_prog/_cursors.nut")

let {makeVertScroll} = require("samples_prog/_basic/components/scrollbar.nut")
let units = freeze({
  ["a-20g"] = Picture("!ui/atlas#a-20g")
})

let atlas = freeze({
  spades = Picture("!ui/atlas#hud_tank_arrow_right_02.svg")
})

let Text = kwarg(function(text, color=null,
        //common component arguments
        padding = null,
        margin = null,
        size = SIZE_TO_CONTENT,
//        children = null,
        hplace=null,
        vplace=null,
        pos =null,
        animations = null,
        transform = null
    ){
    return {
      rendObj = ROBJ_TEXT
      text
      padding
      margin
      color
      hplace
      vplace
      size
      pos
      transform
      animations
    }
  }
)
let Image = kwarg(function(image, color=null, keepAspect = null,
        //common component arguments
        padding = null,
        margin = null,
        size = SIZE_TO_CONTENT,
        hplace=null,
        pos =null,
        vplace=null,
        transform = null,
        animations = null,
    ){
    return {
      rendObj = ROBJ_IMAGE
      image
      color
      keepAspect
      padding
      margin
      hplace
      vplace
      size
      pos
      transform
      animations
    }
  }
)
let Container = kwarg(function(
  padding = null,
  margin = null,
  size = SIZE_TO_CONTENT,
  hplace=null,
  vplace=null,
  halign=null,
  valign = null,
  flow = null,
  pos = null,
  gap = null,
  transform = null,
  animations = null,
  key = null,
  children = null
){
  return {
    hplace
    vplace
    halign
    valign
    size
    children
    flow
    padding
    gap
    margin
    pos
    transform
    animations
    key
  }
})

let Box = kwarg(function(
  padding = null,
  borderColor = null,
  fillColor = null,
  borderWidth = 0,
  borderRadius = 0,
  margin = null,
  size = SIZE_TO_CONTENT,
  hplace=null,
  vplace=null,
  halign=null,
  valign = null,
  flow = null,
  children = null,
  gap = null,
  transform = null,
  animations = null,
  key = null,
  behavior = null,
  pos = null
){
  return {
    borderColor
    fillColor
    borderWidth
    borderRadius
    hplace
    vplace
    halign
    valign
    size
    children
    flow
    gap
    padding
    margin
    pos
    transform
    animations
    key
    behavior
    rendObj = ROBJ_BOX
  }
})

function mkResearchItem(idx) {
  return Box({
    size = static [sh(15),sh(5)]
    margin=static [0,hdpx(10),hdpx(20),hdpx(10)]
    borderColor = Color(20,20,20)
    borderWidth = 2
    children = [
      Box(static {size=[hdpx(30),hdpx(10)] vplace=ALIGN_BOTTOM pos=[hdpx(10),hdpx(10)] fillColor = Color(10,100,10)})//crew button
      Box({ size = flex()
        fillColor = Color(30,30,30)
        children = [
          Container({ //images
            size = flex()
            children = [
              Image(static {size = flex(), image=units["a-20g"]})
              Image({
                size = hdpx(40)
                hplace = ALIGN_RIGHT
                color = Color(0,0,0)
                image=atlas.spades
              })
            ]
          })
          Container({  //plane name
            flow = FLOW_VERTICAL
            size = flex()
            padding=static [hdpx(4),hdpx(4),0,0]
            valign=ALIGN_CENTER

            children = [
              Text({text=idx hplace = ALIGN_RIGHT})//plane_name
              { size = flex()
                flow = FLOW_HORIZONTAL
                halign = ALIGN_RIGHT
                gap = hdpx(4)
                children = [
                  Text({text=100*idx })//price or research_progress
                  static {size = [hdpx(5),flex(1)]}
                  Text({text="1.3" })//BR
                  Text({text="*"})//type
                ]
              } //BR+type
            ]
          })

        ]
      })
      Box(static {size=hdpx(10) hplace = ALIGN_CENTER vplace=ALIGN_TOP pos=[hdpx(10),0] fillColor = Color(120,0,0)})//crew button
    ]
  })
}

function researchLine(_,r) {
  return Box({
    flow = FLOW_HORIZONTAL
    halign = ALIGN_CENTER
    size = static [flex(),SIZE_TO_CONTENT]
    borderWidth = hdpx(1)
    borderColor = Color(5,25,5)
    children = array(9).map(@(_, i) mkResearchItem(i+r*100))
  })
}

function researchTree() {
  return Container({
    flow = FLOW_VERTICAL
    gap = hdpx(10)
    size = static [sw(80),SIZE_TO_CONTENT]
    children = array(20).map(researchLine)
    transform = static {
      pivot = [0, 1]
    }
    animations = static [
      { prop=AnimProp.translate, to=[0,0], from=[0, 500], duration=0.5, play=true, easing=OutCubic }
      { prop=AnimProp.scale, from=[0.01,1], to=[1,1], duration=0.5, play=true, easing=OutCubic }
      { prop=AnimProp.scale, from=[1,1], to=[0.01,1], duration=0.5, playFadeOut=true, easing=OutCubic }
    ]
  })
}

function researchTreeBlock(key) {
  return Container({
    size = static [sw(80),sh(80)]
    hplace = ALIGN_CENTER
    vplace = ALIGN_CENTER
    key
    children = makeVertScroll(researchTree)
  })
}

let generation = Watched(0)
function updateGen(){
  generation.modify(@(v)v+1)
  if (generation.get() <= 500) {
    gui_scene.resetTimeout(0.02, updateGen)
  }
  else{
    generation.set(0)
    profiler.stop()
    dlog("profiler stopped")
  }
}
let start = function start(){
  dlog("profiler started")
  profiler.start()
  updateGen()
}
let btn = {
  behavior = Behaviors.Button
  onClick = start
  children = Box({
    fillColor = Color(200,200,200)
    padding = hdpx(10)
    children = Text({text = "start measure"})
  })
}

return @() {
  children = [
    btn
    researchTreeBlock(generation.get())
  ]
  watch = generation
  size = flex()
  cursor = cursors.active
}
