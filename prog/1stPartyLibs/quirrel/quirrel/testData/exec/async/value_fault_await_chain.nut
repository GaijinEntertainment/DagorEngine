// A value-type (string) fault propagated through a multi-level await chain,
// fire-and-forgotten. A bare value cannot carry a trace slot, so the chain is
// accumulated on a FutureImpl-side array: the throw-site origin is snapshotted
// at exception_trap, and the settle-time walk appends an `awaited at` frame per
// folded ancestor. On the unhandled report path the bare value reaches the
// handler unchanged, with that accumulated trace handed over alongside it (and
// rendered under ERROR TRACE by the default handler).

async function inner() {
    throw "boom"
}

async function mid() {
    await inner()
}

async function top() {
    await mid()
}

function launcher() {
    top()  // fire-and-forget: the returned task-future is discarded
}

launcher()
print("script done\n")
