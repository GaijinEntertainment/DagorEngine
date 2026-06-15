// Locals on the parked-ancestry walk: when `mid` is folded at the settle, its
// frame's locals are read from the suspended generator's saved _stack at the
// await point (not the live stack, which is gone). The chain is fire-and-forgotten
// so the default handler renders it on the unhandled sweep; a non-trivial local
// declared before the await is captured by value on the `awaited at` frame.

async function inner() {
    throw "deep boom"
}

async function mid() {
    local stage = "loading"
    local count = 7
    await inner()              // parked here; stage/count are in scope at the suspend
    assert(false)              // unreachable
}

async function top() {
    await mid()
}

top()   // fire-and-forget, unhandled -> default handler renders origin + awaited-at + parked locals
print("script done\n")
