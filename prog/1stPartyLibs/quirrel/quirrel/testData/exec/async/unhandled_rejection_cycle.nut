from "async" import Promise

// Cycle / leak guard. The caller retains the rejected task-promise in a
// table. The earlier rejected design (strong addref of the caller
// closure) would have formed a GC-uncollectable cycle here, leaking the
// promise until sq_close. PromiseImpl now holds only leaf SQStrings, so
// there is no closure/proto object edge and thus no cycle: the promise
// is still collected and the diagnostic still fires exactly once.

let held = {}

async function failing() {
  throw "cycle"
}

function launcher() {
  held.p <- failing()  // caller retains the task-promise
}

launcher()
print("script done\n")
