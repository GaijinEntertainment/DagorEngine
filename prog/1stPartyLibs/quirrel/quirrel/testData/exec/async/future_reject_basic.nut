from "async" import Future

// Script-side reject faults the future with the given value; getValue reads it
// back; awaiting throws it. reject after settle is a no-op, and a task-future
// cannot be rejected externally.

let p = Future()
p.reject("nope")
print("state: " + p.getState() + "\n")        // faulted
print("getValue: " + p.getValue() + "\n")      // nope

async function awaitIt() {
    try { let _ = await p; print("BUG: no throw\n") }
    catch (e) { print("caught: " + e + "\n") } // nope
}
awaitIt()

let q = Future()
q.resolve(1)
q.reject("late")                               // no-op, already settled
print("q state: " + q.getState() + " value: " + q.getValue() + "\n")  // fulfilled 1

async function task() { return 5 }
let t = task()
try { t.reject("x"); print("BUG: no throw\n") }
catch (e) { print("task reject threw: " + e + "\n") }

print("script done\n")
