from "async" import Future

// p.reject(waiter) where waiter is a no-catch task parked on p: the fold path
// would store the waiter as its own value (self-cycle); the backstop faults it.
// Awaiting wt after the fold marks it handled, keeping the golden trace-free.

let p = Future()
async function waiter() { let _ = await p }   // no catch -> fold candidate on p's fault
let wt = waiter()

async function rejector() { p.reject(wt) }    // runs after waiter parks on p
rejector()

async function consume() {
    try { let _ = await wt; print("BUG: fulfilled\n") }
    catch (e) { print("wt: " + e + "\n") }
}
consume()

print("script done\n")
