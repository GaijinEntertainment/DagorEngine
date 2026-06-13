// A multi-level await chain, fire-and-forgotten. No frame catches, so the whole
// chain folds inline at the settle and the outermost task-future is reported by
// the pump-tick sweep through the installed error handler. The live callstack is
// already unwound (empty CALLSTACK), so the surviving record is the captured
// fault trace: the throw-site origin frame followed by an `awaited at` frame per
// folded async ancestor, inner-to-outer.

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
