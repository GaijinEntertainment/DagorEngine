from "%sqstd/asyncHttp.nut" import TaskHttpGet, TaskHttpMultiGet
from "%darg/ui_imports.nut" import *
from "%darg/laconic.nut" import *

let Promise = require("%sqstd/Promise.nut")
let { Task } = require("%sqstd/monads.nut")

function btn(text, handler){
  return comp(Padding(hdpx(10)), RendObj(ROBJ_SOLID), Colr(60,60,60), comp(Button, OnClick(handler), txt(text)))
}

let consoleStream = Watched([""])
const MAX_ITEMS = 20

function conlog(...){
  let v = consoleStream.get().slice(-(MAX_ITEMS-vargv.len()))
  v.extend(vargv)
  consoleStream.set(v)
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
      gui_scene.setTimeout(0.1, @() eventStream.set(i))
      conlog($"observable chain, resolved {v}")
    }
    else
      conlog($"observable chain, failed {v}")
  })
  gui_scene.setTimeout(0.1, @() eventStream.set(i))
}

let task = TaskHttpGet("https://gaijin.net").map(@(v) conlog(v.as_string().slice(0,100)) ?? conlog("submit new request")).flatMap(@(_) TaskHttpGet("http://ya.ru"))
function checkHttpTask(){
  conlog("submitted requested")
  task.exec(@(...) conlog($"error {" ".join(vargv)}"), function(v) {
    conlog($"success {v.as_string().slice(0,100)}")
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
  watch = consoleStream vplace = ALIGN_BOTTOM rendObj = ROBJ_SOLID size = static [sw(30), flex()] clipChildren = true color = Color(10,10,10) valign = ALIGN_BOTTOM
  children = vflow(consoleStream.get().map(txt), Gap(hdpx(2)))
}
let setTimeout = @(func, timeout) gui_scene.setTimeout(timeout, func)
// Monad Task

// Example 1: Create a Task that waits and returns "hello"
let waitAndSayHelloTask = Task(@(_err, ok) setTimeout(@() ok("hello"), 1))

// Example 2: Wrap a value in a Task
let helloTask = Task.of("hello")

// Example 3: Run a Task
let helloTaskBtn = btn("helloTask", @() helloTask.exec(
    @(err) conlog($"Error: {err}"),
    @(result) conlog($"Result: {result}")
))

// Example 4: Transform result with map
let upperTask = helloTask.map(@(x) x.toupper())
let upperTaskBtn = btn("upperTask", @() upperTask.exec(
    @(err) conlog($"Error: {err}"),
    @(result) conlog($"Result: {result}")
))

// Example 5: Chain async Tasks with flatMap
function nextTaskAfterHello(str) {
  return Task(@(_err, ok) setTimeout(@() ok(str.concat(" world!")), 1))
}

let waitAndSayHelloTaskNext = waitAndSayHelloTask.flatMap(nextTaskAfterHello)

let waitAndSayHelloBtn = btn("waitAndSayHello", @() waitAndSayHelloTaskNext.exec(
    @(err) conlog($"Error: {err}"),
    @(result) conlog($"Result: {result}")
  )
)

// Example 6: Handle errors
let errorTask = Task(@(err, _ok) err("Something went wrong"))
let errorTaskBtn = btn("errorTask", @() errorTask.exec(
    @(err) conlog($"Error: {err}"),
    @(result) conlog($"Result: {result}")
))


// Example 7: Compose and chain
let t = Task.of(5)
    .map(@(x) x * 2)
    .flatMap(@(x) Task.of(x + 3))
    .flatMap(@(x) Task(@(_err, ok) setTimeout(@() ok(x + 100), 0.5)))
    .map(@(x) "".concat($"Result is: ", x))

let composeAndChainBtn = btn("composeAndChainBtn 5+3+100", @() t.exec(
    @(err) conlog($"Error: {err}"),
    @(result) conlog(result)
))

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
      helloTaskBtn
      upperTaskBtn
      waitAndSayHelloBtn
      errorTaskBtn
      composeAndChainBtn
    )
    console
  ]
  size = sh(50)
  rendObj=ROBJ_SOLID
  color =Color(30,30,30)
}