from "%darg/ui_imports.nut" import *
import "math" as math
import "string" as string
import "%sqstd/rand.nut" as Rand
from "dagor.workcycle" import setTimeout
from "%sqstd/underscore.nut" import arrayByRows
let profiler = require("dagor.profiler")
let {exit, get_arg_value_by_name} = require("dagor.system")
let {clock} = require("datetime")
let cursors = require("samples_prog/_cursors.nut")

let rand = Rand()
let isDargStub = type(KEEP_ASPECT_FILL) == "string"

const componentsNum = 2000 //amount of components
const borders = true //show borders on each element

const Frames = 200 //offline only
local testsNum = 5 //offline only
let useFlow = !isDargStub //darg only
const CompsToUpdateEachFrame = 30 //darg only




function simpleComponent(i, watch){
  let pos = useFlow ? null : [sw(math.rand()*80/math.RAND_MAX), sh(math.rand()*80/math.RAND_MAX)]
  let size = [sh(math.rand()*15/math.RAND_MAX+2), sh(math.rand()*15/math.RAND_MAX+2) / (useFlow ? 10 : 1)]
  let color = Color(math.rand()*255/math.RAND_MAX, math.rand()*255/math.RAND_MAX, math.rand()*255/math.RAND_MAX)
  let textCanBePlaced = size[0] > hdpx(150)
  let addText = rand.rint(0, 5)==1
  let text = textCanBePlaced ? function textComp() { return {watch rendObj = ROBJ_TEXT text = addText || !watch.get() ? $"{i}: {watch.get()}" : null}} : null
  let children = []

  if (borders) {
    children.append(function bordersComp() {
                 return {
                   borderWidth = [1,1,1,1]
                   rendObj=ROBJ_FRAME size=flex()
                 }
               }, text)
  }
  let onClick = @() watch.modify(@(v) !v)
  return function comp() {
    return {
      rendObj = ROBJ_SOLID
      size
      key = {}
      pos
      valign = ALIGN_CENTER
      halign = ALIGN_CENTER
      padding = [hdpx(4), hdpx(5)]
      margin = [hdpx(5), hdpx(3)]
      color = watch.get() ? color : Color(0,0,0)
      behavior = Behaviors.Button
      onClick
      watch
      children
    }
  }
}
let watches = []
local num = 0

let comps = []

for (local i=0; i<componentsNum; ++i) {
  let watch = Watched(rand.rint(0, 5) == 1)
  watches.append(watch)
  num++
  comps.append(simpleComponent(i, watch))
}
let rows = []
let rowToHideWatch = Watched(null)
foreach (i, row in arrayByRows(comps, 55)) {
  let k = i
  let r = row
  let col = function(){
    return {key = {} flow = FLOW_HORIZONTAL children = rowToHideWatch.get()!=k ? r : null watch = rowToHideWatch}
  }
  rows.append(col)
}

function benchmark() {
  return {
    size = flex()
    flow = FLOW_VERTICAL
    children = rows
    cursor = cursors.active
  }
}

if (__name__ == "__main__" && isDargStub) {
  let doTest = get_arg_value_by_name("test") != null
  function testUi(entry){
    if (entry==null)
      return true
    if (type(entry)=="function")
      entry=entry()
    let t = type(entry)
    if (t=="table" || t=="class") {
//      if (entry?.rendObj == ROBJ_TEXT && entry?.text)
//        println(entry?.text)
      if ("children" not in entry)
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

  function profile_it(cnt, f) {
    local res = 0
    for (local i = 0; i < cnt; ++i) {
      local start = clock()
      f()
      local measured = clock() - start
      if (i == 0 || measured < res)
        res = measured;
    }
    return res / 1.0
  }

  let testf = function() {
    local frames = Frames
    while(--frames) {
      testUi(benchmark)
    }
  }
  if (doTest)
    println($"profile, tests count:{testsNum}, fastest: {profile_it(testsNum, testf)}")
  else {
    println($"to run benchmark locally add '--test' to commandline")
    testUi(benchmark)
  }
}

if (!isDargStub) {
  gui_scene.setUpdateHandler(function updateHandler(_) {
    array(rand.rint(0, CompsToUpdateEachFrame)).apply(@(_) watches[rand.rint(0, num-1)].modify(@(v) !v))
    rowToHideWatch.set(rand.rint(0, rows.len()-1))
  })
  profiler.start()
  let timeToMeasure = 5
  setTimeout(timeToMeasure, function() {
    profiler.stop()
    dlog("profiler stopped")
  })

  if (get_arg_value_by_name("exit_time") != null)
    setTimeout(get_arg_value_by_name("exit_time").tofloat(), @() exit(0))
}
return benchmark
