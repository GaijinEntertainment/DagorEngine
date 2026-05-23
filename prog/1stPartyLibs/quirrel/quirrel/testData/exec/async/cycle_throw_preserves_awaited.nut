from "async" import Promise

// A cycle throw must not latch `awaited` on promises walked before the
// cycle was detected; otherwise a later cascade rejection on those
// promises would be silently un-warned.

let a = Promise()
let b = Promise()

a.resolve(b)
try { b.resolve(a) } catch (e) { print("cycle caught\n") }

// a is L_Adopted with no waiter; the diagnostic must fire when the
// cascade from b's rejection drives a to L_Rejected.
b.reject("err")
print("script done\n")
