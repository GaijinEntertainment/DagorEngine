from "async" import Future

// Chain-unwrap on .resolve(): outer.resolve(inner) makes `outer` mirror
// `inner`'s settlement. Three sub-cases per inner state:
//   - already_fulfilled: outer fulfills synchronously
//   - pending: outer adopts; later resolution of inner settles outer
//   - faulted: outer faults synchronously with inner's value

async function section_already_fulfilled() {
  print("=== already_fulfilled ===\n")
  let innerP = Future()
  innerP.resolve(123)
  let outerP = Future()
  outerP.resolve(innerP)
  print("outer=" + outerP.getState() + "\n")
  let v = await outerP
  print("waiter got: " + v + "\n")
}

async function section_pending() {
  print("=== pending ===\n")
  let outerP = Future()
  let innerP = Future()
  outerP.resolve(innerP)
  print("after outer.resolve(inner) outer=" + outerP.getState() + " inner=" + innerP.getState() + "\n")
  async function resolver() {
    innerP.resolve("late")
    print("resolver settled inner\n")
  }
  resolver()
  let v = await outerP
  print("waiter got: " + v + "\n")
}

async function section_faulted() {
  print("=== faulted ===\n")
  async function makeInner() { throw "inner-failed" }
  let innerP = makeInner()
  // Force inner to settle (fault) before we resolve outer with it.
  try { let _ = await innerP } catch (_) {}
  let outerP = Future()
  outerP.resolve(innerP)
  print("outer=" + outerP.getState() + "\n")
  try {
    let v = await outerP
    print("unexpected: " + v + "\n")
  } catch (e) {
    print("caught: " + e + "\n")
  }
}

async function runAll() {
  await section_already_fulfilled()
  await section_pending()
  await section_faulted()
  print("script done\n")
}
runAll()
