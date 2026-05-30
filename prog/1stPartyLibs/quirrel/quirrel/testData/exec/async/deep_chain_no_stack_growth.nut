from "async" import Future

// Long chain of `await scalar` and `await alreadyResolved` calls inside a
// single async function. Two properties are exercised:
//
//   1) The chain completes in a single host `pump()` call (the runner
//      reschedules each next step onto the same queue, and pump drains it
//      reentrantly until it's empty).
//   2) The implementation does not recurse on the C stack, so 2000 deep
//      awaits don't smash it or run observably slowly.
//
// FIFO scheduling means fastChain's queued continuations interleave with
// other()'s initial step: after the first iteration of fastChain yields,
// other() runs (FIFO), and only then fastChain's remaining 1999 iterations
// drain in order. So expected output is "other ran" before "fastChain done".

let p = Future()
p.resolve("R")

async function fastChain() {
  local last = null
  for (local i = 0; i < 1000; i++)
    last = await i           // non-Future scalar
  for (local i = 0; i < 1000; i++)
    last = await p           // already-fulfilled Future
  print($"fastChain done last={last}\n")
}

async function other() {
  print("other ran\n")
}

fastChain()
other()
print("script done\n")
