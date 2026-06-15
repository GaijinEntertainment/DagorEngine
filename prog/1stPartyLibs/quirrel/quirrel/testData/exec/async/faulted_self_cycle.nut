from "async" import Future

// A faulted future whose fault value is the very future adopting it must settle
// with a "chaining cycle detected" diagnostic instead of forming an
// uncollectable refcount cycle through _refs_table. Two distinct code paths
// reach that guard, depending on the order of adopt vs fault:
//   - adopt_already_faulted: b is L_Faulted before a.resolve(b); the adopt-time
//     chain walk hits L_Faulted with value==target and settleTerminal substitutes.
//   - adopt_then_fault: a.resolve(b) adopts b while b is still pending; b then
//     faults with `a` as payload, and the settle-time cascade loop walks `a`
//     back to itself and forces the fault with the diagnostic.

async function section_adopt_already_faulted() {
  print("=== adopt already-faulted ===\n")
  let a = Future()
  async function makeB() { throw a }
  let b = makeB()
  try { let _ = await b } catch (_) {}   // force b to L_Faulted first
  a.resolve(b)                           // adopt walk hits L_Faulted, value==target
  print("a=" + a.getState() + " b=" + b.getState() + "\n")
  try { let _ = await a; print("BUG: a not faulted\n") }
  catch (e) { print("a reason type=" + typeof e + " value=" + e + "\n") }
}

async function section_adopt_then_fault() {
  print("=== adopt then fault ===\n")
  let a = Future()
  async function makeB() { throw a }
  let b = makeB()
  a.resolve(b)                           // a adopts b while b is still pending
  // b faults on its first step with `a` as payload; the cascade walks a -> itself
  try { let _ = await a; print("BUG: a not faulted\n") }
  catch (e) {
    print("a=" + a.getState() + " b=" + b.getState() + "\n")
    print("a reason type=" + typeof e + " value=" + e + "\n")
  }
}

async function runAll() {
  await section_adopt_already_faulted()
  await section_adopt_then_fault()
  print("script done\n")
}
runAll()
