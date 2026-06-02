from "async" import Future

// Chain-unwrap on async return: returning a Future from an async function
// adopts that Future's settlement instead of settling the task-future with
// the Future object itself. Matches JS async/await. Four sub-cases:
//   - fulfilled: returned Future already fulfilled
//   - pending: returned Future pending, settled later by a resolver
//   - faulted: inner async throws; propagated to awaiter
//   - chain_2deep: returned Future itself adopts another Future

async function section_fulfilled() {
  print("=== fulfilled ===\n")
  async function inner() {
    let p = Future()
    p.resolve(42)
    return p
  }
  let v = await inner()
  print("got: " + v + "\n")
}

async function section_pending() {
  print("=== pending ===\n")
  let pendingP = Future()
  async function inner() { return pendingP }
  async function resolver() {
    pendingP.resolve("hello")
    print("resolver settled inner\n")
  }
  resolver()
  let v = await inner()
  print("got: " + v + "\n")
}

async function section_faulted() {
  print("=== faulted ===\n")
  async function inner() { throw "boom" }
  try {
    let v = await inner()
    print("unexpected fulfilled: " + v + "\n")
  } catch (e) {
    print("caught: " + e + "\n")
  }
}

async function section_chain_2deep() {
  print("=== chain_2deep ===\n")
  let p2 = Future()
  p2.resolve("deep")
  let p1 = Future()
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
  await section_faulted()
  await section_chain_2deep()
  print("script done\n")
}
runAll()
