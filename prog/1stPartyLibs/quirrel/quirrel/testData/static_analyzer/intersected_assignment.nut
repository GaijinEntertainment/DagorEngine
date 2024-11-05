local curAction = null


function foo() {}

let func = foo()
let leading = foo()

function _throttled(...){
  let doWait = curAction != null
  curAction = @() func.acall([null].extend(vargv))
  if (doWait) {
    return
  }
  if (leading) {
    curAction()
  }
}