// Cycle / leak guard. The caller retains the faulted task-future in a
// table. An earlier strong-addref-of-caller-closure design would have
// formed a GC-uncollectable cycle here, leaking the future until
// sq_close. FutureImpl holds only leaf SQStrings, so there is no
// closure/proto object edge and thus no cycle: the diagnostic still
// fires exactly once through the VM's error handler at the pump tick
// where the throw settles, even though the future itself stays alive
// (held by `held`) until sq_close.

let held = {}

async function failing() {
  throw "cycle"
}

function launcher() {
  held.p <- failing()  // caller retains the task-future
}

launcher()
print("script done\n")
