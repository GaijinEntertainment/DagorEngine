from "async" import Future

// Bug regression: resolve with an already-faulted future whose value is
// the target itself. resolveAndAdopt's chain walk hits L_Faulted with
// value==target; settleTerminal must spot the cycle and produce a
// diagnostic value instead of forming an uncollectable refcount cycle.

let a = Future()
async function makeB() { throw a }
let b = makeB()

async function main() {
  // Force b's step to run so b is L_Faulted before a.resolve(b).
  try { let _ = await b } catch (_) {}
  a.resolve(b)
  print("a=" + a.getState() + " b=" + b.getState() + "\n")
  try { let _ = await a; print("BUG: a not faulted\n") }
  catch (e) {
    print("a reason type=" + typeof e + " value=" + e + "\n")
  }
}
main()
print("script done\n")
