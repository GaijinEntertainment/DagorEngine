from "%darg/ui_imports.nut" import *


let everySecond = Watched(0)
let evenSecond = Computed(@() everySecond.get() & ~1)
let oddSecond = Computed(@() everySecond.get() | 1)
let fourthSecond = Computed(@() everySecond.get() >> 2)

let dataTable = {
  every = everySecond
  even = evenSecond
  odd = oddSecond
  fourth = fourthSecond
}

let dataArray = [
  everySecond
  evenSecond
  oddSecond
  fourthSecond
]


// this must work

let okTable = dataTable.even
let okTableSecond = Computed(@() okTable.get())

let okArray = dataArray[1]
let okArraySecond = Computed(@() okArray.get())

let okAccessSecond = Computed(@()
  [everySecond.get(), evenSecond.get(), oddSecond.get()][fourthSecond.get() % 3])


// but this must not

let dummy = Watched(true) // next computeds will not compile without reference to real observable
let noTableSecond = Computed(@() dummy.get() && dataTable.odd.get())
let noArraySecond = Computed(@() dummy.get() && dataArray[2].get())

let noAccess = [everySecond, evenSecond, oddSecond]
let noAccessSecond = Computed(@() noAccess[fourthSecond.get() % 3].get())


gui_scene.setInterval(1.0, @() everySecond.set(everySecond.get() + 1))

let mkHeader = @(text) {
  rendObj = ROBJ_FRAME
  padding = static [3, 5]
  borderWidth = static [0, 0, 1, 0]
  children = {
    rendObj = ROBJ_TEXT
    text
  }
}

let mkObservable = @(label, observe) @() {
  watch = observe
  rendObj = ROBJ_TEXT
  text = $"{label} : {observe.get()}"
  padding = static [3, 5]
}

return {
  hplace = ALIGN_CENTER
  vplace = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = 10
  children = [
    mkHeader("Basic values")
    mkObservable("Second", everySecond)
    mkObservable("Even", evenSecond)
    mkObservable("Odd", oddSecond)
    mkObservable("Fourth", fourthSecond)

    mkHeader("Exact update")
    mkObservable("Table", okTableSecond)
    mkObservable("Table direct", dataTable.even)
    mkObservable("Array", okArraySecond)
    mkObservable("Array direct", dataArray[1])
    mkObservable("Access", okAccessSecond)

    mkHeader("Incorrect update")
    mkObservable("Table", noTableSecond)
    mkObservable("Array", noArraySecond)
    mkObservable("Access", noAccessSecond)
  ]
}