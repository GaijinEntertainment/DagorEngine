from "async" import Promise

// Awaiting a Pending promise that's never settled. The async function parks
// once and the dispatcher empties cleanly (no requeue keeps it busy). Script
// exits normally; at sq_close, the shutdown dtor rejects the live task with
// "ShutdownError" and pre-marks awaited=true to suppress the unhandled-
// rejection diagnostic.

let p = Promise()

async function consumer() {
  print("consumer waits forever\n")
  let x = await p
  print("unreachable\n")
}

consumer()
print("script done\n")
