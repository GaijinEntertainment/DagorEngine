from "async" import Future

// An async body that throws its own task-future would make it hold itself as its
// fault value (a self-cycle); settleTerminal's self-value backstop faults it with
// a diagnostic instead of leaking.

local fut = null
async function throwsSelf() { throw fut }

async function consume(f) {
    try { let _ = await f; print("BUG: fulfilled\n") }
    catch (e) { print("caught: " + e + "\n") }
}

fut = throwsSelf()
consume(fut)
print("script done\n")
