from "async" import Future

// An async body that returns its own task-future would make it hold itself as
// its value (a self-cycle); the runtime faults the task-future instead.

local fut = null
async function returnsSelf() { return fut }

async function consume(f) {
    try { let _ = await f; print("BUG: fulfilled\n") }
    catch (e) { print("caught: " + e + "\n") }
}

fut = returnsSelf()
consume(fut)
print("script done\n")
