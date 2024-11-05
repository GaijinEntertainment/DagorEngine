from "%darg/ui_imports.nut" import *
from "%darg/laconic.nut" import *

let {
  TaskHttpGet
//  ,TaskHttpMultiGet
} = require("%sqstd/asyncHttp.nut")
let Promise = require("%sqstd/Promise.nut")

function btn(text, handler){
  return comp(Padding(hdpx(10)), RendObj(ROBJ_SOLID), Colr(60,60,60), comp(Button, OnClick(handler), txt(text)))
}

let consoleStream = Watched([""])
const MAX_ITEMS = 20

function conlog(...){
  let v = consoleStream.value.slice(-(MAX_ITEMS-vargv.len()))
  v.extend(vargv)
  consoleStream(v)
}

function checkPromises(){
  Promise(@(resolve, _reject)
    gui_scene.setTimeout(0.2, @() resolve("Promise resolved first one"))
  ).then(function(res){
      conlog(res)
      return Promise(@(resolve, _)
          gui_scene.setTimeout(0.2, @() resolve("Promise resolved second one"))
      )
  }).then(@(res) conlog(res) )
}

function checkObservables(){
  let eventStream = Watched(-1)
  local i = 0
  let isSuccessful = @(v) v < 5
  eventStream.subscribe(function(v) {
    if (isSuccessful(v)) {
      i = i+1
      gui_scene.setTimeout(0.1, @() eventStream(i))
      conlog($"observable chain, resolved {v}")
    }
    else
      conlog($"observable chain, failed {v}")
  })
  gui_scene.setTimeout(0.1, @() eventStream(i))
}

let task = TaskHttpGet("https://gaijin.net").map(@(v) conlog(v.as_string()) ?? conlog("submit new request")).flatMap(@(_) TaskHttpGet("http://ya.ru"))
function checkHttpTask(){
  conlog("submitted requested")
  task.exec(@(...) conlog($"error {" ".join(vargv)}"), function(v) {
    conlog($"success {v.as_string()}")
  })
}

/* Super solution
  //Idea: we create declarative executaion order, that will check correctness of access of results
  // and launch when required results ready
  let storage = mkWatched(persist, "storage"{})
  let loginActions = mkAsyncTasks(
    storage
    [ //array means that we should finish all this tasks before going next
       { id = "Ya", httpGetUrl = "http://ya.ru"}
       { id = "Google", httpGetUrl = "http://google.ru"}
    ]
    @() dlog(2) //just function means it cant be referenced and just would be called
    {
      id = "Timer1"
      repeat = 4
      timeoutMsec = 3000
      action = function(setError, setResult, Google, Ya){ //name is use to reference results. alternatively we can provide table of results, but this is errorprone
        gui_scene.setTimeout(Google.as_string.len()%7 + Ya.as_string.len()%13, function() {
          if (rand()%2==0)
            setResult("time is odd")
          else {
            setError("not good luck")
          }
        }
      )}
    }
  )

  loginActions.exec()
*/

let console = @() {
  watch = consoleStream vplace = ALIGN_BOTTOM rendObj = ROBJ_SOLID size = [sw(30), flex()] clipChildren = true color = Color(10,10,10) valign = ALIGN_BOTTOM
  children = vflow(consoleStream.value.map(txt), Gap(hdpx(2)))
}
return {
  flow = FLOW_HORIZONTAL
  gap = hdpx(50)
  padding = hdpx(10)
  children = [
    vflow(
      Gap(hdpx(2))
      btn("check promise", checkPromises)
      btn("check HttpTask monad", checkHttpTask)
      btn("check observables", checkObservables)
    )
    console
  ]
  size = [sh(50), sh(50)]
  rendObj=ROBJ_SOLID
  color =Color(30,30,30)
}