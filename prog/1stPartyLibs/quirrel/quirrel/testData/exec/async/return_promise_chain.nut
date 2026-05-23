from "async" import Promise

// Chain-unwrap on async return: returning a Promise from an async function
// adopts that Promise's settlement instead of settling the task-promise with
// the Promise object itself. Matches JS async/await. Four sub-cases:
//   - fulfilled: returned Promise already fulfilled
//   - pending: returned Promise pending, settled later by a resolver
//   - rejected: returned Promise rejected; propagated to awaiter
//   - chain_2deep: returned Promise itself adopts another Promise

async function section_fulfilled() {
  print("=== fulfilled ===\n")
  async function inner() {
    let p = Promise()
    p.resolve(42)
    return p
  }
  let v = await inner()
  print("got: " + v + "\n")
}

async function section_pending() {
  print("=== pending ===\n")
  let pendingP = Promise()
  async function inner() { return pendingP }
  async function resolver() {
    pendingP.resolve("hello")
    print("resolver settled inner\n")
  }
  resolver()
  let v = await inner()
  print("got: " + v + "\n")
}

async function section_rejected() {
  print("=== rejected ===\n")
  async function inner() {
    let p = Promise()
    p.reject("boom")
    return p
  }
  try {
    let v = await inner()
    print("unexpected fulfilled: " + v + "\n")
  } catch (e) {
    print("caught: " + e + "\n")
  }
}

async function section_chain_2deep() {
  print("=== chain_2deep ===\n")
  let p2 = Promise()
  p2.resolve("deep")
  let p1 = Promise()
  p1.resolve(p2)   // p1 adopts p2 -> p1 fulfilled with "deep"
  async function f() { return p1 }
  let v = await f()
  print("got: " + v + "\n")
  print("p1.getState=" + p1.getState() + "\n")
  print("p2.getState=" + p2.getState() + "\n")
}

async function runAll() {
  await section_fulfilled()
  await section_pending()
  await section_rejected()
  await section_chain_2deep()
  print("script done\n")
}
runAll()
