from "async" import Future

// Settlement chain: consumerB wakes from b's resolution and inside its
// resumption resolves a, which has consumerA parked on it. Verifies that a
// settle->wake->settle chain inside the runner walks correctly through
// scheduleStep without re-entering resolveTaskOrManual mid-iteration.

let a = Future()
let b = Future()

async function consumerA() {
  let x = await a
  print("A got: ")
  print(x)
  print("\n")
}

async function consumerB() {
  let y = await b
  print("B got: ")
  print(y)
  print("\n")
  // Resolve a from inside the resumed step. consumerA's parked frame should
  // wake on the next dispatcher iteration.
  a.resolve("from-B")
}

consumerA()
consumerB()

// Settle b synchronously so consumerB's await sees a Fulfilled future on its
// initial step and schedules itself for an immediate Send.
b.resolve("from-main")

print("script done\n")
