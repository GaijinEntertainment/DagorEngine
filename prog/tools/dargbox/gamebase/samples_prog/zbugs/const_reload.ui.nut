from "%darg/ui_imports.nut" import *
from "console" import command

let ticker = Watched(0)
gui_scene.setInterval(1, @() ticker.set(ticker.get() + 1))

local index = 0

let mkText = @(cond, val) {
  rendObj = ROBJ_TEXT
  text = $"{++index} - {cond ? "OK" : "FAILED"} {val}"
  color = cond ? 0xFFFFFF : 0xFF0000
}

let function mkButton(label, action) {
  let sf = Watched(0)
  return @() {
    rendObj = ROBJ_BOX
    watch = sf
    padding = 10
    behavior = Behaviors.Button
    borderColor = 0xFFFFFF
    fillColor = sf.get() & S_ACTIVE ? 0x556677
      : sf.get()  & S_HOVER ? 0x334455
      : 0x112233
    onClick = action
    onElemState = @(s) sf.set(s)
    children = {
      rendObj = ROBJ_TEXT
      text = label
    }
  }
}

let buttons = {
  flow = FLOW_HORIZONTAL
  gap = 10
  children = [
    mkButton("Soft reload", @() command("scripts.soft_reload"))
    mkButton("Hard reload", @() command("scripts.hard_reload"))
  ]
}

let strConst = "string"
let strArray = static ["string"]
let strTable = static { string = 1 }

/**
 How to use:
  1. load example into dargbox
  2. made little mistake in the code and reload it at the dargbox
  3. revert error and reload script at the dargbox again
  4. anonymous structures and local variables constants will perform errors
  5. after exposing error first time it will not reproduce again, dargbox restart needed
 */
return function() {
  let varConst = "string"
  let varArray = static ["string"]
  let varTable = static { string = 1 }

  index = 0
  return {
    watch = ticker
    hplace = ALIGN_CENTER
    vplace = ALIGN_CENTER
    flow = FLOW_VERTICAL
    gap = 10
    children = [
        { rendObj = ROBJ_TEXT, text = $"Component updates: {ticker.get()}" }

        mkText(typeof "" == "string", "string")
        mkText((static ["string"]).contains(typeof ""), static ["string"])
        mkText(typeof "" in static { string = 1 }, static { string = 1 })

        mkText(typeof "" == varConst, varConst)
        mkText(varArray.contains(typeof ""), varArray)
        mkText(typeof "" in varTable, varTable)

        mkText(typeof "" == strConst, strConst)
        mkText(strArray.contains(typeof ""), strArray)
        mkText(typeof "" in strTable, strTable)

        buttons
    ]
  }
}
