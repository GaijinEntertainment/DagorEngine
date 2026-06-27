from "async" import Future

// reject() with no argument faults with null; getState distinguishes it from a
// pending future (whose getValue throws).

async function consume(fut) { try { let _ = await fut } catch (_) {} }

let r = Future()
r.reject()
print("faulted state: " + r.getState() + "\n")             // faulted
print("getValue is null: " + (r.getValue() == null) + "\n") // true
consume(r)   // mark the fault handled

let p = Future()
print("pending state: " + p.getState() + "\n")             // pending
try { let _ = p.getValue(); print("BUG: pending no throw\n") }
catch (_) { print("pending getValue throws\n") }

print("script done\n")
