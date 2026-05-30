// A named async function faults (throws) and its task-future is
// fire-and-forgotten from a named script function. The pump-tick sweep
// routes the unhandled fault through the VM's installed error handler,
// passing the bare thrown value ("boom"). sqstd_aux_printerror surfaces
// it as "AN ERROR HAS OCCURRED [boom]".

async function failing() {
  throw "boom"
}

function launcher() {
  failing()  // fire-and-forget: the returned task-future is discarded
}

launcher()
print("script done\n")
