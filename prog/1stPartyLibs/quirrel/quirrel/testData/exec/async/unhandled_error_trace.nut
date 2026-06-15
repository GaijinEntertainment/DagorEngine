// An async function faults with a value and its task-future is
// fire-and-forgotten. The pump-tick sweep routes the unhandled fault through
// the installed error handler. The live callstack is already unwound (empty
// CALLSTACK), so the default handler prints the captured origin frames
// under "ERROR TRACE" - the only surviving record of the throw site.

async function failing() {
  throw "boom"
}

function launcher() {
  failing()  // fire-and-forget: the returned task-future is discarded
}

launcher()
print("script done\n")
