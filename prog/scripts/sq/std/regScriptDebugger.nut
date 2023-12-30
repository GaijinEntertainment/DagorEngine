let sqdebugger = require_optional("sqdebugger")

function do_register(printfn = null) {
  if (sqdebugger == null)
    return

  if (printfn == null) {
    let logFn = require("log.nut")
    printfn = logFn().debugTableData
  }

  sqdebugger?.setObjPrintFunc(printfn)
}

return do_register
