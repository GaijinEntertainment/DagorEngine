from "async" import Future

// Dropping a pending combinator graph -- its `done` plus the children parked on
// never-settling inputs -- and forcing GC must not crash. The children stay
// parked; sq_close teardown releases them. Nothing here ever settles or faults,
// so the only output is the survival marker.

let dbg = require("debug")

function makeAndDrop() {
  let a = Future()
  let b = Future()  // a and b never settle
  let _all = Future.all([a, b])
  let _race = Future.race([a, b])
  // _all, _race, a, b all leave scope here; only the spawned children hold them
}

makeAndDrop()
dbg.collectgarbage()
print("gc survived\n")
