from "async" import Future

// Bug regression: cycle when an adopter propagates back into itself.
// a.resolve(b) makes a adopt b; b then faults with a as payload. The
// cascade walks `a` from b.waiters back to itself; without the loop-level
// cycle guard, a.value would become a (uncollectable refcount cycle
// through _refs_table). The guard must substitute and force fault so a
// settles with a diagnostic reason instead.

let a = Future()
async function makeB() { throw a }
let b = makeB()
a.resolve(b)

async function waiter() {
  try { let _ = await a; print("BUG: a not faulted\n") }
  catch (e) {
    print("a=" + a.getState() + " b=" + b.getState() + "\n")
    print("a reason type=" + typeof e + " value=" + e + "\n")
  }
}
waiter()
print("script done\n")
