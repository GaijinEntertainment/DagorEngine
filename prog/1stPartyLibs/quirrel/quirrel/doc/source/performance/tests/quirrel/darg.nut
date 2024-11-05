local rand
local RAND_MAX
try {
  rand = getroottable()["rand"]
  RAND_MAX = getroottable()["RAND_MAX"]
} catch(e){
  local math = require("math")
  rand = math.rand
  RAND_MAX = math.RAND_MAX
}

const CompsToUpdateEachFrame = 30
const ROBJ_TEXT = 0
const ROBJ_FRAME= 1
const ROBJ_SOLID= 2
const ALIGN_CENTER = 0
enum Behaviors {
  Button = 0
}
const FLOW_VERTICAL = 0

class Watched {
  value = null
  constructor(value=null) {
    this.value = value
  }
  get = function(){
    return this.value
  }
}

function rint(max){
  return (rand()/RAND_MAX*max).tointeger()
}
const screenWidth = 1920.0
const screenHeight= 1080.0
function sw(val){
  return (val/100.0*screenWidth).tointeger()
}
function sh(val){
  return (val/100.0*screenHeight).tointeger()
}
function Color(r,g,b,a=255){
  return (a.tointeger() << 24) + (r.tointeger() << 16) + (g.tointeger() << 8) + b.tointeger()
}
function hdpx(val){
  return val/sh(100)*screenHeight
}
local  defflex = {}
function flex(val=null){
  return val == null ? defflex : {val=val}
}



const componentsNum = 2000 //amount of components
const borders = true //show borders on each element

function simpleComponent(i, watch){
  local  pos = [sw(rand()*80/RAND_MAX), sh(rand()*80/RAND_MAX)]
  local  size = [sh(rand()*15/RAND_MAX+2), sh(rand()*15/RAND_MAX+2)]
  local  color = Color(rand()*255/RAND_MAX, rand()*255/RAND_MAX, rand()*255/RAND_MAX)
  local  textCanBePlaced = size[0] > hdpx(150)
  local  addText = rint(5)==1
  local  text = textCanBePlaced ? function() {
      return {
        watch=watch rendObj = ROBJ_TEXT
        text = addText || !watch.get() ? ("" + i + watch.get()) : null
      }
    } : null
  local  children = []

  if (borders) {
    children.append(function() {
                 return {
                   borderWidth = [1,1,1,1]
                   rendObj=ROBJ_FRAME size=flex()
                 }
               })
    children.append(text)
  }
  return function() {
    return {
      rendObj = ROBJ_SOLID
      size = size
      key = {}
      pos = pos
      valign = ALIGN_CENTER
      halign = ALIGN_CENTER
      padding = [hdpx(4), hdpx(5)]
      margin = [hdpx(5), hdpx(3)]
      color = watch.get() ? color : Color(0,0,0)
      behavior = Behaviors.Button
      onClick = @() watch.modify(@(v) !v)
      watch = watch
      children = children
    }
  }
}

local num = 0
local children = []
for (local i=0; i<componentsNum; ++i) {
  children.append(simpleComponent(i, Watched(i%3==0)))
}

function benchmark() {
  return {
    size = flex()
    flow = FLOW_VERTICAL
    children = children
  }
}

function testUi(entry){
  if (entry==null)
    return true
  if (type(entry)=="function")
    entry=entry()
  local  t = type(entry)
  if (t=="table" || t=="class") {
    if (!("children" in entry))
      return true
    if ("array"==type(entry.children)) {
      foreach (child in entry.children) {
        if (!testUi(child))
          return false
      }
      return true
    }
    return testUi(entry.children)
  }
  return false
}

const Frames = 100 //offline only
local testsNum = 20 //offline only

local profile_it
try {
  profile_it = getroottable()["loadfile"]("profile.nut")()
  if (profile_it == null)
    throw "no loadfile"
} catch(e) profile_it = require("profile.nut")

function testf() {
  local frames = Frames
  while(--frames) {
    testUi(benchmark)
  }
}
print("\"profile darg ui\", "+profile_it(testsNum, testf) + ", "+ testsNum+"\n")
