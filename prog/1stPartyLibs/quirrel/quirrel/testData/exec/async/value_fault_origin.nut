// A value-type (integer) fault thrown by a synchronous helper called from an
// async body, fire-and-forgotten. The origin sub-stack (helper + the async
// step) is snapshotted at the throw before the unwind and reaches the unhandled
// report path as the bare value, with the captured trace handed over alongside
// it. The default handler renders the value's typed description.

function helper() {
    throw 42
}

async function failing() {
    helper()
}

function launcher() {
    failing()  // fire-and-forget
}

launcher()
print("script done\n")
