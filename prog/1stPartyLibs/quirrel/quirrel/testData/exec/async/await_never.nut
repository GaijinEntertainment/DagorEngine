from "async" import Future

// Awaiting a Pending future that's never settled. The async function parks
// once and the dispatcher empties cleanly (no requeue keeps it busy). Script
// exits normally; at sq_close, the refs-table teardown collects the live
// task-future without firing an unhandled-error diagnostic.

let p = Future()

async function consumer() {
  print("consumer waits forever\n")
  let x = await p
  print("unreachable\n")
}

consumer()
print("script done\n")
